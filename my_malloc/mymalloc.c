#include "mymalloc.h"

static const long max_size = MAX_SIZE; //Maximum size for Virtual Memory
static const long swap_size = SWAP_SIZE;
extern char myblock[MAX_SIZE] = {}; //8MB memory block
extern char swapblock[SWAP_SIZE] = {}; //16 MB Swap Block
static size_t page_size; //Page size for system
static int threads_allocated; //number of threads that allocated space in Virtual Memory currently
static int pages_allocated;
static void* shallocRegion; //first shalloc metadata
static Node* meta_start; //the first page metadata
static Node* swap_meta_start;
static int numb_of_pages; //number of total pages in the block (can be empty pages)
static int numb_of_swap_pages;
static BETWEEN_OR_END allocationFlag;
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
		printf("THREAD: %p,PAGE OFFSET: %d, PAGE SADDR: %d, PAGE EDDR: %d\n", page->thread,page->page_offset,page->page_start_addr, page->page_end_addr);
		}
		page = page + 1;
		numbpages --;
	}
	page = (Node*)swap_meta_start;
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

}


//Prints the internal memory of the current thread (size allocated, block used, etc)
void printInternalMemory(gthread_task_t* owner){
	placePagesContig(owner);
	Node* page = findThreadPage(owner);
	if(page == NULL){
		return;
	}
	Page_Internal* PI = (Page_Internal*)page->page_start_addr;
	printf("============================================================\n");
	while((void*)PI != (void*)getEndAddr(owner,NULL)){
		if(PI->used == TRUE){
			printf("PI STARTING ADDR: %d, PI ENDING ADDR: %d\n",(void*)PI, (void*)getNextPI(PI));
			printf("BLOCK USED: TRUE  BLOCK SIZE: %d\n", PI->space);
		}
		else{
			printf("PI STARTING ADDR: %d, PI ENDING ADDR: %d\n",(void*)PI, (void*)getNextPI(PI));
			printf("BLOCK USED: FALSE  BLOCK SIZE: %d\n", PI->space);
		}
		PI = getNextPI(PI);
	}
	printf("============================================================\n");
}


//gets space left in total memory block.
int getSpaceLeft(){
	int pagesLeft = (numb_of_swap_pages + numb_of_pages) - pages_allocated;
	int bytesleft = pagesLeft * (page_size);
	return bytesleft;
}




