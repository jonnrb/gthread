#include "mymalloc.h"

static const long max_size = MAX_SIZE; //Maximum size for Virtual Memory
extern char myblock[MAX_SIZE] = {}; //8MB memory block
static long page_size; //Page size for system
static int threads_allocated; //number of threads that allocated space in Virtual Memory currently
extern Node* shallocRegion;
//////////////////////////////////////////////////////////////////////
//MALLOC//
//////////////////////////////////////////////////////////////////////

//Goes through each node in the array and prints the attributes of each node
void debug(char* str){
	printf("DEBUG: %s\n", str);
	return;
}

void printpagemem(){
	Node* start = (Node*)&myblock[0];
	Node* page = start + 1;
	Node* pageInternals = page + 1;
	int pageCount = 0;
	while(page->type != VM){
		if(page->used == TRUE){
			printf("PAGE: %d | USED: TRUE | PAGEOWNER: %p\n",pageCount, (void*)&page->thread);
			pageInternals = page + 1;
			while(pageInternals->space != 0){
			    if(pageInternals->used == 0){
			        printf("       USED: True----Space: %d\n",pageInternals->space);
			    }
			    else{
			        printf("       USED: False----Space: %d\n",pageInternals->space);
			    }
			    pageInternals = (Node*)((char*)(pageInternals+1)+(pageInternals->space));
			}

			if(pageInternals->space == 0 && pageInternals->used == FALSE){
			    printf("------------------------------------------------------\n");
			}
		}
		page = ((Node*)((char*)(page+1) + page_size) + 1);
		pageCount++;
	}
}




void swapPages(Node* source, Node* target){
	if(source == target){
		printf("Can't swap the same pages!\n");
		return;
	}
	else if(source->type != PAGE_START || target->type != PAGE_START){
		printf("Either the source, or target inputs arent the start of pages.\n");
		return;
	}
	char temp[page_size+2*sizeof(Node)]; //temporary holder of source page
	memcpy(temp, source, page_size+2*sizeof(Node)); //copy source to temporary array
	memcpy(source, target, page_size+2*sizeof(Node)); //copy target page over to source page
	memcpy(target, temp, page_size+2*sizeof(Node)); //copy source page over to target page
	//swap complete
	return;
}









//initializes the array
//creates a start node, and an end node
void initblock(){
	threads_allocated = 0; //initializes threads allocated counter
	int total = 0;
	int sizeNode = sizeof(Node);
	int spaceleft = max_size;

	Node* start = (Node*)&myblock[0];
	start->space = max_size - 2*sizeNode;
	start->used = FALSE;
	start->type = VM;
	total = total + sizeof(Node); //calculated space used
	spaceleft = spaceleft - sizeof(Node); //calculating space left after Start Node is created

	Node* pageIterator = start; //create new reference to first node

	//iterate and create a PAGE_START node and PAGE_END node until
	//space left is less than that of a page size and 2 nodes associated with each page
	//and an additional node to signify the end of virtual memory


	while(spaceleft > (page_size + 3*sizeof(Node))){
		pageIterator = pageIterator + 1;
		pageIterator->space = page_size;
		pageIterator->used = FALSE;
		pageIterator->type = PAGE_START;
		pageIterator->thread = NULL;
		pageIterator = (Node*)((char*)(pageIterator + 1)+page_size);
		pageIterator->space = 0;
		pageIterator->used = FALSE;
		pageIterator->type = PAGE_END;
		pageIterator->thread = NULL;
		spaceleft = spaceleft - (page_size + 2*sizeof(Node));
		total = total + 2*sizeof(Node) + page_size;


	}
	//end of VirtualMemory node
	Node* end = (Node*)&myblock[total];
	end->space = 0;
	end->used = FALSE;
	end->type = VM;
	//shalloc region creation
	Node* shallocNode = end - (8*sizeof(Node) + 4 * page_size); //backtrack four pages
	shallocNode->space = 4*page_size + 7*sizeof(Node); //create shalloc metadata
	shallocNode->used = FALSE;
	shallocNode->type = SHALLOC;

}


