#include"vm_pool.H"
#include "page_table.H"
#include "frame_pool.H"




void VMPool::initial_block(block* ptr,unsigned long _size, unsigned long _str_addr){
        ptr->block_size=_size;
        ptr->str_addr=_str_addr;
        ptr->next=NULL;
        ptr->previous=NULL;
}

VMPool::VMPool(unsigned long _base_address,
			   unsigned long _size,
			   FramePool *_frame_pool,
			   PageTable *_page_table){
		base_address = _base_address;
		size = _size;
		frame_pool = _frame_pool;
		page_table = _page_table;
                /*Allocate a free virual memory block at the begining*/
                /*More blocks can be allocated by VMPool::allocate*/
		first_block_ptr = (block*)(PageTable::get_kernal_mem_pool()->get_frame()*4096);
		unused_block = first_block_ptr;
		numberOfBlocks = 1;
		initial_block(unused_block,size,base_address);
		used_block = NULL;
		page_table->register_vmpool(this);
}

void VMPool::allocate_block(block* ptr){
		if(ptr->previous == NULL){
			unused_block = ptr->next;
		}else{
			ptr->previous->next = ptr->next;
		}
                if(ptr->next!=NULL) ptr->next->previous=ptr->previous;
                ptr->next = used_block;
		if(ptr->next!=NULL) ptr->next->previous=ptr;
		ptr->previous = NULL;
		used_block = ptr;
}

void VMPool::merge_block(block*ptr){

            if((ptr->next!=NULL)&&(ptr->str_addr + ptr->block_size == ptr->next->str_addr)){
			ptr->block_size = ptr->block_size + ptr->next->block_size;
			ptr->next = ptr->next->next;
			if((ptr->next)!=NULL) ptr->next->previous = ptr;
}

             unsigned long boundary=ptr->previous->str_addr + ptr->previous->block_size;
            if((ptr->previous!=NULL)&&(boundary == ptr->str_addr)){
			ptr->previous->block_size = ptr->previous->block_size + ptr->block_size;
			ptr->previous->next = ptr->next;
			if(ptr->next!=NULL) ptr->next->previous = ptr->previous;
}

}

unsigned long VMPool::allocate(unsigned long _size){
	block* ava_block;
	ava_block = unused_block;
        /*Find a block that has enough space to allocate the size*/
	while(ava_block!=NULL){
		if(ava_block->block_size>=_size) break;
		ava_block = ava_block->next;
	}

	if(ava_block->block_size == _size){// the size of current block is just the same as reuqired size
                allocate_block(ava_block);

	}else{//there is not enough space in current block, allocate a new one
		numberOfBlocks++;
		block* new_block;
		if((numberOfBlocks*sizeof(block))>4096){
			new_block = (block*)(PageTable::get_kernal_mem_pool()->get_frame()*4096);
			numberOfBlocks = 1;
		}else{
			new_block = first_block_ptr + sizeof(block);
		}

		new_block->str_addr = ava_block->str_addr + _size;
		new_block->block_size = ava_block->block_size - _size;
		new_block->previous = ava_block->previous;
		new_block->next = ava_block->next;
		/*Check whether the doubly-linked-list is empty or have only one block or have  several block and insert the new_block in to the list*/
		if(new_block->previous == NULL){
			unused_block = new_block;
		}else{
			new_block->previous->next = new_block;
		}
           
		if(new_block->next!=NULL) new_block->next->previous = new_block;
		
		ava_block->block_size = _size;
		ava_block->next = used_block;
		if(ava_block->next!=NULL) ava_block->next->previous = ava_block;
		ava_block->previous = NULL;
		used_block = ava_block;
		
	}
	return used_block->str_addr;
}

void VMPool::release(unsigned long _start_address){
	block* released_block;
	released_block = used_block;
	
	while(released_block!=NULL){
		if(released_block->str_addr==_start_address) break;
		released_block = released_block->next;
	}
        /*Find the corresponding page in the paing system and free the page*/
    unsigned long s=released_block->str_addr;
    unsigned long e=released_block->str_addr+released_block->block_size;
    
	for(unsigned long i = s; i<e; i+=4096){
		page_table->free_page(i/4096);
	}
	/*Check whether the list is empty*/
        /*Remove the node from used_block list*/
	if(released_block->previous==NULL){
		used_block = released_block->next;
	}else{
		released_block->previous->next = released_block->next;
	}
	if(released_block->next!=NULL) released_block->next->previous = released_block->previous;

	block* free_block;
	free_block = unused_block;
        /*Add the released block to unused_block list*/
	if(free_block==NULL){
		unused_block = released_block;
		released_block->next = NULL;
		released_block->previous = NULL;
	}else{/*If current the unusedblock list is not empty find the right place to insert the released block*/
		while(free_block->next!=NULL){
			if(free_block->str_addr>released_block->str_addr) break;	
			free_block = free_block->next;
		}
	     
		if(free_block->str_addr<released_block->str_addr){
			released_block->next = NULL;
			free_block->next = released_block;
			released_block->previous = free_block;
		}else if(free_block->previous==NULL){
			released_block->next = free_block;
			released_block->previous = NULL;
			free_block->previous = released_block;
			unused_block = released_block;
		}else{
			released_block->next = free_block;
			free_block->previous->next = released_block;
			released_block->previous = free_block->previous;
			free_block->previous = released_block;
		}
                merge_block(released_block);
		
	}
}
/* Returns FALSE if the address is not valid. An address is not valid
 * if it is not part of a region that is currently allocated.
 */

bool VMPool::is_legitimate(unsigned long _address){
	block* current_blk;
	current_blk = used_block;
	while(current_blk!=NULL){
        unsigned long s=current_blk->str_addr;
        unsigned long e=current_blk->str_addr + current_blk->block_size;
		if(_address>=s && _address<e) return true;
		current_blk = current_blk->next;
	}
	return false;
}


FramePool* VMPool::get_frame_pool(){
	return frame_pool;
}
