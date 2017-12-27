#include "sfmm.h"
#include "sfmm_helpers.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

static size_t num_allocated_blocks = 0;
static size_t num_blocks_with_splinters = 0;
static size_t total_amt_of_padding = 0;
static size_t total_amt_of_splintering = 0;
static size_t num_coalesces = 0;
static size_t max_aggregate_payload = 0;

/* head of free list; points to block's header, not its payload */
sf_free_header *freelist_head = NULL;

/* initial brk; points to the bottom of the heap */
void *heap_start = NULL;


void * sf_malloc(size_t req_size) {

  /* minimal constraints on size: (0, 4*4096) <-- not inclusive */
  if(!req_size){
    errno = EINVAL;
    return NULL;
  }
  if(req_size > 4*4096){
    errno = ENOMEM;
    return NULL;
  }

  /* declare the pointer to be returned */
  void *ptr;

  /* figure out how much padding will be needed to align block */
  size_t padding = padding_amt(req_size);

  /* thus the total size of the block, not accounting for possible splinters */
  size_t total_size = req_size + padding + SF_HEADER_SIZE + SF_FOOTER_SIZE;

  /* try finding a free block in the free list */
  ptr = best_fit(total_size);

  /* if no adequate free block was found, grab memory from the heap */
  if(!ptr){
    ptr = get_mem_from_heap(total_size);
  }

  /* if the heap is already maxed out, return */
  if(!ptr){
    /* errno will be set to ENOMEM inside get_mem_from_heap() */
    return NULL;
  }
  
  /* at this point, ptr should be pointing to a valid free block */
  /* whose size is greater than or equal to the necessary size */

  num_allocated_blocks += 1;//stats
  
  /* keep a couple variables to hold splinter info */
  size_t splinter = 0;
  size_t splinter_size = 0;

  /* if the free block is exactly the right size, all info is ready */
  /* to be entered into the block's header and footer; do nothing */
  
  /* else, if the free block is bigger than necessary... */
  if( (((sf_header *)ptr)->block_size)<<4 > total_size ){
    
    /* if splitting would cause a splinter, swallow the splinter */
    if( ( ((((sf_header *)ptr)->block_size)<<4) - total_size) < MINIMUM_BLOCK_SIZE){
      splinter = 1;
      splinter_size = ((((sf_header *)ptr)->block_size)<<4) - total_size;
      total_size = (((sf_header *)ptr)->block_size)<<4;
    }
    
    /* otherwise, split the block into two pieces, */
    /* the first of which is perfectly sized and will be returned */
    else{
      ptr = split_block(ptr, total_size, 1);
    }

  }

  /* remove the block from the free list, since it'll no longer be free */
  remove_from_free_list(ptr);

  /* and set the block's boundary tags */
  set_boundary_tags(ptr,            /* pointer to the start of the block */
		    1,              /* is allocated */
		    splinter,       /* has splinter? */
		    total_size>>4,  /* total block size */
		    req_size,       /* size requested by user */
		    splinter_size,  /* splinter size (0 if no splinter) */
		    padding);       /* padding amount */
		      
  /* return a pointer to the PAYLOAD, not the beginning of the block */
  return (void *)((char *)(ptr + SF_HEADER_SIZE));

}