void clearPage(Node* page){
	memset(page->page_start_addr, 0, page_size);
	return;
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
		metadata->page_offset = 0;
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
		swapmetadata->page_offset = 0;
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
	if(source == NULL || target == NULL){
		return;
	}
	void* tempstartaddr = source->page_start_addr;
	void* tempendaddr = source->page_end_addr;
	char temp[page_size]; //temporary holder of source page
	memcpy((void*)temp, source->page_start_addr, page_size); //copy source to temporary array
	memcpy(source->page_start_addr, target->page_start_addr, page_size); //copy target page over to source page
	memcpy(target->page_start_addr, (void*)temp, page_size); //copy source page over to target page
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
	if(page==NULL){
		return 0;
	}

	Node* swapout = NULL;
	while(page != NULL){
		n = page->page_offset;
		swapout = findNthPage(n);
		swapPages(swapout, page);
		page = page->next_page;
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
	Node* swapmeta = (Node*)swap_meta_start;
	metacounter = 0;
	while(metacounter < numb_of_swap_pages){
		if(swapmeta->thread == NULL){
			return swapmeta;
		}
		swapmeta = swapmeta + 1;
		metacounter++;
	}
	return NULL;
}

//returns the number of empty pages left in the memory block
int howManyEmptyPages(){
	int emptyP = (numb_of_pages+numb_of_swap_pages) - pages_allocated;
	return emptyP;
}



//Traverses the array, searching for sections with size 'size', and returning it.
//If no partition is found, and the index has overreached, then it returns NULL
Page_Internal* findOpenSpace(int size, Node* pageMData){
	debug("In findOpenSpace()");
	Page_Internal* PI = (Page_Internal*)pageMData->page_start_addr;
	//find open space in between
	while((void*)getNextPI(PI) != getEndAddr(NULL, pageMData)){
		if(PI->used == FALSE && PI->space > sizeof(Page_Internal) + size){
			debug("Allocation found to be in between.");
			printf("Found PI with starting address %d and space %d\n", (void*)PI, PI->space);
			allocationFlag = BETWEEN;
			return PI;
		}

		PI = getNextPI(PI);
	}

	//find open space at the end
	printf("SPACELEFT : %d\n", getSpaceLeft());
	if(sizeof(Page_Internal) + size < getSpaceLeft() + PI->space && PI->used == FALSE){
		debug("Allocation found to be in end.");
		printf("Found PI with starting address %d and space %d\n", (void*)PI,PI->space);
		allocationFlag = END;
		return PI;
	}
	return NULL;

}




//When a valid partition is found, a node is created to hold the space after the partition, and
//the current node's size is readjusted to reflect the size partitioned
void* allocate(Page_Internal* PI, int size, gthread_task_t* owner){
	printf("IN ALLOCATE(), size=%d\n",size);
		int spacecalc;
		Page_Internal* unusedPI; //PI for PI at the end of an used block
		int spaceleft = PI->space; //calculation of spaceleft
		printf("PI->space %d\n",spaceleft);
		Node* firstPage = findThreadPage(owner); //get the first page of the thread
		Node* Currentpage = whichPage(PI); //findout which page contains the block that was found to be open
		int Offset = Currentpage->page_offset;
		//space calculations
		int spaceNeeded = size + sizeof(Page_Internal);
		int spaceToEnd = abs(((char*)(PI+1) - (char*)Currentpage->page_end_addr));
		printf("Space to End of Current Page is %d\n",spaceToEnd);
		spaceNeeded = spaceNeeded - spaceToEnd;
		//if spaceNeeded is negative, then no new pages are needed
		//spaceNeeded is positive, new pages are needed
		Node* tempPage;
		int empty_pages_left = howManyEmptyPages();
		int pagesNeeded = spaceNeeded / page_size;
		//checks if the pages needed are able to be allocated
		if(pagesNeeded > empty_pages_left){
			return NULL;
		}
		Node* savedNextPage = Currentpage->next_page;
		int PagesCreated = 0;
		while(spaceNeeded > 0){
			PagesCreated = 1;
			tempPage = getEmptyPage();
			Currentpage->next_page = tempPage;
			tempPage->first_page = firstPage;
			tempPage->thread = owner;
			Offset++;
			tempPage->page_offset = Offset;
			Currentpage = tempPage;
			spaceNeeded = spaceNeeded - page_size;
			pages_allocated++;
			if(spaceNeeded < 0){
				spaceNeeded = spaceNeeded + page_size; //make positive again
				tempPage->next_page = savedNextPage;
				break;
			}
		}
		placePagesContig(owner);
		PI->space = size;
		PI->used = TRUE;
		if(allocationFlag == END){
			unusedPI = getNextPI(PI);
			unusedPI->used = FALSE;
			if(PagesCreated != 0){
			unusedPI->space = page_size - spaceNeeded;
			}
			else{
			unusedPI->space	= spaceToEnd - (size + sizeof(Page_Internal));
			}

		}
		else if(allocationFlag == BETWEEN){
			Page_Internal* unusedPI = getNextPI(PI);
			unusedPI->used = FALSE;
			unusedPI->space = spaceleft - (size+sizeof(Page_Internal));
		}
		printf("ALLOCATE PI SADDR: %d, PI EDDR: %d, PI->Space: %d\n", (void*)(PI), (void*)getNextPI(PI),PI->space);
		printf("ALLOCATE UnusedPI SADDR: %d, UnusedPI EDDR: %d, UnusedPI->Space: %d\n",(void*)unusedPI, (void*)getNextPI(unusedPI),unusedPI->space);

		firstPage->space_allocated = firstPage->space_allocated + size;
		printf("Total space allocated %d\n", firstPage->space_allocated);
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
			page->page_offset = 0;
			nodestart = (Page_Internal*)page->page_start_addr;
			nodestart->space = (page_size - sizeof(Page_Internal));
			nodestart->used = FALSE;
			threads_allocated++;
			pages_allocated++;
			return page;
}



//mymalloc
void* mymalloc(size_t size, gthread_task_t *owner){
	printf("+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+\n");
	printf("DEBUG: Size request %ld, gthread ID: %p\n", size, (void*)owner);
	allocationFlag = NONE;
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
    	printf("Allocation request will reach memory capacity if allocated. Denied allocation request.\n");
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
    printf("Success\n");
    if(ptr == NULL){
    	printf("Unsuccessful allocation attempt, likely not enough free pages were found\n");
    }
    //returns void pointer to the caller
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
		if((void*)getNextPI(PI) == getEndAddr(owner, NULL)){
			break;
		}
	}
	return NULL;
}


//concatenates adjacent unused page_internal nodes
//returns the concatenated Page_Internal
Page_Internal* concatenate(Page_Internal* PI, gthread_task_t* owner){
	Page_Internal* prevPI = (Page_Internal*)findThreadPage(owner)->page_start_addr;
	Page_Internal* nextPI = getNextPI(PI);
	Page_Internal* returnPI = PI;
	if((void*)nextPI != (void*)getEndAddr(owner,NULL) && nextPI->used == FALSE){
		PI->space = PI->space + nextPI->space + sizeof(Page_Internal);
		returnPI = PI;
	}
	while(getNextPI(prevPI) != PI){
		if(prevPI == PI){
			return returnPI;
		}
		prevPI = getNextPI(prevPI);
	}
	if(prevPI->used == FALSE){
		prevPI->space = prevPI->space + PI->space + sizeof(Page_Internal);
		returnPI = prevPI;
	}
	return returnPI;
}




