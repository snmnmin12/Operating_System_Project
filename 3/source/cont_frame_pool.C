/*
 File: ContFramePool.C
 
 Author: Mingmin Song
 Date  : 6/17/2017
 
 */

/*--------------------------------------------------------------------------*/
/* 
 POSSIBLE IMPLEMENTATION
 -----------------------

 The class SimpleFramePool in file "simple_frame_pool.H/C" describes an
 incomplete vanilla implementation of a frame pool that allocates 
 *single* frames at a time. Because it does allocate one frame at a time, 
 it does not guarantee that a sequence of frames is allocated contiguously.
 This can cause problems.
 
 The class ContFramePool has the ability to allocate either single frames,
 or sequences of contiguous frames. This affects how we manage the
 free frames. In SimpleFramePool it is sufficient to maintain the free 
 frames.
 In ContFramePool we need to maintain free *sequences* of frames.
 
 This can be done in many ways, ranging from extensions to bitmaps to 
 free-lists of frames etc.
 
 IMPLEMENTATION:
 
 One simple way to manage sequences of free frames is to add a minor
 extension to the bitmap idea of SimpleFramePool: Instead of maintaining
 whether a frame is FREE or ALLOCATED, which requires one bit per frame, 
 we maintain whether the frame is FREE, or ALLOCATED, or HEAD-OF-SEQUENCE.
 The meaning of FREE is the same as in SimpleFramePool. 
 If a frame is marked as HEAD-OF-SEQUENCE, this means that it is allocated
 and that it is the first such frame in a sequence of frames. Allocated
 frames that are not first in a sequence are marked as ALLOCATED.
 
 NOTE: If we use this scheme to allocate only single frames, then all 
 frames are marked as either FREE or HEAD-OF-SEQUENCE.
 
 NOTE: In SimpleFramePool we needed only one bit to store the state of 
 each frame. Now we need two bits. In a first implementation you can choose
 to use one char per frame. This will allow you to check for a given status
 without having to do bit manipulations. Once you get this to work, 
 revisit the implementation and change it to using two bits. You will get 
 an efficiency penalty if you use one char (i.e., 8 bits) per frame when
 two bits do the trick.
 
 DETAILED IMPLEMENTATION:
 
 How can we use the HEAD-OF-SEQUENCE state to implement a contiguous
 allocator? Let's look a the individual functions:
 
 Constructor: Initialize all frames to FREE, except for any frames that you 
 need for the management of the frame pool, if any.
 
 get_frames(_n_frames): Traverse the "bitmap" of states and look for a 
 sequence of at least _n_frames entries that are FREE. If you find one, 
 mark the first one as HEAD-OF-SEQUENCE and the remaining _n_frames-1 as
 ALLOCATED.

 release_frames(_first_frame_no): Check whether the first frame is marked as
 HEAD-OF-SEQUENCE. If not, something went wrong. If it is, mark it as FREE.
 Traverse the subsequent frames until you reach one that is FREE or 
 HEAD-OF-SEQUENCE. Until then, mark the frames that you traverse as FREE.
 
 mark_inaccessible(_base_frame_no, _n_frames): This is no different than
 get_frames, without having to search for the free sequence. You tell the
 allocator exactly which frame to mark as HEAD-OF-SEQUENCE and how many
 frames after that to mark as ALLOCATED.
 
 needed_info_frames(_n_frames): This depends on how many bits you need 
 to store the state of each frame. If you use a char to represent the state
 of a frame, then you need one info frame for each FRAME_SIZE frames.
 
 A WORD ABOUT RELEASE_FRAMES():
 
 When we releae a frame, we only know its frame number. At the time
 of a frame's release, we don't know necessarily which pool it came
 from. Therefore, the function "release_frame" is static, i.e., 
 not associated with a particular frame pool.
 
 This problem is related to the lack of a so-called "placement delete" in
 C++. For a discussion of this see Stroustrup's FAQ:
 http://www.stroustrup.com/bs_faq2.html#placement-delete
 
 */
/*--------------------------------------------------------------------------*/


/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "cont_frame_pool.H"
#include "console.H"
#include "utils.H"
#include "assert.H"

/*--------------------------------------------------------------------------*/
/* DATA STRUCTURES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* CONSTANTS */
ContFramePool* ContFramePool::pool_list = NULL;
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* FORWARDS */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */
/*
@bitmap is the current bitmap
@i is the current index of the bitmap
@mask is the current mask used to operate on bitmap
@return the last index if we can find continuous address
*/
unsigned int checkContinuous(unsigned char* bitmap, unsigned int i, unsigned char mask, unsigned int _n_frames) {
    unsigned char temp = bitmap[i];
    temp = temp ^ mask;
    //I used on variable to check the bitmap[i] is all used
    //all even bits are zero, then is all used, 10101010 = AA
    for (int j = 1; j < _n_frames; j++) {
        mask = mask >> 2;
        if ((temp & 0xAA) == 0x0) {
            i = i + 1;
            temp = bitmap[i];
            if (temp != 0xFF) { 
 		    //Console::puts("Fails in check continuous!\n");
		    return i;
	   }
            mask = 0x80;
        }
        temp = temp ^ mask;
    }
    return -1;
}
/*--------------------------------------------------------------------------*/
/* METHODS FOR CLASS   C o n t F r a m e P o o l */
/*--------------------------------------------------------------------------*/