void * sf_realloc(void *ptr, size_t req_size) {

  /* invalid ptr */
  if(!is_an_allocated_block(ptr)){
    errno = EINVAL;
    return NULL;
  }

  /* call with req_size of 0 simply frees the block @ ptr */
  if(!req_size){
    sf_free(ptr);
    return NULL;
  }

  /* eyyy time to actually edit/resize memory blocks */

  /* note: I got tired and my code started to fall apart here */
  /*       pretty sure the function works, but it looks a mess */

  /* info about the block provided by the user */
  sf_header *given_block = (sf_header *)((char *)ptr - SF_HEADER_SIZE);
  size_t given_block_size = (given_block->block_size)<<4;

  /* info about updated block based on req_size */
  size_t padding = padding_amt(req_size);
  size_t new_block_total_size = req_size + padding + SF_HEADER_SIZE + SF_FOOTER_SIZE;
  
  /* new total block size same as current block size */
  /* (this case may occur when the requested size is only
      a couple bytes different from the block's original
      requested size) */
  if(new_block_total_size == given_block_size){
    set_boundary_tags(given_block,
		      1,                     /* is allocated */
		      0,                     /* no splinter (perfect fit) */
		      given_block_size>>4,   /* total block size */
		      req_size,              /* size requested */
		      0,                     /* splinter size */
		      padding);              /* padding amount */
    return ptr;
  }
  
  /* shrinking: new block smaller than old */
  if(new_block_total_size < given_block_size){

    /* if splitting would cause a splinter ... */
    if( (given_block_size - new_block_total_size) < MINIMUM_BLOCK_SIZE){

      /* if the adjacent block is a free block, have it take the splinter */
      if(next_block_is_free(given_block)){

	/* give splinter to adjacent free block */
	coalesce_splinter((char *)given_block + new_block_total_size,
			    (char *)given_block + given_block_size);
	/* and update header and footer of self */
	set_boundary_tags(given_block,
			  1,
			  0,
			  new_block_total_size>>4,
			  req_size,
			  0,
			  padding);
      }
      
      /* else, swallow the splinter into this block */
      else{
	set_boundary_tags(given_block,
			  1,                                       /* is allocated */
			  1,                                       /* has splinter */
			  given_block_size>>4,                     /* total block size */
			  req_size,                                /* size requested */
			  given_block_size - new_block_total_size, /* splinter size */
			  padding);                                /* padding amount */
      }
    }
    
    /* otherwise, split the block */
    else{

      split_block(given_block, new_block_total_size, 0);
      /* since split_block adds both resultant blocks to the free list,
	 remove this from the free list, since we've split it only to realloc */
      remove_from_free_list(given_block);
      /* and attempt to coalesce that second block with the other block
	 adjacent to it, if there is one */
      coalesce((char *)given_block + new_block_total_size);
      
      set_boundary_tags(given_block,
			1,                        /* is allocated */
			0,                        /* no splinter */
			new_block_total_size>>4,  /* total block size */
			req_size,                 /* size requested by user */
			0,                        /* splinter size */
			padding);                 /* padding amount */

    }

    /* even if the block was downsized, its starting location didn't change */
    return ptr;
  }
  
  /* growing */
  else{

    /* extend current block into adjacent block if it's free and big enough */
    if(next_block_can_accommodate(given_block, new_block_total_size - given_block_size)){

      /* coalesce */
      coalesce_two_blocks(given_block, (char *)given_block + given_block_size, 0);
      /* remove from free list b/c it's not actually free; is just a feature
         of coalesce_two_blocks to add the resulting block to the free list*/
      remove_from_free_list(given_block);
      /* and update local variable to hold the updated block size */
      given_block_size = (given_block->block_size)<<4;


      if(given_block_size == new_block_total_size){
	set_boundary_tags(given_block,
			  1,                                        /* is allocated */
			  0,                                        /* no splinter */
			  given_block_size>>4,                      /* total block size */
			  req_size,                                 /* size requested */
			  0,  /* splinter size */
			  padding);                                 /* padding amount */
      }

      /* if splitting the composite block would cause a splinter, swallow it */
      /* note: don't have to worry about asking an adjacent free block to take
	 the splinter instead, b/c we've just coalesced with that adj. free block */
      else if( (given_block_size - new_block_total_size) < MINIMUM_BLOCK_SIZE){
	set_boundary_tags(given_block,
			  1,                                        /* is allocated */
			  1,                                        /* has splinter */
			  given_block_size>>4,                      /* total block size */
			  req_size,                                 /* size requested */
			  given_block_size - new_block_total_size,  /* splinter size */
			  padding);                                 /* padding amount */
      }

      /* otherwise, split the block */
      else{
	split_block(given_block, new_block_total_size, 0);
	remove_from_free_list(given_block);
	set_boundary_tags(given_block,
			  1,                        /* is allocated */
			  0,                        /* no splinter */
			  new_block_total_size>>4,  /* total block size */
			  req_size,                 /* size requested by user */
			  0,                        /* splinter size */
			  padding);                 /* padding amount */
      }

      /* once again, haven't changed the starting place of the block, only resized */
      return ptr;
    }

    /* going to have to find a new home for the memory T_T */
    /* check the free list first;
       if that fails, get mem from heap;
       if _that_ fails, out of memory, so return NULL */
    
    /* save pointer to and size of original payload such that
       the original payload can be copied to the new block later */
    void *old_payload = ptr;
    size_t num_bytes_to_be_copied = given_block->requested_size;

    /* stick the given block into the free list so that there's a chance
       to coalesce it with the previous block */
    /* in another implementation, this approach could be dangerous; however,
       I know that these helper functions will not (... should not, at 
       least...) corrupt the data in the original payload, as I wrote them */
    /* initialize_free_block(given_block, given_block_size); */
    /* add_to_free_list(given_block); */
    /* coalesce(given_block); */

    /* NOTE: the following code is nearly equivalent to that in sf_malloc */
    
    /* try finding a free block in the free list */
    ptr = best_fit(new_block_total_size);

    if(ptr){
      initialize_free_block(given_block, given_block_size);
      add_to_free_list(given_block);
      coalesce(given_block);
      size_t splinter = 0;
      size_t splinter_size = 0;
    
      /* if the free block is exactly the right size, all info is ready */
      /* to be entered into the block's header and footer; do nothing */
    
      /* else, if the free block is bigger than necessary... */
      if( (((sf_header *)ptr)->block_size)<<4 > new_block_total_size ){
      
	/* if splitting would cause a splinter, swallow the splinter */
	if( ( ((((sf_header *)ptr)->block_size)<<4) - new_block_total_size) < MINIMUM_BLOCK_SIZE){
	  splinter = 1;
	  splinter_size = ((((sf_header *)ptr)->block_size)<<4) - new_block_total_size;
	  new_block_total_size = (((sf_header *)ptr)->block_size)<<4;
	}
    
	/* otherwise, split the block into two pieces */
	else{
	  ptr = split_block(ptr, new_block_total_size, 1);
	}

      }

      /* remove the block from the free list, since it'll no longer be free */
      remove_from_free_list(ptr);

      /* and set the block's boundary tags */
      set_boundary_tags(ptr,            /* pointer to the start of the block */
			1,              /* is allocated */
			splinter,       /* has splinter? */
			new_block_total_size>>4,  /* total block size */
			req_size,       /* size requested by user */
			splinter_size,  /* splinter size (0 if no splinter) */
			padding);       /* padding amount */
		      
      void *new_payload = (void *)((char *)(ptr + SF_HEADER_SIZE));

      memmove(new_payload, old_payload, num_bytes_to_be_copied);
    
      /* return a pointer to the PAYLOAD, not the beginning of the block */
      return new_payload;

    }

    /* if no adequate free block was found, grab memory from the heap */
    else{
      /* if the heap is already maxed out, return */
      ptr = get_mem_from_heap(new_block_total_size);
      if(!ptr){
	/* errno will be set to ENOMEM inside get_mem_from_heap() */
	return NULL;
      }

      //given_block = ptr;
      
      //TRY AGAIN TO EXTEND
      /* extend current block into adjacent block if it's free and big enough */
      if(next_block_can_accommodate(given_block, new_block_total_size - given_block_size)){
	/* coalesce */
	coalesce_two_blocks(given_block, (char *)given_block + given_block_size, 0);
	/* remove from free list b/c it's not actually free; is just a feature
	   of coalesce_two_blocks to add the resulting block to the free list*/
	remove_from_free_list(given_block);
	/* and update local variable to hold the updated block size */
	given_block_size = (given_block->block_size)<<4;



	if(given_block_size == new_block_total_size){
	  set_boundary_tags(given_block,
			    1,                                        /* is allocated */
			    0,                                        /* no splinter */
			    given_block_size>>4,                      /* total block size */
			    req_size,                                 /* size requested */
			    0,  /* splinter size */
			    padding);                                 /* padding amount */
	}


	/* if splitting the composite block would cause a splinter, swallow it */
	/* note: don't have to worry about asking an adjacent free block to take
	   the splinter instead, b/c we've just coalesced with that adj. free block */

	if( (given_block_size - new_block_total_size) < MINIMUM_BLOCK_SIZE){
	  set_boundary_tags(given_block,
			    1,                                        /* is allocated */
			    1,                                        /* has splinter */
			    given_block_size>>4,                      /* total block size */
			    req_size,                                 /* size requested */
			    given_block_size - new_block_total_size,  /* splinter size */
			    padding);                                 /* padding amount */
	}

	/* otherwise, split the block */
	else{
	  split_block(given_block, new_block_total_size, 0);
	  remove_from_free_list(given_block);
	  set_boundary_tags(given_block,
			    1,                        /* is allocated */
			    0,                        /* no splinter */
			    new_block_total_size>>4,  /* total block size */
			    req_size,                 /* size requested by user */
			    0,                        /* splinter size */
			    padding);                 /* padding amount */
	}

	void *new_payload = (void *)( ((char *)(given_block) + SF_HEADER_SIZE));

	memmove(new_payload, old_payload, num_bytes_to_be_copied);
    
	/* return a pointer to the PAYLOAD, not the beginning of the block */
	return new_payload;

      }



      //IF FAIL, GO STRAIGHT TO PTR

      size_t splinter = 0;
      size_t splinter_size = 0;
    
      /* if the free block is exactly the right size, all info is ready */
      /* to be entered into the block's header and footer; do nothing */
    
      /* else, if the free block is bigger than necessary... */
      if( (((sf_header *)ptr)->block_size)<<4 > new_block_total_size ){
      
	/* if splitting would cause a splinter, swallow the splinter */
	if( ( ((((sf_header *)ptr)->block_size)<<4) - new_block_total_size) < MINIMUM_BLOCK_SIZE){
	  splinter = 1;
	  splinter_size = ((((sf_header *)ptr)->block_size)<<4) - new_block_total_size;
	  new_block_total_size = (((sf_header *)ptr)->block_size)<<4;
	}
    
	/* otherwise, split the block into two pieces */
	else{
	  ptr = split_block(ptr, new_block_total_size, 1);
	}

      }

      /* remove the block from the free list, since it'll no longer be free */
      remove_from_free_list(ptr);

      /* and set the block's boundary tags */
      set_boundary_tags(ptr,            /* pointer to the start of the block */
			1,              /* is allocated */
			splinter,       /* has splinter? */
			new_block_total_size>>4,  /* total block size */
			req_size,       /* size requested by user */
			splinter_size,  /* splinter size (0 if no splinter) */
			padding);       /* padding amount */
		      
      void *new_payload = (void *)((char *)(ptr + SF_HEADER_SIZE));

      memmove(new_payload, old_payload, num_bytes_to_be_copied);
    
      /* return a pointer to the PAYLOAD, not the beginning of the block */
      return new_payload;
      

      
      
    }


    
    
  }

}

