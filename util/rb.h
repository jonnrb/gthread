/**
 * author: JonNRb <jonbetti@gmail.com>
 * license: MIT
 * file: @gthread//util/rb.h
 * info: red black tree
 */

#ifndef UTIL_RB_H_
#define UTIL_RB_H_

#include <stdint.h>

/**
 * node structure to be embedded in your data object.
 *
 * NOTE: it is expected that there will be a `uint64_t` immediately following
 * the embedded node (which will be used as a sorting key).
 */
typedef struct _gthread_rb_node gthread_rb_node_t;
struct _gthread_rb_node {
  gthread_rb_node_t* link[2];
  int red;
  uint64_t sort_key[0];  // place a `uint64_t` sort key immediately after this
                         // struct in the data struct
};

/**
 * represents a whole tree. should be initialized to NULL.
 */
typedef gthread_rb_node_t* gthread_rb_tree_t;

/**
 * initializes a node to default values
 */
void gthread_rb_construct(gthread_rb_node_t* node);

/**
 * takes a pointer to |tree| and |node| to insert. will update |tree| if
 * necessary.
 */
void gthread_rb_push(gthread_rb_tree_t* tree, gthread_rb_node_t* node);

/**
 * takes a pointer to the |tree| and returns a pointer to the minimum node in
 * the tree, which is removed from |tree|
 */
gthread_rb_node_t* gthread_rb_pop_min(gthread_rb_tree_t* tree);

#endif  // UTIL_RB_H_
