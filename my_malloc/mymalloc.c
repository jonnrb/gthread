#include "mymalloc.h"

static const long max_size = MAX_SIZE; //Maximum size for Virtual Memory
static const long swap_size = SWAP_SIZE;
extern char myblock[MAX_SIZE] = {}; //8MB memory block
extern char swapblock[SWAP_SIZE] = {}; //16 MB Swap Block
static long page_size; //Page size for system
static int threads_allocated; //number of threads that allocated space in Virtual Memory currently
static int pages_allocated;
static void* shallocRegion; //first shalloc metadata
static Node* meta_start; //the first page metadata
static Node* swap_meta_start;
static int numb_of_pages; //number of total pages in the block (can be empty pages)
static int numb_of_swap_pages;
Node* findThreadPage(gthread_task_t *owner);
void* getEndAddr(gthread_task_t* owner, Node* inppage);



/////////////////////////GENERAL FUNCTIONS////////////////////////////////////////



//prints out in format "DEBUG: INPUT STRING"
void debug(char* str){
	printf("DEBUG: %s\n", str);
	return;
}


//pointer arithmatic to get new Page_Internal node.
Page_Internal* getNextPI(Page_Internal* PI){
	return (Page_Internal*)((char*)(PI+1) + PI->space);
}


//gets end of address space for the current thread
void* getEndAddr(gthread_task_t* owner, Node* inppage){
	Node* page;
	if(inppage != NULL){
		page = inppage ->first_page;
	}
	else{
	    page = findThreadPage(owner);
	}
	if(page == NULL){
		return NULL;
	}
	while(page->next_page != NULL){
		page = page->next_page;
	}
	return page->page_end_addr;
}

//prints all the allocated pages for threads, marking the current thread with "CURRENT THREAD: string"
void printThread(gthread_task_t* owner){
	Node* page = (Node*)meta_start;
	int numbpages = numb_of_pages;
	while(numbpages > 0){
		if(page->thread != NULL){
			if(page->thread == owner){
				printf("CURRENT THREAD -->>>");
			}
		printf("THREAD: %p, PAGE START ADDRESS: %d, PAGE END ADDRESS: %d\n", page->thread, page->page_start_addr, page->page_end_addr);
		}
		page = page + 1;
		numbpages --;
	}
	/*page = (Node*)swap_meta_start;
	numbpages = numb_of_swap_pages;
	while(numbpages > 0){
		if(page->thread != NULL){
			if(page->thread == owner){
				printf("CURRENT THREAD -->>>");
			}
		printf("THREAD: %p, PAGE START ADDRESS: %d, PAGE END ADDRESS: %d\n", page->thread, page->page_start_addr, page->page_end_addr);
		}
		page = page + 1;
		numbpages --;
	}
	*/
}


//Prints the internal memory of the current thread (size allocated, block used, etc)
void printInternalMemory(gthread_task_t* owner){
	Node* page = findThreadPage(owner);
	Page_Internal* PI = (Page_Internal*)page->page_start_addr;
	while((void*)PI != (void*)getEndAddr(owner,NULL)){
		if(PI->used == TRUE){
			printf("BLOCK USED: TRUE  BLOCK SIZE: %d\n", PI->space);
		}
		else{
			printf("BLOCK USED: FALSE  BLOCK SIZE: %d\n", PI->space);
		}
		PI = getNextPI(PI);
	}
}


//gets space left in total memory block.
int getSpaceLeft(){
	int pagesLeft = numb_of_pages - pages_allocated;
	int bytesleft = pagesLeft * (page_size);
	return bytesleft;
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
	int max = max_size - (sizeof(Page_Internal) + 4*page_size); //decrement by shalloc space
	double ratio = (double)sizeof(Node)/(double)page_size;
	double metaSpace = (double)max*ratio;
	metaSpace = ceil(metaSpace);
	numb_of_pages = (int)metaSpace/sizeof(Node);
	max = max - metaSpace;
	int maxaddr = (int)max;
	meta_start = (Node*)&myblock[maxaddr-1];
}

//sets metadata start address and number of pages expected on this machine for swap block
void swap_metadata_start_addr(){
	int max = swap_size; //decrement by shalloc space
	double ratio = (double)sizeof(Node)/(double)page_size;
	double metaSpace = (double)max*ratio;
	metaSpace = ceil(metaSpace);
	numb_of_swap_pages = (int)metaSpace/sizeof(Node);
	max = max - metaSpace;
	int maxaddr = (int)max;
	swap_meta_start = (Node*)&swapblock[maxaddr-1];
}






//returns for other source files to get static variables (extern seems to not work)
void* getShallocRegion(){
	return shallocRegion;
}

