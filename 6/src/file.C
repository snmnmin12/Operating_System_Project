/*
     File        : file.C

     Author      : Riccardo Bettati
     Modified    : 2017/05/01

     Description : Implementation of simple File class, with support for
                   sequential read/write operations.
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
#include "file.H"
#include "file_system.H"

/*--------------------------------------------------------------------------*/
/* CONSTRUCTOR */
/*--------------------------------------------------------------------------*/

//File::File() {
File::File() {

    //Console::puts("In file constructor.\n");
    file_id = 0;
    file_size = 0;
    inode_no = 0;
    inode_size = 0;
    cur_block = 0; 
    cur_size = 0;
    cur_position = 0;
    file_system = NULL;
    disk = NULL;
}
File::File(FileSystem* _file_system, SimpleDisk * _disk) {
    /* We will need some arguments for the constructor, maybe pointer to disk
     block with file management and allocation data. */
    //Console::puts("In file constructor.\n");
    file_id = 0;
    file_size = 0;
    inode_no = 0;
    inode_size = 0;
    cur_size = 0;
    cur_block = 0; 
    cur_position = 0;
    file_system = _file_system;
    disk = _disk;
}

/*--------------------------------------------------------------------------*/
/* FILE FUNCTIONS */
/*--------------------------------------------------------------------------*/

int File::Read(unsigned int _n, char * _buf) {

	if (_n == 0) return 0;

	//lock first
	while(file_system->is_locked(file_id));
	file_system->lock(file_id);

	//Console::puts("reading from file\n");
        unsigned int acc_size = 0;
	unsigned int pre_size = 0;
	I_Node* i_node  = NULL;
	unsigned int start_inode = inode_no;
        //unsigned char temp1[512];
	//Console::puts("inode_no: ");Console::puti(inode_no);Console::puts("\n");
        //Console::puts("inode_size: ");Console::puti(inode_size);Console::puts("\n");
	for (int j = 0; j < inode_size; j++) {
		disk->read(start_inode, temp1);
		i_node = (I_Node*) temp1;
		acc_size += i_node->curr_size;
		if ((pre_size <= cur_size) && (acc_size > cur_size)) {
			break;
		}
		start_inode = i_node->next_block;
		pre_size = acc_size;
	}

       // Console::puts("next_block: ");Console::puti(i_node->next_block);Console::puts("\n");
	unsigned int count = _n;
	unsigned int block_index = cur_size - pre_size;
	unsigned int k = 0;

	if (EoF() && count >0)  return 0;
	if (block_index >= 125) {
		Console::puts("You should not be here!\n");
		for(;;);	
	}
	//Console::puts("block_index: ");Console::puti(block_index);
	//Console::puts("block_no: ");Console::puti(i_node->blocks[block_index]);
        //Console::puts("\n");
	//for(;;);

	disk->read(i_node->blocks[block_index], buff);
	//cur_size += 1;
	for (; k < count; k++, cur_position++) {
		if (EoF())  break;
		if (cur_position >= 512) {
			block_index++;
			//Console::puts("k: ");Console::puti(k);Console::puts("\n");	
			//Console::puts("count: ");Console::puti(count);Console::puts("\n");
			if (block_index == 125) {
				start_inode = i_node->next_block;
				disk->read(start_inode, temp1);
				i_node = (I_Node*) temp1;
				block_index = 0;
			}
			disk->read(i_node->blocks[block_index], buff);
			cur_size += 1;
			cur_position = 0;
		}
		_buf[k] = buff[cur_position];
	}
	//Console::puts("File size: ");Console::puti(cur_size); Console::puts("\n");
	//Console::puts("k: ");Console::puti(k); Console::puts("\n");
        file_system->unlock(file_id);
	return k;
	//Console::puts("Read completed!");
}


