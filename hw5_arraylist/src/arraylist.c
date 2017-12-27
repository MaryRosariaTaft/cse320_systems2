#include "arraylist.h"
#include <errno.h>

/**
 * @visibility HIDDEN FROM USER
 * @return     true on success, false on failure
 */
static bool resize_al(arraylist_t *self){
  //fail if NULL is passed
  if(self == NULL){
    //this block should never be reached since resize_al is called only internally and
    // a NULL arraylist_t argument should have been caught previously, but just in case
    errno = EINVAL;
    return false;
  }

  //grow
  if(self->length == self->capacity){
    //realloc to twice current capacity
    void *new_base = realloc(self->base, (self->capacity)*(self->item_size)*2);
    if(new_base == NULL){
      errno = ENOMEM;
      return false;
    }
    //update struct members
    self->capacity = (self->capacity)*2;
    self->base = new_base;
    //clear the new region
    memset(self->base + (self->length)*(self->item_size), 0, (self->length)*(self->item_size));
    //and return
    return true;
  }

  //shrink
  else if(self->length == (self->capacity)/2 - 1){
    //do not shrink if current capacity is INIT_SZ (<-- lower bound)
    if(self->capacity == INIT_SZ){
      return true;
    }
    //else, realloc to half current capacity
    void *new_base = realloc(self->base, (self->capacity)*(self->item_size)/2);
    if(new_base == NULL){
      //not sure why this block might be reached--shrinking shouuuldn't be a problem;
      // but I'm not sure, some error might happen somewhere, so, again, just in case
      errno = ENOMEM;
      return false;
    }
    //update struct members
    self->capacity = (self->capacity)/2;
    self->base = new_base;
    //and return
    return true;
  }

  //do nothing
  else{
    //yet again, this block should never be reached, but just in case
    return false;
  }
}


arraylist_t * new_al(size_t item_size){
  //fail if 0 is passed (arraylist would be rendered useless)
  if(item_size == 0){
    errno = EINVAL;
    return NULL;
  }

  //allocate space for the arraylist struct (metadata)
  arraylist_t *al = (arraylist_t *)malloc(sizeof(arraylist_t));
  if(al == NULL){
    errno = ENOMEM;
    return NULL;
  }

  //clear-allocate space for the arraylist itself
  void *base = calloc(INIT_SZ, item_size); //num elements, size of each
  if(base == NULL){
    free(al);
    errno = ENOMEM;
    return NULL;
  }

  //initialize semaphores
  sem_t locke;
  int tmp = sem_init(&locke, 0, 1);
  if(tmp == -1){
    //errno should be set by sem_init TODO: might have to set anyway, unsure
    return NULL;
  }
  sem_t ack;
  tmp = sem_init(&ack, 0, 1);
  if(tmp == -1){
    return NULL;
  }
  sem_t lox;
  tmp = sem_init(&lox, 0, 1);
  if(tmp == -1){
    return NULL;
  }
  
  //initialize remaining struct members if preceding allocations were successful
  al->capacity = INIT_SZ;
  al->length = 0;
  al->item_size = item_size;
  al->base = base;
  al->john = locke;
  al->removing = 0;
  al->bagel = lox;
  al->foreach_count = 0;
  
  return al;
}


size_t insert_al(arraylist_t *self, void *data){
  //fail if either param is NULL
  if(self == NULL || data == NULL){
    errno = EINVAL;
    return UINT_MAX;
  }

  //grow if arraylist is already at capacity
  sem_wait(&(self->john));
  if(self->capacity == self->length){
    bool b = resize_al(self);
    if(!b){
      //errno should have already been set by resize_al(), I think, I hope TODO: check
      sem_post(&(self->john));
      return UINT_MAX;
    }
  }
  sem_post(&(self->john));
  //TODO:check that it's okay to v here and then p again right after
  // (thus breaking up the two parts of the function)
  // I think it is b/c other threads should be able to edit the list after
  // resizing AS LONG AS they're aware of the updated value of capacity
  sem_wait(&(self->john));
  //insert at end by copying data
  size_t index = self->length;
  void *dest = self->base + index*(self->item_size);
  memcpy(dest, data, self->item_size);

  //increment length
  self->length = self->length + 1;
  sem_post(&(self->john));

  return index;
}


size_t get_data_al(arraylist_t *self, void *data){
  //fail if arraylist ptr is NULL...
  if(self == NULL){
    errno = EINVAL;
    return UINT_MAX;
  }

  //...or if arraylist is empty
  if(self->length == 0){
    errno = EINVAL;
    return UINT_MAX;
  }

  //return first element (index 0) if data is NULL
  if(data == NULL){
    return 0;
  }

  
  //else, loop through to attempt to find a match for data in the arraylist
  int index = 0;
  if(!(self->removing)){
    sem_wait(&(self->john));
  }
  void *ptr = self->base; //initialize to base address
  while(index < self->length){
    //if data matches value of element at current index, return that index
    if(memcmp(data, ptr, self->item_size) == 0){
      if(!(self->removing)){
	sem_post(&(self->john));
      }
      return index;
    }
    index++;
    ptr = ptr + self->item_size; //increment by 1 element (item_size bytes) on each iteration
  }
  if(!(self->removing)){
    sem_post(&(self->john));
  }

  //if no value was returned within the loop, no match was found
  errno = EINVAL;
  return UINT_MAX;
}


