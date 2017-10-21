/**
 * author: JonNRb <jonbetti@gmail.com>
 * license: MIT
 * file: @gthread//util/rb.cc
 * info: red black tree
 */

/**
 * guided by the wisdom of
 * http://eternallyconfuzzled.com/tuts/datastructures/jsw_tut_rbtree.aspx
 */

#include "util/rb.h"

#include <errno.h>
#include <stdlib.h>

static inline int is_red(gthread_rb_node_t* node) {
  return node != NULL && node->red;
}

static int correct_rb_height(gthread_rb_tree_t tree) {
  if (tree == NULL) return 1;  // NULL is a black node (black-height == 1)

  const int redness = is_red(tree);

  // red cannot neighbor red
  if (redness && (is_red(tree->link[0]) || is_red(tree->link[1]))) return -1;

  int black_height;

  // left and right subtrees must be correct and have same black height
  if ((black_height = correct_rb_height(tree->link[0])) < 0) return -1;
  if (black_height != correct_rb_height(tree->link[1])) return -1;

  return black_height + !redness;
}

/**
 * rrot = 1 (rrot = 0 is flipped)
 *
 *      C        B
 *     /        / \
 *    B    =>  A   C
 *   /
 *  A
 */
static void rotate(gthread_rb_tree_t* tree, int rrot) {
  gthread_rb_node_t* save = (*tree)->link[!rrot];

  (*tree)->link[!rrot] = save->link[rrot];
  save->link[rrot] = (*tree);

  (*tree)->red = 1;
  save->red = 0;

  *tree = save;
}

/**
 * rrot = 1 (rrot = 0 is flipped)
 *
 *     E            E           C
 *    /            /          /   \
 *   A            C          A     E
 *    \     =>   / \    =>    \   /
 *     C        A   D          B D
 *    / \        \
 *   B  D         B
 */
static void rotate_double(gthread_rb_tree_t* tree, int rrot) {
  if (tree == NULL) return;
  rotate(&(*tree)->link[!rrot], !rrot);
  rotate(tree, rrot);
}

/**
 * |tree| must be a black node with red children
 *
 * |path| encodes the index on the bottom layer for |tree| in the 2 level
 * subtree with root |grandparent|
 *
 * the case where |tree| has only one parent implies its parent is the root
 * which must be a black node (0, 1, 2, or 3)
 */
static inline void color_flip(gthread_rb_tree_t* tree,
                              gthread_rb_tree_t* parent,
                              gthread_rb_tree_t* grandparent, uint8_t path) {
  if (is_red((*tree)->link[0]) && is_red((*tree)->link[1])) {
    (*tree)->red = 1;
    (*tree)->link[0]->red = 0;
    (*tree)->link[1]->red = 0;
  }

  if (grandparent != NULL && is_red(*parent) && is_red(*tree)) {
    if (!(path % 3)) {
      rotate(grandparent, !(path % 2));
    } else {
      rotate_double(grandparent, path % 2);
    }
  }
}

static void insert_impl(gthread_rb_tree_t* tree, gthread_rb_node_t* node) {
  gthread_rb_tree_t* it = tree;
  gthread_rb_tree_t* parent = NULL;
  gthread_rb_tree_t* grandparent = NULL;
  uint8_t path = 0;

  for (;;) {
    if (*it == NULL) {
      *it = node;
      path |= 4;
    }

    color_flip(it, parent, grandparent, path & 3);

    if (path & 4) break;

    int insert_right = *node->sort_key > *(*it)->sort_key;
    grandparent = parent;
    parent = it;
    it = &(*it)->link[insert_right];
    path = (path % 2) * 2 + insert_right;
  }
}

static gthread_rb_node_t* pop_impl(gthread_rb_tree_t* tree, int from_right) {
  if (*tree == NULL) return NULL;

  if ((*tree)->link[from_right] == NULL) {
    gthread_rb_node_t* ret = *tree;
    *tree = (*tree)->link[!from_right];
    return ret;
  }

  gthread_rb_tree_t* it = tree;
  gthread_rb_tree_t* parent = NULL;
  gthread_rb_node_t* sibling = NULL;

  for (;;) {
    // TODO(jonnrb): pop out special cases to make them easier to understand
    if (!is_red(*it) && !is_red((*it)->link[from_right])) {
      if (is_red((*it)->link[!from_right])) {
        rotate(it, from_right);
      } else if (sibling != NULL) {
        int recolor = 0;
        if (is_red(sibling->link[from_right])) {
          rotate_double(parent, from_right);
          recolor = 1;
        } else if (is_red(sibling->link[!from_right])) {
          rotate(parent, from_right);
          recolor = 1;
        } else {
          (*parent)->red = 0;
          sibling->red = 1;
          (*it)->red = 1;
        }

        if (recolor) {
          (*it)->red = 1;
          (*parent)->red = 1;
          (*parent)->link[0]->red = 0;
          (*parent)->link[1]->red = 0;
        }
      }
    }

    if ((*it)->link[from_right] == NULL) {
      gthread_rb_node_t* ret = *it;
      *it = (*it)->link[!from_right];

      (*tree)->red = 0;

      ret->link[0] = NULL;
      ret->link[1] = NULL;
      return ret;
    }

    parent = it;
    it = &(*it)->link[from_right];
    sibling = (*parent)->link[!from_right];
  }
}

void gthread_rb_construct(gthread_rb_node_t* node) {
  node->red = 1;
  node->link[0] = NULL;
  node->link[1] = NULL;
}

int gthread_is_rb_correct(gthread_rb_tree_t tree) {
  return correct_rb_height(tree) >= 0;
}

void gthread_rb_push(gthread_rb_tree_t* tree, gthread_rb_node_t* node) {
  if (tree == NULL || node == NULL) {
    errno = EINVAL;
    return;
  }

  node->link[0] = NULL;
  node->link[1] = NULL;

  if (*tree == NULL) {
    *tree = node;
    return;
  }

  insert_impl(tree, node);

  (*tree)->red = 0;
}

gthread_rb_node_t* gthread_rb_pop_min(gthread_rb_tree_t* tree) {
  if (tree == NULL) {
    errno = EINVAL;
    return NULL;
  }

  return pop_impl(tree, 0);
}
