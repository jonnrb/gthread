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
	Page_Internal* PI= NULL;
	int pagecounter = 0;
	while(pagecounter < threads_allocated){
		printf("Page #: %d\n",pagecounter);
		PI = (Page_Internal*)page->page_start_addr;
		while((void*)PI != (void*)page->page_end_addr){
			if(PI->used == FALSE){
				printf("       USED: FALSE -+- SPACE: %d\n",PI->space);
			}
			else{
				printf("       USED: TRUE  -+- SPACE: %d\n",PI->space);
			}
			PI = (Page_Internal*)((char*)(PI+1) + PI->space);
		}
		printf("======================================================\n");
		pagecounter++;
		page = page + 1;
	}
}


//ceil function (my linux needed a special flag to compile math.h, didn't wanna deal)
double ceil(double num) {
    int inum = (int)num;
    if (num == (double)inum) {
        return (double)inum;
    }
    return (double)(inum + 1);
}

//sets metadata start address and number of pages expected on this machine
void metadata_start_addr(){
	int max = max_size - (2*sizeof(Node) + 4*page_size); //decrement by shalloc space
	printf("Max Size = %d, sizeof(Node) = %d, page_size = %d\n",max_size,sizeof(Node), page_size);
	printf("Max Size - Shalloc: %d\n",max);
	double ratio = (double)sizeof(Node)/(double)page_size;
	double metaSpace = (double)max*ratio;
	printf("Metaspace: %d\n",(int)metaSpace);
	metaSpace = ceil(metaSpace);
	numb_of_pages = (int)metaSpace/sizeof(Node);
	printf("Number of Pages: %d\n",numb_of_pages);
	max = max - metaSpace;
	int maxaddr = (int)max;
	meta_start = (Node*)&myblock[maxaddr-1];
}




//takes in two metadata nodes and swaps the page of the address they refer to
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
	memcpy(temp, source->page_start_addr, page_size); //copy source to temporary array
	memcpy(source->page_start_addr, target->page_start_addr, page_size); //copy target page over to source page
	memcpy(target->page_start_addr, temp, page_size); //copy source page over to target page
	//swap complete
	return;
}




void* getShallocRegion(){
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
		metadata->used = FALSE;
		metadata->page_start_addr = (void*)(&myblock[0] + page_size*metaIterator);
		metadata->page_end_addr = (void*)(&myblock[0] + page_size*(metaIterator+1));
		metadata = metadata+1;
		metaIterator++;
	}
	//end of VirtualMemory node

	//shalloc region creation
	Page_Internal* shallocNode = (Node*)((char*)&myblock[max_size - 1] - (sizeof(Node) + 4*page_size)); //backtrack four pages
	shallocNode->space = 4*page_size; //create shalloc metadata
	shallocNode->used = FALSE;
	shallocRegion = shallocNode;
}





//Traverses the array, searching for sections with size 'size', and returning it.
//If no partition is found, and the index has overreached, then it returns NULL
Page_Internal* traversePageInternals(int size, Node* metadata){
    Page_Internal* PI =  (Page_Internal*)metadata->page_start_addr;
    //if page is not used
    if(PI->used == FALSE){
    	if((size + 2*sizeof(Page_Internal)) > page_size){
    		printf("In mymalloc()-->traversePageInternals(), requested size and metadata does not fit in page.\n");
    		return NULL;
    	}
    	return PI;
    }

    while((void*)PI != (void*)metadata->page_end_addr){ //check if & is ness
    	//if the location is not used, has space greater than the size requested, and is inside the page (THREAD_PTR)
    	if(PI->used == FALSE && PI->space >= size + sizeof(Page_Internal)){
    		return PI;
    	}
    	PI = (Page_Internal*)((char*)(PI+1) + PI->space);
    }
    return NULL;
}






