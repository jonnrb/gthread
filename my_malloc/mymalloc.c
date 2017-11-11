#include "mymalloc.h"

static const long max_size = MAX_SIZE; //Maximum size for Virtual Memory
extern char myblock[MAX_SIZE] = {}; //8MB memory block
static long page_size; //Page size for system
static int threads_allocated; //number of threads that allocated space in Virtual Memory currently
static Node* shallocRegion; //first shalloc metadata
static Node* meta_start; //the first page metadata
static int numb_of_pages; //number of total pages in the block (can be empty pages)
//////////////////////////////////////////////////////////////////////
//MALLOC//
//////////////////////////////////////////////////////////////////////

//Goes through each node in the array and prints the attributes of each node
void debug(char* str){
	printf("DEBUG: %s\n", str);
	return;
}

void printpagemem(){
	Node* page = meta_start;
	Node* pageInternals = page + 1;
	int pageCount = 0;
	while(pageCount < numb_of_pages){
		if(page->used == TRUE){
			while(pageInternals != page->page_end_addr){
			printf("PAGE: %d | USED: TRUE | PAGEOWNER: %p\n",pageCount, (void*)&page->thread);
			pageInternals = (Node*)page->page_start_addr;
			while(pageInternals->space != 0){
			    if(pageInternals->used == TRUE){
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
			pageInternals = (Node*)((char*)(pageInternals+1)+pageInternals->space);
		}
		}
		page = page + 1;
		pageCount++;
	}
}


//sets metadata start address and number of pages expected on this machine
void metadata_start_addr(){
	double max = max_size - (2*sizeof(Node) + 4*page_size); //decrement by shalloc space
	double ratio = sizeof(Node)/page_size;
	double metaSpace = max*ratio;
	metaSpace = ceil(metaSpace);
	numb_of_pages = metaSpace/sizeof(Node);
	max = max - metaSpace;
	long maxaddr = (long)max;
	meta_start = (Node*)&myblock[maxaddr-1];
}

/*							2k
 * [Page Start] ------------xxxxxxxxxxxxxxxxxxxxxxxxx [Page End][Page Start]xxxxxxxxxxxxxxxxxxxxxx----------------[Page End]
 * 	4k space				^ptr=3k
 * -----------xxxxxxxxxxxxxxxxxxxxxxxx|xxxxxxxxxxxxxxxxxxxxxx--------------
 *
 *
 *
 */




void swapPages(Node* source, Node* target){
	if(source == target){
		printf("Can't swap the same pages!\n");
		return;
	}
	else if(source->type != PAGE_METADATA || target->type != PAGE_METADATA){
		printf("Either the source, or target inputs arent the start of pages.\n");
		return;
	}
	char temp[page_size]; //temporary holder of source page
	memcpy(temp, source, page_size); //copy source to temporary array
	memcpy(source, target, page_size); //copy target page over to source page
	memcpy(target, temp, page_size); //copy source page over to target page
	//swap complete
	return;
}




Node* getShallocRegion(){
	return shallocRegion;
}




//initializes the array
//creates a start node, and an end node
void initblock(){
	threads_allocated = 0; //initializes threads allocated counter
	metadata_start_addr(); //sets meta_start and number_of_pages
	Node* metadata = meta_start;
	int metaIterator = 0;
	while(metaIterator < numb_of_pages){
		metadata->thread = NULL;
		metadata->type = PAGE_METADATA;
		metadata->used = false;
		metadata->page_start_addr = (void*)(&myblock[0] + page_size*metaIterator);
		metadata->page_end_addr = (void*)(&myblock[0] + page_size*(metaIterator+1));
		metadata = metadata+1;
		metaIterator++;
	}
	//end of VirtualMemory node
	Node* end = (Node*)&myblock[max_size-1] - 1;
	end->space = 0;
	end->used = FALSE;
	end->type = VM;
	//shalloc region creation
	Node* shallocNode = (Node*)((char*)end - (sizeof(Node) + 4*page_size)); //backtrack four pages
	shallocNode->space = 4*page_size; //create shalloc metadata
	shallocNode->used = FALSE;
	shallocNode->type = SHALLOC;
	shallocNode->thread = NULL;
	shallocRegion = shallocNode;
}


void printPageStructure(Node* Page){
	Node* it = Page->page_start_addr;
	while(it  != Page->page_end_addr){
		if(it->used == TRUE){
			printf("BLOCK USED: TRUE ");
		}
		else{
			printf("BLOCK USED: FALSE ");
		}
		printf("SPACE: %d", it->space);
		printf("\n");
		it = (Node*)((char*)(it+1)+ it->space);
		if(it == Page->page_end_addr){
			printf("PAGE END\n");
		}
	}
}


//Traverses the array, searching for sections with size 'size', and returning it.
//If no partition is found, and the index has overreached, then it returns NULL
Node* traversePageInternals(int size, Node* metadata){
    Node* ptr =  (Node*)metadata->page_end_addr;
    //if page is not used
    if(ptr->used == FALSE){
    	if((size + 2*sizeof(Node)) > page_size){
    		return NULL;
    	}
    	return ptr;
    }

    while(ptr != metadata->page_end_addr){ //check if & is ness
    	//if the location is not used, has space greater than the size requested, and is inside the page (THREAD_PTR)
    	if(ptr->used == FALSE && ptr->space >= size + sizeof(Node) && ptr->type == THREAD_PTR){
    		return ptr;
    	}
    	ptr = (Node*)((char*)(ptr+1) + ptr->space);
    }
    return NULL;
}



//[----|-----M|-----|-----|--------------------------------MMMMMMMSSS]



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
	Node* page = meta_start;
	while(number_of_threads > 0){
		//page found, return
		if(page->thread == owner){
			return page->page_start_addr;
		}
		//else iterate
		number_of_threads--;
		page = page + 1;
	}
return NULL;
}


void* createThreadPage(gthread_task_t *owner){
	Node* page = meta_start;
	int metacounter = 0;
	Node* nodestart;
	while(metacounter<numb_of_pages){
		if(page->thread == NULL){
			page->used = TRUE;
			page->thread = owner;
			nodestart = (Node*)page->page_start_addr;
			nodestart->space = (page_size - sizeof(Node));
			nodestart->type = THREAD_PTR;
			nodestart->used = FALSE;
			return page;
		}
		page = page + 1;
		metacounter++;
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

    if(meta_start == NULL){
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
Node* leftp = (Node*)page->page_start_addr;
//while the next node does not = the node currently being worked on
while(((Node*)((char*)(leftp+1)+leftp->space)) != ptr) {
	leftp = (Node*)((char*)(leftp+1)+leftp->space);
}
return leftp;
}


//Checks right of the pointer, if a node exists, returns it
//used for concatenation of two unused nodes in the main free function
//returns a pointer to the node that is after the node being freed
Node* checkRight(Node* ptr, Node* page){
if(ptr == NULL){
    //error
    return NULL;
	}
		//ptr is already the rightmost node
	Node* temp = (Node*)((char*)(ptr+1)+ptr->space);
if(temp == page->page_end_addr){
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
		page = page - 1; //PAGE_METADATA node
		page->thread = NULL;
		page->used = FALSE;
	}
}


//Frees the usage of the space taken up by this pointer, changes
//the metadata associated with it to inactive (used = FALSE), and concatenates
//the space with any adjacent unused nodes.
void myfree(void* p, gthread_task_t *owner){
	//check if p is in shalloc region
	if(p > shallocRegion){
		myfreeShalloc(p);
		return;
	}
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
	Node* rightp = checkRight(ptr, page);

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


