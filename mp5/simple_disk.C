/*
     File        : simple_disk.c

     Author      : Riccardo Bettati
     Modified    : 10/04/01

     Description : Block-level READ/WRITE operations on a simple LBA28 disk 
                   using Programmed I/O.
                   
                   The disk must be MASTER or SLAVE on the PRIMARY IDE controller.

                   The code is derived from the "LBA HDD Access via PIO" 
                   tutorial by Dragoniz3r. (google it for details.)
*/

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

    /* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "assert.H"
#include "utils.H"
#include "console.H"
#include "simple_disk.H"

/*--------------------------------------------------------------------------*/
/* CONSTRUCTOR */
/*--------------------------------------------------------------------------*/
extern FramePool* SYSTEM_FRAME_POOL;
SimpleDisk::SimpleDisk(DISK_ID _disk_id, unsigned int _size) {
   disk_id   = _disk_id;
   disk_size = _size;
}

/*--------------------------------------------------------------------------*/
/* DISK CONFIGURATION */
/*--------------------------------------------------------------------------*/

unsigned int SimpleDisk::size() {
  return disk_size;
}

/*--------------------------------------------------------------------------*/
/* SIMPLE_DISK FUNCTIONS */
/*--------------------------------------------------------------------------*/

void SimpleDisk::issue_operation(DISK_OPERATION _op, unsigned long _block_no) {

  outportb(0x1F1, 0x00); /* send NULL to port 0x1F1         */
  outportb(0x1F2, 0x01); /* send sector count to port 0X1F2 */
  outportb(0x1F3, (unsigned char)_block_no); 
                         /* send low 8 bits of block number */
  outportb(0x1F4, (unsigned char)(_block_no >> 8)); 
                         /* send next 8 bits of block number */
  outportb(0x1F5, (unsigned char)(_block_no >> 16)); 
                         /* send next 8 bits of block number */
  outportb(0x1F6, ((unsigned char)(_block_no >> 24)&0x0F) | 0xE0 | (disk_id << 4));
                         /* send drive indicator, some bits, 
                            highest 4 bits of block no */

  outportb(0x1F7, (_op == READ) ? 0x20 : 0x30);

}

BOOLEAN SimpleDisk::is_ready() {
   return (inportb(0x1F7) & 0x08);
}

void SimpleDisk::read(unsigned long _block_no, unsigned char * _buf) {
/* Reads 512 Bytes in the given block of the given disk drive and copies them 
   to the given buffer. No error check! */

  issue_operation(READ, _block_no);

  wait_until_ready();

  /* read data from port */
  int i;
  unsigned short tmpw;
  for (i = 0; i < 256; i++) {
    tmpw = inportw(0x1F0);
    _buf[i*2]   = (unsigned char)tmpw;
    _buf[i*2+1] = (unsigned char)(tmpw >> 8);
  }
}

void SimpleDisk::write(unsigned long _block_no, unsigned char * _buf) {
/* Writes 512 Bytes from the buffer to the given block on the given disk drive. */

  issue_operation(WRITE, _block_no);

  wait_until_ready();

  /* write data to port */
  int i; 
  unsigned short tmpw;
  for (i = 0; i < 256; i++) {
    tmpw = _buf[2*i] | (_buf[2*i+1] << 8);
    outportw(0x1F0, tmpw);
  }

}

BlockingDisk::BlockingDisk(DISK_ID _disk_id,unsigned int _size):SimpleDisk(_disk_id,_size){
    queue* q ;
    q->Array=(Thread**)(SYSTEM_FRAME_POOL->get_frame());
    q->head=0;
    q->tail=0;
    blockq=q;
    
}

void BlockingDisk::read(unsigned long _block_no, unsigned char *_buf){
    SimpleDisk::read(_block_no, _buf);
}

void BlockingDisk::write(unsigned long _block_no, unsigned char *_buf){
    SimpleDisk::write(_block_no, _buf);
}

Thread* BlockingDisk::get_thread(){
    if (!is_ready()) {
        return NULL;
    }else{
        Thread* x;
        x=blockq->Array[blockq->head];
        blockq->head++;
        return x;
    }
    
}
