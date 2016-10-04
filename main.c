/******************************************************
#   By: Carol Toro
#   File Created: 10/03/2016
#   Last Modified: 10/03/2016
#   Filename: main.c
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

#include "simple_shell.h"

int main()
{

    struct Shell *curUser = sh_init();

    /*Catch signals to ignore CTRL-C*/
    struct sigaction ignoreC;
    ignoreC.sa_handler = SIG_IGN;
    ignoreC.sa_flags = 0;
    sigfillset(&(ignoreC.sa_mask));
    sigaction(SIGINT, &ignoreC, NULL);

    while (1) {
        sh_catch_bg(curUser);
        sh_get_commands(curUser);
        sh_parse_args(curUser);
        sh_command_ground(curUser);
        sh_identify_command(curUser);
    }


    /*Clean up*/
    sh_free(curUser);

    exit(0);

}