ContFramePool::ContFramePool(unsigned long _base_frame_no,
                             unsigned long _n_frames,
                             unsigned long _info_frame_no,
                             unsigned long _n_info_frames)
{
    // TODO: IMPLEMENTATION NEEEDED!
        // Bitmap must fit in a single frame!
    //assert(_nframes <= FRAME_SIZE * 8);
    
    base_frame_no = _base_frame_no;
    nframes = _n_frames;
    nFreeFrames = _n_frames;
    info_frame_no = _info_frame_no;
    
    // If _info_frame_no is zero then we keep management info in the first
    //frame, else we use the provided frame to keep management info
    if(info_frame_no == 0) {
        bitmap = (unsigned char *) (base_frame_no * FRAME_SIZE);
    } else {
        bitmap = (unsigned char *) (info_frame_no * FRAME_SIZE);
    }
    
    // Number of frames must be "fill" the bitmap!
   // assert ((nframes % 8 ) == 0);
    // Everything ok. Proceed to mark all bits in the bitmap
    for(int i=0; i*4 < _n_frames; i++) {
        bitmap[i] = 0xFF;
    }
    
    // Mark the first frame as being used if it is being used
    if(_info_frame_no == 0) {
        bitmap[0] = 0x3F;
        nFreeFrames--;
    }
    
    if (ContFramePool::pool_list ==  NULL) 
         next = NULL;
     else  
        next = ContFramePool::pool_list;
    ContFramePool::pool_list = this;

    Console::puts("Frame Pool initialized\n");    
}
unsigned long ContFramePool::help_get_frames(unsigned int _n_frames, unsigned int start_index)
{
	
    //if (_n_frames > nFreeFrames) return 0;
    //if (start_index > nframes/4) return 0;
    // Find a frame that is not being used and return its frame index.
    // Mark that frame as being used in the bitmap.

    unsigned int i = start_index; 
    unsigned int frame_no = base_frame_no;
    unsigned int max_map_index = nframes/4;
    while (i < max_map_index && (bitmap[i] & 0xAA) == 0x0) {
        i++;
    }
    //handle the case when pool is exhausted
    if (i == max_map_index)
	return 0;
    frame_no += i * 4;
    unsigned char mask = 0x80;
    unsigned char mask2 = 0xC0;
    while ((mask & bitmap[i]) == 0x0) {
        mask = mask >> 2;
	mask2 = mask2 >> 2;
        frame_no++;
    }
    unsigned int index = checkContinuous(bitmap, i, mask, _n_frames);
    if (index !=-1) {
	//Console::puts("The new index is: "); Console::puti(index);Console::puts("\n");
	return help_get_frames(_n_frames, index);
    }
    nFreeFrames--;
    
    // Update bitmap
    bitmap[i] = bitmap[i] ^ mask2;

    //to process the rest bits
    for (int j = 1; j < _n_frames; j++) {
        mask = mask >> 2;
        if ((bitmap[i] & 0xAA) == 0x0) {
            i=i+1;
            mask = 0x80;
        }
        bitmap[i] = bitmap[i] ^ mask;
        nFreeFrames--;
    }
    return frame_no;
}
unsigned long ContFramePool::get_frames(unsigned int _n_frames)
{
    // TODO: IMPLEMENTATION NEEEDED!
    //assert(false);
    //(nFreeFrames > 0);
    //assert(_n_frames <= nFreeFrames);
    return help_get_frames(_n_frames, 0);
    // Find a frame that is not being used and return its frame index.
    // Mark that frame as being used in the bitmap.
    /*
    unsigned int frame_no = base_frame_no;
    
    unsigned int i = 0; 
    while ((bitmap[i] & 0xAA) == 0x0) {
        i++;
    }
  
    frame_no += i * 4;
    
    unsigned char mask = 0x80;
    unsigned char mask2 = 0xC0;
    while ((mask & bitmap[i]) == 0x0) {
        mask = mask >> 2;
	mask2 = mask2 >> 2;
        frame_no++;
    }
    if (checkContinuous(bitmap, i, mask, _n_frames)!=-1) {
       //Console::puts("Fails in checking\n");
	//Console::puti(bitmap[i]);	
        for(;;);
    }
    nFreeFrames--;
    
    // Update bitmap
    bitmap[i] = bitmap[i] ^ mask2;

    //to process the rest bits
    for (int j = 1; j < _n_frames; j++) {
        mask = mask >> 2;
        if ((bitmap[i] & 0xAA) == 0x0) {
            i=i+1;
            mask = 0x80;
        }
        bitmap[i] = bitmap[i] ^ mask;
        nFreeFrames--;
    }
    //Console::puts("bitmap[i] index is: ");Console::puti(i);Console::puts("\n");
    //Console::puts("bitmap[i] is: ");Console::puti(bitmap[i]);Console::puts("\n");
    return (frame_no);
    */
}

