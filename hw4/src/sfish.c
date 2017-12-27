#include "sfish.h"

const char *err_msgs[] = {
  "'exit' shouldn't be followed by arguments.",
  "'help' shouldn't be followed by arguments.",
  "'cd' shouldn't be followed by more than one argument.",
  "Error changing directory.",
  "'pwd' shouldn't be followed by arguments.",
  "Path of current working directory too long (PATH_MAX exceeded).",
  "Error forking.",
  "Command not found.",
  "Can't resolve the given path.",
  "Error redirecting.",
  "Error opening file.",
  "Error upon calling dup2().",
  "'alarm' shouldn't be followed by more than one argument.",
  "dummy!"
};


/*
  mangles cmd and mallocs tokens
*/
void tokenize(char *cmd, char ***tokens, int *num_tokens){

  //allocate memory
  char *cmd_cpy = strdup(cmd);
  int len = num_toks(cmd_cpy, ' ') + 1; //plus NULL-ptr terminator
  free(cmd_cpy);
  *tokens = (char **)malloc(len * sizeof(char *));
  //printf("len (num tokens + 1): %d\n", len);
  
  //generate array of tokens
  char *tmp = NULL;
  int i = 0;
  while(cmd){
    tmp = strsep(&cmd, " ");
    if(strcmp(tmp, "")){
      //add token to array
      (*tokens)[i] = tmp;
      i++;
    }
  }
  (*tokens)[i] = NULL;
  *num_tokens = i;
}


int num_toks(char *cmd, char c){
  char *tmp = NULL;
  int count = 0;
  while(cmd){
    tmp = strsep(&cmd, " ");
    if(strcmp(tmp, "")){
      count++;
    }
  }
  return count;
}


//returns 0 on success,
//       -1 on failure,
//       42 on exit
int parse(char **tokens, int num_tokens){

  if(!num_tokens) return -1;

  if(contains(tokens, num_tokens, ">") ||
     contains(tokens, num_tokens, "<") ||
     contains(tokens, num_tokens, "|") ){
    return redirect(tokens, num_tokens);
  }

  if(!strcmp(tokens[0], "exit")){
    if(num_tokens > 1){
      print_err(EXIT_ERR); //exit shouldn't have args
      return -1;
    }
    return 42;
  }else if(!strcmp(tokens[0], "help")){
    if(num_tokens > 1){
      print_err(HELP_ERR); //help shouldn't have args
      return -1;
    }
    print_help_msg();
  }else if(!strcmp(tokens[0], "cd")){
    //NOTE: ideally, should prooobably do error-checking on return val of getcwd()
    int tmp;
    if(num_tokens == 1){ //cd to ~
      char *oldpwd = getcwd(NULL, 0);
      tmp = chdir(getenv("HOME"));
      if(!tmp){
	  char *pwd = getcwd(NULL, 0);
	  setenv("OLDPWD",oldpwd,1);
	  setenv("PWD",pwd,1);
	  free(pwd);
      }
      free(oldpwd);
    }else if(num_tokens == 2){ //cd to path provided, if it exists
      if(!strcmp(tokens[1], "-")){ //cd to prev dir
	tmp = chdir(getenv("OLDPWD"));
	if(!tmp){
	  char *tmp = getenv("OLDPWD");
	  setenv("OLDPWD", getenv("PWD"), 1);
	  setenv("PWD", tmp, 1);
	}
      }else{ //cd to path provided
	char *oldpwd = getcwd(NULL, 0);
	tmp = chdir(tokens[1]);
	if(!tmp){
	  char *pwd = getcwd(NULL, 0);
	  setenv("OLDPWD",oldpwd,1);
	  setenv("PWD",pwd,1);
	  free(pwd);
	}
	free(oldpwd);
      }
      if(tmp){
	print_err(CHDIR_ERR); //error cd'ing (most likely, no prev dir OR path provided doesn't exist; not sure why cd'ing to HOME might cause an error, but this does check for that)
	return -1;
      }
    }else{
      print_err(CD_ERR); //cd shouldn't have more than 2 args
      return -1;
    }
  }else if(!strcmp(tokens[0], "pwd")){
    if(num_tokens > 1){
      print_err(PWD_ERR); //pwd shouldn't have more than 2 args
      return -1;
    }
    int f = fork();
    if(f == -1){
      print_err(FORK_ERR); //error forking
      return -1;
    }
    if(!f){ //child will run pwd then exit
      char *s = getcwd(NULL,0);
      if(!s){
	print_err(GETCWD_ERR); //length of path of current dir might exceed PATH_MAX
	exit(EXIT_FAILURE);
      }
      printf("%s\n",s);
      free(s);
      exit(EXIT_SUCCESS);
    }else{ //parent will wait for child to terminate
      wait(NULL);      
    }
  }else if(!strcmp(tokens[0], "alarm")){
    if(num_tokens != 2){
      print_err(ALARM_ERR);
      return -1;
    }
    printf("Aw, darn.  I didn't implement this.\n");
    return -1;
  }else{
    int f = fork();
    if(f == -1){
      print_err(FORK_ERR); //error forking
      return -1;
    }
    if(!f){
      my_execvp(tokens[0], tokens);
      //if the above line is passed, execution has failed (execv() should not return)
      print_err(EXEC_ERR); //failed due to invalid command, probably
      exit(EXIT_FAILURE);
    }else{
      waitpid(f, NULL, 0);
    }
  }
  
  return 0;
}


