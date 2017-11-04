#ifndef _MYMALLOC_H
#define _MYMALLOC_H
#define malloc(x) mymalloc(x)
#define free(x) myfree(x)
#define MAX_SIZE 8388608
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
void * mymalloc(size_t size);
void myfree(void * data);
void printmem();
typedef enum truth {TRUE, FALSE} BOOLEAN;
typedef struct _node{
    int space;
    BOOLEAN used;
} Node;
static char myblock[MAX_SIZE];


#endif //_MYMALLOC_H