void File::Write(unsigned int _n, const char * _buf) {
	if (_n == 0) return;

	//lock first
	while(file_system->is_locked(file_id));
	file_system->lock(file_id);
	
	
	//Console::puts("writing to file\n");
        unsigned int acc_size = 0;
	unsigned int pre_size = 0;
	I_Node* i_node  = NULL;
	unsigned int start_inode = inode_no;
	unsigned int curr_inode = 0;
	//unsigned char temp1[512];
	//Console::puts("Inode_no: ");Console::puti(inode_no); Console::puts("\n");
	for (int j = 0; j < inode_size; j++) {
		disk->read(start_inode, temp1);
		i_node = (I_Node*) temp1;
		curr_inode = start_inode;
		start_inode = i_node->next_block;
	}
	//Console::puts("Inode_size: ");Console::puti(inode_size); Console::puts("\n");
        unsigned int section_pos = file_size - ((inode_size-1)*125 + i_node->curr_size-1)*512;
	//Console::puts("The i_node->curr_size: ");Console::puti(i_node->curr_size);Console::puts("\n");
        //Console::puts("section_pos: ");Console::puti(section_pos);Console::puts("\n");

	if (i_node->curr_size == 125 && section_pos == 512) {
			if (i_node->curr_size == 125 && section_pos == 512) {
				check_inode_full(&curr_inode, i_node);
			}
/*
		inode_size += 1;
		i_node->next_block = file_system->getFreeBlock();
		disk->write(curr_inode, temp1);	
		curr_inode = i_node->next_block;
		disk->read(curr_inode, temp1);
		i_node = (I_Node*) temp1;	
		i_node->file_id = file_id;
		i_node->next_block = 0;
		i_node->curr_size = 0;
		memset(i_node->blocks, 0, 500);	
		disk->write(curr_inode, temp1);	
	Console::puts("i_node->next_block: ");Console::puti(i_node->next_block);Console::puts("\n");
Console::puts("i_node->blocks[i_node->curr_size- 1]: ");Console::puti(i_node->blocks[i_node->curr_size- 1]); Console::puts("\n");*/	
	}

	if (i_node->curr_size == 0) {
		i_node->curr_size += 1;	
		i_node->blocks[0] = file_system->getFreeBlock();
		disk->write(curr_inode, temp1);
	//Console::puts("i_node->blocks[0]: "); Console::puti(i_node->blocks[i_node->curr_size-1]); Console::puts("\n");
	} 
	//Console::puts("current_inode: "); Console::puti(curr_inode); Console::puts("\n");
        //Console::puts("inode_next: "); Console::puti(i_node->next_block); Console::puts("\n");
        section_pos = file_size - ((inode_size-1)*125 + i_node->curr_size-1)*512;

	//data
	unsigned int remaining = 512 - section_pos;
	//Console::puts("remaining: "); Console::puti(remaining); Console::puts("\n");
	//Console::puts("_n: "); Console::puti(_n); Console::puts("\n");
	if (_n <= remaining) {
		disk->read(i_node->blocks[i_node->curr_size-1], buff);
		memcpy(&buff[section_pos], _buf,_n);
		disk->write(i_node->blocks[i_node->curr_size-1], buff);
	} else {
		if (remaining > 0) {
			disk->read(i_node->blocks[i_node->curr_size-1], buff);
			memcpy(&buff[section_pos], _buf ,remaining);
			disk->write(i_node->blocks[i_node->curr_size-1], buff);
			section_pos += remaining;
			if (i_node->curr_size == 125 && section_pos == 512) {
				check_inode_full(&curr_inode, i_node);
			}
		}
		//calculate how many datablocks reuired
		unsigned int count = _n - remaining;
                //unsigned int total_required = count / 512;
		//unsigned int offset = count % 512;
		//if (offset > 0) total_required += 1;
	//Console::puts("count: "); Console::puti(count); Console::puts("\n");
	//Console::puts("_n: "); Console::puti(_n); Console::puts("\n");
	//Console::puts("section_pos: "); Console::puti(section_pos); Console::puts("\n");
		disk->read(curr_inode, temp1);
		i_node = (I_Node*) temp1;
		unsigned int already_read = remaining;
		unsigned int new_block_no;
		while (count > 0) {

			i_node->curr_size += 1;		
			new_block_no = file_system->getFreeBlock();
			i_node->blocks[i_node->curr_size-1] = new_block_no;
//Console::puts("i_node->curr_size: "); Console::puti(i_node->curr_size); Console::puts("\n");
//Console::puts("i_node->blocks[i_node->curr_size- 1]: "); Console::puti(new_block_no); Console::puts("\n");
//Console::puts("already_read: "); Console::puti(already_read); Console::puts("\n");
			if (count < 512) {
				memset(temp, 0, 512);
				memcpy(temp, &_buf[already_read], count);
//Console::puts("i_node->blocks[i_node->curr_size- 1]: "); Console::puti(new_block_no); Console::puts("\n");
				disk->write(new_block_no, temp);
				already_read += count;
				count = 0;
			} else {
				memcpy(temp, &_buf[already_read], 512);
				disk->write(new_block_no, temp);
				count = count - 512;
				already_read += 512;
			}
			//Console::puts("The i_node->curr_size: ");Console::puti(i_node->curr_size);
			//	Console::puts("\n");
			if (i_node->curr_size == 125) {
	//section_pos = file_size + _n - ((inode_size-1)*125 + i_node->curr_size-1)*512;
	//Console::puts("The i_node->curr_size: ");Console::puti(i_node->curr_size);Console::puts("\n");
	//Console::puts("Count: ");Console::puti(count);Console::puts("\n");
        //Console::puts("section_pos: ");Console::puti(section_pos);Console::puts("\n");
				if (count > 0) {
					check_inode_full(&curr_inode, i_node);
					/*inode_size += 1;
					i_node->next_block = file_system->getFreeBlock();
		                        disk->write(curr_inode, buff);
					curr_inode = i_node->next_block;
					disk->read(curr_inode, buff);
					i_node = (I_Node*) buff;
					i_node->file_id = file_id;
					i_node->next_block = 0;
					i_node->curr_size = 0;
					memset(i_node->blocks, 0, 500);
	Console::puts("i_node->next_block: ");Console::puti(i_node->next_block);Console::puts("\n");
Console::puts("i_node->blocks[i_node->curr_size- 1]: ");Console::puti(i_node->blocks[i_node->curr_size- 1]); Console::puts("\n"); */

				}else {
    					disk->write(curr_inode, temp1);
				}
			}
			
		}
		disk->write(curr_inode, temp1);
		
	}

	file_size += _n;
	file_system->unlock(file_id);
	//section_pos = file_size - ((inode_size-1)*125 + i_node->curr_size-1)*512;
	//Console::puts("section_pos: "); Console::puti(section_pos); Console::puts("\n");
	//Console::puts("Inode_cur Size: "); Console::puti(i_node->curr_size); Console::puts("\n");

}