void printPageStructure(Node* PageStart){
	Node* it = PageStart;
	while(it->type != PAGE_END){
		if(it->used == TRUE){
			printf("BLOCK USED: TRUE ");
		}
		else{
			printf("BLOCK USED: FALSE ");
		}
		printf("SPACE: %d", it->space);
		printf("\n");
		it = (Node*)((char*)(it+1)+ it->space);
		if(it->type == PAGE_END){
			printf("PAGE END\n");
		}
	}
}

//Traverses the array, searching for sections with size 'size', and returning it.
//If no partition is found, and the index has overreached, then it returns NULL
Node* traversePageInternals(int size, Node* PageStart){
    Node* ptr =  PageStart;
    //if page is not used
    if(ptr->used == FALSE){
    	if((size + 2*sizeof(Node)) > page_size){
    		return NULL;
    	}
    	return ptr;
    }

    while(ptr->type != PAGE_END){
    	//if the location is not used, has space greater than the size requested, and is inside the page (THREAD_PTR)
    	if(ptr->used == FALSE && ptr->space >= size + sizeof(Node) && ptr->type == THREAD_PTR){
    		return ptr;
    	}
    	ptr = (Node*)((char*)(ptr+1) + ptr->space);
    }
    return NULL;
}



//When a valid partition is found, a node is created to hold the space after the partition, and
//the current node's size is readjusted to reflect the size partitioned
void* allocateNewInsidePage(Node* nodeptr, int size){
	Node* temptr = NULL;
	//Calculate the space left over after allocating enough for the current request
	int totalspace = nodeptr->space;
	totalspace = totalspace - (size + sizeof(Node));
	//temptr is a pointer that will increment to the location after the requested space
	//and create a new node to indicate the space left
	if(totalspace>0){
    temptr = nodeptr;
    temptr = (Node*)((char*)(temptr+1) + size);
    temptr->space = totalspace;
    temptr->used = FALSE;
    temptr->type = THREAD_PTR;
	}

    //the input to the function is then adjusted to indicate the correct size requested
    //and its active 'used' flag
    nodeptr->space = size;
    nodeptr->used = TRUE;
    //a void pointer to the space after the node(the space requested), is returned to the main malloc function
    void* ptr = (void*)(nodeptr+1);
    return ptr;
}


//if thread page is found, returns first node inside the page
void* findThreadPage(gthread_task_t *owner){
	int number_of_threads = threads_allocated;
	Node* page = (Node*)&myblock[0];
	page = page + 1;
	while(number_of_threads > 0){
		//page found, return
		if(page->type == PAGE_START && page->thread == owner){
			page = page + 1;
			return page;
		}
		//else iterate
		number_of_threads--;
		page = (Node*)((char*)(page+1)+page_size)+1;
	}
return NULL;
}


void* createThreadPage(gthread_task_t *owner){
	Node* page = (Node*)&myblock[0]; //VM Start node
	page = page + 1; //increment to first page node
	Node* nodeptr;
	//continue until end of VM is reached (break condition is inside)
	while(page->type != SHALLOC){
		if(page->type == PAGE_START && page->thread == NULL){ //found unused page
			page->used = TRUE;
			page->thread = owner;
			nodeptr = page;
			//sets up first memory allocation inside the page
				nodeptr = nodeptr + 1;
				nodeptr -> type = THREAD_PTR;
				nodeptr->space = page_size - sizeof(Node);
				nodeptr->used = FALSE;
			threads_allocated++;
			return (page+1);
		}
		//increment 1 PAGE_START, and the page size, and another PAGE_END node
		page = (Node*)((char*)(page + 1) + page_size)+1;
	}
	return NULL; //no space
}




void* mymalloc(size_t size, gthread_task_t *owner){
	page_size = sysconf(_SC_PAGESIZE);
	void* ptr = NULL;
    //initialize myblock if not initialized
    if(size < 1){
        printf("Input argument of integer value 0 or less is not valid. Please enter a positive non-zero integer.\n");
        return NULL;
    }
    if(owner == NULL){
    	 printf("Thread no longer exists or invalid thread input.\n");
    	 return NULL;
    }
    //initialize

    if(myblock[0] == 0){
    	initblock();
    }


    //traverse array until thread page is found
    void* page = findThreadPage(owner);
    if(page == NULL){ //if thread does not have an allocated space
    	page = createThreadPage(owner);
    }
    if(page == NULL){ //no space for thread left in VM
    	printf("No space left in VM for requested thread Memory Allocation.\n");
    	return NULL;
    }

    //traverse array until free space is found
    Node* nodeptr = traversePageInternals(size, page);
    //if the end is reached, no valid space is found
    if(nodeptr == NULL){
    	printf("Invalid space request, capacity overflow.\n");
    	return NULL;
    }
    ptr = allocateNewInsidePage(nodeptr, size);
    //returns void pointer to the caller
    return ptr;
}