//stats
void update_some_of_the_globals(){
  num_blocks_with_splinters = 0;
  total_amt_of_padding = 0;
  total_amt_of_splintering = 0;

  sf_free_header *current = freelist_head;
  while(current){
    sf_header *hd = (sf_header *)current;
    num_blocks_with_splinters += hd->splinter;
    total_amt_of_padding += hd->padding_size;
    total_amt_of_splintering += hd->splinter_size;
    current = current->next;
  }
}

void sf_free(void *ptr) {

  if(!is_an_allocated_block(ptr)){
    errno = EINVAL;
    return;
  }

  
  ptr = (char *)ptr - SF_HEADER_SIZE;
  size_t tmp = (((sf_header *)ptr)->block_size)<<4;
  
  initialize_free_block(ptr, tmp);
  add_to_free_list(ptr);

  coalesce(ptr);

  num_allocated_blocks -= 1;//stats
  
  return;
}


int sf_info(info *ptr) {
  if(!heap_start || !ptr){
    return -1;
  }

  ptr->allocatedBlocks = num_allocated_blocks;
  update_some_of_the_globals();
  ptr->splinterBlocks = num_blocks_with_splinters;
  ptr->padding = total_amt_of_padding;
  ptr->splintering = total_amt_of_splintering;
  ptr->coalesces = num_coalesces;
  //ptr->peakMemoryUtilization = max_aggregate_payload / (sf_sbrk(0) - heap_start);
  ptr->peakMemoryUtilization = max_aggregate_payload;
  
  return 0; //KEK
}




