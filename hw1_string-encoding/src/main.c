#include "hw1.h"
#include "my_string.h"

int main(int argc, char **argv) {

    FILE *in = 0;
    FILE *out = 0;
    
    /* check for valid args */
    char c = validargs(argc, argv, &in, &out);
    if(c){
      //printf("success!\n");
    }else{
      //printf("help - failure\n");
      USAGE(EXIT_FAILURE);
      return EXIT_FAILURE;
    }

    /* help menu */
    if(c>>7){
      //printf("help - success\n");
      USAGE(EXIT_SUCCESS);
    }

    /* substitution */
    if(c>>6){
      //printf("sub\n");
      if(c & 0b00100000){
	//printf("decode\n");
	caesar_cipher(&in, &out, 0-(int)(c&0b00011111));
      }else{
	//printf("encode\n");
	caesar_cipher(&in, &out, (int)(c&0b00011111));
      }
    }

    /* tutnese */
    else{
      //printf("tut\n");
      if(c & 0b00100000){
	//printf("decode\n");
	if(tutnese_decode(&in, &out)){
	  return EXIT_FAILURE;
	}
      }else{
	//printf("encode\n");
	tutnese_encode(&in, &out);
      }
    }

    //printf("main files:\nin: %p\nout: %p\n",in,out);
    fclose(in);
    fclose(out);

    //todo: handle -DINFO (not sure where even to handle this)
    
    return EXIT_SUCCESS;
}
