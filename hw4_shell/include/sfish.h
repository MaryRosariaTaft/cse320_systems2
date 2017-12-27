#ifndef SFISH_H
#define SFISH_H
#include <readline/readline.h>
#include <readline/history.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

typedef enum{
  EXIT_ERR,
  HELP_ERR,
  CD_ERR,
  CHDIR_ERR,
  PWD_ERR,
  GETCWD_ERR,
  FORK_ERR,
  EXEC_ERR,
  PATH_ERR,
  REDIRECT_ERR,
  OPEN_ERR,
  DUP_ERR,
  ALARM_ERR,
  DUMMY
} err_num;

/* const char *err_msgs[] = { */
/*   "'exit' shouldn't be followed by args todo", */
/*   "'help' should not be followed by arguments todoooo", */
/*   "'cd' error todo", */
/*   "'chdir' error todo", */
/*   "'pwd' error todo", */
/*   "'fork' error todo", */
/*   "Unable to execute command.", */
/*   "Can't resolve the given path.", */
/*   "dummy!" */
/* }; */


void tokenize(char *cmd, char ***tokens, int *num_tokens);
int num_toks(char *s, char c);
int parse(char **tokens, int num_tokens);
int contains(char **tokens, int num_tokens, char *s);
int redirect(char **tokens, int num_tokens);
int redirect_gt(char **tokens, int num_tokens, int i);
int redirect_lt(char **tokens, int num_tokens, int i);
int redirect_lt_gt(char **tokens, int num_tokens, int i, int j);
int redirect_one_pipe(char **tokens, int num_tokens, int i);
int redirect_two_pipes(char **tokens, int num_tokens, int i, int j);
void print_help_msg();
void my_execvp(char *file, char *argv[]);
char * search_PATH(char *file);
void print_err(err_num n);

#endif