//returns page size
int getPageSize(){
	return page_size;
}
//returns address of region where metadata starts in memory block
void* getMetaStart(){
	return (void*)meta_start;
}
//returns address of region where metadata starts in swap file block
void* getSwapMetaStart(){
	return (void*)swap_meta_start;
}




//initializes the array
//creates a start node, and an end node
void initblock(){
	threads_allocated = 0; //initializes threads allocated counter
	metadata_start_addr(); //sets meta_start and number_of_pages
	swap_metadata_start_addr();
	Node* metadata = (Node*)meta_start;
	//creating metadata for memoryblock
	int metaIterator = 0;
	while(metaIterator < numb_of_pages){
		metadata->thread = NULL;
		metadata->first_page = NULL;
		metadata->next_page = NULL;
		metadata->page_start_addr = (void*)(&myblock[0] + page_size*metaIterator);
		metadata->page_end_addr = (void*)(&myblock[0] + page_size*(metaIterator+1));
		metadata = metadata+1;
		metaIterator++;
	}
	//creating metadata for swapfileblock
	Node* swapmetadata = (Node*)swap_meta_start;
	metaIterator = 0;
	while(metaIterator < numb_of_swap_pages){
		swapmetadata->thread = NULL;
		swapmetadata->first_page = NULL;
		swapmetadata->next_page = NULL;
		swapmetadata->page_start_addr = (void*)(&swapblock[0] + page_size*metaIterator);
		swapmetadata->page_end_addr = (void*)(&swapblock[0] + page_size*(metaIterator+1));
		swapmetadata = swapmetadata+1;
		metaIterator++;
	}


	//shalloc region creation
	Page_Internal* shallocNode = (Page_Internal*)((char*)&myblock[max_size - 1] - (sizeof(Page_Internal) + 4*page_size)); //backtrack four pages
	shallocNode->space = 4*page_size; //create shalloc metadata
	shallocNode->used = FALSE;
	shallocRegion = (void*)shallocNode;
}



//returns the page in which the PageInternal pointer belongs to
Node* whichPage(Page_Internal* PI){
	void* addr = (void*)PI;
	Node* page = (Node*)meta_start;
	//start = lower address end = higher address
	void* start; void* end;
	int numbpages = numb_of_pages;
	while(numbpages>0){
		end = page->page_end_addr;
		start = page->page_start_addr;
		if(addr >= start && addr < end){
			return page;
		}
		page = page + 1;
		numbpages--;
	}
	numbpages = numb_of_swap_pages;
	page = (Node*)swap_meta_start;
	while(numbpages>0){
			end = page->page_end_addr;
			start = page->page_start_addr;
			if(addr >= start && addr < end){
				return page;
			}
			page = page + 1;
			numbpages--;
		}
	return NULL;
}


void swapPages(Node* source, Node* target){
	if(source == target){
		return;
	}
	void* tempstartaddr = source->page_start_addr;
	void* tempendaddr = source->page_end_addr;
	char temp[page_size]; //temporary holder of source page
	memmove((void*)temp, source->page_start_addr, page_size); //copy source to temporary array
	memmove(source->page_start_addr, target->page_start_addr, page_size); //copy target page over to source page
	memmove(target->page_start_addr, (void*)temp, page_size); //copy source page over to target page
	source->page_start_addr = target->page_start_addr;
	source->page_end_addr = target->page_end_addr;
	target->page_start_addr = tempstartaddr;
	target->page_end_addr = tempendaddr;
	//swap complete
	return;
}


//finds metadata of the nth page in the memory block
Node* findNthPage(int n){
	void* start = (void*)&myblock[0];
	start = (void*)((char*)start + n*page_size);
	Node* meta = (void*)meta_start;
	while(meta->page_start_addr != start){
		meta = meta + 1;
	}
	return meta;
}

//takes in metadata, and places all the pages associated with the thread of the metadata
//at address 0 of mem block, contiguously (for malloc call)
int placePagesContig(gthread_task_t* owner){
	int n = 0;
	Node* page = findThreadPage(owner);
	Node* swapout = NULL;
	while(page != NULL){
		swapout = findNthPage(n);
		swapPages(swapout, page);
		page = page->next_page;
		n = n + 1;
	}
	return n;

}

//if thread page is found, returns page metadata
Node* findThreadPage(gthread_task_t *owner){
	int numbpages = numb_of_pages;
	Node* page = (Node*)meta_start;
	while(numbpages > 0){
		//page found, return
		if(page->thread == owner){
			return (page->first_page);
		}
		//else iterate
		numbpages--;
		page = page + 1;
	}
	//traverse swap pages
	numbpages = numb_of_swap_pages;
	page = (Node*)swap_meta_start;
	while(numbpages>0){
		if(page->thread == owner){
			return (page->first_page);
		}
		numbpages--;
		page = page + 1;
	}
return NULL;
}