int contains(char **tokens, int num_tokens, char *s){
  int i = 0;
  while(num_tokens){
    if(!strcmp(tokens[num_tokens-1], s)){
      i++;
    }
    num_tokens--;
  }
  return i;
}


//precon: tokens contains s
//        (will retrun num_tokens [which is out-of-bounds] if invalid arg passed)
int index_of(char **tokens, int num_tokens, char *s){
  int i = 0;
  while(i < num_tokens){
    if(!strcmp(tokens[i], s)){
      return i;
    }
    i++;
  }
  return num_tokens;
}


//returns 0 on success,
//       -1 on failure
int redirect(char **tokens, int num_tokens){

  int lt = contains(tokens, num_tokens, "<");
  int gt = contains(tokens, num_tokens, ">");
  int pipsqueak = contains(tokens, num_tokens, "|");

  /* if( (pipsqueak && (lt || gt)) || */
  /*     (pipsqueak > 2) || */
  /*     (lt > 1) || */
  /*     (gt > 1) */
  /*   ){ */
  /*   return -1; */
  /* } */

  if(!lt && gt == 1 && !pipsqueak){
    //printf("prog > out.txt\n");
    int i = index_of(tokens, num_tokens, ">");
    if(i == 0 || i + 1 == num_tokens){
      print_err(REDIRECT_ERR);
      return -1;
    }
    return redirect_gt(tokens, num_tokens, i);
  }else if(lt == 1 && !gt && !pipsqueak){
    //printf("prog < in.txt\n");
    int i = index_of(tokens, num_tokens, "<");
    if(i == 0 || i + 1 == num_tokens){
      print_err(REDIRECT_ERR);
      return -1;
    }
    return redirect_lt(tokens, num_tokens, i);
  }else if(lt == 1 && gt == 1 && !pipsqueak){
    //printf("prog < in.txt > out.txt\n");
    int i = index_of(tokens, num_tokens, "<");
    int j = index_of(tokens, num_tokens, ">");
    if(i > j || i + 1 == j || i == 0 || j + 1 == num_tokens){
      print_err(REDIRECT_ERR);
      return -1;
    }
    return redirect_lt_gt(tokens, num_tokens, i, j);
  }else if(!lt && !gt && pipsqueak == 1){
    //printf("prog1 | prog2\n");
    int i = index_of(tokens, num_tokens, "|");
    if(i == 0 || i + 1 == num_tokens){
      print_err(REDIRECT_ERR);
      return -1;
    }
    return redirect_one_pipe(tokens, num_tokens, i);
  }else if(!lt && !gt && pipsqueak == 2){
    //printf("prog1 | prog2 | prog3\n");
    int i = index_of(tokens, num_tokens, "|");
    int j = index_of(tokens + i + 1, num_tokens - i - 1, "|");
    if(i == 0 || j == 0 || i + j + 2 == num_tokens){
      print_err(REDIRECT_ERR);
      return -1;
    }
    return redirect_two_pipes(tokens, num_tokens, i, j);
  }else{
    print_err(REDIRECT_ERR);
    return -1;
  }
  
  return 0;
}

//FOR THE FOLLOWING FIVE FUNCTIONS:
//it is ensured that the redirection syntax is valid.
//it is not ensured, however, that (where applicable) the given
// commands are executable or the given files exist.

int redirect_gt(char **tokens, int num_tokens, int i){
  //printf("index of >: %d\n", i);
  int f = fork();
  if(!f){
    //note: somehow, ideally, the fail cases should return -1
    //      but whatever; nothing crashes, and the return val doesn't affect behavior
    int fd = open(tokens[i + 1], O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU | S_IRGRP | S_IROTH);
    if(fd == -1){
      print_err(OPEN_ERR);
      exit(EXIT_FAILURE);
    }
    int d = dup2(fd, STDOUT_FILENO);
    if(d == -1){
      print_err(DUP_ERR);
      exit(EXIT_FAILURE);
    }
    tokens[i] = NULL;
    my_execvp(tokens[0], tokens);
    print_err(EXEC_ERR);
    exit(EXIT_FAILURE);
  }else{
    waitpid(f, NULL, 0);
  }
  return 0;
}