size_t padding_amt(size_t req_size){
  return (req_size % sizeof(long double)) ? sizeof(long double) - (req_size % sizeof(long double)) : 0;
}


void * best_fit(size_t size){

  if(!freelist_head){
    return NULL;
  }

  sf_free_header *best = NULL;
  sf_free_header *current = freelist_head;

  while(current){
    /* return immediately if perfect fit */
    if( ((current->header).block_size)<<4 == size ){
      return current;
    }
    /* if current's block size > needed size... */
    else if( ((current->header).block_size)<<4 > size ){
      /* if there's already a tentative 'best', check against current */
      if(best){
	if( ((current->header).block_size)<<4 < ((best->header).block_size)<<4){
	  best = current;
	}
      }
      /* else, immediately assign best to current */
      else{
	best = current;
      }
    }
    /* and iterate */
    current = current->next;
  }

  /* note that default return val is NULL per the initialization */
  return best;
  
}


void * get_mem_from_heap(size_t size){

  /* if first call to get_mem_from_heap, save pointer to heap bottom */
  if(!heap_start){
    heap_start = sf_sbrk(0);
  }

  /* if heap would be maxed out, set errno and return  */
  if((char *)sf_sbrk(0) + 4096*num_pages_needed(size) - (char *)heap_start > 4*4096){
    errno = ENOMEM;
    return NULL;
  }

  void *new_block = sf_sbrk(4096*num_pages_needed(size));

  initialize_free_block(new_block, (char *)sf_sbrk(0) - (char *)new_block);
  add_to_free_list(new_block);

  /* coalesce new_block with end of free_list if possible */
  new_block = coalesce(new_block);

  /* if(((sf_free_header *)new_block)->prev){ */
  /*   sf_footer *prev_adjacent_footer = ((sf_footer *)((char *)new_block - SF_FOOTER_SIZE)); */
  /*   void *prev_adjacent_block = (char *)new_block - ((prev_adjacent_footer->block_size)<<4); */
  /*   if( ((sf_free_header *)new_block)->prev == prev_adjacent_block){ */
  /*     new_block = coalesce_two_blocks(prev_adjacent_block, new_block, 1); */
  /*   } */
  /* } */
  
  return new_block;
  
}


