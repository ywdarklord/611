#include "frame_pool.H"



FramePool::FramePool(unsigned long _base_frame_no,
             unsigned long _nframes,
             unsigned long _info_frame_no) {

	base_frame_no = _base_frame_no;
	nframes = _nframes;
	info_frame_no = _info_frame_no;
    unsigned long free_pool_pointer_position;
    
	if (_info_frame_no == 0) {
	  /*Kernel Frame Pool
       I assign the first frame position (i.e. 512*4096) in the kernel_frame_pool as the
       frame to hold the kernel frame management information
       */
        free_pool_pointer_position=((2 MB)/(4 KB))*FRAME_SIZE;
    }else{
        /* Process Frame Pool
         Here we put the process frame pool management information
         in the first free frame in the kernel frame pool (i.e. 513*4096)
         */
        free_pool_pointer_position=_info_frame_no*FRAME_SIZE;
    }
    
    /* Pointer our frame pool management data structure to the right position */
   
	free_frames = (oneByte * ) (free_pool_pointer_position);
    
    /*
     I use bit map method to manage the free frames. So one byte can hold 
     8 frames usage info. Then the total byte we need is nframes/8. Initilize
     them to 0 at first.
     */
	for (unsigned long i = 0; i < (nframes/8); i++) {
		*(free_frames + i) = 0x00;
	}
    
    /* We must set the frame number, which holding 
       the kernel frame management info as USED. Because 
       this frame number is assigned by us manually not from
       get_frame() method.

    */
    if(info_frame_no==0){
        
       *(free_frames+0) |= (1 << 0);
    }
        
        
}

unsigned long FramePool::get_frame() { 
    unsigned long i;
	unsigned char val;
	unsigned int j = 0;
    
    for (i=0; i< (nframes/8); i++) {
		val = *(this->free_frames + i);
 
		if (val != 255) // Find a byte is not 11111111, that means some frame is free.
			break;
	}

	if (val == 255) {
		/* If after finish the loop, we still did not find any free frame, return 0*/
		return 0;
	}
    
    /* Find which frame is free from our potential free frames value */
    /* For example if val=00000011 then j will be equal to 3*/
	
	while (true) {
		if ((val & (0x01 << j)) == 0) {break;}
                j++;
	}
    /* Calculate the actual fame number in the memory 
       i tell us which row our free frame is, j tell us in this
       row i, where is the free frame. i*8+j we get the frame number
       by plus base_frame_no, we now get the frame position in the memory
     
     */
    
	unsigned long free_frame_no = (i*8 + j)+base_frame_no;
    
    // Mark this frame no as USED frame in our frame management
    *(this->free_frames+i) |=(0x01<<j);
			
	return free_frame_no;
}

void FramePool::mark_inaccessible(unsigned long _base_frame_no,
                          unsigned long _nframes) {
    /* We need to subtract the base_frame_no from _base_frame_no to get
       the acutally position of this frame in our free_frames
     
     */
    unsigned long actual_position=_base_frame_no - base_frame_no;
    // Set the corrosponding bit to 1
	for (unsigned long i = 0; i < _nframes; i++) {
        *(free_frames+(actual_position+i)/8) |=(1<<((actual_position+i)%8));
        
	}
}

void FramePool::release_frame(unsigned long _frame_no) {
	unsigned long actual_position=_frame_no - base_frame_no;
    unsigned char mask=0x00;
    unsigned int offset=actual_position%8;
    mask |=(0x01<<offset);
    mask = ~mask;
    *(free_frames+(actual_position/8)) &= mask;
    
}