//When a valid partition is found, a node is created to hold the space after the partition, and
//the current node's size is readjusted to reflect the size partitioned
void* allocateNewInsidePage(Page_Internal* PI, int size){
	Page_Internal* tempPI = NULL;
	//Calculate the space left over after allocating enough for the current request
	int totalspace = PI->space;
	totalspace = totalspace - (size + sizeof(Page_Internal));
	//temptr is a pointer that will increment to the location after the requested space
	//and create a new node to indicate the space left
	if(totalspace>0){
    tempPI = PI;
    tempPI = (Page_Internal*)((char*)(tempPI+1) + size);
    tempPI->space = totalspace;
    tempPI->used = FALSE;
	}

    //the input to the function is then adjusted to indicate the correct size requested
    //and its active 'used' flag
    PI->space = size;
    PI->used = TRUE;
    //a void pointer to the space after the node(the space requested), is returned to the main malloc function
    void* ptr = (void*)(PI+1);
    return ptr;
}

//TODO change NUMBER OF THREADS to NUMBER OF PAGES
//if thread page is found, returns page metadata
void* findThreadPage(gthread_task_t *owner){
	int number_of_threads = threads_allocated;
	Node* page = meta_start;
	while(number_of_threads > 0){
		//page found, return
		if(page->thread == owner){
			return page;
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
	Page_Internal* nodestart;
	while(metacounter<numb_of_pages){
		if(page->thread == NULL){
			page->used = TRUE;
			page->thread = owner;
			nodestart = (Page_Internal*)page->page_start_addr;
			nodestart->space = (page_size - sizeof(Page_Internal));
			nodestart->used = FALSE;
			threads_allocated++;
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
    Page_Internal* nodeptr = traversePageInternals(size, page);
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
Page_Internal* checkLeft(Page_Internal* ptr, Node* pageMetaData){
	//ptr is already the left most node
if(ptr == NULL){
	//error
	return NULL;
}

Page_Internal* leftp = (Page_Internal*)pageMetaData->page_start_addr;
//while the next node does not = the node currently being worked on
while(((Page_Internal*)((char*)(leftp+1)+leftp->space)) != ptr) {
	leftp = (Page_Internal*)((char*)(leftp+1)+leftp->space);
}
return leftp;
}


//Checks right of the pointer, if a node exists, returns it
//used for concatenation of two unused nodes in the main free function
//returns a pointer to the node that is after the node being freed
Page_Internal* checkRight(Page_Internal* ptr, Node* pageMetaData){
if(ptr == NULL){
    //error
    return NULL;
	}
		//ptr is already the rightmost node
Page_Internal* temp = (Page_Internal*)((char*)(ptr+1)+ptr->space);
if((void*)(temp) ==(void*)pageMetaData->page_end_addr){
	return NULL;
}
return temp;
}



//checks if the pointer inputted in the 'free()' function is inside the memory array
//returns TRUE if it is, false if it is not
BOOLEAN does_pointer_exist(void* p, gthread_task_t* owner){
Node* page = findThreadPage(owner);
if(page == NULL){
	return FALSE;
}
Node* current = (Node*)page->page_start_addr;
//current is not at the end
while((void*)current != (void*)page->page_end_addr){
    if((void*)(current+1) == p ){
    return TRUE;}
    current = (Node*)((char*)(current+1)+current->space);
}
return FALSE;
}



void check_and_free_Page(Node* page){
Page_Internal* PI = (Page_Internal*)page->page_start_addr;
if(PI->space == (page_size-sizeof(Page_Internal)) && PI->used == FALSE){
	page->thread = NULL;
	page->used = FALSE;
}
}


//Frees the usage of the space taken up by this pointer, changes
//the metadata associated with it to inactive (used = FALSE), and concatenates
//the space with any adjacent unused nodes.
void myfree(void* p, gthread_task_t *owner){
	//check if p is in shalloc region
	if(p > (void*)shallocRegion){
		myfreeShalloc(p);
		return;
	}
    //checks if pointer is in the array
    BOOLEAN valid = does_pointer_exist(p, owner);

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
	check_and_free_Page(page);

	return;
}


