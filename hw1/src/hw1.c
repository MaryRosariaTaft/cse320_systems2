#include "my_string.h"
#include "hw1.h"


char validargs(int argc, char **argv, FILE **in, FILE **out) {

  char mode_byte = 0; //default return value (0) indicates failure

  /* no args */
  if(argc <= 1){
    //printf("no args\n");
    return 0;
  }

  /* help menu */
  if(!my_strcmp(*(argv+1),"-h")){
    //printf("help\n");
    return 0b10000000;
  }
  
  /* substitution cipher */
  else if(!my_strcmp(*(argv+1),"-s") && (argc == 5 || argc == 6)){
    //printf("sub\n");
    mode_byte |= 0b01000000;

    /* encode/decode */
    if(!my_strcmp(*(argv+2),"-e")){
      //printf("encode\n");
    }else if(!my_strcmp(*(argv+2),"-d")){
      //printf("decode\n");
      mode_byte |= 0b00100000;
    }else{
      //printf("fail\n");
      return 0;
    }

    /* file i/o */
    if(setup_io(argv,in,out)){
      return 0;
    }

    /* shamt */
    if(argc == 6){
      //printf("shamt\n");
      char *nptr = *(argv+5);
      char *tmp = NULL;
      char **endptr = &tmp;
      int shamt = (int)strtol(nptr,endptr,10);
      if(*nptr != '\0' && **endptr == '\0'){ //if a valid string was provided
	//printf("shamt: %d\n",(int)(shamt%my_strlen(Alphabet)));
	mode_byte |= (int)(shamt%my_strlen(Alphabet));
      }else{
	//printf("invalid shamt input\n");
	return 0;
      }
    }else{
      //printf("default to shamt 320\n");
      mode_byte |= (int)(320%my_strlen(Alphabet));
    }
  }

  /* tutnese */
  else if(!my_strcmp(*(argv+1),"-t") && argc == 5){
    //printf("tut\n");

    /* encode/decode */
    if(!my_strcmp(*(argv+2),"-e")){
      //printf("encode\n");
    }else if(!my_strcmp(*(argv+2),"-d")){
      //printf("decode\n");
      mode_byte |= 0b00100000;
    }else{
      //printf("fail\n");
      return 0;
    }

    /* file i/o */
    if(setup_io(argv,in,out)){
      return 0;
    }

    /* lsb filler */
    mode_byte |= 20;
  }

  /* invalid args */
  else{
    //printf("fail\n");
  }

  //printf("mode byte: 0x%x\n",mode_byte);
  return mode_byte;

}


int setup_io(char **argv, FILE **in, FILE **out){
  
  /* input */
  if(!my_strcmp(*(argv+3),"-")){
    //printf("stdin\n");
    *in = stdin;
  }else{
    //printf("infile\n");
    *in = fopen(*(argv+3), "r");
    //fclose(*in);
  }

  /* output */
  if(!my_strcmp(*(argv+4),"-")){
    //printf("stdout\n");
    *out = stdout;
  }else{
    //printf("outfile\n");
    *out = fopen(*(argv+4), "w");
    //fclose(*out);
  }

  /* i/o error */
  if(*in == NULL || *out == NULL){
    //printf("i/o error\n");
    return -1;
  }

  return 0;
}


void caesar_cipher(FILE **in, FILE **out, int shamt){ //shift left
  //printf("encode shamt: %d\n",shamt);
  //printf("cipher files:\nin: %p\nout: %p\n",*in,*out);

  char c;

  while((c = fgetc(*in)) != EOF){
    if(c >= 97 && c <= 122){
      c -= 32;
    }
    fputc(caesar_shift(c,shamt), *out);
  }
  fputc('\n', *out);
  
}


char caesar_shift(char c, int shamt){
  //printf("in shift\n");
  int i = my_strchr(Alphabet, c);
  if(i == -1 || !shamt){
    return c;
  }
  i = (i+shamt)%my_strlen(Alphabet);
  if(i >= 0){
    return *(Alphabet + i);
  }
  return *(Alphabet + my_strlen(Alphabet) + i);

}


void tutnese_encode(FILE **in, FILE **out){

  char prev = 0;
  char cur;

  /* loop through chars on infile */
  while((cur = fgetc(*in)) != EOF){
    if((cur == prev) ||
       ((cur >= 97 && cur <= 122) && (cur - 32 == prev)) ||
       ((cur >= 65 && cur <=  90) && (cur + 32 == prev)) ){
      write_encoded(out, &prev, &cur);
      prev = 0;
    }
    else{
      write_encoded(out, &prev, NULL);
      prev = cur;
    }
  }

  /* there's surely a better way to do this, but here's one last iteration to handle the char preceding EOF */
  if((cur == prev) ||
     ((cur >= 97 && cur <= 122) && (cur - 32 == prev)) ||
     ((cur >= 65 && cur <=  90) && (cur + 32 == prev)) ){
    write_encoded(out, &prev, &cur);
    prev = 0;
  }
  else{
    write_encoded(out, &prev, NULL);
    prev = cur;
  }

  /* newline! */
  fputc('\n', *out);
  
  return;

}


