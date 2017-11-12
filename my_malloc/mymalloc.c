#include "mymalloc.h"

static const long max_size = MAX_SIZE; //Maximum size for Virtual Memory
extern char myblock[MAX_SIZE] = {}; //8MB memory block
static long page_size; //Page size for system
static int threads_allocated; //number of threads that allocated space in Virtual Memory currently
static int pages_allocated;
static void* shallocRegion; //first shalloc metadata
static Node* meta_start; //the first page metadata
static int numb_of_pages; //number of total pages in the block (can be empty pages)

typedef
struct free_space{ //structure to pass information between traversePageInternals and allocateNewInsidePage functions
	Page_Internal* PI;
	Node* page;
} FreeSpace;

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
	int max = max_size - (2*sizeof(Node) + 4*page_size); //decrement by shalloc space
	double ratio = (double)sizeof(Node)/(double)page_size;
	double metaSpace = (double)max*ratio;
	metaSpace = ceil(metaSpace);
	numb_of_pages = (int)metaSpace/sizeof(Node);
	max = max - metaSpace;
	int maxaddr = (int)max;
	meta_start = (Node*)&myblock[maxaddr-1];
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
		metadata->page_start_addr = (void*)(&myblock[0] + page_size*metaIterator);
		metadata->page_end_addr = (void*)(&myblock[0] + page_size*(metaIterator+1));
		metadata = metadata+1;
		metaIterator++;
	}
	//end of VirtualMemory node

	//shalloc region creation
	Page_Internal* shallocNode = (Page_Internal*)((char*)&myblock[max_size - 1] - (sizeof(Page_Internal) + 4*page_size)); //backtrack four pages
	shallocNode->space = 4*page_size; //create shalloc metadata
	shallocNode->used = FALSE;
	shallocRegion = (void*)shallocNode;
}





//Traverses the array, searching for sections with size 'size', and returning it.
//If no partition is found, and the index has overreached, then it returns NULL
FreeSpace* findOpenSpace(int size, Node* pageMData){
	FreeSpace* FS;
	int spaceOffset = 0;
	Page_Internal* pageIter = (Page_Internal*)pageMData->page_start_addr;
	while((void*)((char*)(pageIter+1) + pageIter->space) != pageMData->page_end_addr){
		if(pageIter->space > (sizeof(Page_Internal) + size)){
			FS->PI = pageIter;
			FS->page = pageMData;
			return FS;
		}
		if((void*)((char*)(pageIter+1) + pageIter->space) == pageMData->page_end_addr && pageMData->next_page != NULL){
			pageMData = pageMData->next_page;
			spaceOffset = pageIter->extendedspace;
			if(spaceOffset!=0){
				spaceOffset = spaceOffset - pageIter->space;
				while(spaceOffset > page_size && pageMData->next_page != NULL){ //traverse pages
					spaceOffset = spaceOffset - page_size;
					pageMData = pageMData->next_page;
				}
				pageIter = (Page_Internal*)((char*)pageMData->page_start_addr + spaceOffset);
			}
			pageIter = (Page_Internal*)pageMData->page_start_addr;

		}
		pageIter = (Page_Internal*)((char*)(pageIter+1) + pageIter->space);

	}

	if((pageIter->space + getSpaceLeft()) > (size + sizeof(Page_Internal))){
		FS->PI = pageIter;
		FS->page = pageMData;
		return FS;
	}
	if((size + sizeof(Page_Internal)) > getSpaceLeft){
		printf("Unable to allocate resources, capacity will be reached.\n");
		return NULL;
	}

    return NULL;
}



Node* getEmptyPage(){
	Node* metadata = meta_start;
	int metacounter = 0;
	while(metacounter < numb_of_pages){
		if(metadata->thread == NULL){
			return metadata;
		}
		metacounter++;
	}
	return NULL;
}


//When a valid partition is found, a node is created to hold the space after the partition, and
//the current node's size is readjusted to reflect the size partitioned
void* allocate(FreeSpace* FS, int size, gthread_task_t* owner){
	Page_Internal* temp;
	Page_Internal* PI = FS->PI;
	int space = PI->space;
	Node* page = FS->page;
	Node* newPage;
	if(PI->space < size){
		//multiple pages needed
		//PI->space untouched as it would point to the end of the array;
		int extSpace = size;
		PI->extendedspace = size;
		PI->used = TRUE;
		extSpace = extSpace - PI->space;
		while(extSpace>0){
			newPage = getEmptyPage();
			page->next_page = newPage;
			newPage->thread = owner;
			newPage->next_page = NULL;
			page = newPage;
			extSpace = extSpace - page_size;
			pages_allocated++;
		}
		extSpace = extSpace + page_size; //it was negative, so need to make it positive again
		temp = (Page_Internal*)((char*)(page->page_start_addr) + extSpace);
		temp->extendedspace = 0;
		temp->space = page_size - extSpace;
		temp->space = FALSE;

	}
	else{
		//inside current page
		PI->space = size;
		PI->extendedspace = 0;
		PI->used = TRUE;
		temp = (Page_Internal*)((char*)(PI + 1) + PI->space);
		temp->extendedspace = 0;
		temp->used = FALSE;
		temp->space = space - size;
	}
	return (void*) (PI + 1);
}

