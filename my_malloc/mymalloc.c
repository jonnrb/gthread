#include "my_malloc/mymalloc.h"

#include <fcntl.h>
#include <sys/mman.h>

#include "concur/mutex.h"
#include "my_malloc/log.h"
#include "my_malloc/protection.h"

#define MAX_SIZE 8388608
#define SWAP_SIZE 16777216

static gthread_mutex_t malloc_mu;

size_t max_size = -1; //Maximum size for Virtual Memory
void* myblock = NULL; //8MB memory block
void* myblock_userdata = NULL;

size_t swap_size = -1;
void* swapblock = NULL; //16 MB Swap Block
void* swapblock_userdata = NULL;

size_t page_size = 0; //Page size for system

int threads_allocated; //number of threads that allocated space in Virtual Memory currently
int pages_allocated;
void* shallocRegion; //first shalloc metadata
Node* meta_start; //the first page metadata
Node* swap_meta_start;
int numb_of_pages; //number of total pages in the block (can be empty pages)
int numb_of_swap_pages;

void* getEndAddr(gthread_task_t* owner, Node* inppage);

/////////////////////////GENERAL FUNCTIONS////////////////////////////////////////

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
			debug("%s THREAD: %p,PAGE OFFSET: %d, PAGE SADDR: %p, PAGE EDDR: %p", page->thread == owner ? "C ->" : "    ", page->thread,page->page_offset,page->page_start_addr, page->page_end_addr);
		}
		page = page + 1;
		numbpages --;
	}
	page = (Node*)swap_meta_start;
	numbpages = numb_of_swap_pages;
	while(numbpages > 0){
		if(page->thread != NULL){
			debug("%s THREAD: %p, PAGE START ADDRESS: %p, PAGE END ADDRESS: %p", page->thread == owner ? "C ->" : "    ", page->thread, page->page_start_addr, page->page_end_addr);
		}
		page = page + 1;
		numbpages --;
	}

}