//////////////////////////////////////////////////////////////////////////////////////
//FREE//
/////////////////////////////////////////////////////////////////////////////////////




//Checks left of the pointer, if a node exists, returns it
//used for concatenation of two unused nodes in the main free function
//returns a pointer to the node that is before the node being freed
Node* checkLeft(Node* ptr, Node* page){
	//ptr is already the left most node
if(ptr == NULL){
	//error
	return NULL;
}
if(ptr == page){
	return NULL;
}
Node* leftp = page;
printPageStructure(leftp);
//while the next node does not = the node currently being worked on
while(((Node*)((char*)(leftp+1)+leftp->space)) != ptr) {
	leftp = (Node*)((char*)(leftp+1)+leftp->space);
}
return leftp;
}


//Checks right of the pointer, if a node exists, returns it
//used for concatenation of two unused nodes in the main free function
//returns a pointer to the node that is after the node being freed
Node* checkRight(Node* ptr){
if(ptr == NULL){
    //error
    return NULL;
	}
		//ptr is already the rightmost node
	Node* temp = (Node*)((char*)(ptr+1)+ptr->space);
if(temp->type == PAGE_END){
	return NULL;
}
return temp;
}



//checks if the pointer inputted in the 'free()' function is inside the memory array
//returns TRUE if it is, false if it is not
BOOLEAN checkpoint(void* p, gthread_task_t* owner){
Node* current = findThreadPage(owner);
if(current == NULL){
	return FALSE;
}
current = current - 1;
if(current->used == FALSE){ //if page is indicated to be FALSE, no pointer should be allocated
	printf("Thread does not have an allocated page!\n");
	return FALSE;
}
else{
	current = current + 1;
}
//current is not at the end
while(current->space!=0){
    if((void*)(current+1) == p ){
    return TRUE;}
    current = (Node*)((char*)(current+1)+current->space);
}
return FALSE;
}



void check_and_freePage(Node* page){
	if(page == NULL){
		return;
	}
	if(page->used == FALSE){ //page is empty
		page = page - 1; //PAGE_START node
		page->thread = NULL;
		page->used = FALSE;
	}
}


//Frees the usage of the space taken up by this pointer, changes
//the metadata associated with it to inactive (used = FALSE), and concatenates
//the space with any adjacent unused nodes.
void myfree(void* p, gthread_task_t *owner){

    //checks if pointer is in the array
    BOOLEAN valid = checkpoint(p, owner);

    //if pointer is not in the array, throw error, return
    if(valid == FALSE){
        printf("Invalid input pointer. Pointer must be at the start of a previously allocated area of the memory array\n");
        return;
    }

    //converts pointer to a node type, and subtracts it by 1 to get to the node that corresponds to it
    Node* ptr = (Node*)p;
    ptr = ptr-1;

    //Checks node, if null or the 'used' flag is found to be false, then the pointer was not allocated for
	    if(ptr->used == FALSE){
		//error
		printf("Error, attempted to free an unallocated pointer!\n");
		return;
	}

	//At this point, the pointer exists, and the node corresponding to it is valid
	//The node's flag is set to unused = 'FALSE'
	ptr -> used = FALSE;

	Node* page = findThreadPage(owner);
	if(page == NULL){
		return;
	}
	//Finds the pointer left the node
	Node* leftp = checkLeft(ptr, page);
	//	//Finds the pointer right the node
	Node* rightp = checkRight(ptr);

	//concatenates the space of the right node, and the current node
	if(rightp != NULL && rightp->used == FALSE ){
		ptr->space = ptr->space + rightp->space + sizeof(Node);
		rightp = NULL;
	}
    //concatenates the space of the left node, and the current node
	if(leftp != NULL && leftp->used == FALSE){
		leftp->space = leftp->space + ptr->space + sizeof(Node);
		ptr = NULL;
	}
	check_and_freePage(page);

	return;
}


