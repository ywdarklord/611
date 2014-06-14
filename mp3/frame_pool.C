#include"frame_pool.H"

FramePool* FramePool::pools[2];//One for kernel memory pool and one for process memory pool
int FramePool::numberOfFramePools = 0;

FramePool::FramePool(unsigned long _base_frame_no,
				     unsigned long _nframes,
					 unsigned long _info_frame_no){
	base_frame_no = _base_frame_no;
	nframes = _nframes;

	if(_info_frame_no == 0){
                  /*Kernel Frame Pool
       I assign the first frame position (i.e. 512*4096) in the kernel_frame_pool as the
       frame to hold the kernel frame management information
       */
		
		bitmap = (unsigned char*)(((2 MB)/(4 KB))*FRAME_SIZE);
		bitmap[0] = 1;

	}else{
		info_frame_no = _info_frame_no;
		bitmap = (unsigned char*)(info_frame_no*FRAME_SIZE);
		
	}
        for(int i = 1; i<nframes/8; i++){
			bitmap[i] = 0;
	}
	pools[numberOfFramePools] = this;
	numberOfFramePools++;
}

unsigned long FramePool::get_frame(){
	unsigned long i = 0;
	for(;i<nframes/8; i++){
		if(bitmap[i]!=0xFF){// Find a byte is not 11111111, that means some frame is free.
			break;
		}
	}
	if(i==nframes/8){
/* If after finish the loop, we still did not find any free frame, return 0*/
		Console::puts("NO Frame, should not happen");
		return 0;
	}
	else{   /* Find which frame is free from our potential free frames value */
    /* For example if val=00000011 then j will be equal to 3*/
	
		unsigned char val = bitmap[i];
		unsigned long j = 0;
		while(true){
			if((val &(1<<j))==0)
				break;	
			j ++;
		}
	bitmap[i]|=0x01<<j;
 /* Calculate the actual fame number in the memory 
       i tell us which row our free frame is, j tell us in this
       row i, where is the free frame. i*8+j we get the frame number
       by plus base_frame_no, we now get the frame position in the memory
     
     */
	return base_frame_no+8*i+j;
	}
}

void FramePool::mark_inaccessible(unsigned long _base_frame_no,
					   unsigned long _nframes){
	 /* We need to subtract the base_frame_no from _base_frame_no to get
       the acutally position of this frame in our free_frames
     
     */
    unsigned long actual_position=_base_frame_no - base_frame_no;
    // Set the corrosponding bit to 1
	for (unsigned long i = 0; i < _nframes; i++) {
        *(bitmap+(actual_position+i)/8) |=(1<<((actual_position+i)%8));
        
	}
}

void FramePool::release_frame(unsigned long _frame_no){
	unsigned long current_frame_no;
	unsigned long current_nframe;
	int i;
	for(i = 0; i<numberOfFramePools; i++){
		current_frame_no = pools[i]->base_frame_no;
		current_nframe = pools[i]->nframes;
		if((_frame_no<=current_frame_no+current_nframe)&&(_frame_no>=current_frame_no)){
			break;
		}
	}

    unsigned long actual_position=_frame_no - pools[i]->base_frame_no;
    unsigned char mask=0x00;
    unsigned int offset=actual_position%8;
    mask |=(0x01<<offset);
    mask = ~mask;
    *(pools[i]->bitmap+(actual_position/8)) &= mask;
    
}