//Prints the internal memory of the current thread (size allocated, block used, etc)
void printInternalMemory(gthread_task_t* owner){
	debug("PRINTING INTERNAL MEMORY");
	Node* page = findThreadPage(owner);
	if(page == NULL){
		return;
	}
	Page_Internal* PI = (Page_Internal*)page->page_start_addr;
	debug("============================================================");
	while((void*)PI != (void*)getEndAddr(owner,NULL)){
		if(PI->used == TRUE){
			debug("PI STARTING ADDR: %p, PI ENDING ADDR: %p",(void*)PI, (void*)getNextPI(PI));
			debug("BLOCK USED: TRUE  BLOCK SIZE: %d", PI->space);
		}
		else{
			debug("PI STARTING ADDR: %p, PI ENDING ADDR: %p",(void*)PI, (void*)getNextPI(PI));
			debug("BLOCK USED: FALSE  BLOCK SIZE: %d", PI->space);
		}
		PI = getNextPI(PI);
	}
	debug("============================================================");
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

//sets metadata start address and number of pages expected on this machine
void metadata_start_addr(){
  size_t malloc_space = max_size - (4*page_size); //decrement by shalloc space
  numb_of_pages = malloc_space / (page_size + sizeof(Node));

  meta_start = (Node*)myblock;
  myblock_userdata =
    (void*)((char*)myblock + malloc_space - numb_of_pages * page_size);

  debug("meta_start=%p, myblock_userdata=%p", meta_start, myblock_userdata);
}

//sets metadata start address and number of pages expected on this machine for swap block
void swap_metadata_start_addr(){
	numb_of_swap_pages = swap_size / (page_size + sizeof(Node));
	swap_meta_start = (Node*)swapblock;
	swapblock_userdata =
		(void*)((char*)swapblock + swap_size - numb_of_swap_pages * page_size);
}

//initializes the array
//creates a start node, and an end node
void initblock(){
  gthread_sched_uninterruptable_lock();
  if (page_size == 0) {
    if (gthread_mutex_init(&malloc_mu, NULL)) {
      fprintf(stderr, "could not initialize mutex\n");
      fflush(stderr);
      abort();
    }
  }
  gthread_sched_uninterruptable_unlock();

  gthread_mutex_lock(&malloc_mu);
  if (page_size == 0) {
    long ret = sysconf(_SC_PAGESIZE);
    if (ret == -1) {
      perror("Could not get system page size");
      abort();
    }
    page_size = (size_t)ret;
    debug("page_size is %zu", page_size);
  } else {
    gthread_mutex_unlock(&malloc_mu);
    return;
  }

  max_size = (MAX_SIZE / page_size + !!(MAX_SIZE % page_size)) * page_size;
  myblock = mmap(NULL, max_size,
                 PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
  if (myblock == MAP_FAILED) {
    perror("could not mmap() memory");
  }
  assert(myblock != MAP_FAILED);
  debug("myblock located at %p with size %zu", myblock, max_size);

  int fd = open("gthread_swap", O_RDWR | O_CREAT, S_IRWXU | S_IRWXG | S_IRWXO);
  if (fd < 0) {
    perror("could not open swap file; swap memory will not be file backed");
  }

  swap_size = (SWAP_SIZE / page_size + !!(SWAP_SIZE % page_size)) * page_size;
  if (fd >= 0) {
    if (ftruncate(fd, swap_size) < 0) {
      perror("could not reserve space for swap file");
      fd = -1;
    }
  }
  swapblock = mmap(NULL, swap_size, PROT_READ | PROT_WRITE,
                   (fd >= 0 ? MAP_SHARED : MAP_ANONYMOUS | MAP_PRIVATE),
                   fd, 0);
  if (swapblock == MAP_FAILED) {
    perror("could not mmap() memory");
  }
  assert(swapblock != MAP_FAILED);
  debug("swapblock located at %p with size %zu", swapblock, swap_size);

	threads_allocated = 0; //initializes threads allocated counter

	metadata_start_addr(); //sets meta_start and number_of_pages
	//creating metadata for memoryblock
	for(int metaIterator = 0; metaIterator < numb_of_pages; metaIterator++){
		assert((uintptr_t)&meta_start[metaIterator] < (uintptr_t)myblock_userdata);
		meta_start[metaIterator].thread = NULL;
		meta_start[metaIterator].first_page = NULL;
		meta_start[metaIterator].next_page = NULL;
		meta_start[metaIterator].page_offset = 0;
		meta_start[metaIterator].page_start_addr = (void*)((char*)myblock_userdata + page_size*metaIterator);
		meta_start[metaIterator].page_end_addr = (void*)((char*)myblock_userdata + page_size*(metaIterator+1));
		meta_start[metaIterator].protected = TRUE;
		assert((uintptr_t)meta_start[metaIterator].page_end_addr <= (uintptr_t)((char*)myblock + max_size - 4 * page_size));
	}
	gthread_malloc_protect_region(myblock_userdata, numb_of_pages * page_size);

	//creating metadata for swapfileblock
	swap_metadata_start_addr();
	for(int metaIterator = 0; metaIterator < numb_of_swap_pages; metaIterator++){
		assert((uintptr_t)&swap_meta_start[metaIterator] < (uintptr_t)swapblock_userdata);
		swap_meta_start[metaIterator].thread = NULL;
		swap_meta_start[metaIterator].first_page = NULL;
		swap_meta_start[metaIterator].next_page = NULL;
		swap_meta_start[metaIterator].page_offset = 0;
		swap_meta_start[metaIterator].page_start_addr = (void*)((char*)swapblock_userdata + page_size*metaIterator);
		swap_meta_start[metaIterator].page_end_addr = (void*)((char*)swapblock_userdata + page_size*(metaIterator+1));
		swap_meta_start[metaIterator].protected = TRUE;
		assert((uintptr_t)swap_meta_start[metaIterator].page_end_addr <= (uintptr_t)((char*)swapblock + swap_size));
	}
	gthread_malloc_protect_region(swapblock_userdata, numb_of_swap_pages * page_size);

	//shalloc region creation
	Page_Internal* shallocNode = (Page_Internal*)((char*)myblock + max_size - 4 * page_size); //backtrack four pages
	shallocNode->space = 4 * page_size - sizeof(Page_Internal); //create shalloc metadata
	shallocNode->used = FALSE;
	shallocRegion = (void*)shallocNode;
	debug("shallocRegion=%p", shallocRegion);

	// set segv handler
	gthread_malloc_protection_init();

  gthread_mutex_unlock(&malloc_mu);
}

//returns the page in which the PageInternal pointer belongs to
Node* whichPage(void* addr){
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
	if (source->thread != gthread_task_current() && source->protected == TRUE) {
		gthread_malloc_unprotect_region(source->page_start_addr, page_size);
	}
	if (target->thread != gthread_task_current() && target->protected == TRUE) {
		gthread_malloc_unprotect_region(target->page_start_addr, page_size);
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
	if (source->thread != gthread_task_current() && source->protected == TRUE) {
		gthread_malloc_protect_region(source->page_start_addr, page_size);
	}
	if (target->thread != gthread_task_current() && target->protected == TRUE) {
		gthread_malloc_protect_region(target->page_start_addr, page_size);
	}
	//swap complete
	return;
}

//finds metadata of the nth page in the memory block
Node* findNthPage(int n){
	void* start = (void*)((char*)myblock_userdata + n*page_size);
	Node* meta = (void*)meta_start;
	int counter = 0;
	while(meta->page_start_addr != start){
		meta = meta + 1;
		counter++;
		if(counter == numb_of_pages){
			meta = swap_meta_start;
		}else if (counter == numb_of_swap_pages + numb_of_pages){
			return NULL;
		}
	}
	return meta;
}

void rintPageSADDR(){
	int x = 0;
	Node* meta = meta_start;
	while(x<numb_of_pages){
		debug("Starting SADDR: %p, EADDR: %p, counter: %d",meta->page_start_addr,meta->page_end_addr, x);
		meta = meta + 1;
		x++;
	}
}

void printSwapSADDR(){
	int x = 0;
	Node* meta = swap_meta_start;
	while(x<numb_of_swap_pages){
		debug("SWAP SADDR: %p, EADDR: %p, counter: %d",meta->page_start_addr,meta->page_end_addr, x);
		meta = meta + 1;
		x++;
	}
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
	Node* metadata = meta_start;
	for(int metacounter = 0; metacounter < numb_of_pages; metacounter++){
		if(metadata->thread == NULL){
			return metadata;
		}
		metadata++;
	}
	//swap file check
	Node* swapmeta = (Node*)swap_meta_start;
	for(int metacounter = 0; metacounter < numb_of_swap_pages; metacounter++){
		if(swapmeta->thread == NULL){
			return swapmeta;
		}
		swapmeta++;
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
Page_Internal* findOpenSpace(int size, Node* pageMData, BETWEEN_OR_END* allocationFlag){
	Page_Internal* PI = (Page_Internal*)pageMData->page_start_addr;
	//find open space in between
	while((void*)getNextPI(PI) != getEndAddr(NULL, pageMData)){
		if(PI->used == FALSE && PI->space > sizeof(Page_Internal) + size){
			debug("Found PI with starting address %p and space %d", (void*)PI, PI->space);
			*allocationFlag = BETWEEN;
			return PI;
		}

		PI = getNextPI(PI);
	}

	if(sizeof(Page_Internal) + size < getSpaceLeft() + PI->space && PI->used == FALSE){
		debug("Found PI with starting address %p and space %d", (void*)PI,PI->space);
		*allocationFlag = END;
		return PI;
	}
	return NULL;

}

//When a valid partition is found, a node is created to hold the space after the partition, and
//the current node's size is readjusted to reflect the size partitioned
void* allocate(Page_Internal* PI, int size, gthread_task_t* owner, BETWEEN_OR_END allocationFlag){
		Page_Internal* unusedPI; //PI for PI at the end of an used block
		int spaceleft = PI->space; //calculation of spaceleft
		Node* firstPage = findThreadPage(owner); //get the first page of the thread
		Node* Currentpage = whichPage(PI); //findout which page contains the block that was found to be open
		int Offset = Currentpage->page_offset;
		//space calculations
		int spaceNeeded = size + sizeof(Page_Internal);
		int spaceToEnd = labs(((char*)(PI+1) - (char*)Currentpage->page_end_addr));
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
		firstPage->space_allocated = firstPage->space_allocated + size;
		return (void*)(PI+1);
}

//creates first page for thread
Node* createThreadPage(gthread_task_t *owner){
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

void* mymalloc(size_t size, gthread_task_t *owner){
	debug("+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+_+");
	debug("Size request %zu, gthread ID: %p", size, (void*)owner);
	void* ptr = NULL;
    //initialize myblock if not initialized
    if(size < 1){
        // malloc() does this but there is no good reason for us to
        fprintf(stderr, "mymalloc does not support 0 sized allocations\n");
        return NULL;
    }
    if(owner == NULL){
    	 fprintf(stderr, "Thread no longer exists or invalid thread input.\n");
    	 return NULL;
    }

    //initialize entire memory array, create metadata nodes at the end of the memory block (before shalloc region)
    if(meta_start == NULL){
        initblock();
    }

    //traverse array until the pages for the requested thread is found, if
    //not found, create new thread page.
    Node* page = findThreadPage(owner);
    if(page == NULL){ //if thread does not have an allocated space
        page = createThreadPage(owner);
        if (page != NULL) {
            debug("created thread page at %p with page_start_addr=%p", page, page->page_start_addr);
        }
    }
    if(page == NULL){ //no space for thread left in VM
    	fprintf(stderr, "No space left in VM for requested thread Memory Allocation.\n");
    	return NULL;
    }
    if(page->space_allocated + size > numb_of_pages*page_size){
    	//not enough space left
    	fprintf(stderr, "Allocation request will reach memory capacity if allocated. Denied allocation request.\n");
    	return NULL;
    }
    //places pages contiguously at the start of the array
    placePagesContig(owner);
    //traverse array until free space is found
    BETWEEN_OR_END allocationFlag = NONE;
    Page_Internal* PI = findOpenSpace(size, page, &allocationFlag);
    //if the end is reached, no valid space is found
    if(PI == NULL){
    	fprintf(stderr, "Invalid space request, capacity overflow.\n");
    	return NULL;
    }
    ptr = allocate(PI, size, owner, allocationFlag);
    debug("Success");
    if(ptr == NULL){
    	fprintf(stderr, "Unsuccessful allocation attempt, likely not enough free pages were found\n");
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

void FreePages(Page_Internal* PI, int freePages, Node* startingPage, BETWEEN_OR_END allocationFlag){
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
			if (page_iterator->protected == FALSE) {
				gthread_malloc_protect_region(page_iterator->page_start_addr, page_size);
				page_iterator->protected = TRUE;
			}
			page_iterator = nextPage;
		}
		threads_allocated--;
		return;
	}
	else if(PI_starting_addr == firstPage->page_start_addr){
		//first Page involved in clear, but first page must be kept to keep reference
			freePages--;
			startingPage = startingPage->next_page;
		}
	debug("prevPage offset is: %d", prevPage->page_offset);
	while(freePages>0 && startingPage != NULL){
		prevPage->next_page = startingPage->next_page;
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
	debug("Page allocated %d", pages_allocated);
	debug("End of page ADDRESS: %p", (void*)getEndAddr(NULL,prevPage));
	debug("End of end Page_internal Address %p", (void*)getNextPI(PI));
	debug("Returning from FreePages()");
}

void clearUnusedPages(Page_Internal* PI, gthread_task_t* owner){
	Node* page = findThreadPage(owner);
	Node* page_iterator = (Node*)page;
	Node* startingFreePage = NULL;
	void* PI_ending_addr = (void*)getNextPI(PI);
	int freePages = 0;

	//if the freed Page_Internal is at the end
	if(PI_ending_addr == getEndAddr(owner,NULL)){
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
			FreePages(PI, freePages, startingFreePage, END);
		}
		return;
	}
	else{
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
			FreePages(PI, freePages, startingFreePage, BETWEEN);
		}
		return;
	}

	return;
}

//Frees the usage of the space taken up by this pointer, changes
//the metadata associated with it to inactive (used = FALSE), and concatenates
//the space with any adjacent unused nodes.
void myfree(void* p, gthread_task_t *owner){
	debug("IN MYFREE(), gthread ID: %p", (void*)owner);

	//check if p is in shalloc region
	if((uintptr_t)p >= (uintptr_t)shallocRegion){
		debug("(p=%p > shallocRegion=%p), delegating free to shalloc", p, shallocRegion);
		myfreeShalloc(p);
		return;
	}

	//checks if pointer is in the array
	Page_Internal* PI = does_pointer_exist(p, owner);
	//if pointer is not in the array, throw error, return
	if(PI == NULL){
    debug("bad free p=%p", p);
    fprintf(stderr, "Invalid input pointer. Pointer must be at the start of a "
                    "previously allocated area of the memory array\n");
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
