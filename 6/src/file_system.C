/*
     File        : file_system.C

     Author      : Riccardo Bettati
     Modified    : 2017/05/01

     Description : Implementation of simple File System class.
                   Has support for numerical file identifiers.
 */

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

    /* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "assert.H"
#include "console.H"
#include "file_system.H"


/*--------------------------------------------------------------------------*/
/* CONSTRUCTOR */
/*--------------------------------------------------------------------------*/

FileSystem::FileSystem() {
    Console::puts("In file system constructor.\n");
    memset(bitmap, 0, BITMAP_SIZE);
    //memset(buff, 0, BLOCK_SIZE);
    size = 0;
    file_size = 0;
    files = NULL;
    file_locks = NULL;
    inode_index = 0;
}

/*--------------------------------------------------------------------------*/
/* FILE SYSTEM FUNCTIONS */
/*--------------------------------------------------------------------------*/

bool FileSystem::Mount(SimpleDisk * _disk) {
    Console::puts("mounting file system form disk\n");
    disk = _disk;

    //get how many files there
    memset(super_block_data, 0, SUPER_BLOCK_SIZE);
    unsigned int num = SUPER_BLOCK_SIZE/512;
    for (int i = 0; i < num; i++) {
	disk->read(i, buff);
	memcpy(&super_block_data[i*512], buff, 512);
    }

    SuperBlock* superblock = (SuperBlock*) super_block_data;
    file_size = superblock->fsize;
    inode_index = superblock->inode_index;

    if (file_size != 0) {
	files = (File*) new File[file_size];
 
	for (int i = 0; i < file_size; i++) {
	files[i].file_id = superblock->files_no[i];
	files[i].file_size = superblock->files_size[i];
	files[i].inode_no = superblock->inode_no[i];
	files[i].inode_size = superblock->inode_size[i];
	}
    }
    
    //update bitmap
    int num_bitmap = BITMAP_SIZE/512;
    for (int i = 0; i < num_bitmap; i++) {
	disk->read(num+i, buff);
	memcpy(&bitmap[i*512],buff, BLOCK_SIZE);
    }

    return true;
   //update data
   //no need
}

bool FileSystem::Format(SimpleDisk * _disk, unsigned int _size) {

    Console::puts("formatting disk\n");
    int num_of_blocks = _size/BLOCK_SIZE;
    if (num_of_blocks*BLOCK_SIZE < _size)
	num_of_blocks += 1; 
   
    //update superblock in disk section 1 and 2
   // unsigned char data[SUPER_BLOCK_SIZE];
    memset(super_block_data, 0, SUPER_BLOCK_SIZE);
    SuperBlock* superblock = (SuperBlock*) super_block_data;

    superblock->fsize = 0;
    superblock->inode_index = 8;
    memset(superblock->files_no, 0, MAX_FILE_SIZE);
    memset(superblock->files_size, 0, MAX_FILE_SIZE);
    memset(superblock->inode_no, 0, MAX_FILE_SIZE);
    memset(superblock->inode_size, 0, MAX_FILE_SIZE);

    unsigned int num = SUPER_BLOCK_SIZE/512;
    for (int i = 0; i < num; i++) {
	memcpy(buff, &super_block_data[i*512],512);
	_disk->write(i, buff);
    }
    
    //update bitmap in disk section
    int num_bitmap = BITMAP_SIZE/512;
    for (int i = 0; i < num_bitmap; i++) {
	memset(buff, 0, 512);
	if (i == 0) {
		 buff[0] = 0xFF;
	}
	_disk->write(num+i, buff);
    }

    //update data section in later
    unsigned int start_block_no = num+num_bitmap;
    memset(buff, 0, 512);
    for (int i = 0; i < num_of_blocks; i++) {
	_disk->write(i+start_block_no, buff);
    }
    return true;
    
}

File * FileSystem::LookupFile(int _file_id) {
    //Console::puts("looking up file\n");
    for (int i = 0; i < file_size; i++) {
	if (files[i].file_id  == _file_id) {
		//File* new_file = new File(this, disk);
		//new_file->file_id = _file_id;
    		//new_file->file_size = files[i].file_size;
   		//new_file->inode_no = files[i].inode_no;
    		//new_file->inode_size = files[i].inode_size;
		//return 	new_file;
		//while(is_locked(_file_id));
		//lock(_file_id);
		return &files[i];
	}
    }
    return NULL;
}

bool FileSystem::CreateFile(int _file_id) {
    Console::puts("creating file\n");
    for (int i = 0; i < file_size; i++) {
		if (files[i].file_id  == _file_id) {
        		Console::puts("File Exists\n");
		return false;
	}
     }
     
    File* new_file = new File(this, disk);
    new_file->file_id = _file_id;
    new_file->inode_no = getFreeBlock();

    //update inode block
    disk->read(new_file->inode_no, buff);
    I_Node* i_node = (I_Node*) buff;
    i_node->file_id = _file_id;
    i_node->next_block = 0;
    i_node->curr_size = 0;
    memset(i_node->blocks, 0, 500);
    new_file->inode_size += 1;
    disk->write(new_file->inode_no,buff);
    
    if (file_size == 0) {
	files = (File*) new File[1];
        files[0] = *new_file;
    } else {
	
       File* new_files = (File*) new File[file_size + 1];
       for (int i = 0; i < file_size; i++) {
	   new_files[i] = files[i];
       }
       new_files[file_size] = *new_file;
       delete[] files;
       files = new_files;	
    }

    //add locks
    update_add_locks();

    file_size++;
    //assert(false);

    //update superblock in disk and bitmap in disk
    updateDisk();
    return true;
    
}

