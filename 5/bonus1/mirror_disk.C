#include "assert.H"
#include "utils.H"
#include "console.H"
#include "mirror_disk.H"
#include "thread.H"
#include "scheduler.H"
extern Scheduler * SYSTEM_SCHEDULER;
/*--------------------------------------------------------------------------*/
/* CONSTRUCTOR */
/*--------------------------------------------------------------------------*/

MirroredDisk::MirroredDisk(DISK_ID _disk_id, unsigned int _size) 
  : SimpleDisk(_disk_id, _size) {
}

bool MirroredDisk::is_A_ready() {
	return ((Machine::inportb(0x1F7) & 0x08) != 0);
}

bool MirroredDisk::is_B_ready() {
	return ((Machine::inportb(0x177) & 0x08) != 0);
}

void MirroredDisk::issue_operation(DISK_OPERATION _op, unsigned long _block_no) {
          
	  //send instructions to controller A
	  Machine::outportb(0x1F1, 0x00); /* send NULL to port 0x1F1         */
	  Machine::outportb(0x1F2, 0x01); /* send sector count to port 0X1F2 */
	  Machine::outportb(0x1F3, (unsigned char)_block_no);
		                 /* send low 8 bits of block number */
	  Machine::outportb(0x1F4, (unsigned char)(_block_no >> 8));
		                 /* send next 8 bits of block number */
	  Machine::outportb(0x1F5, (unsigned char)(_block_no >> 16));
		                 /* send next 8 bits of block number */
	  Machine::outportb(0x1F6, ((unsigned char)(_block_no >> 24)&0x0F) | 0xE0 | (id() << 4));
		                 /* send drive indicator, some bits, 
		                    highest 4 bits of block no */
	  Machine::outportb(0x1F7, (_op == READ) ? 0x20 : 0x30);

          // send instructions to controller B
         
	  Machine::outportb(0x171, 0x00); 
	  Machine::outportb(0x172, 0x01); 
	  Machine::outportb(0x173, (unsigned char)_block_no);
	  Machine::outportb(0x174, (unsigned char)(_block_no >> 8));
	  Machine::outportb(0x175, (unsigned char)(_block_no >> 16));
	  Machine::outportb(0x176, ((unsigned char)(_block_no >> 24)&0x0F) | 0xE0 | (id() << 4));
	  Machine::outportb(0x177, (_op == READ) ? 0x20 : 0x30); 
}

void MirroredDisk::read(unsigned long _block_no, unsigned char * _buf) {

  issue_operation(READ, _block_no);

  if (!(is_A_ready()||is_B_ready())) {
        Console::puts("CPU is switched to the other devices*********************!\n");
   	SYSTEM_SCHEDULER->resume(Thread::CurrentThread()); 
        SYSTEM_SCHEDULER->yield();
   }

  /* read data from port */
  int i;
  if (is_B_ready()) {
          //B ready, then read B
	  unsigned short tmpw;
	  for (i = 0; i < 256; i++) {
	 	tmpw = Machine::inportw(0x170);
		_buf[i*2]   = (unsigned char)tmpw;
		_buf[i*2+1] = (unsigned char)(tmpw >> 8);
	  }

  } else if (is_A_ready()) {
         // A ready then read A
	 unsigned short tmpw;
	  for (i = 0; i < 256; i++) {
	 	tmpw = Machine::inportw(0x1F0);
		_buf[i*2]   = (unsigned char)tmpw;
		_buf[i*2+1] = (unsigned char)(tmpw >> 8);
	  }
  }

}


void MirroredDisk::write(unsigned long _block_no, unsigned char * _buf) {
  // -- REPLACE THIS!!!
  //SimpleDisk::read(_block_no, _buf);
   issue_operation(WRITE, _block_no);

   if (!(is_A_ready() && is_B_ready())) {
	Console::puts("CPU is switched to the other devices*********************!\n");
   	SYSTEM_SCHEDULER->resume(Thread::CurrentThread()); 
        SYSTEM_SCHEDULER->yield();
   }

  /* write data to port */
  int i; 
  unsigned short tmpw;
  for (i = 0; i < 256; i++) {
    tmpw = _buf[2*i] | (_buf[2*i+1] << 8);
    Machine::outportw(0x170, tmpw);
    Machine::outportw(0x1F0, tmpw);
  }


}