void File::Reset() {
	//Console::puts("reset current position in file\n");
	while(file_system->is_locked(file_id));
	file_system->lock(file_id);
	cur_block = 0; 
	cur_size = 0;
	cur_position = 0;
	file_system->unlock(file_id);
}

void File::Rewrite() {
	Console::puts("erase content of file\n");
	unsigned int start_inode = inode_no;
	for (int j = 0; j < inode_size; j++) {
		disk->read(start_inode, buff);
		I_Node* i_node = (I_Node*) buff;
		//unsigned char temp[512];
		memset(temp,0,512);
		for (int k = 0; k < i_node->curr_size; k++) {
			disk->write(i_node->blocks[k], temp);
		}
		start_inode = i_node->next_block;
	}
}

bool File::EoF() {
	//Console::puts("testing end-of-file condition\n");
	if (file_size == 0) return true;
	if (((cur_size)*512 + cur_position) == file_size)
		return true;
	return false;	
}

void File::check_inode_full(unsigned int* curr_inode, I_Node* i_node) {
		inode_size += 1;
		i_node->next_block = file_system->getFreeBlock();
		disk->write(*curr_inode, temp1);	
		*curr_inode = i_node->next_block;
		disk->read(*curr_inode, temp1);
		i_node = (I_Node*) temp1;	
		i_node->file_id = file_id;
		i_node->next_block = 0;
		i_node->curr_size = 0;
		memset(i_node->blocks, 0, 500);	
		disk->write(*curr_inode, temp1);	
	//Console::puts("i_node->next_block: ");Console::puti(i_node->next_block);Console::puts("\n");
//Console::puts("i_node->blocks[i_node->curr_size- 1]: ");Console::puti(i_node->blocks[i_node->curr_size- 1]); Console::puts("\n");	
}
