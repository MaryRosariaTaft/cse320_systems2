#include "debug.h"
#include "arraylist.h"
#include "foreach.h"
#include <errno.h>

static pthread_key_t key; //will store key value for thread using it
static pthread_once_t key_once = PTHREAD_ONCE_INIT; 
/* static pthread_key_t key1; //will store key value for thread using it */
/* static pthread_once_t key_once1 = PTHREAD_ONCE_INIT;  */

static void make_key(){
  pthread_key_create(&key, NULL);
  //todo: error-check? but errno should be set by _key_create()..
}
/* static void make_key1(){ */
/*   pthread_key_create(&key1, NULL); */
/* } */

void * foreach_init(arraylist_t *items){
  //fail if items is NULL
  if(items == NULL){
    return NULL;
  }

  //...or if it has no elements
  if(items->length == 0){
    return NULL;
  }

  size_t *index_al;
  /* arraylist_t **al_ptr; */

  //initialize key but once
  pthread_once(&key_once, make_key);
  /* pthread_once(&key_once1, make_key1); */

  //if (since?) no value is associated with the new key yet...
  if( (index_al = pthread_getspecific(key)) == NULL){
    index_al = (size_t *)malloc(sizeof(size_t));
    //...associate it with index...
    int tmp;
    if ( (tmp = pthread_setspecific(key, index_al)) != 0){
      errno = tmp;
      return NULL;
    }
  }

  /* if( (al_ptr = pthread_getspecific(key1)) == NULL){ */
  /*   al_ptr = (arraylist_t **)malloc(sizeof(arraylist_t *)); */
  /*   int tmp1; */
  /*   if ( (tmp1 = pthread_setspecific(key1, al_ptr)) != 0){ */
  /*     errno = tmp1; */
  /*     return NULL; */
  /*   } */
  /* }     */
  
  //...and initialize index
  *((size_t *)pthread_getspecific(key)) = 0;

  /* *((arraylist_t **)pthread_getspecific(key1)) = items; */

  /* items->foreach_count += 1; */
  /* if(items->foreach_count == 1){ */
  /*   sem_wait(&((*((arraylist_t **)pthread_getspecific(key1)))->bagel)); */
  /* } */

  //return ptr to copy of data at index
  return get_index_al(items, *((size_t *)pthread_getspecific(key)));
}

void * foreach_next(arraylist_t *items, void *data){
  //fail if items is NULL
  if(items == NULL){
    return NULL;
  }

  //...or if it has no elements
  if(items->length == 0){
    return NULL;
  }

  //if some data is passed...
  if(data != NULL){
    //...update current list element to contain that data
    void *dest = items->base + (*((size_t *)pthread_getspecific(key)))*(items->item_size);
    memcpy(dest, data, items->item_size);
  }
  
  //increment index
  *((size_t *)pthread_getspecific(key)) = *((size_t *)pthread_getspecific(key)) + 1;

  //if end of list, clean up and return NULL
  if(*((size_t *)pthread_getspecific(key)) == items->length){
    /* sem_post(&((*((arraylist_t **)pthread_getspecific(key1)))->bagel)); */
    free(pthread_getspecific(key));
    pthread_key_delete(key);
    return NULL;
  }
  //else, return ptr to copy of data at (updated) index
  return get_index_al(items, *((size_t *)pthread_getspecific(key)));
  
}

size_t foreach_index(){
  if(pthread_getspecific(key) == NULL){
    return UINT_MAX;
  }
  return *((size_t *)pthread_getspecific(key));
}

bool foreach_break_f(){
  /* sem_post(&((*((arraylist_t **)pthread_getspecific(key1)))->bagel)); */
  free(pthread_getspecific(key));
  pthread_key_delete(key);
  return true;
}

//todo
int apply(arraylist_t *items, int (*application)(void *)){
  int ret = 0;

  return ret;
}
