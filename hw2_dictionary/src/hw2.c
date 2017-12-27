#include "hw2.h"

void make_lowercase(char *s){
  while(*s != '\0'){
    *s = tolower(*s);
    s++;
  }
}

int processDictionary(){

  //debug("in processDictionary\n");

  dict->num_words = 0;

  /* one interation per line in dFile */
  char line[MAX_SIZE];
  while(fgets(line, MAX_SIZE+1, dFile))
    {
      /* add word to dictionary */
      char *cor = strtok(line, " \t\n\v\f\r");
      make_lowercase(cor);
      struct dict_word *currWord = addWord(cor);
      if(currWord == NULL)
        {
	  return -1;
        }
      //debug("word: %s\n",currWord->word);

      /* one iteration per word in line, except the first word */
      char *mis;
      while((mis = strtok(NULL, " \t\n\v\f\r")))
	{
	  /* add misspellings to word */
	  make_lowercase(mis);
	  struct misspelled_word *currMisspelling = addMisspelledWord(currWord, mis);
	  if(currMisspelling == NULL)
	    {
	      return -1;
	    }
	  //debug("\t misspelling: %s\n", currMisspelling->word);
	}
    }
  return 0;
}

struct dict_word * addWord(char *word){
  /* declare and allocate memory for new struct */
  struct dict_word *currWord;
  if((currWord = (struct dict_word *)malloc(sizeof(struct dict_word))) == NULL)
    {
      return NULL;
    }
  /* initialize struct members (except misspelled[]) */
  strcpy(currWord->word, word);
  currWord->misspelled_count = 0;
  currWord->num_misspellings = 0;
  currWord->next = dict->word_list;
  /* update dictionary head */
  dict->word_list = currWord;
  /* and increment dictionary size */
  dict->num_words += 1;
  return currWord;
}

struct misspelled_word * addMisspelledWord(struct dict_word *correctWord, char *word){
  /* declare and allocate memory for new struct */
  struct misspelled_word *misspelledWord;
  if((misspelledWord = (struct misspelled_word *)malloc(sizeof(struct misspelled_word))) == NULL)
    {
      return NULL;
    }
  /* initialize misspelledWord struct members */
  strcpy(misspelledWord->word, word);
  misspelledWord->misspelled = false;
  misspelledWord->correct_word = correctWord;
  misspelledWord->next = m_list;
  /* update correctWord struct members */
  (correctWord->misspelled)[(correctWord->num_misspellings)++] = misspelledWord;
  /* update m_list head */
  m_list = misspelledWord;
  /* update m_list_length */
  m_list_length++;
  return misspelledWord;
}

int spellCheck(){

  char word[WORDLENGTH];
  char *wdPtr = word;
  
  char c;
  while((c = fgetc(iFile)) != EOF)
    {
      c = tolower(c);
      if(strchr(" \t\n\v\f\r", c) != NULL)
	{
	  //is whitespace
	  *wdPtr = '\0';
	  wdPtr = word;
	  if(strlen(wdPtr) > 0)
	    {
	      //debug("wdPtr: %s\n",wdPtr);
	      processWord(wdPtr);
	    }
	  //fwrite(wdPtr, strlen(wdPtr)+1, 1, oFile);
	  fputc(c, oFile);
	}
      else
	{
	  //assumedly is alphanumeric...
	  *(wdPtr++) = c;
  	}
      //printf("%c",c);
    }
  return 0;
}

void processWord(char *inputWord){

  //debug("in processWord(): %s\n", inputWord);
  //debug("\tlen: %d\n", strlen(inputWord));

  /* handle leading and trailing punctuation */
  char *frontptr = inputWord;
  char *endptr = inputWord + strlen(inputWord) - 1; //note: strlen(inputWord) is guaranteed to be > 0

  while(strchr("~`!@#$%^&*()_-+={[}]|\\:;\"'<,>.?/", *frontptr) && *frontptr != '\0')
    {
      /* write leading punctuation to file */
      fputc(*frontptr, oFile);
      frontptr++;
    }
  if(*frontptr == '\0')
    {
      return;
    }
  inputWord = frontptr;

  int i = 0;
  char buff[WORDLENGTH] = "";
  while(strchr("~`!@#$%^&*()_-+={[}]|\\:;\"'<,>.?/", *endptr) && endptr != frontptr)
    {
      /* save trailing punctuation to buffer */
      buff[i] = *endptr;
      endptr--;
      i++;
    }
  *(endptr+1) = '\0';
  
  

  //debug("\tpost-front-punct stuff: %s\n", inputWord);
 

  /* actually process the word */
  if(foundMisspelledMatch(inputWord))
    {
      fprintf(oFile, "%s", inputWord);
    }
  else if(foundDictMatch(inputWord))
    {
      fprintf(oFile, "%s", inputWord);
    }
  else
    {
      fprintf(oFile, "%s", inputWord);
      if(num_misspellings_to_add >= 0)
	{
	  struct dict_word *new_word = addWord(inputWord);
	  dict_was_edited = true;
	  if(num_misspellings_to_add > 0)
	    {
	      char **random_misspellings = gentypos(num_misspellings_to_add, inputWord);
	      /* oy, memory allocation. */
	      int oy;
	      char oybuff[WORDLENGTH];
	      
	      for(oy = 0; oy < num_misspellings_to_add; oy++)
		{
		  strcpy(oybuff, random_misspellings[oy]);
		  addMisspelledWord(new_word, oybuff);
		}
	      
	      for(oy = 0; oy < num_misspellings_to_add; oy++)
		{
		  free(*(random_misspellings+oy));
		}	
	      free(random_misspellings);
	    }
	}
    }

  /* write trailing punctuation to file from buffer */
  for(i = strlen(buff) - 1; i >=0; i--)
    {
      fputc(buff[i], oFile);
    }

  
}