//////////////////////////////////////////////////////////////////////
//MALLOC//
//////////////////////////////////////////////////////////////////////






//Traverses the array, searching for sections with size 'size', and returning it.
//If no partition is found, and the index has overreached, then it returns NULL
//sets freespace
Page_Internal* findOpenSpace(int size, Node* pageMData){
	Page_Internal* PI = (Page_Internal*)pageMData->page_start_addr;
	while(PI->nextPI != NULL){
		if(PI->used == FALSE && PI->space > sizeof(Page_Internal) + size){
			return PI;
		}
		PI = getNextPI(PI);
	}
	if(sizeof(Page_Internal) + size < getSpaceLeft() + PI->space && PI->used == FALSE){
		return PI;
	}
	return NULL;

}


//returns an empty page in the memory block. Returns NULL if empty page not found
Node* getEmptyPage(){
	Node* metadata = (Node*)meta_start;
	int metacounter = 0;
	while(metacounter < numb_of_pages){
		if(metadata->thread == NULL){
			return metadata;
		}
		metadata = metadata + 1;
		metacounter++;
	}
	//swap file check
	metadata = (Node*)swap_meta_start;
	metacounter = 0;
	while(metacounter < numb_of_swap_pages){
		if(metadata->thread == NULL){
			return metadata;
		}
		metadata = metadata + 1;
		metacounter++;
	}
	return NULL;
}

//returns the number of empty pages left in the memory block
int howManyEmptyPages(){
	int emptyP = (numb_of_pages+numb_of_swap_pages) - pages_allocated;
	return emptyP;
}



//When a valid partition is found, a node is created to hold the space after the partition, and
//the current node's size is readjusted to reflect the size partitioned
void* allocate(Page_Internal* PI, int size, gthread_task_t* owner){
	printf("IN ALLOCATE(), size=%d\n",size);
	Page_Internal* unusedPI;
	int spaceleft;
	Node* firstPage = findThreadPage(owner);
	//don't need to allocate new pages
	if(PI->space > sizeof(Page_Internal) + size){
		spaceleft = PI->space - (sizeof(Page_Internal) + size);
		unusedPI = (Page_Internal*)((char*)(PI+1) +  size);
		unusedPI->space = spaceleft;
		unusedPI->used = FALSE;
		unusedPI->nextPI = NULL;
		PI->nextPI = unusedPI;
		PI->used = TRUE;
		PI->space = size;
	}
	//must allocate new pages
	else{
		//problem lies somewhere in here
		Node* page = whichPage(PI);
		int spacecalc = size;
		spacecalc = spacecalc - PI->space; //size after end of current page
		Node* tempPage;


		int empty_pages_left = howManyEmptyPages();
		double pagesNeeded = ceil((double)spacecalc/(double)page_size);
		if(pagesNeeded > empty_pages_left){
			return NULL;
		}

		while(spacecalc > 0){
			tempPage = getEmptyPage();
			page->next_page = tempPage;
			tempPage->first_page = firstPage;
			tempPage->thread = owner;
			page = tempPage;
			spacecalc = spacecalc - page_size;
			pages_allocated++;
		}
		//at this point spacecalc is negative so must make positive by adding page_size
		spacecalc = spacecalc + page_size;
		spaceleft = page_size - (spacecalc + sizeof(Page_Internal));
		unusedPI = (Page_Internal*)((char*)page->page_start_addr + spacecalc);
		unusedPI->space = spaceleft;
		unusedPI->used = FALSE;
		unusedPI->nextPI = NULL;
		PI->nextPI = unusedPI;
		PI->used = TRUE;
		PI->space = size;
		placePagesContig(owner);
	}
	firstPage->space_allocated = firstPage->space_allocated + size;
	return (void*)(PI+1);
}








//creates first page for thread
Node* createThreadPage(gthread_task_t *owner){
	debug("In createThreadPage()");
	Node* page = getEmptyPage();
	if(page == NULL){
		return NULL;
	}
			Page_Internal* nodestart;
			page->thread = owner;
			page->first_page = page;
			page->space_allocated = 0;
			nodestart = (Page_Internal*)page->page_start_addr;
			nodestart->space = (page_size - sizeof(Page_Internal));
			nodestart->used = FALSE;
			nodestart->nextPI = NULL;
			threads_allocated++;
			pages_allocated++;
			return page;
}



