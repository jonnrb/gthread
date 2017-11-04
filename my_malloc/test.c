/*
Author: Abdulrahman Althobaiti. RUID: 158006706 Section: 06
Author: Peter Santana-Kroh. RUID: 161007620 Section: 05
Professor: Francisco, John-Austen
Assignment 1 - A better malloc() and free()
*/
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include <stdlib.h>
#include <errno.h>
#include "mymalloc.h"

int main()
{
 int i;
 int x_2;
 char* p_3;
 char* p_4;
 char* q_4;
 char* p_5;
 char* p_6;
 char* p_7;
 char* q_7;
 char temp;
 char* p_9;
 char* q_9;
 char** p = (char**)malloc(50*sizeof(char*))  ;







 /*test case 9*/

 printf("Test case 9 (5pts):\n");
 printf("Expected output: memory saturation\n");
 printf("Actual output:\n");
 p_9 = malloc(3000);
 q_9 = malloc(3500);
 printf("\n");
 printf("\n");
 while( getchar() != '\n' );
 free(p_9);



 return(0);
}