bool FileSystem::DeleteFile(int _file_id) {
    Console::puts("deleting file\n");
    unsigned int i = 0;
    for (; i < file_size; i++) {
	if (files[i].file_id  == _file_id)
             break;
    }
    // if file not exists, then return false;
    if (i == file_size)
	return false;

    while(is_locked(_file_id));
    lock(_file_id);

    //now start deleting
    unsigned int inode_no   =  files[i].inode_no;
    unsigned int inode_size =  files[i].inode_size;
    for (int j = 0; j < inode_size; j++) {
	disk->read(inode_no, buff);
	I_Node* i_node = (I_Node*) buff;
	memset(temp,0,512);
	for (int k = 0; k < i_node->curr_size; k++) {
		disk->write(i_node->blocks[k], temp);
		releaseBitmap(i_node->blocks[k]);
 	}
	unsigned int next_block = i_node->next_block;
        memset(buff, 0, 512);
        disk->write(inode_no, buff);
	releaseBitmap(inode_no);
	inode_no = next_block;
    }
   
    //update files array
    if (file_size == 1) {
	delete[] files;
	files = NULL;
    } else {
    	
	File* new_files = (File*) new File[file_size - 1];
        for (int j = 0, k = 0; j < file_size-1, k < file_size; k++) {
	   if (k != i) {
	   	new_files[j] = files[k];
                j++;
 	   }
        }
	delete[] files;
 	files = new_files;
    }
    //delete locks
    update_delete_locks(i);
    file_size--;
    //Console::puts("File size: "); Console::puti();	
    updateDisk();
   return true;
}

unsigned int FileSystem::getFreeBlock() {
        check_free_block();
        updateBitmap(inode_index);
	inode_index++;
	return inode_index-1;
}

void FileSystem::updateBitmap(unsigned int startIndex) {
	unsigned int bitmap_index = startIndex/8;
	unsigned int bitmap_offset = startIndex % 8;
	bitmap[bitmap_index] = bitmap[bitmap_index] ^ (0x80 >> bitmap_offset);
}

void FileSystem::releaseBitmap(unsigned int startIndex) {
	unsigned int bitmap_index  = startIndex / 8;
        unsigned int bitmap_offset = startIndex % 8;
        bitmap[bitmap_index] &= ~(1 << bitmap_offset);
}

void FileSystem::updateDisk() {

    //update super block in disk
    unsigned int num = SUPER_BLOCK_SIZE/512;
    for (int i = 0; i < num; i++) {
	disk->read(i, buff);
	memcpy(&super_block_data[i*512], buff, 512);
    }

    SuperBlock* superblock = (SuperBlock*) super_block_data;

    superblock->fsize = file_size;
    superblock->inode_index = inode_index;

    for (int i = 0; i < file_size; i++) {
	superblock->files_no[i]   = files[i].file_id;
	superblock->files_size[i] = files[i].file_size;
	superblock->inode_no[i]   = files[i].inode_no;
	superblock->inode_size[i] = files[i].inode_size;
    }

    for (int i = 0; i < num; i++) {
	memcpy(buff, &super_block_data[i*512], 512);
        disk->write(i, buff);
    }
    
    //update bitmap in disk
    int num_bitmap = BITMAP_SIZE/512;
    for (int i = 0; i < num_bitmap; i++) {
	memcpy(buff, &bitmap[i*512], 512);
	disk->write(num+i, buff);
    }
}

void FileSystem::check_free_block() {
    unsigned int max_blocks = MAX_SYSTEM_SIZE/BLOCK_SIZE;
    unsigned int greaterAgain = 0;
    unsigned int bitmap_index;
    unsigned int bitmap_offset;
    while (1) {
	if (inode_index >= max_blocks) {
		inode_index = 8;
		greaterAgain += 1;
   	}
	if (greaterAgain == 2) {
		Console::puts("The disk is fully occupied, no extra!\n");
		for(;;);
        }
	bitmap_index =  inode_index/8;
	bitmap_offset = inode_index % 8;
	if ((bitmap[bitmap_index] & (0x80 >> bitmap_offset)) == 0x0) 
		break;
	inode_index++;
    }
}

bool FileSystem::is_locked(int file_id) {
    unsigned int i = 0;
    for (; i < file_size; i++) {
	if (files[i].file_id  == file_id)
             break;
    }
	return file_locks[i];
}

bool FileSystem::lock(int file_id) {
    unsigned int i = 0;
    for (; i < file_size; i++) {
	if (files[i].file_id  == file_id)
             break;
    }
    if (i == file_size) return false;
    // if file not exists, then return false;
    file_locks[i] = true;
    return true;
}
bool FileSystem::unlock(int file_id) {
    unsigned int i = 0;
    for (; i < file_size; i++) {
	if (files[i].file_id  == file_id)
             break;
    }
    if (i == file_size) return false;
    // if file not exists, then return false;
    file_locks[i] = false;
    return true;
}
void FileSystem::update_delete_locks(int i) {
    if (file_size == 1) {
	delete[] file_locks;
	file_locks = NULL;
    } else {
    	
	bool* new_locks = (bool*) new bool[file_size - 1];
        for (int j = 0, k = 0; j < file_size-1, k < file_size; k++) {
	   if (k != i) {
	   	new_locks[j] = file_locks[k];
                j++;
 	   }
        }
	delete[] file_locks;
 	file_locks = new_locks;
    }
}
void FileSystem::update_add_locks() {
    if (file_size == 0) {
	file_locks =  new bool[1];
        file_locks[0] = false;
    } else {
	
       bool* new_locks =  new bool[file_size + 1];
       for (int i = 0; i < file_size; i++) {
	   new_locks[i] = file_locks[i];
       }
       new_locks[file_size] = false;
       delete[] file_locks;
       file_locks = new_locks;	
    }
}
