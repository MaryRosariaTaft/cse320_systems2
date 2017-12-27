#include "debug.h"
#include "arraylist.h"
#include "foreach.h"
/*
int main(int argc, char *argv[]){

}
*/
//todo:REMOVEALLOFTHEBELOW(multi)
/*
void * insert_routine(void *al){
  char c = 'A' + 5;
  insert_al(al, &c);
  return NULL;
}

void * delete_routine(void *al){
  char *c = remove_index_al(al,5);
  free(c);
  return NULL;
}

void tmp(void *ptr){
  printf("not_actually_freeing:%c\n",*((char *)ptr));
  return;
}
void print_length(arraylist_t *self){
  printf("~LENGTH: %lu\n", self->length);
}
void print_cap(arraylist_t *self){
  printf("~CAPACITY: %lu\n", self->capacity);
}
int main(int argc, char *argv[]){
  int i;
  pthread_t tid[200];
  arraylist_t *al = new_al(sizeof(char));
  for(i=0;i<100;i++){
    pthread_create(&tid[i], NULL, insert_routine, al);
  }
  for(i=100;i<200;i++){
    pthread_create(&tid[i], NULL, delete_routine, al);
  }
  for(i=0;i<200;i++){
    pthread_join(tid[i], NULL);
  }
  print_length(al);
  print_cap(al);
  delete_al(al, tmp);
  free(al);
  
}
*/


//todo:REMOVEALLOFTHEBELOW(single)

void tmp(void *ptr){
  printf("not_actually_freeing:%c\n",*((char *)ptr));
  return;
}
void print_length(arraylist_t *self){
  printf("~LENGTH: %lu\n", self->length);
}
void print_cap(arraylist_t *self){
  printf("~CAPACITY: %lu\n", self->capacity);
}
void print_base(arraylist_t *self){
  printf("~BASE ADDR: %p\n", self->base);
}
void foo(void *ptr){
  *((char*)ptr) = *((char*)ptr) - 1;
}
int main(int argc, char *argv[]){
  arraylist_t *al = new_al(sizeof(char));
  int i;

  //insert A-Z sequentially
  for(i=0;i<26;i++){
    char c = 'A' + i;
    size_t index = insert_al(al, &c);
    printf("inserted %c at index %lu\n", c, index);
  }
  printf("\n");
  
  //print all elements sequentially
  for(i=0;i<26;i++){
    char *c = get_index_al(al,i);
    printf("reading value at index %d: %c\n",i,*c);
    free(c);
  }
  printf("\n");

  //length and capacity after performing insertions
  printf("\nafter inserting...\n");
  print_length(al);
  print_cap(al);
  printf("\n");

  //remove last 8 letters via index
  for(i=25;i>=18;i--){
    char *c = remove_index_al(al,i);
    printf("removed %c\n", *c);
    free(c);
  }
  printf("\n");

  //remove every other letter for 6 iterations via its value
  for(i=0;i<6;i++){
    char c = 'A' + 2*i;
    bool b = remove_data_al(al,&c);
    if(b){
      printf("removed %c\n", c);
    }
  }
  printf("\n");

  //length and capacity after performing removals
  printf("\nafter partial removing...\n");
  print_length(al);
  print_cap(al);
  printf("\n");

  foreach(char, c, al){
    if(*c == 'F'){
      printf("c_brk: %c\n",*c);
      foreach_break;
    }
    printf("c_bef: %c\n",*c);
    foo(c);
    printf("c_aft: %c\n",*c);
  }
  printf("\n");

  //delete and free
  delete_al(al, tmp);
  free(al);
  
  return 0;
}