void write_encoded(FILE **out, char *prev, char *cur){
  
  /* not a double */
  if(cur == NULL){
    if(!(*prev)){ //writing nothing
      return;
    }
    fputc(*prev, *out);
    fputs(char_to_syl(prev), *out);
  }

  /*is a double */
  else{
    //NOTE: handles non-alpha chars as though they're not even doubles but just two distinct occurrences of the letter
    if(*prev >= 97 && *prev <= 122){
      fputc('s', *out);
      fputs("qua", *out); //qua if in tut, quat if not
      if(!my_strcmp(char_to_syl(prev),"") && (*prev == 'a' || *prev == 'A' || *prev == 'e' || *prev == 'E' || *prev == 'i' || *prev == 'I' || *prev == 'o' || *prev == 'O' || *prev == 'u' || *prev == 'U')){
	fputc('t', *out);
      }
      fputc(*cur, *out);
      fputs(char_to_syl(cur), *out);
    }else if(*prev >= 65 && *prev <= 90){
      fputc('S', *out);
      fputs("qua", *out); //qua if in tut, quat if not
      if(!my_strcmp(char_to_syl(prev),"") && (*prev == 'a' || *prev == 'A' || *prev == 'e' || *prev == 'E' || *prev == 'i' || *prev == 'I' || *prev == 'o' || *prev == 'O' || *prev == 'u' || *prev == 'U')){
	fputc('t', *out);
      }
      fputc(*cur, *out);
      fputs(char_to_syl(cur), *out);
    }else{
      fputc(*prev, *out);
      fputc(*cur, *out);
    }
  }

  return;
}


char * char_to_syl(char *c){

  int i = 0;

  while(*(Tutnese+i)){
    //printf("%s ",*(Tutnese+i)); //syls
    //printf("%s ",*(Tutnese+i)+1); //all but first chars of syls
    if(*c == **(Tutnese+i)){
      return *(Tutnese+i)+1;
    }else if(*c >= 65 || *c <= 90){ //uppercase
      if(*c + 32 == **(Tutnese+i)){
	return *(Tutnese+i)+1;
      }
    }
    i++;
  }
  
  return "";
}


