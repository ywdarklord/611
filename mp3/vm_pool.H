/*
    File: vm_pool.H

    Author: R. Bettati, Nikhil Gupta
            Department of Computer Science
            Texas A&M University
    Date  : 10/26/2010

    Description: Management of the Virtual Memory Pool


*/

#ifndef _VM_POOL_H_                   // include file only once
#define _VM_POOL_H_

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "utils.H"
#include "frame_pool.H"
#include"page_table.H"
/*--------------------------------------------------------------------------*/
/* DATA STRUCTURES */
/*--------------------------------------------------------------------------*/

/* Forward declaration of class PageTable */
class PageTable;
struct block{
	unsigned long str_addr;
	unsigned long block_size;
	block* next;
	block* previous;
};


/*--------------------------------------------------------------------------*/
/* V M  P o o l  */
/*--------------------------------------------------------------------------*/



class VMPool { /* Virtual Memory Pool */
private:
   /* -- DEFINE YOUR VIRTUAL MEMORY POOL DATA STRUCTURE(s) HERE. */
	unsigned long base_address;
	unsigned long size;
	FramePool * frame_pool;
	PageTable * page_table;
        /*I use two doubly-linked-list to manage the free virutal memory and allocate virtual memory*/
	block* used_block;
	block* unused_block;
	block* first_block_ptr;
	unsigned int numberOfBlocks;

public:
   VMPool(unsigned long _base_address,
          unsigned long _size,
          FramePool *_frame_pool,
          PageTable *_page_table);
   /* Initializes the data structures needed for the management of this
    * virtual-memory pool.
    * _base_address is the logical start address of the pool.
    * _size is the size of the pool in bytes.
    * _frame_pool points to the frame pool that provides the virtual
    * memory pool with physical memory frames.
    * _page_table points to the page table that maps the logical memory
    * references to physical addresses. */

   unsigned long allocate(unsigned long _size);
   /* Allocates a block of _size bytes of memory from the virtual
    * memory pool. If successful, returns the virtual address of the
    * start of the allocated block of memory. If fails, returns 0. */

   void release(unsigned long _start_address);
   /* Releases a block of previously allocated memory. The block
    * is identified by its start address, which was returned when the
    * block was allocated. */

   bool is_legitimate(unsigned long _address);
   /* Returns FALSE if the address is not valid. An address is not valid
    * if it is not part of a block that is currently allocated. */

   FramePool* get_frame_pool();
   void initial_block(block* ptr,unsigned long _size, unsigned long _str_addr);
   void allocate_block(block* ptr);
   void merge_block(block*ptr);
};

#endif