int num_pages_needed(size_t size){
  /* if this isn't the first malloc call... */
  if(sf_sbrk(0) != heap_start){
    /* check if last block before current brk is free, because if so,
       any new memory pulled from the heap will be coalesced with it,
       and this should decrease the size passed to sf_sbrk()  */
    sf_footer *footer_of_last_block_in_mem = (sf_footer *)(sf_sbrk(0)-SF_FOOTER_SIZE);
    if(!footer_of_last_block_in_mem->alloc){
      size = size - ((footer_of_last_block_in_mem->block_size)<<4);
    }
  }
  int num_pages = ((int)size)/4096;
  int ceil = size%4096 ? 1 : 0;
  return num_pages + ceil;
}


//precon: block_size has been right-shifted by 4
void set_boundary_tags(void *            block,
		       uint64_t          alloc,
		       uint64_t       splinter,
		       uint64_t     block_size,
		       uint64_t requested_size,
		       uint64_t  splinter_size,
		       uint64_t   padding_size){

  sf_header *header = (sf_header *)block;
  header->alloc = alloc;
  header->splinter = splinter;
  header->block_size = block_size;
  header->requested_size = requested_size;
  header->splinter_size = splinter_size;
  header->padding_size = padding_size;


  sf_footer *footer = (sf_footer *)((char *)(block) + (block_size<<4) - SF_FOOTER_SIZE);
  footer->alloc = alloc;
  footer->splinter = splinter;
  footer->block_size = block_size;
  
}