void FreePages(Page_Internal* PI, int freePages, Node* startingPage){
	debug("FREEPAGES + FREEPAGES + FREEPAGES + FREEPAGES + FREEPAGES====================");
	printf("PI SADDR %d, PI EDDR %d\n", (void*)PI, (void*)getNextPI(PI));
	printf("FreePages: %d\n", freePages);
	printf("First Page to be cleared: %d, Page Offset %d\n", (void*)startingPage->page_start_addr, startingPage->page_offset);
	//find page before the page that is set to be freed
	void* PI_starting_addr = (void*)PI;
	void* PI_ending_addr = (void*)getNextPI(PI);
	Node* firstPage = startingPage->first_page;
	Node* prevPage = firstPage;
	if(prevPage != startingPage){
		while(prevPage->next_page != startingPage){
			prevPage = prevPage->next_page;
		}
	}

	Node* page_iterator = (Node*)prevPage;
	Node* nextPage = NULL;
	//clear EVERYTHING if none of the pages are being used
	if(PI_starting_addr == firstPage->page_start_addr && PI_ending_addr == getEndAddr(NULL, firstPage)){
		//free all the pages
		debug("Condition for FULL CLEAR to the end");
		while(page_iterator != NULL){
			nextPage = page_iterator->next_page;
			page_iterator->first_page = NULL;
			page_iterator->next_page = NULL;
			page_iterator->thread = NULL;
			page_iterator->page_offset = 0;
			pages_allocated--;
			page_iterator = nextPage;
		}
		threads_allocated--;
		return;
	}
	else if(PI_starting_addr == firstPage->page_start_addr){
		//first Page involved in clear, but first page must be kept to keep reference
		debug("first Page involved but not cleared");
			freePages--;
			startingPage = startingPage->next_page;
		}
	printf("prevPage offset is: %d\n", prevPage->page_offset);
	while(freePages>0 && startingPage != NULL){
		debug("STANDARD CLEAR");
		prevPage->next_page = startingPage->next_page;
		if(prevPage->next_page == NULL){
			printf("PrevPage's next page is NULL\n");
		}
		startingPage->first_page = NULL;
		startingPage->next_page = NULL;
		startingPage->thread = NULL;
		startingPage->page_offset = 0;
		pages_allocated--;
		startingPage = prevPage->next_page;

		freePages--;
	}
	if(allocationFlag == END){
		while((void*)getNextPI(PI) != getEndAddr(NULL, firstPage)){
			PI->space = PI->space - page_size;
		}
	}
	printf("Page allocated %d\n", pages_allocated);
	printf("End of page ADDRESS: %d\n", (void*)getEndAddr(NULL,prevPage));
	printf("End of end Page_internal Address %d\n", (void*)getNextPI(PI));
	debug("Returning from FreePages()");
	debug("FREEPAGES + FREEPAGES + FREEPAGES + FREEPAGES + FREEPAGES++++++++++++++++++++++++");
}



void clearUnusedPages(Page_Internal* PI, gthread_task_t* owner){
	Node* page = findThreadPage(owner);
	Node* page_iterator = (Node*)page;
	Node* startingFreePage = NULL;
	debug("In clearUnusedPages()");
	void* PI_ending_addr = (void*)getNextPI(PI);
	int freePages = 0;

	//if the freed Page_Internal is at the end
	if(PI_ending_addr == getEndAddr(owner,NULL)){
		debug("Condition for PARTIAL CLEAR to the end");
		while(page_iterator != NULL){
				//(PI+1) is used to insure that PI data does not get corrupted if the page starting address is inbetween PI and PI+1
				if(page_iterator->page_start_addr >= (void*)(PI + 1)){
					if(freePages == 0){
						startingFreePage = page_iterator;
					}
					freePages++;
				}
				page_iterator = page_iterator->next_page;
			}
		if(freePages > 0){
			allocationFlag = END;
			FreePages(PI, freePages, startingFreePage);
		}
		return;
	}
	else{
		debug("Condition for PARTIAL CLEAR IN BETWEEN");
		//iterate through pages of this thread to see if the pages belong to the free'd page_internal
		while(page_iterator != NULL){
			//(PI+1) is used to insure that PI data does not get corrupted if the page starting address is inbetween PI and PI+1
			if(page_iterator->page_start_addr >= (void*)(PI) && page_iterator->page_end_addr <= (void*)getNextPI(PI)){
				if(freePages == 0){
					startingFreePage = page_iterator;
				}
				freePages++;
			}
			page_iterator = page_iterator->next_page;
		}
		if(freePages > 0){
			allocationFlag = BETWEEN;
			FreePages(PI, freePages, startingFreePage);
		}
		return;
	}

	return;
}



//Frees the usage of the space taken up by this pointer, changes
//the metadata associated with it to inactive (used = FALSE), and concatenates
//the space with any adjacent unused nodes.
void myfree(void* p, gthread_task_t *owner){
	printf("IN MYFREE(), gthread ID: %p\n", (void*)owner);
	allocationFlag = NONE;
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
	//clearUnusedPages(PI,owner);
	//concatenate any unused regions in memory
	Page_Internal* concatenatedPI = concatenate(PI, owner);
	clearUnusedPages(concatenatedPI, owner);
	return;
}