bool foundMisspelledMatch(char *inputWord){
  struct misspelled_word *listPtr = m_list;
  while(listPtr != NULL)
    {
      if(strcasecmp(inputWord, listPtr->word) == 0)
        {
	  strcpy(inputWord, (listPtr->correct_word)->word);
	  listPtr->misspelled = true;
	  (listPtr->correct_word)->misspelled_count++;
	  num_misspellings_in_input++;
	  return true;
        }
      listPtr = listPtr->next;
    }
  return false;
}


bool foundDictMatch(char *inputWord){
  struct dict_word *listPtr = dict->word_list;
  while(listPtr != NULL)
    {
      if(strcasecmp(inputWord, listPtr->word) == 0)
	{
	  return true;
	}
      listPtr = listPtr->next;
    }
  return false;
}


int writeNewDict(struct dict_word *currWord){
  if(currWord != NULL)
    {
      /*recurse*/
      writeNewDict(currWord->next);

      /*and then do stuff*/
      int error_check = 0;
      error_check = fprintf(dFile, "%s ", currWord->word);
      if(error_check < 0) return -1;
      int i;
      for(i = 0; i < currWord->num_misspellings; i++){
	error_check = fprintf(dFile, "%s ", ((currWord->misspelled)[i])->word);
	if(error_check < 0) return -1;
      }
      error_check = fprintf(dFile, "\n");
      if(error_check < 0) return -1;
    }  
  return 0;
}


int printStats(){
  int error_check;
  error_check = fprintf(stderr, "\n%s%d\n%s%lu\n%s%lu\n%s%d\n%s\n",	    \
			"Total number of words in dictionary: ", dict->num_words, \
			"Size of dictionary (in bytes): ", sizeof(struct dictionary) + (dict->num_words)*sizeof(struct dict_word), \
			"Size of misspelled word list (in bytes): ", m_list_length*sizeof(struct misspelled_word), \
			"Total number of misspelled words: ", num_misspellings_in_input,	\
			"Top 3 misspelled words: "			\
			);
  if(error_check < 0) return -1;
  printTopThree();
  return 0;
}


/* ick this is like 3*O(n) time ick */
void printTopThree(){
  struct dict_word *tmp;
  struct dict_word *top1, *top2, *top3;
  int i;
  bool first_found;
  
  /* first */
  tmp = dict->word_list;
  top1 = dict->word_list;
  while(tmp != NULL)
    {
      if(tmp->misspelled_count >= top1->misspelled_count)
	{
	  top1 = tmp;
	}
      tmp = tmp->next;
    }
  fprintf(stderr, "%s (%d times): ", top1->word, top1->misspelled_count);
  first_found = true;
  for(i = 0; i < top1->num_misspellings; i++)
    {
      if(((top1->misspelled)[i])->misspelled)
	{
	  if(!first_found)
	    {
	      fprintf(stderr, ", ");
	    }
	  first_found = false;
	  fprintf(stderr, "%s", ((top1->misspelled)[i])->word);
	}
    }
  fprintf(stderr, "\n");
  
  /* second */
  tmp = dict->word_list;
  top2 = dict->word_list;
  while(tmp != NULL)
    {
      if(tmp != top1 && tmp->misspelled_count >= top2->misspelled_count)
	{
	  top2 = tmp;
	}
      tmp = tmp->next;
    }
  fprintf(stderr, "%s (%d times): ", top2->word, top2->misspelled_count);
  first_found = true;
  for(i = 0; i < top2->num_misspellings; i++)
    {
      if(((top2->misspelled)[i])->misspelled)
	{
	  if(!first_found)
	    {
	      fprintf(stderr, ", ");
	    }
	  first_found = false;
	  fprintf(stderr, "%s", ((top2->misspelled)[i])->word);
	}
    }
  fprintf(stderr, "\n");

  /* third */
  tmp = dict->word_list;
  top3 = dict->word_list;
  while(tmp != NULL)
    {
      if(tmp != top1 && tmp != top2 && tmp->misspelled_count >= top3->misspelled_count)
	{
	  top3 = tmp;
	}
      tmp = tmp->next;
    }
  fprintf(stderr, "%s (%d times): ", top3->word, top3->misspelled_count);
  first_found = true;
  for(i = 0; i < top3->num_misspellings; i++)
    {
      if(((top3->misspelled)[i])->misspelled)
	{
	  if(!first_found)
	    {
	      fprintf(stderr, ", ");
	    }
	  first_found = false;
	  fprintf(stderr, "%s", ((top3->misspelled)[i])->word);
	}
    }
  fprintf(stderr, "\n");

}


void freeWordList(struct dict_word *currWord){
  if(currWord != NULL)
    {
      freeWordList(currWord->next);

      //free word
      //debug("FREEDd %s\n", currWord->word);
      free(currWord);
    }
}

void freeMList(struct misspelled_word *currWord){
  if(currWord != NULL)
    {
      freeMList(currWord->next);

      //free word
      //debug("FREEDm %s\n", currWord->word);
      free(currWord);
    }
}

