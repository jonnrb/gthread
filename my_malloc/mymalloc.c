#include "mymalloc.h"


static char myblock[5000] = {};
//////////////////////////////////////////////////////////////////////
//MALLOC//
//////////////////////////////////////////////////////////////////////

//Goes through each node in the array and prints the attributes of each node
void printmem()
{
Node* start = (Node*)&myblock[0];
while(start->space != 0){
    if(start->used == 0){
        printf("USED: True----Space: %d\n",start->space);
    }
    else{
        printf("USED: False----Space: %d\n",start->space);
    }
     start = (Node*)((char*)(start+1)+(start->space));
}

if(start->space == 0 && start->used == FALSE){
    printf("------------------------------------------------------\n");
}
    }



//initializes the array
//creates a start node, and an end node
void initblock(){
	int total = 5000;
	int sizeNode = sizeof(Node);
	total = total - 2*sizeNode; //1 node for the start node, and 1 for the end
	Node* start = (Node*)&myblock[0];
	start->space = total;
	start->used = FALSE;
	total = 5000;
	Node* end = (Node*)&myblock[(total - sizeof(Node))];
	end->space = 0;
	end->used = FALSE;
}



//Traverses the array, searching for sections with size 'size', and returning it.
//If no partition is found, and the index has overreached, then it returns NULL
Node* traverse(int size){
    Node* ptr =  (Node*)&myblock[0];
    while(ptr->used == TRUE || ptr->space < (size + sizeof(Node))){
    	//increment node pointer by shifting the pointer by 1 node space + node->size
        ptr = (Node*)((char*)(ptr+1)+(ptr->space));
        //last node, denoting the end of the memory array
        if(ptr->space == 0  && ptr->used == FALSE){
            return NULL;
        }
}
return ptr;
}



//When a valid partition is found, a node is created to hold the space after the partition, and
//the current node's size is readjusted to reflect the size partitioned
void* allocatenew(Node* nodeptr, int size){

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
	}

    //the input to the function is then adjusted to indicate the correct size requested
    //and its active 'used' flag
    nodeptr->space = size;
    nodeptr->used = TRUE;
    //a void pointer to the space after the node(the space requested), is returned to the main malloc function
    void* ptr = (void*)(nodeptr+1);
    return ptr;
}






void* mymalloc(size_t size){
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
    Node* nodeptr = traverse(size);
    //if the end is reached, no valid space is found
    if(nodeptr == NULL){
    	printf("Invalid space request, capacity overflow.\n");
    return NULL;
    }
    //another verification for the metadata
    else if(nodeptr->space >= size + sizeof(Node) && nodeptr->used == FALSE){
        //allocate new space and return
        ptr = allocatenew(nodeptr, size);
    }

    //returns void pointer to the caller
    return ptr;
}

//////////////////////////////////////////////////////////////////////////////////////
//FREE//
/////////////////////////////////////////////////////////////////////////////////////




//Checks left of the pointer, if a node exists, returns it
//used for concatenation of two unused nodes in the main free function
//returns a pointer to the node that is before the node being freed
Node* checkLeft(Node* ptr){
	//ptr is already the left most node
if(ptr == NULL){
	//error
	return NULL;
}
if(ptr ==(Node*) &myblock[0]){
	return NULL;
}
Node* leftp = (Node*)&myblock[0];
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
if(temp->space == 0 && temp->used == FALSE){
	return NULL;
}
return temp;
}



//checks if the pointer inputted in the 'free()' function is inside the memory array
//returns TRUE if it is, false if it is not
BOOLEAN checkpoint(void* p){
Node* current = (Node*)&myblock[0];
//current is not at the end
while(current->space!=0){
    if((void*)(current+1) == p ){
    return TRUE;}
    current = (Node*)((char*)(current+1)+current->space);
}
return FALSE;
}



//Frees the usage of the space taken up by this pointer, changes
//the metadata associated with it to inactive (used = FALSE), and concatenates
//the space with any adjacent unused nodes.
void free(void* p){

    //checks if pointer is in the array
    BOOLEAN valid = checkpoint(p);

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
	//Finds the pointer left the node
	Node* leftp = checkLeft(ptr);
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
	return;
}