int redirect_lt(char **tokens, int num_tokens, int i){
  //printf("index of <: %d\n", i);
  int f = fork();
  if(!f){
    //note: somehow, ideally, the fail cases should return -1
    //      but whatever; nothing crashes, and the return val doesn't affect behavior
    int fd = open(tokens[i + 1], O_RDONLY);
    if(fd == -1){
      print_err(OPEN_ERR);
      exit(EXIT_FAILURE);
    }
    int d = dup2(fd, STDIN_FILENO);
    if(d == -1){
      print_err(DUP_ERR);
      exit(EXIT_FAILURE);
    }
    tokens[i] = NULL;
    my_execvp(tokens[0], tokens);
    print_err(EXEC_ERR);
    exit(EXIT_FAILURE);
  }else{
    waitpid(f, NULL, 0);
  }
  return 0;
}

int redirect_lt_gt(char **tokens, int num_tokens, int i, int j){
  //printf("index of <: %d\n", i);
  //printf("index of >: %d\n", j);
  int f = fork();
  if(!f){
    int fd1 = open(tokens[i + 1], O_RDONLY);
    int fd2 = open(tokens[j + 1], O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU | S_IRGRP | S_IROTH);
    if(fd1 == -1 || fd2 == -1){
      print_err(OPEN_ERR);
      exit(EXIT_FAILURE);
    }
    int d1 = dup2(fd1, STDIN_FILENO);
    int d2 = dup2(fd2, STDOUT_FILENO);
    if(d1 == -1 || d2 == -1){
      print_err(DUP_ERR);
      exit(EXIT_FAILURE);
    }
    tokens[i] = NULL;
    my_execvp(tokens[0], tokens);
    print_err(EXEC_ERR);
    exit(EXIT_FAILURE);
  }else{
    waitpid(f, NULL, 0);
  }
  return 0;
}

int redirect_one_pipe(char **tokens, int num_tokens, int i){
  //printf("index of |: %d\n", i);

  char **tokens0 = (char **)malloc(sizeof(char *) * (i + 3));
  int j = 0;
  while(j < i){
    tokens0[j] = tokens[j];
    j++;
  }
  tokens0[j] = ">";
  tokens0[j + 1] = "tmp.txt";
  tokens0[j + 2] = NULL;

  char **tokens1 = (char **)malloc(sizeof(char *) * (num_tokens - i - 1 + 3));
  j = 0;
  while(j < num_tokens - i - 1){
    tokens1[j] = tokens[i + 1 + j];
    j++;
  }
  tokens1[j] = "<";
  tokens1[j + 1] = "tmp.txt";
  tokens1[j + 2] = NULL;

  parse(tokens0, i + 2);
  parse(tokens1, num_tokens - i - 1 + 2);

  free(tokens0);
  free(tokens1);

  int r = remove("tmp.txt");
  if(r == -1){
    //oy vey.  not sure when this would be reached but might as well have it here
    return -1;
  }
  
  return 0;
}

/* int redirect_one_pipe(char **tokens, int num_tokens, int i){ */
/*   //printf("index of |: %d\n", i); */

/*   int pipefd[2]; */
/*   int p = pipe(pipefd); */
/*   if(p == -1){ */
/*     //todo: error */
/*     printf("errrrrrpiping\n"); */
/*     return -1; */
/*   } */
/*   int f = fork(); */
/*   if(!f){ */
/*     int d = dup2(pipefd[0], STDIN_FILENO); */
/*     if(d == -1){ */
/*       print_err(DUP_ERR); */
/*       exit(EXIT_FAILURE); */
/*     } */
/*     close(pipefd[1]); */
/*     //close(pipefd[0]); */
/*     tokens[i] = NULL; */
/*     my_execvp(tokens[0], tokens); */
/*     print_err(EXEC_ERR); */
/*     exit(EXIT_FAILURE); */
/*   }else{ */
/*     int f1 = fork(); */
/*     if(!f1){ */
/*       int d = dup2(pipefd[1], STDOUT_FILENO); */
/*       if(d == -1){ */
/* 	print_err(DUP_ERR); */
/* 	exit(EXIT_FAILURE); */
/*       } */
/*       close(pipefd[0]); */
/*       //close(pipefd[1]); */
/*       my_execvp((tokens + i + 1)[0], tokens + i + 1); */
/*       print_err(EXEC_ERR); */
/*       exit(EXIT_FAILURE); */
/*     }else{ */
/*       close(pipefd[0]); */
/*       close(pipefd[1]); */
/*       waitpid(f, NULL, 0); */
/*       waitpid(f1, NULL, 0); */
/*     } */
/*   } */

