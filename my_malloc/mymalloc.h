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
//3 different types of metadata (Page, Thread memory, and Virtual Memory)
typedef enum type {PAGE_METADATA,THREAD_PTR, VM, SHALLOC} TYPE;
typedef struct _node{
    int space;
    BOOLEAN used;
    TYPE type; //Page or pointer for thread
    gthread_task_t* thread;
    struct _node* next_page;
    void* page_start_addr; //pointer where page starts
    void* page_end_addr; //pointer where page end (start of next page)
} Node;

typedef struct page_internal{
	int space;
	BOOLEAN used;
} Page_Internal;
void * mymalloc(size_t size, gthread_task_t* owner);
void myfree(void * data, gthread_task_t* owner);
void* shalloc(size_t size);
void myfreeShalloc(void* p);
void swapPages(Node* source, Node* target);
void initblock();

void printpagemem();
void printshallocmem();
void debug(char* str);
Node* getShallocRegion();

extern char myblock[MAX_SIZE];


#endif //_MYMALLOC_H