//TODO change NUMBER OF THREADS to NUMBER OF PAGES
//if thread page is found, returns page metadata
void* findThreadPage(gthread_task_t *owner){
	int number_of_threads = threads_allocated;
	Node* page = meta_start;
	while(number_of_threads > 0){
		//page found, return
		if(page->thread == owner){
			return page->first_page;
		}
		//else iterate
		number_of_threads--;
		page = page + 1;
	}
return NULL;
}


void* createThreadPage(gthread_task_t *owner){
	Node* page = getEmptyPage();
	if(page == NULL){
		return NULL;
	}
			Page_Internal* nodestart;
			page->thread = owner;
			page->first_page = page;
			nodestart = (Page_Internal*)page->page_start_addr;
			nodestart->space = (page_size - sizeof(Page_Internal));
			nodestart->used = FALSE;
			threads_allocated++;
			pages_allocated++;
			return page;

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
    //the next 8 lines are for new threads;
    void* page = findThreadPage(owner);
    if(page == NULL){ //if thread does not have an allocated space
    	page = createThreadPage(owner);
    }
    if(page == NULL){ //no space for thread left in VM
    	printf("No space left in VM for requested thread Memory Allocation.\n");
    	return NULL;
    }

    //traverse array until free space is found
    Page_Internal* nodeptr = findOpenSpace(size, page);
    //if the end is reached, no valid space is found
    if(nodeptr == NULL){
    	printf("Invalid space request, capacity overflow.\n");
    	return NULL;
    }
    ptr = allocate(nodeptr, size);
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
Page_Internal* prevleftp = NULL;
//while the next node does not = the node currently being worked on
while(leftp != ptr){
	prevleftp = leftp;
	leftp = (Page_Internal*)((char*)(leftp+1) + leftp->space);
	if((void*)leftp == (void*)pageMetaData->page_end_addr){
		pageMetaData=pageMetaData->next_page;
		leftp = (Page_Internal*)((char*)(pageMetaData->page_start_addr) + leftp->extendedspace);
	}
	if(leftp == ptr){
		break;
	}
}
return prevleftp;
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
int extspace = ptr->extendedspace;
Page_Internal* temp;
if(ptr->extendedspace == ptr->space){
	temp = (Page_Internal*)((char*)(ptr+1)+ptr->space);
}
else{
	while(extspace>0){
		pageMetaData = pageMetaData->next_page;
		extspace = extspace - page_size;
	}
	extspace = extspace + page_size;
	temp = (Page_Internal*)((char*)pageMetaData->page_start_addr + extspace);

}
if((void*)(temp) ==(void*)pageMetaData->page_end_addr){
	return NULL;
}
return temp;
}



//checks if the pointer inputted in the 'free()' function is inside the memory array
//returns TRUE if it is, false if it is not
BOOLEAN does_pointer_exist(void* p, gthread_task_t* owner){
Node* page = findThreadPage(owner);
FreeSpace FS = NULL;
if(page == NULL){
	return NULL;
}
Page_Internal* current = (Page_Internal*)page->page_start_addr;
int extSpace = 0;
//current is not at the end
while((void*)current != (void*)page->page_end_addr){
    if((void*)(current+1) == p && current->used == TRUE){
    	FS->PI = current;
    	FS->page = page;
    	return FS;
    }
    extSpace = current->extendedspace;
    current = (Page_Internal*)((char*)(current+1)+current->space);
    if((void*)current == (void*)page->page_end_addr){ //if end of page will be reached, check other pages
    		if(page->next_page != NULL){
    			page = page->next_page;
    			current = (Page_Internal)((char*)(page->page_start_addr) + extSpace); //moves pointer to node in next pages
    		}
    }
}
return NULL;
}



void check_and_free_Page(Node* page){


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
    FreeSpace FS = does_pointer_exist(p, owner);

    //if pointer is not in the array, throw error, return
    if(FS == NULL){
        printf("Invalid input pointer. Pointer must be at the start of a previously allocated area of the memory array\n");
        return;
    }

    //converts pointer to a node type, and subtracts it by 1 to get to the node that corresponds to it
    Page_Internal* ptr = FS->PI;

	//At this point, the pointer exists, and the node corresponding to it is valid
	//The node's flag is set to unused = 'FALSE'
	ptr -> used = FALSE;

	Node* page = FS->page;
	//Finds the pointer left the node
	Node* firstPage = findThreadPage();
	firstPage = firstPage->first_page;
	Page_Internal* leftp = checkLeft(ptr, firstPage);
	//	//Finds the pointer right the node
	Page_Internal* rightp = checkRight(ptr, page);

	//concatenates the space of the right node, and the current node
	if(rightp != NULL && rightp->used == FALSE ){
		if(ptr->space == ptr->extendedspace){
		ptr->space = ptr->space + rightp->space + sizeof(Page_Internal);
		rightp = NULL;
		}
		else{
			ptr->extendedspace = ptr->extendedspace + rightp->extendedspace;
		}
	}
    //concatenates the space of the left node, and the current node
	if(leftp != NULL && leftp->used == FALSE){
		if(leftp->extendedspace > leftp->space){
			leftp->extendedspace = leftp->extendedspace + ptr->extendedspace + sizeof(Page_Internal);
		}
		else{
		leftp->space = leftp->space + ptr->space + sizeof(Page_Internal);
		}
		ptr = NULL;
	}
	check_and_free_Page(page);

	return;
}


