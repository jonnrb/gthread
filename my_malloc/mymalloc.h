#ifndef _MYMALLOC_H
#define _MYMALLOC_H
#define MAX_SIZE 8388608
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

//PAGE METADATA
typedef struct _node{
    gthread_task_t* thread; //thread to which the page belongs
    struct _node* next_page; //linked list structure to indicate contiguous pages
    struct _node* first_page; //pointer to the first page metadata to which traversal can be done
    void* page_start_addr; //pointer where page starts
    void* page_end_addr; //pointer where page end (start of next page)
} Node;


//Internal Metadata of the page (think mymalloc)
typedef struct page_internal{
	int space; //space inside the current page
	struct page_internal* nextPI ;
	BOOLEAN used;
} Page_Internal;


void * mymalloc(size_t size, gthread_task_t* owner);
void myfree(void * data, gthread_task_t* owner);
void* shalloc(size_t size);
void myfreeShalloc(void* p);
void swapPages(Node* source, Node* target);
void initblock();

//debugging prints
void printThreadMemory(gthread_task_t* owner);
void printpages();
void printShallocRegion();
void debug(char* str);
void printThread(gthread_task_t* owner);
int placePagesContig(gthread_task_t* owner);


void* getShallocRegion(); //returns starting address of shalloc address space
int getPageSize();
void* getMetaStart();

extern char myblock[MAX_SIZE];

#endif //_MYMALLOC_H
