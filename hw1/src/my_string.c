//returns index of first occurrence of c in s, or -1 if not found
int my_strchr(char *s, char c){
  int i = 0;
  while(*(s+i)){
    if(*(s+i) == c){
      return i;
    }
    i++;
  }
  return -1;
}

int my_strcmp(char *s1, char *s2){
  int i = 0;
  while(*(s1+i)==*(s2+i) && *(s1+i) && *(s2+i)){
    i++;
  }
  return *(s1+i) - *(s2+i);
}


int my_strlen(char *s){
  int i = 0;
  while(*(s+i)){
    i++;
  }
  return i;
}