void * get_index_al(arraylist_t *self, size_t index){
  //fail if self is NULL or index is negative...
  if(self == NULL || index < 0){
    errno = EINVAL;
    return NULL;
  }
  
  //...or if arraylist is empty
  if(self->length == 0){
    errno = EINVAL;
    return NULL;
  }

  //allocate space for the data to be returned
  void *dest = malloc(self->item_size);
  if(dest == NULL){
    errno = ENOMEM;
    return NULL;
  }

  //in case another thread called delete_al() after the above malloc...
  if(self->item_size == 0){
    free(dest);
    errno = EINVAL;
    return NULL;
  }

  if(!(self->removing)){
    sem_wait(&(self->john));
  }
  //if the given index exceeds the arraylist length,
  // treat it as though it's the index of the last item
  if(index >= self->length){
    index = self->length - 1;
  }
  
  //determine the location of the data
  void *ptr;
  ptr = self->base + index*(self->item_size);

  //and return a copy thereto, in the space previously allocated
  void *ret = memcpy(dest, ptr, self->item_size);
  if(!(self->removing)){
    sem_post(&(self->john));
  }
  return ret;
}


bool remove_element_at_index(arraylist_t *self, size_t index){

  //(hesitantly...) no fail cases: this function shouldn't be reached with invalid params

  void *ptr;

  //if *not* last element in list, shift all other elements down by 1 (item_size bytes)
  if(index < self->length - 1){
    ptr = self->base + index*(self->item_size);
    memmove(ptr, ptr + self->item_size, (self->length - 1 - index)*self->item_size);
  }

  //clear the bits of the last memory cell
  ptr = self->base + (self->length - 1)*self->item_size;
  memset(ptr, 0, self->item_size);

  //and decrement arraylist length
  self->length = self->length - 1;

  //shrink if length has reached half-capacity
  if(self->length == (self->capacity)/2 - 1){
    bool b = resize_al(self);
    if(!b){
      //hopefully this block is never reached...
      //todo: errno..?
      return false; //or return b, whatever
    }
  }

  return true;
}


bool remove_data_al(arraylist_t *self, void *data){
  //fail if self is NULL
  if(self == NULL){
    errno = EINVAL;
    return false;
  }

  //abort if iteration is happening
  int tmp = sem_trywait(&(self->bagel));
  if(tmp == -1){
    return false;
  }

  //retrieve the index of the item to remove using get_data_al()
  sem_wait(&(self->john));
  self->removing = 1;
  size_t index = get_data_al(self, data);
  if(index == UINT_MAX){
    //errno should have been set by get_data_al() TODO: check
    self->removing = 0;
    sem_post(&(self->john));
    sem_post(&(self->bagel));
    return false;
  }

  //remove the element at that index
  bool b = remove_element_at_index(self, index);
  self->removing = 0;
  sem_post(&(self->john));
  sem_post(&(self->bagel));
  if(!b){
    //todo: set errno..?
    return false; //or return b, whatever
  }
  
  return b;
}


void * remove_index_al(arraylist_t *self, size_t index){
  //fail if self is NULL
  if(self == NULL){
    errno = EINVAL;
    return NULL;
  }

  //abort if iteration is happening
  int tmp = sem_trywait(&(self->bagel));
  if(tmp == -1){
    return NULL;
  }
  
  //retrieve the data at the given index using get_index_al()
  sem_wait(&(self->john));
  self->removing = 1;
  void *data = get_index_al(self, index);
  if(data == NULL){
    //errno should have been set by get_index_al() TODO: check
    self->removing = 0;
    sem_post(&(self->john));
    sem_post(&(self->bagel));
    return NULL; //same as return data todo
  }

  //if the given index exceeds the arraylist length,
  // treat it as though it's the index of the last item
  // (same behavior as in get_index_al())
  if(index >= self->length){
    index = self->length - 1;
  }
  
  //remove the element at index
  bool b = remove_element_at_index(self, index);
  self->removing = 0;
  sem_post(&(self->john));
  sem_post(&(self->bagel));
  if(!b){
    //todo: set errno..?
    return NULL;
  }
  
  return data;
  
}


void delete_al(arraylist_t *self, void (*free_item_func)(void *)){
  //fail if self is NULL
  if(self == NULL){
    errno = EINVAL;
    return;
  }

  sem_wait(&(self->john));
  //if the user passes in a freeing function, call it on every element in the list
  if(free_item_func != NULL){
    int index = 0;
    void *ptr = self->base; //initialize to base address
    while(index < self->length){
      (*free_item_func)(ptr);
      index++;
      ptr = ptr + self->item_size; //increment by 1 element (item_size bytes) on each iteration
    }
  }
  
  //free the arraylist (but not the struct containing its metadata)
  free(self->base);

  //assign dummy values to struct members until user frees the struct itself
  self->capacity = 0;
  self->length = 0;
  self->item_size = 0;
  self->base = NULL;

  sem_post(&(self->john));

  //and destroy the lock
  sem_destroy(&(self->john));

  return;
}

