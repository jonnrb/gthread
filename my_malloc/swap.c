#include "mymalloc.h"

//takes in two metadata nodes and swaps the page of the address they refer to
void swapPages(Node* source, Node* target){
	int page_size = sysconf(_SC_PAGESIZE);
	char temp[page_size]; //temporary holder of source page
	memcpy(temp, source->page_start_addr, page_size); //copy source to temporary array
	memcpy(source->page_start_addr, target->page_start_addr, page_size); //copy target page over to source page
	memcpy(target->page_start_addr, temp, page_size); //copy source page over to target page
	//swap complete
	return;
}




