#ifndef _MYMALLOC_H
#define _MYMALLOC_H
#define MAX_SIZE 8388608
#define SWAP_SIZE 16777216
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>
#include <string.h>
#include "arch/atomic.h"
#include "sched/sched.h"
#include "sched/task.h"
#include "util/compiler.h"
#include "util/list.h"
#include "util/rb.h"


typedef enum truth {TRUE, FALSE} BOOLEAN;
typedef enum flag{BETWEEN, END, NONE} BETWEEN_OR_END;
//PAGE METADATA
typedef struct _node{
    gthread_task_t* thread; //thread to which the page belongs
    struct _node* next_page; //linked list structure to indicate contiguous pages
    struct _node* first_page; //pointer to the first page metadata to which traversal can be done
    void* page_start_addr; //pointer where page starts
    void* page_end_addr; //pointer where page end (start of next page)
    int space_allocated; //number of bytes allocated in total for this thread (this number is only valid in first page structure)
    int page_offset; //offset from address space 0 that this page is in (incase pages in between gets cleared)
} Node;


//Internal Metadata of the page (think mymalloc)
typedef struct page_internal{
	int space; //space inside the current page
	BOOLEAN used;
} Page_Internal;


void * mymalloc(size_t size, gthread_task_t* owner);
void myfree(void * data, gthread_task_t* owner);
void* shalloc(size_t size);
void myfreeShalloc(void* p);
void swapPages(Node* source, Node* target);
void initblock(); //initializes memory block
int placePagesContig(gthread_task_t* owner); //places pages owned by thread at start of memory in contig fashion
Node* whichPage(void* addr); //locate page associated with an address
Node* findThreadPage(gthread_task_t *owner);

//debugging prints
void printInternalMemory(gthread_task_t* owner);
void printpages();
void printShallocRegion();
void debug(char* str);
void printThread(gthread_task_t* owner);

extern void* myblock;
extern void* swapblock;
extern void* shallocRegion;
extern size_t page_size;

#endif //_MYMALLOC_H
