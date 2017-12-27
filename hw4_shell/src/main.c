#include "sfish.h"
#include "debug.h"

/*
 * As in previous hws the main function must be in its own file!
 */

//tokenize cmd to determine number of args--return char **, array of ptrs to the args WHICH HAS BEEN DYNAMICALLY ALLCOATED AND MAKE SURE TO FREE IT LATER W/IN MAIN
//determine WHETHER the set of tokens is valid; IF SO, respond accordingly:
//   built-in thingys: help, exit, cd [-], pwd (FORK), alarm ##
//   executables (have to manually search PATH T_T )
//   redirection >, <. < >, |, | |
//   *OPTIONALLY 1>, 2>, &>, >>


static void handle_sigusr2(int signo){
  //todo: should use write() instead of printf().  printf is not async-sig-safe
  printf("Well that was easy.\n");
}

void handle_sigchld(int signo, siginfo_t *s, void *p){
  //todo: should use write() ripriprip
  clock_t cpu_time = s->si_utime + s->si_stime;
  unsigned long mills = cpu_time * 1000 / CLOCKS_PER_SEC;
  printf("Child with PID %d has died. It spent %lu milliseconds utilizing the CPU\n", s->si_pid, mills);
}

int main(int argc, char const *argv[], char* envp[]){

  //signal handling
  rl_catch_signals = 0;
  signal(SIGUSR2, handle_sigusr2);
  struct sigaction s;
  s.sa_sigaction = handle_sigchld;
  sigfillset(&s.sa_mask);
  s.sa_flags = SA_SIGINFO;
  sigaction(SIGCHLD, &s, NULL);
  sigset_t sig_set;
  sigemptyset(&sig_set);
  sigaddset(&sig_set, SIGTSTP);
  sigprocmask(SIG_BLOCK, &sig_set, NULL);
  
  //initial prompt
  char *cmd;
  char *cwd = getcwd(NULL, 0);
  char *prompt = (char *)malloc(sizeof(char) * (strlen("<mtaft> : <") + strlen(cwd) + 5)); // plus >, plus ' ', plus $, plus ' ', plus \0
  strcpy(prompt, "<mtaft> : <");
  strcat(prompt, cwd);
  strcat(prompt, "> $ \0");

  //repl
  while((cmd = readline(prompt)) != NULL) {

    //readline function to allow user to up/down arrow through history
    add_history(cmd);

    //tokenize and parse user input
    char **tokens = NULL;
    int num_tokens = 0;
    tokenize(cmd, &tokens, &num_tokens);
    int ret = parse(tokens, num_tokens);

    //free all allocated mem
    free(tokens);
    free(cmd);
    free(prompt);
    free(cwd);

    //exit if requested by user
    if(ret == 42){
      exit(EXIT_SUCCESS);
    }

    //update prompt
    cwd = getcwd(NULL, 0);
    prompt = (char *)malloc(sizeof(char) * (strlen("<mtaft> : <") + strlen(cwd) + 5)); // plus >, plus ' ', plus $, plus ' ', plus \0
    strcpy(prompt, "<mtaft> : <");
    strcat(prompt, cwd);
    strcat(prompt, "> $ \0");

  }

  //this line should never be reached, but...
  return EXIT_SUCCESS;

}
