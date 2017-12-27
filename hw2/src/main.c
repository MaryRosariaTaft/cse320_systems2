#include "hw2.h"
#include "debug.h"

int main(int argc, char *argv[]){

  char DEFAULT_DICT_FILE[] = "rsrc/dictionary.txt";
  DEFAULT_INPUT = stdin;
  DEFAULT_OUTPUT = stdout;

  dict_was_edited = false;
  strcpy(actual_dict_file, DEFAULT_DICT_FILE);
  m_list_length = 0;
  num_misspellings_in_input = 0;
  
  /* allocate space for dict */
  if((dict = (struct dictionary *)malloc(sizeof(struct dictionary))) == NULL)
    {
      //printf("ERROR: OUT OF MEMORY.\n");
      exit(EXIT_FAILURE);
    }

  /* and initialize its members */
  dict->num_words = 0;
  dict->word_list = NULL;
  
  /* initialize m_list to null */
  m_list = NULL;

  /* init i/o/d files to default values */
  oFile = DEFAULT_OUTPUT;
  iFile = DEFAULT_INPUT;
  dFile = fopen(DEFAULT_DICT_FILE, "r");
  if(dFile == NULL)
    {
      free(dict);
      exit(EXIT_FAILURE);
    }
  num_misspellings_to_add = -1;

  /* parse args */
  bool failed = false; //see note in case 'o'
  opterr = 0; //suppress getopt errors from printing
  int opt;
  while((opt = getopt(argc, argv, "ho:i:d:A:")) != -1)
    {
      //TODO: don't accept "-isrc/dict.txt" or "-A 5" or the like (this time, whitespace <does> matter...)
      switch(opt)
	{
	case 'h':
	  //debug("option h\n");
	  free(dict);
	  free(m_list);
	  if(oFile) fclose(oFile);
	  if(iFile) fclose(iFile);
	  if(dFile) fclose(dFile);
	  USAGE(EXIT_SUCCESS);
	  break;
	case 'o':
	  optarg--;
	  if(*optarg == 'o'){
	    failed = true;
	    break;
	  }
	  optarg++;
	  oFile = fopen(optarg, "w");
	  if(oFile == NULL)
	    {
	      failed = true; //save exit(EXIT_FAILURE) till end in case -h was invoked
	    }
	  break;
	case 'i':
	  optarg--;
	  if(*optarg == 'i'){
	    failed = true;
	    break;
	  }
	  optarg++;
	  iFile = fopen(optarg, "r");
	  if(iFile == NULL)
	    {
	      failed = true;
	    }
	  break;
	case 'd':
	  optarg--;
	  if(*optarg == 'd'){
	    failed = true;
	    break;
	  }
	  optarg++;

	  strcpy(actual_dict_file, optarg);
	  dFile = fopen(optarg, "r");
	  if(dFile == NULL)
	    {
	      failed = true;
	    }
	  break;
	case 'A':
	  optarg--;
	  if(*optarg != 'A'){
	    failed = true;
	    break;
	  }
	  optarg++;
	  char *endptr;
	  num_misspellings_to_add = strtol(optarg, &endptr, 10);
	  if(*endptr != '\0' || num_misspellings_to_add < 0 || num_misspellings_to_add > 5)
	    {
	      failed = true;
	    }
	  //debug("\tn = %d\n", num_misspellings_to_add);
	  break;
	default:
	  //debug("invalid options\n");
	  failed = true;
	}
    }

  if(failed)
    {
      freeWordList(dict->word_list);
      free(dict);
      freeMList(m_list);
      if(oFile) fclose(oFile);
      if(iFile) fclose(iFile);
      if(dFile) fclose(dFile);
      USAGE(EXIT_FAILURE);
    }

  /* process dictionary */
  int tmp = processDictionary();
  if(tmp == -1)
    {
      //debug("error parsing dictionary\n", NULL);
      freeWordList(dict->word_list);
      free(dict);
      freeMList(m_list);
      if(oFile) fclose(oFile);
      if(iFile) fclose(iFile);
      if(dFile) fclose(dFile);
      exit(EXIT_FAILURE);
    }
  
  /* perform spell check on input and write updates to output */
  tmp = spellCheck();
  if(tmp == -1)
    {
      //debug("error spell-checking\n", NULL);
      freeWordList(dict->word_list);
      free(dict);
      freeMList(m_list);
      if(oFile) fclose(oFile);
      if(iFile) fclose(iFile);
      if(dFile) fclose(dFile);
      exit(EXIT_FAILURE);      
    }
  
  
  if(dict_was_edited)
    {
      /* open new dict file */
      //printf("writing new dict! old dictfileful: %s\n", actual_dict_file);
      char buff_copy[MAX_SIZE];
      char buff_new[MAX_SIZE];
      strcpy(buff_copy, actual_dict_file);
      strcpy(buff_new, dirname(buff_copy));
      //printf("writing new dict! old dictfiledir: %s\n", dirname(buff));
      strcat(buff_new, "/new_");
      strcpy(buff_copy, actual_dict_file);
      strcat(buff_new, basename(buff_copy));
      //printf("writing new dict! old dictfilenam: %s\n", basename(buff));
      if(dFile) fclose(dFile);
      dFile = fopen(buff_new, "w");

      /* and write to it */
      tmp = writeNewDict(dict->word_list);
      if(tmp == -1)
	{
	  //debug("error writing new dictionary\n", NULL);
	  freeWordList(dict->word_list);
	  free(dict);
	  freeMList(m_list);
	  if(oFile) fclose(oFile);
	  if(iFile) fclose(iFile);
	  if(dFile) fclose(dFile);
	  exit(EXIT_FAILURE);      
	}
    }
  
  //check if stuff was inserted into linked lists properly
  //terrible way to debug lol but it worked!!!
  /*
  debug("\nWORD LIST\n",NULL);
  while(dict->word_list){
    debug("word in dict->word_list: %s\n", (dict->word_list)->word);
    for(int i=0;i<5;i++)
      {
	debug("\tmisspelling: %s\n", (((dict->word_list)->misspelled)[i])->word);
      }
    debug("\tnum_mis: %d\n",(dict->word_list)->num_misspellings);
    debug("\tmis_cnt: %d\n",(dict->word_list)->misspelled_count);
    (dict->word_list) = (dict->word_list)->next;
  }
  debug("\nM LIST\n",NULL);
  while(m_list){
    debug("word in m_list: %s\n", m_list->word);
    debug("\tmisp? %d\n",m_list->misspelled);
    m_list = m_list->next;
  }
  */
  
  tmp = printStats();
  if(tmp == -1)
    {
      //debug("error printing stats\n", NULL);
      freeWordList(dict->word_list);
      free(dict);
      freeMList(m_list);
      if(oFile) fclose(oFile);
      if(iFile) fclose(iFile);
      if(dFile) fclose(dFile);
      exit(EXIT_FAILURE);      
    }
  

#if 0

  strcpy(line, "\n--------DICTIONARY WORDS--------\n");
  fwrite(line, strlen(line)+1, 1, oFile);
  printWords(dict->word_list , oFile);

#endif

  //printf("\n--------FREED WORDS--------\n");
  freeWordList(dict->word_list);
  free(dict);
  freeMList(m_list);

  if(oFile) fclose(oFile);
  if(iFile) fclose(iFile);
  if(dFile) fclose(dFile);

  exit(EXIT_SUCCESS);
}
