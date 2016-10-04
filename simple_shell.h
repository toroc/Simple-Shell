/******************************************************
#   CS 344 Operating Systems
#   By: Carol Toro
#   File Created: 2/16/2016
#   Last Modified: 2/24/2016
#   Filename: simple_shell.c
#   Description: This program implements a shell in C 
#       which runs command line instructions and returns
#       results. The shell supports redirection of standard
#       input and standard output. The shell also supports
#       foreground and background processes. The shell has
#       three built in commands: exit, cd, and status. The
#       shell supports comments and blank lines.
#
#
******************************************************/
#include <errno.h>
#include <dirent.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>


#define MAX_ARGS 512
#define MAX_LEN 2048

/*Global Variables*/
char g_last_status[MAX_LEN];
volatile sig_atomic_t flag; /*0=not terminated 1=terminated*/
volatile sig_atomic_t sig_num; /*store signal term #*/

/*Data structure to house user's commands*/
struct Shell 
{
    char *user_input; /*string to capture input*/
    int args_count; /*number of arguments in command*/
    char **arguments; /*array of strings to contain each arg*/
    int ground; /*0 = fore, 1=back*/
    int status; 
    int read; /*0=false, 1=true*/
    int write; /*0=false, 1=true*/
    int bg_PIDs[MAX_ARGS]; /*Array containing bg PIDs*/
    int bg_count; /*Count of bg PIDs*/
};


typedef struct Shell Shell;

/*User object prototypes*/
Shell* sh_init();
void sh_free(struct Shell *this_shell);

/*Command line helper functions*/
void sh_get_commands(struct Shell *this_shell);
void sh_parse_args(struct Shell *this_shell);
void sh_identify_command(struct Shell *this_shell);
void sh_command_ground(struct Shell *this_shell);

/*Built-in Shell Commands*/
void sh_change_directory(struct Shell *this_shell);
void sh_command_status(struct Shell *this_shell);
void exitShell(struct Shell *this_shell);

/*Command execution via processes*/
void sh_other_command(struct Shell *this_shell);
void sh_execute_command(struct Shell *this_shell);
void sh_fg_process(struct Shell *this_shell);
void sh_reg_fg_process(struct Shell *this_shell);
void sh_bg_process(struct Shell *this_shell);

/*Program helper functions*/
void sh_catch_interr(int signo);
void sh_kill_zombies(struct Shell *this_shell);
void sh_catch_bg(struct Shell *this_shell);




