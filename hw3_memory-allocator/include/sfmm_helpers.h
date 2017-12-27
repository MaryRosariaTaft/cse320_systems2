#ifndef SFMM_HELPERS_H
#define SFMM_HELPERS_H
#include <errno.h>


#define MINIMUM_BLOCK_SIZE 32 //SF_HEADER_SIZE + sizeof(long double) + SF_FOOTER_SIZE


/*
 *  @param req_size The size requested by the user.
 * 
 *  @return the amount of padding which would be required
 *          to align a block whose payload is req_size to
 *          the size of the largest data type of the system.
 */
size_t padding_amt(size_t req_size);

/*
 *  @param size The minimum number of bytes required to accommodate
 *              payload, padding, header, and footer.
 *
 *  @return A pointer to the smallest free block from the explicit
 *          free list which is greater than or equal to size, if
 *          there exists such a block; otherwise, it returns NULL.
 *
 *  Note that this function returns immediately (rather than proceeding
 *  with its search through the free list) if a perfectly-sized block
 *  (block->block_size == size) is found.
 */
void * best_fit(size_t size);

/*
 *  This function requests memory from the heap using sf_sbrk
 *  and adds it to the free list.
 *
 *  If the block at the old top of the heap is free, that
 *  old block and the newly-acquired block will be coalesced.
 *
 *  If sf_sbrk(0) > 4*4096, set errno to ENOMEM.
 *
 *  @param size The minimum number of bytes required to accommodate
 *              payload, padding, header, and footer.
 *
 *  @return a pointer to the new free block (possibly coalesced),
 *          or NULL if failed
 */
void * get_mem_from_heap(size_t size);


//returns #pages which would be needed if size were passed as a param to get_mem_from_heap
int num_pages_needed(size_t size);


////////////////////////////////////////////


//sets header and footer of block which STARTS at *block
void set_boundary_tags(void *            block,
		       uint64_t          alloc,
		       uint64_t       splinter,
		       uint64_t     block_size,
		       uint64_t requested_size,
		       uint64_t  splinter_size,
		       uint64_t   padding_size);


void initialize_free_block(void *ptr, size_t size);


//DOES NOT EDIT THE BLOCK HEADER OR FOOTER.  simply adds it to the free list.
void add_to_free_list(void *ptr);


//DOES NOT EDIT THE BLOCK HEADER OR FOOTER.  simply removes it from the free list.
void remove_from_free_list(void *ptr);


////////////////////////////////////////////


//splits block, adds both pieces to to free list, returns ptr to START of first block (NOT payload)
void * split_block(void *ptr, size_t size_of_first_block, int already_free);


//todo ick this isn't ready ick ick ick
void * coalesce(void *ptr);

//coalesces two blocks
//returns pointer to beginning of resultant block (it should equal first)
void * coalesce_two_blocks(void *ptr1, void *ptr2, int already_free);

void * coalesce_splinter(void *splint, void *free_block);


////////////////////////////////////////////


int is_an_allocated_block(void *ptr);

int next_block_is_free(sf_header *current_block);

int next_block_can_accommodate(sf_header *current_block, size_t total_size);




#endif