//mymalloc
void* mymalloc(size_t size, gthread_task_t *owner){
	printf("+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+\n");
	printf("DEBUG: Size request %ld, gthread ID: %p\n", size, (void*)owner);
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

    //initialize entire memory array, create metadata nodes at the end of the memory block (before shalloc region)
    if(meta_start == NULL){
    	initblock();
    }


    //traverse array until the pages for the requested thread is found, if
    //not found, create new thread page.
    Node* page = findThreadPage(owner);
    //printf("Page starting addr: %d\n", page->page_start_addr);
    if(page == NULL){ //if thread does not have an allocated space
    	page = createThreadPage(owner);
    }
    if(page == NULL){ //no space for thread left in VM
    	printf("No space left in VM for requested thread Memory Allocation.\n");
    	return NULL;
    }
    if(page->space_allocated + size > numb_of_pages*page_size){
    	//not enough space left
    	printf("Allocation request will reach memory capcity if allocated. Denied allocation request.\n");
    	return NULL;
    }
    //places pages contiguously at the start of the array
    placePagesContig(owner);

    //traverse array until free space is found
    Page_Internal* PI = findOpenSpace(size, page);
    //if the end is reached, no valid space is found
    if(PI == NULL){
    	printf("Invalid space request, capacity overflow.\n");
    	return NULL;
    }
    ptr = allocate(PI, size, owner);
    if(ptr == NULL){
    	printf("Unsuccessful allocation attempt, likely not enough free pages were found\n");
    }
    //returns void pointer to the caller
    printf("Pages allocated %d\n", pages_allocated);
    return ptr;
}

//////////////////////////////////////////////////////////////////////////////////////
//FREE//
/////////////////////////////////////////////////////////////////////////////////////






//checks if the pointer inputted in the 'free()' function is inside the memory array
//returns TRUE if it is, false if it is not
Page_Internal* does_pointer_exist(void* p, gthread_task_t* owner){
	Node* page = findThreadPage(owner);
	if(page == NULL){
		return NULL;
	}
	Page_Internal* PI = (Page_Internal*)page->page_start_addr;
	while(1){
		if((void*)(PI+1) == (void*)p){
			return PI;
		}
		PI = getNextPI(PI);
		if(PI->nextPI == NULL){
			break;
		}
	}
	return NULL;
}


//concatenates adjacent unused page_internal nodes
void concatenate(Page_Internal* PI, gthread_task_t* owner){
	Page_Internal* prevPI = (Page_Internal*)findThreadPage(owner)->page_start_addr;
	Page_Internal* nextPI = getNextPI(PI);

	if((void*)nextPI != (void*)getEndAddr(owner,NULL) && nextPI->used == FALSE){
		PI->nextPI = nextPI->nextPI;
		PI->space = PI->space + nextPI->space + sizeof(Page_Internal);
	}
	while(getNextPI(prevPI) != PI){
		if(prevPI == PI){
			return;
		}
		prevPI = getNextPI(prevPI);
	}
	if(prevPI->used == FALSE){
		prevPI->nextPI = PI->nextPI;
		prevPI->space = prevPI->space + PI->space + sizeof(Page_Internal);
	}
}




void FreePages(Page_Internal* PI, int freePages, Node* startingPage){
	PI->space = PI->space - freePages*page_size;
	Node* prevPage = startingPage->first_page;
	if(prevPage!=startingPage){
	while(prevPage->next_page != startingPage){
		prevPage = prevPage->next_page;
	}
	}

	while(freePages>0 || startingPage != NULL){
		if(prevPage != startingPage){
			prevPage->next_page = startingPage->next_page;
		}
		startingPage->first_page = NULL;
		startingPage->next_page = NULL;
		startingPage->thread = NULL;
		pages_allocated--;
		startingPage = prevPage->next_page;
		freePages--;
	}
}



void clearUnusedPages(Page_Internal* PI, gthread_task_t* owner){
	Node* page = findThreadPage(owner);
	Node* page_iterator = (Node*)page;
	Node* startingFreePage = NULL;
	void* PI_starting_addr = (void*)PI;
	void* PI_ending_addr = (void*)getNextPI(PI);
	int freePages = 0;
	while(page->next_page != NULL){
		if(PI_starting_addr <= page->page_start_addr && PI_ending_addr >= page->page_end_addr){
			if(freePages == 0){
				startingFreePage = page;
			}
			freePages++;
		}
	}
	FreePages(PI, freePages, startingFreePage);

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
	//place pages contiguously from the owner
	placePagesContig(owner);
    //checks if pointer is in the array
    Page_Internal* PI = does_pointer_exist(p, owner);

    //if pointer is not in the array, throw error, return
    if(PI == NULL){
        printf("Invalid input pointer. Pointer must be at the start of a previously allocated area of the memory array\n");
        return;
    }


	//At this point, the pointer exists, and the node corresponding to it is valid
	//The node's flag is set to unused = 'FALSE'
	PI -> used = FALSE;
	Node* firstPage = findThreadPage(owner);
	//adjusts total space allocated
	firstPage->space_allocated = firstPage->space_allocated - PI->space;
	clearUnusedPages(PI,owner);
	//concatenate any unused regions in memory
	concatenate(PI, owner);

	return;
}