/*   return 0; */
/* } */

int redirect_two_pipes(char **tokens, int num_tokens, int i, int w){
  //printf("index of 1st |: %d\n", i);
  //printf("index of 2nd | relative to 2nd prog: %d\n", j);

  char **tokens0 = (char **)malloc(sizeof(char *) * (i + 3));
  int j = 0;
  while(j < i){
    tokens0[j] = tokens[j];
    j++;
  }
  tokens0[j] = ">";
  tokens0[j + 1] = "tmp1.txt";
  tokens0[j + 2] = NULL;

  char **tokens1 = (char **)malloc(sizeof(char *) * (w + 5));
  int k = i + 1;
  j = 0;
  while(k < w + i + 1){
    tokens1[j] = tokens[k];
    j++;
    k++;
  }
  tokens1[j] = "<";
  tokens1[j + 1] = "tmp1.txt";
  tokens1[j + 2] = ">";
  tokens1[j + 3] = "tmp2.txt";
  tokens1[j + 4] = NULL;

  char **tokens2 = (char **)malloc(sizeof(char *) * (num_tokens - i - w - 2 + 3));
  k = i + w + 2;
  int l = 0;
  while(k < num_tokens){
    tokens2[l] = tokens[k];
    k++;
    l++;
  }
  tokens2[l] = "<";
  tokens2[l + 1] = "tmp2.txt";
  tokens2[l + 2] = NULL;

  parse(tokens0, i + 2);
  parse(tokens1, w + 4);
  parse(tokens2, l + 2);
  
  free(tokens0);
  free(tokens1);
  free(tokens2);

  int r = remove("tmp1.txt");
  int r1 = remove("tmp2.txt");
  if(r == -1 || r1 == -1){
    //oy vey.  not sure when this would be reached but might as well have it here
    return -1;
  }

  return 0;
}

void print_help_msg(){
  printf("SBU sfish, version 0.0.0\n" \
	 "\n" \
	 "\thelp\t\tDisplays this menu.\n" \
	 "\texit\t\tKills this shell.\n" \
	 "\tcd [- | dir]\tChanges the working directory.\n" \
	 "\t\t\tOptions:\n" \
	 "\t\t\t\t (none)  Changes to home directory.\n" \
	 "\t\t\t\t - \t Changes to previous directory.\n" \
	 "\t\t\t\t dir \t Changes to directory provided by 'dir'.\n" \
	 "\tpwd\t\tPrints the absolute path of the present working direcory.\n" \
	 "\talarm\t\tTODO I have no clue.\n" \
	 );
}


void my_execvp(char *file, char **argv){
  char *path;
  if(!strchr(file, '/')){
    //does NOT contain slash
    path = search_PATH(file);
  }else{
    //DOES contain slash
    path = realpath(file, NULL);
    //TODO: use stat(2) instead..???? or is this okay
  }
  if(path){
    argv[0] = path;
    execv(path, argv);
    free(path);
  }
  //note: if this line is reached, my_execvp will return
  //      error-handling will be taken care of in parse()
}


//returns absolute path if file found
//        NULL upon failure
char * search_PATH(char *file){
  char *PATH = strdup(getenv("PATH"));
  char *free_this_ptr = PATH;

  char *tmp = PATH;
  char *return_this_ptr = NULL;
  while(PATH){
    tmp = strsep(&PATH, ":");
    if(strcmp(tmp, "")){
      char *path_to_eval = (char *)malloc(sizeof(char) * (strlen(tmp) + 1 + strlen(file) + 1)); //plus /, plus \0
      strcpy(path_to_eval, tmp);
      strcat(path_to_eval, "/");
      strcat(path_to_eval, file);
      struct stat buf;
      int i = stat(path_to_eval, &buf);
      //printf("\tpath to eval: %s\n", path_to_eval);
      if(!i){ //found the file!
	return_this_ptr = path_to_eval;
	break;
      }
      free(path_to_eval);
    }
  }

  free(free_this_ptr);
  return return_this_ptr;
}

/*
  help
  exit
  cd
  cd [path]
  cd -
  pwd
  alarm..?
  prog [args] > output.txt
  prog [args] < input.txt
  prog [args] < input.txt > output.txt
  prog1 [args] | prog2 [args]
  prog1 [args] | prog2 [args] | prog3 [args]
  
 */

void print_err(err_num n){
  if(n < DUMMY){
    fprintf(stderr, "%s\n", err_msgs[n]);
  }
}
