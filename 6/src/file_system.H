/* 
    File: file_system.H

    Author: R. Bettati
            Department of Computer Science
            Texas A&M University
    Date  : 10/04/05

    Description: Simple File System.
    

*/

#ifndef _FILE_SYSTEM_H_                   // include file only once
#define _FILE_SYSTEM_H_

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "file.H"
#include "simple_disk.H"

/*--------------------------------------------------------------------------*/
/* DATA STRUCTURES */ 
/*--------------------------------------------------------------------------*/
#define BLOCK_SIZE 512
#define MAX_FILE_SIZE 63
#define SUPER_BLOCK_SIZE 1024
#define BITMAP_SIZE 3072
#define MAX_SYSTEM_SIZE 10485760
static unsigned char buff[BLOCK_SIZE];
static unsigned char temp[BLOCK_SIZE];
static unsigned char temp1[BLOCK_SIZE];
static unsigned char super_block_data[SUPER_BLOCK_SIZE];
/* -- (none) -- */
struct SuperBlock {
	unsigned int fsize;
        unsigned int inode_index;
	unsigned int files_no[MAX_FILE_SIZE];
	unsigned int files_size[MAX_FILE_SIZE];
	unsigned int inode_no[MAX_FILE_SIZE];
	unsigned int inode_size[MAX_FILE_SIZE];
};
struct I_Node {
	unsigned int file_id;
        unsigned int next_block;
	unsigned int curr_size;
	unsigned int blocks[125];
};
/*--------------------------------------------------------------------------*/
/* FORWARD DECLARATIONS */ 
/*--------------------------------------------------------------------------*/
/* -- (none) -- */
/*--------------------------------------------------------------------------*/
/* F i l e S y s t e m  */
/*--------------------------------------------------------------------------*/

class FileSystem {

friend class File; /* -- not sure if we need this; feel free to delete */

//private:
public:
     /* -- DEFINE YOUR FILE SYSTEM DATA STRUCTURES HERE. */
     SimpleDisk * disk;
     unsigned int size;
     unsigned char bitmap[BITMAP_SIZE];
     File* files;
     bool* file_locks;
     unsigned int file_size;
     unsigned int inode_index;
     
public:

    FileSystem();
    /* Just initializes local data structures. Does not connect to disk yet. */
    
    bool Mount(SimpleDisk * _disk);
    /* Associates this file system with a disk. Limit to at most one file system per disk.
     Returns true if operation successful (i.e. there is indeed a file system on the disk.) */
    
    static bool Format(SimpleDisk * _disk, unsigned int _size);
    /* Wipes any file system from the disk and installs an empty file system of given size. */
    
    File * LookupFile(int _file_id);
    /* Find file with given id in file system. If found, return the initialized
     file object. Otherwise, return null. */

    bool CreateFile(int _file_id);
    /* Create file with given id in the file system. If file exists already,
     abort and return false. Otherwise, return true. */
    
    bool DeleteFile(int _file_id);
    /* Delete file with given id in the file system; free any disk block occupied by the file. */
   
    unsigned int getFreeBlock();
    /* get one free block from the disk*/

    //thread-safe design
    bool lock(int file_id);
    bool unlock(int file_id);
    bool is_locked(int index);
    void update_delete_locks(int i);
    void update_add_locks();

    //update the disk
    void updateDisk(); 

    void releaseBitmap(unsigned int startIndex); // set the bitmap to be unused;

    void updateBitmap(unsigned int startIndex); //update the bitmap to set it used

    void check_free_block(); /*check if the current block is free*/
};
#endif
