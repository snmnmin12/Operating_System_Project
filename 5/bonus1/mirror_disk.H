#ifndef _MIRROR_DISK_H_
#define _MIRROR_DISK_H_

#include "simple_disk.H"

class MirroredDisk : public SimpleDisk {
public:
   MirroredDisk(DISK_ID _disk_id, unsigned int _size); 
   /* Creates a BlockingDisk device with the given size connected to the 
      MASTER or SLAVE slot of the primary ATA controller.
      NOTE: We are passing the _size argument out of laziness. 
      In a real system, we would infer this information from the 
      disk controller. */

   /* Return true if disk is ready to transfer data from/to disk, false otherwise. */
   bool is_A_ready(); /*Controller 0x1F0 is controller A with disk c*/
   bool is_B_ready(); /*Controller 0x170 is controller B with disk d*/

   virtual void issue_operation(DISK_OPERATION _op, unsigned long _block_no);
   /*Issue operatoin to both controller A with disk c and controller B with disk d*/

   virtual void read(unsigned long _block_no, unsigned char * _buf);
   /* Reads 512 Bytes from the given block of the disk and copies them 
      to the given buffer. No error check! */

   virtual void write(unsigned long _block_no, unsigned char * _buf);
   /* Writes 512 Bytes from the buffer to the given block on the disk. */

};

#endif
