#ifndef _MYMALLOC_H
#define _MYMALLOC_H
#define malloc(x) mymalloc(x)
#define free(x) myfree(x)

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
static char myblock[5000];


#endif //_HEADER_H