void initialize_free_block(void *ptr, size_t size){
  set_boundary_tags(ptr,      /* pointer to the start of the block */
		    0,        /* not allocated */
		    0,        /* no splinter */
		    size>>4,  /* block size */
		    0,        /* size requested is irrelevant */
		    0,        /* splinter size is 0, no splinter */
		    0);       /* padding is 0, no padding */  
}


void add_to_free_list(void *ptr){

  sf_free_header *block_to_add = (sf_free_header *)ptr;

  /* free list is currently empty */
  if(!freelist_head){
    freelist_head = block_to_add;
    block_to_add->prev = NULL;
    block_to_add->next = NULL;
    return;
  }

  /* inserting at head */
  if(block_to_add < freelist_head){
    freelist_head->prev = block_to_add;
    block_to_add->prev = NULL;
    block_to_add->next = freelist_head;
    freelist_head = block_to_add;
    return;
  }
  
  sf_free_header *current = freelist_head;

  /* iterate through list */
  while(current < block_to_add){

    /* inserting at tail */
    if(!current->next){
      current->next = block_to_add;
      block_to_add->prev = current;
      block_to_add->next = NULL;
      return;
    }
    
    current = current->next;

  }

  /* inserting between two nodes */
  /* current is now the node _before which_ to insert */
  block_to_add->prev = current->prev;
  block_to_add->next = current;
  (block_to_add->prev)->next = block_to_add;
  current->prev = block_to_add;

  return;
  
}


/* precondition: ptr points to a valid memory block in the free list */
void remove_from_free_list(void *ptr){

  sf_free_header *block_to_remove = (sf_free_header *)ptr;

  /* removing head */
  if(block_to_remove == freelist_head){
    freelist_head = freelist_head->next;
    /* do conditionally b/c we may have removed the only element in the list */
    if(freelist_head){
      freelist_head->prev = NULL;
    }
  }
  /* not removing head, i.e., has a 'prev' block */
  else{
    (block_to_remove->prev)->next = block_to_remove->next;
    /* not removing tail, i.e., has a 'next' block */
    if(block_to_remove->next){
      (block_to_remove->next)->prev = block_to_remove->prev;
    }
  }
  
  return;
  
  
}

//precon: size_of_first_block is a multiple of the largest data type
void * split_block(void *ptr, size_t size_of_first_block, int already_free){

  if(already_free){
    remove_from_free_list(ptr);
  }
  
  sf_free_header *ptr1 = (sf_free_header *)ptr;
  sf_free_header *ptr2 = (sf_free_header *)((char *)ptr + size_of_first_block);

  size_t tmp = ((ptr1->header).block_size)<<4;

  initialize_free_block(ptr1, size_of_first_block);
  initialize_free_block(ptr2, tmp - size_of_first_block);
  
  add_to_free_list(ptr1);
  add_to_free_list(ptr2);

  return ptr1;
  
}

//precon: ptr points to a free block
void * coalesce(void *ptr){

  size_t c = 0;//stats
  
  /* try coalescing with prev */
  if(((sf_free_header *)ptr)->prev){
    sf_footer *prev_adjacent_footer = ((sf_footer *)((char *)ptr - SF_FOOTER_SIZE));
    void *prev_adjacent_block = (char *)ptr - ((prev_adjacent_footer->block_size)<<4);
    if( ((sf_free_header *)ptr)->prev == prev_adjacent_block){
      ptr = coalesce_two_blocks(prev_adjacent_block, ptr, 1);
      c = 1;//stats
    }
  }

  /* try coalescing the new composite block with next */
  if(((sf_free_header *)ptr)->next){
    void *next_adjacent_block = (char *)ptr + ((((sf_header *)ptr)->block_size)<<4);
    if( ((sf_free_header *)ptr)->next == next_adjacent_block){
      ptr = coalesce_two_blocks(ptr, next_adjacent_block, 1);
      c = 1;//stats
    }
  }

  num_coalesces += c;//stats
  
  return ptr;
  
}