void ContFramePool::mark_inaccessible(unsigned long _base_frame_no,
                                      unsigned long _n_frames)
{
    // TODO: IMPLEMENTATION NEEEDED!
    //assert(false);
    int i ;
    for(i = _base_frame_no; i < _base_frame_no + _n_frames; i++){
        mark_inaccessible(i);
    }
    nFreeFrames -= _n_frames;
}

void ContFramePool::mark_inaccessible(unsigned long _frame_no)
{
    // Let's first do a range check.
    assert ((_frame_no >= base_frame_no) && (_frame_no < base_frame_no + nframes));
    
    unsigned int bitmap_index = (_frame_no - base_frame_no) / 4;
    unsigned char mask = 0x80 >> (2*((_frame_no - base_frame_no) % 4));
    unsigned char mask2 = 0xC0 >> (2*((_frame_no - base_frame_no) % 4));
    
    // Is the frame being used already?
    assert((bitmap[bitmap_index] & mask) != 0);
    
    // Update bitmap
    bitmap[bitmap_index] ^= mask2;
    nFreeFrames--;
}

void ContFramePool::release_frames(unsigned long _first_frame_no)
{
    // TODO: IMPLEMENTATION NEEEDED!
    //assert(false);
    ContFramePool* curr = ContFramePool::pool_list;
    while ((curr->base_frame_no>_first_frame_no) ||
		(curr->base_frame_no + curr->nframes < _first_frame_no))
    {
            if(curr->next!=NULL)
                curr=curr->next;//
            else{
                Console::puts("Err cannot release frame, was not contained in any frame pools\n");             
		return;
                }
    }
    //bool show = false;
    //if (__first_frame_no == 1395) {
//	show = true;
  //  }
    unsigned int bitmap_index = (_first_frame_no - curr->base_frame_no) / 4;
    unsigned char mask = 0x80 >> (2*((_first_frame_no - curr->base_frame_no) % 4));
    unsigned char mask2 = 0xC0 >> (2*((_first_frame_no - curr->base_frame_no) % 4));     

    //head of sequence is 00
    if (!((curr->bitmap[bitmap_index] & mask2) == 0x00)) {
        Console::puts("Err this is not the head of sequence!\n");
        Console::puts("mask is : "); Console::puti(mask2);Console::puts(" ");
	Console::puti(curr->bitmap[bitmap_index]); Console::puts("\n");
        return;
    } 

    //make the head of sequence to be freed
    curr->bitmap[bitmap_index] ^= mask2;
    mask2 = mask2 >> 2;
    mask = mask >> 2;
    //if ((curr->bitmap[bitmap_index] & 0xAA) == 0x00) {
    if (mask == 0x0) {
            bitmap_index += 1;
            mask = 0x80;
            mask2 = 0xC0;
    }
  // Console::puts("curr-index: "); Console::puti(bitmap_index);Console::puts("\n");
  // Console::puts("curr->bitmap[index]: "); Console::puti(curr->bitmap[bitmap_index]);Console::puts("\n");
    //here I used two bit for one frame, for head-sequence the two bit is 00
    // if frame used so the first bit is 0, for non-head-sequece, the second bit is 1
   while (((curr->bitmap[bitmap_index] & mask) == 0x00)  && ((curr->bitmap[bitmap_index] & mask2) != 0x00)) { 
        curr->bitmap[bitmap_index] ^= mask;
	//Console::puts("bitmap_index: "); Console::puti(bitmap_index);Console::puts("\n");
	//Console::puts("bitmap: "); Console::puti(curr->bitmap[bitmap_index]);Console::puts("\n");
        curr->nFreeFrames++;
        mask = mask >> 2;
        mask2 = mask2 >> 2;
        //if ((curr->bitmap[bitmap_index] & 0xAA) == 0x00) {
	if (mask == 0x0) {
            bitmap_index += 1;
            mask = 0x80;
            mask2 = 0xC0;
          }
    }
}

unsigned long ContFramePool::needed_info_frames(unsigned long _n_frames)
{
    // TODO: IMPLEMENTATION NEEEDED!
    //assert(false);
    return  _n_frames / (16*1024) + (_n_frames % (16*1024) > 0 ? 1 : 0);
}
