#include "mymalloc.h"

//////////////////////////////////////////////////////////////////////
//SHALLOC//
//////////////////////////////////////////////////////////////////////

//Goes through each Page_Internal in the array and prints the attributes of each Page_Internal

void printShallocRegion(){
	Page_Internal* start = (Page_Internal*)getShallocRegion();
	printf("SHALLOC REGION +=-++=-++=-++=-++=-++=-++=-++=-++=-+\n");
	while((void*)start != (void*)&myblock[MAX_SIZE - 1]){
		if(start->used == TRUE){
			printf("SHALLOC USED: TRUE, SPACE: %d\n", start->space);
		}
		else{
			printf("SHALLOC USED: FALSE, SPACE: %d\n", start->space);
		}

		start = (Page_Internal*)((char*)(start + 1) + start->space);
	}

}


//Traverses the array, searching for sections with size 'size', and returning it.
//If no partition is found, and the index has overreached, then it returns NULL
Page_Internal* traverse(int size){
    Page_Internal* ptr =  (Page_Internal*)getShallocRegion();
    while(ptr->used == TRUE || ptr->space < (size + sizeof(Page_Internal))){
    	//increment Page_Internal pointer by shifting the pointer by 1 Page_Internal space + Page_Internal->size
        ptr = (Page_Internal*)((char*)(ptr+1)+(ptr->space));
        //last Page_Internal, denoting the end of the memory array
        if((void*)ptr == (void*)&myblock[MAX_SIZE - 1]){
            return NULL;
        }
}
return ptr;
}



//When a valid partition is found, a Page_Internal is created to hold the space after the partition, and
//the current Page_Internal's size is readjusted to reflect the size partitioned
void* allocatenew(Page_Internal* Page_Internalptr, int size){

Page_Internal* temptr = NULL;
	//Calculate the space left over after allocating enough for the current request
	int totalspace = Page_Internalptr->space;
	totalspace = totalspace - (size + sizeof(Page_Internal));
	//temptr is a pointer that will increment to the location after the requested space
	//and create a new Page_Internal to indicate the space left
	if(totalspace>0){
    temptr = Page_Internalptr;
    temptr = (Page_Internal*)((char*)(temptr+1) + size);
    temptr->space = totalspace;
    temptr->used = FALSE;
	}

    //the input to the function is then adjusted to indicate the correct size requested
    //and its active 'used' flag
    Page_Internalptr->space = size;
    Page_Internalptr->used = TRUE;
    //a void pointer to the space after the Page_Internal(the space requested), is returned to the main malloc function
    void* ptr = (void*)(Page_Internalptr+1);
    return ptr;
}






void* shalloc(size_t size){
	void* ptr = NULL;
    //initialize myblock if not initialized
    if(size < 1){
        printf("Input argument of integer value 0 or less is not valid./n Please enter a positive non-zero integer.");
        return NULL;
    }
    //initialize
    if(myblock[0] == 0){
    	initblock();
    }

    //traverse array until free space is found
    Page_Internal* Page_Internalptr = traverse(size);
    //if the end is reached, no valid space is found
    if(Page_Internalptr == NULL){
    return NULL;
    }
    //another verification for the metadata
    else if(Page_Internalptr->space >= size + sizeof(Page_Internal) && Page_Internalptr->used == FALSE){
        //allocate new space and return
        ptr = allocatenew(Page_Internalptr, size);
    }

    //returns void pointer to the caller
    return ptr;
}

//////////////////////////////////////////////////////////////////////////////////////
//FREE//
/////////////////////////////////////////////////////////////////////////////////////




//Checks left of the pointer, if a Page_Internal exists, returns it
//used for concatenation of two unused Page_Internals in the main free function
//returns a pointer to the Page_Internal that is before the Page_Internal being freed
Page_Internal* checkLeftShalloc(Page_Internal* ptr){
	//ptr is already the left most Page_Internal
if(ptr == NULL){
	//error
	return NULL;
}
if((void*)ptr ==getShallocRegion()){
	return NULL;
}
Page_Internal* leftp = (Page_Internal*)getShallocRegion();
//while the next Page_Internal does not = the Page_Internal currently being worked on
while(((Page_Internal*)((char*)(leftp+1)+leftp->space)) != ptr) {
	leftp = (Page_Internal*)((char*)(leftp+1)+leftp->space);
}
return leftp;
}


//Checks right of the pointer, if a Page_Internal exists, returns it
//used for concatenation of two unused Page_Internals in the main free function
//returns a pointer to the Page_Internal that is after the Page_Internal being freed
Page_Internal* checkRightShalloc(Page_Internal* ptr){
if(ptr == NULL){
    //error
    return NULL;
	}
		//ptr is already the rightmost Page_Internal
	Page_Internal* temp = (Page_Internal*)((char*)(ptr+1)+ptr->space);
if(temp->space == 0 && temp->used == FALSE){
	return NULL;
}
return temp;
}



//checks if the pointer inputted in the 'free()' function is inside the memory array
//returns TRUE if it is, false if it is not
BOOLEAN checkpointShalloc(void* p){
Page_Internal* current = (Page_Internal*)getShallocRegion();
//current is not at the end
while((void*)current != (void*)&myblock[MAX_SIZE - 1]){
    if((void*)(current+1) == p ){
    return TRUE;}
    current = (Page_Internal*)((char*)(current+1)+current->space);
}
return FALSE;
}



//Frees the usage of the space taken up by this pointer, changes
//the metadata associated with it to inactive (used = FALSE), and concatenates
//the space with any adjacent unused Page_Internals.
void myfreeShalloc(void* p){

    //checks if pointer is in the array
    BOOLEAN valid = checkpointShalloc(p);

    //if pointer is not in the array, throw error, return
    if(valid == FALSE){
        printf("Invalid input pointer. Pointer must be at the start of a previously allocated area of the memory array\n");
        return;
    }
    //converts pointer to a Page_Internal type, and subtracts it by 1 to get to the Page_Internal that corresponds to it
    Page_Internal* ptr = (Page_Internal*)p;
    ptr = ptr-1;

    //Checks Page_Internal, if null or the 'used' flag is found to be false, then the pointer was not allocated for
	    if(ptr->used == FALSE){
		//error
		printf("Error, attempted to free an unallocated pointer!\n");
		return;
	}

	//At this point, the pointer exists, and the Page_Internal corresponding to it is valid
	//The Page_Internal's flag is set to unused = 'FALSE'
	ptr -> used = FALSE;
	//Finds the pointer left the Page_Internal
	Page_Internal* leftp = checkLeftShalloc(ptr);
	//	//Finds the pointer right the Page_Internal
	Page_Internal* rightp = checkRightShalloc(ptr);
	//concatenates the space of the right Page_Internal, and the current Page_Internal
	if(rightp != NULL && rightp->used == FALSE ){
		ptr->space = ptr->space + rightp->space + sizeof(Page_Internal);
		rightp = NULL;
	}
    //concatenates the space of the left Page_Internal, and the current Page_Internal
	if(leftp != NULL && leftp->used == FALSE){
		leftp->space = leftp->space + ptr->space + sizeof(Page_Internal);
		ptr = NULL;
	}
	return;
}