//precon: first and second point to the BEGINNING of two blocks, ptr2 is FREE
//        which are ADJACENT in memory && address(first)<address(2nd)
void * coalesce_two_blocks(void *ptr1, void *ptr2, int already_free){

  if(already_free){
    remove_from_free_list(ptr1);
  }
  remove_from_free_list(ptr2);

  sf_free_header *ptr = ptr1;

  size_t tmp = (((((sf_free_header *)ptr1)->header).block_size)<<4)
             + (((((sf_free_header *)ptr2)->header).block_size)<<4);

  initialize_free_block(ptr, tmp);
  
  add_to_free_list(ptr);

  return ptr;
}

void * coalesce_splinter(void *splint, void *free_block){
  remove_from_free_list(free_block);
  initialize_free_block(splint, (((((sf_free_header *)free_block)->header).block_size)<<4)+16);
  add_to_free_list(splint);
  return splint;
}


//takes a pointer to the PAYLOAD as a param.
//returns 1 if block is a valid allocated block,
//        0 otherwise.
int is_an_allocated_block(void *ptr){

  /* fail if NULL */
  if(!ptr){
    //printf("ptr is NULL\n");
    return 0;
  }

  /* set ptr to beginning of BLOCK rather than PAYLOAD */
  ptr = (char *)ptr - SF_HEADER_SIZE;

  /* no memory has been allocated yet */
  if(!heap_start || !ptr){
    //printf("!heap_start\n");
    return 0;
  }
  /* ptr is out of bounds */
  if(ptr < heap_start || ((void *)((char *)ptr + MINIMUM_BLOCK_SIZE)) > sf_sbrk(0)){
    //printf("ptr out of bounds\n");
    return 0;
  }
  /*ptr-heap_start is not a multiple of 16 */
  if((((unsigned long)((char *)ptr - (char *)heap_start))%sizeof(long double))){
    //printf("ptr misaligned\n");
    return 0;
  }

  sf_header *header = (sf_header *)ptr;
  size_t block_size = (header->block_size)<<4;

  /* supposed block_size is 0 */
  if(!block_size){
    //printf("supposed block_size is 0\n");
    return 0;
  }

  /* header data is internally inconsistent */
  if(block_size != header->requested_size + header->splinter_size + header->padding_size
                 + SF_HEADER_SIZE + SF_FOOTER_SIZE){
    //printf("header data internally inconsistent\n");
    //sf_blockprint(header);
    return 0;
  }
  
  /*  end of block is out of bounds*/
  if((((void *)((char *)header + block_size)) > sf_sbrk(0)) ){
    //printf("end of supposed block out of bounds\n");
    return 0;
  }

  sf_footer *footer = (sf_footer *)((char *)header + block_size - SF_FOOTER_SIZE);

  /* header and footer are inconsistent */
  if(footer->block_size != header->block_size ||
     footer->splinter   != header->splinter   ||
     footer->alloc      != header->alloc){
    //printf("header and footer data don't match\n");
    return 0;
  }

  /* block is valid, but it's already free */
  if(!header->alloc){
    //printf("is a block, but it's already free\n");
    return 0;
  }

  return 1;
}


int next_block_is_free(sf_header *current_block){
  if((void *)((char *)current_block + (current_block->block_size<<4))
     == sf_sbrk(0)){
    return 0;
  }
  return !(((sf_header *)((char *)current_block + (current_block->block_size<<4)))->alloc);
}

int next_block_can_accommodate(sf_header *current_block, size_t total_size){
  //make sure not to exceed sf_sbrk(0)
  //size_t size_to_handle = total_size - ((current_block->block_size)<<4)
  //sf_header *next_block = current_block + current_block->block_size
  //if(next_block
  if(next_block_is_free(current_block)){
    return (((sf_header *)((char *)current_block + (current_block->block_size<<4)))->block_size)<<4 >= total_size;
  }
  return 0;
}


/**
 * All functions you make for the assignment must be implemented in this file.
 * Do not submit your assignment with a main function in this file.
 * If you submit with a main function in this file, you will get a zero.
 */

/**
 * You should store the head of your free list in this variable.
 * Doing so will make it accessible via the extern statement in sfmm.h
 * which will allow you to pass the address to sf_snapshot in a different file.
 */