//return 0 on success, -1 on failure
int tutnese_decode(FILE **in, FILE **out){

  char cur;
  
  while((cur = fgetc(*in)) != EOF){
    char *mapping = char_to_syl(&cur);
    int is_double = 0;
    
    /* no mapping */
    if(!my_strcmp(mapping,"")){
      //printf("no mapping of %c\n",cur);

      if(cur == 's' || cur == 'S'){
	*(buffer+0) = fgetc(*in);
	*(buffer+1) = fgetc(*in);
	*(buffer+2) = fgetc(*in);
	*(buffer+3) = '\0';
	//printf("buff: %s\n",buffer);
	if(!my_strcmp(buffer,"qua")){
	  //printf("correct-unmapped\n");
	  is_double = 1;
	}else{
	  ungetc(*(buffer+2),*in);
	  ungetc(*(buffer+1),*in);
	  ungetc(*(buffer+0),*in);
	}
      }
      
      if(!is_double){
	fputc(cur, *out);
      }else{ //"squa"||"Squa" detected
	//fputc('*', *out);
	/*SAME CODE AS BELOW LAWL*/

	///////////////


	char doubled = fgetc(*in);

	/* squat */
	if(doubled == 't'){
	  doubled = fgetc(*in);
	  if(doubled != 'a' && doubled != 'A' && doubled != 'e' && doubled != 'E' && doubled != 'i' && doubled != 'I' && doubled != 'o' && doubled != 'O' && doubled != 'u' && doubled != 'U'){ //squat followed by a non-vowel is invalid
	    return -1;
	  }else{
	    if(cur == 's'){
	      if(doubled < 97){
		fputc(doubled+32, *out);
	      }else{
		fputc(doubled, *out);
	      }
	    }else{
	      if(doubled >= 97){
		fputc(doubled-32, *out);
	      }else{
		fputc(doubled, *out);
	      }
	    }
	    fputc(doubled, *out);
	  }
	}

	/* squa */
	else{
	  if(!((doubled >= 97 && doubled <= 122) || (doubled >= 65 && doubled <= 90))){ //squa-doubled symbols are incorrect/invalid (NOTE: NOT accounting for the case that neither s nor q has a mapping)
	    //printf("symbolsqua\n");
	    return -1;
	  }
	  if(doubled != 'a' && doubled != 'A' && doubled != 'e' && doubled != 'E' && doubled != 'i' && doubled != 'I' && doubled != 'o' && doubled != 'O' && doubled != 'u' && doubled != 'U'){
	    //move infile stream to the next desired char (e.g., 'b' in "squayuckbub")
	    char *mapping1 = char_to_syl(&doubled);
	    if(my_strcmp(mapping1,"")){
	      //printf("has mapping\n");
	      int j = 0;
	      while(*(mapping1+j)){
		if(*(mapping1+j) != fgetc(*in)){
		  return -1;
		}
		j++;
	      }
	    }
	    //then (if no error has occurred) write two of doubled to outfile, accounting for case of the first based on case of 's'/'S'
	    if(cur == 's'){
	      if(doubled < 97){
		fputc(doubled+32, *out);
	      }else{
		fputc(doubled, *out);
	      }
	    }else{
	      if(doubled >= 97){
		fputc(doubled-32, *out);
	      }else{
		fputc(doubled, *out);
	      }
	    }
	    fputc(doubled, *out);

	  }else{ //squa followed by a vowel is invalid
	    return -1;
	  }

	}	
	///////////////

	
      }
    }
    /* has a mapping */
    else{
      int i = 0;
      while(*(mapping+i)){
	if(*(mapping+i) != (*buffer = fgetc(*in))){
	  if(cur == 's' || cur == 'S'){
	    //
	    *(buffer+1) = fgetc(*in);
	    *(buffer+2) = fgetc(*in);
	    *(buffer+3) = '\0';
	    //printf("buff: %s\n",buffer);
	    if(!my_strcmp(buffer,"qua")){
	      //printf("correct-mapped\n");
	      is_double = 1;
	      break;
	    }else{
	      return -1;
	    }
	    //
	  }else{
	    return -1;
	  }
	}
	i++;
      }

      if(!is_double){
	fputc(cur, *out);
      }else{ //"squa"||"Squa" detected

	char doubled = fgetc(*in);

	/* squat */
	if(doubled == 't'){
	  doubled = fgetc(*in);
	  if(doubled != 'a' && doubled != 'A' && doubled != 'e' && doubled != 'E' && doubled != 'i' && doubled != 'I' && doubled != 'o' && doubled != 'O' && doubled != 'u' && doubled != 'U'){ //squat followed by a non-vowel is invalid
	    return -1;
	  }else{
	    if(cur == 's'){
	      if(doubled < 97){
		fputc(doubled+32, *out);
	      }else{
		fputc(doubled, *out);
	      }
	    }else{
	      if(doubled >= 97){
		fputc(doubled-32, *out);
	      }else{
		fputc(doubled, *out);
	      }
	    }
	    fputc(doubled, *out);
	  }
	}

	/* squa */
	else{
	  if(!((doubled >= 97 && doubled <= 122) || (doubled >= 65 && doubled <= 90))){ //squa-doubled symbols are incorrect/invalid (NOTE: NOT accounting for the case that neither s nor q has a mapping)
	    //printf("symbolsqua\n");
	    return -1;
	  }
	  if(doubled != 'a' && doubled != 'A' && doubled != 'e' && doubled != 'E' && doubled != 'i' && doubled != 'I' && doubled != 'o' && doubled != 'O' && doubled != 'u' && doubled != 'U'){
	    //move infile stream to the next desired char (e.g., 'b' in "squayuckbub")
	    char *mapping1 = char_to_syl(&doubled);
	    if(my_strcmp(mapping1,"")){
	      //printf("has mapping\n");
	      int j = 0;
	      while(*(mapping1+j)){
		if(*(mapping1+j) != fgetc(*in)){
		  return -1;
		}
		j++;
	      }
	    }
	    //then (if no error has occurred) write two of doubled to outfile, accounting for case of the first based on case of 's'/'S'
	    if(cur == 's'){
	      if(doubled < 97){
		fputc(doubled+32, *out);
	      }else{
		fputc(doubled, *out);
	      }
	    }else{
	      if(doubled >= 97){
		fputc(doubled-32, *out);
	      }else{
		fputc(doubled, *out);
	      }
	    }
	    fputc(doubled, *out);

	  }else{ //squa followed by a vowel is invalid
	    return -1;
	  }
	}
      }
      
    }
  }

  //todo: handle EOF iteration case thing
  
  return 0;
}
