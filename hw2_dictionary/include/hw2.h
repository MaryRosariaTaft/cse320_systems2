#ifndef HW_H
#define HW_H

#include <unistd.h> //for main.c
#include <libgen.h> //for main.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include "debug.h"
#include "mispelling.h"

#define MAX_SIZE 256
#define WORDLENGTH 50
#define MAX_MISSPELLED_WORDS 5

#define USAGE(return_code) do{ \
  fprintf(stderr, "%s\n", \
    "Usage: spell [-h] [-o OUT_FILE] [-i IN_FILE] [-d DICT_FILE] [-An]\n\n" \
    "Spell Checker using a custom dictionary. Auto corrects any known misspellings in the text.\n" \
    "Additional function to add new words and randomized misspellings to the dictionary.\n\n" \
    "Option arguments:\n" \
      "\t-h\tDisplays this usage.\n" \
      "\t-o\tOUT_FILE filename, if omitted output to stdout\n" \
      "\t-i\tIN_FILE filename, if omitted input comes from stdin\n" \
      "\t-d\tfor the dictionary filename, if omitted use default \"rsrc/dictionary.txt\"\n" \
      "\t-An\tAutomatically add n (in range 0-5) random misspellings for any word not in the dictionary.\n"); \
  exit(return_code); \
  } while(0)

FILE *DEFAULT_INPUT;
FILE *DEFAULT_OUTPUT;
struct dictionary *dict;
struct misspelled_word *m_list;

FILE *oFile;
FILE *iFile;
FILE *dFile;
int num_misspellings_to_add;

bool dict_was_edited;
char actual_dict_file[MAX_SIZE];
int m_list_length;
int num_misspellings_in_input;


struct dictionary{
  int num_words; //initialize to 0
  struct dict_word *word_list; //initialize to NULL
};

struct dict_word{
  char word[WORDLENGTH];
  int misspelled_count; //initialize to 0
  int num_misspellings; //initialize to 0
  struct misspelled_word *misspelled[MAX_MISSPELLED_WORDS];
  struct dict_word *next; //initialize to head of dict->word_list
};

struct misspelled_word{
  char word[WORDLENGTH];
  bool misspelled; //initialize to false
  struct dict_word *correct_word;
  struct misspelled_word *next; //initialize to head of m_list
};


void make_lowercase(char *s);


int processDictionary();


struct dict_word * addWord(char *word);


struct misspelled_word * addMisspelledWord(struct dict_word *correctWord, char *word);


int spellCheck();


void processWord(char *inputWord);


bool foundMisspelledMatch(char *inputWord);


bool foundDictMatch(char *inputWord);


int writeNewDict(struct dict_word *currWord);


int printStats();


void printTopThree();


void freeWordList(struct dict_word *currWord);


void freeMList(struct misspelled_word *currWord);


#endif
