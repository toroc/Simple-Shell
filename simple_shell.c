/******************************************************
#   By: Carol Toro
#   File Created: 2/16/2016
#   Last Modified: 10/03/2016
#   Filename: simple_shell.h
#   Description: Shell function definitions.
#
******************************************************/
#include "simple_shell.h"


/******************************************************
#   Shell Structure Functions
******************************************************/
/******************************************************
#   sh_init
#   @desc: allocate memory for shell data structure
#       and return pointer to shell object
#   @param: n/a
#   @return: struct Shell *the_shell
******************************************************/
Shell* sh_init()
{

    /*Allocate memory for shell and args*/
    Shell *the_shell = malloc(sizeof(Shell));
    the_shell->user_input = malloc(MAX_LEN* sizeof(char*));
    the_shell->arguments = (char**)malloc(MAX_ARGS*sizeof(char*));
    the_shell->bg_count=0;

    int i = 0;

    /*Allocate memory for array of strings*/
    for (i = 0; i < MAX_ARGS; i++) 
    {
        the_shell->arguments[i] = (char*)malloc(20 * sizeof(char));
    }

    return the_shell;

}
/******************************************************
#   sh_free
#   @desc: Deallocates the memory used up by shell
#   @param: pointer to shell object
#   @return: void
******************************************************/
void sh_free(struct Shell *this_shell)
{
    free(this_shell);
}

/******************************************************
#   Command Line helper functions
******************************************************/
/******************************************************
#   sh_get_commands
#   @desc: prompts cml and reads commands
#   @param: pointer to shell object
#   @return:
******************************************************/
void sh_get_commands(struct Shell *this_shell)
{
    /*Prompt user for commands*/
    fprintf(stdout, ": ");
    fflush(stdout);
    fgets(this_shell->user_input, 2048, stdin);

}

/******************************************************
#   sh_parse_args
#   @desc: parses the command stored in  user's user_input into
#       individual arguments stored in user's arguments
#   @param: pointer to shell object
#   @return: void
******************************************************/
void sh_parse_args(struct Shell *this_shell)
{

    /*point to the user input*/
    char *buf = this_shell->user_input;
    char **args = this_shell->arguments;
    int num = 0;


    while ((*buf != '\0')) {


         /*Strip whitespace*/
        while ((*buf == ' ') || (*buf == '\n')) {
            *buf++ = '\0';
        }

        /*Save the argument.*/
        *args++ = buf;

       /*Skip ober valid arguments*/
        while ((*buf != '\0') && (*buf != ' ') && (*buf != '\n')) {
            buf++;
        }

        num++;
    }

    num--;
    this_shell->args_count = num;
    *args--;
    *args = NULL;
}

/******************************************************
#   sh_identify_command
#   @desc: identifies whether built in command, blank line
#       comment, or unix command
#   @param: pointer to shell object
#   @return: void
******************************************************/
void sh_identify_command(struct Shell *this_shell)
{

    char *cd_str = "cd";
    int cd_str_len = (unsigned)strlen(cd_str);
    char *exit_str = "exit";
    int exit_str_len = (unsigned)strlen(exit_str);
    char *status_str = "status";
    int status_str_len = (unsigned)strlen(status_str);

    int command_len = (unsigned)strlen(this_shell->user_input);
    /*Make copy of user input w/out \n */
    char args[command_len - 1];
    strcpy(args, this_shell->user_input);

    /*Command was blank line*/
    if (args[0] == '\0') {
        //printf("User typed a blank line\n");
        /*Return to command line*/
        return;
    }
    else if (args[0] == '#') { /*Command was a comment*/
        //printf("User typed a comment\n");
        /*Return to command line*/
        return;
    }
    /*Command is CD*/
    else if (strncmp(args, cd_str, cd_str_len) == 0) {

        /*Send command to function*/
        sh_change_directory(this_shell);
    }
    /*Command is to exit*/
    else if (strncmp(args, exit_str, exit_str_len) == 0) {
        //printf("User wants to exit\n");
        exitShell(this_shell);

    }
    /*Command is status*/
    else if (strncmp(args, status_str, status_str_len) == 0) {

        sh_command_status(this_shell);
    }
    /*Command was unix command*/
    else {

        /*Send command to function*/
        sh_other_command(this_shell);
    }

}

/******************************************************
#   sh_command_ground
#   @desc: identifies whether unix command should be a
#       background command or foreground, and if foreground
#       whether there should be redirection
#   @param: pointer to shell object
#   @return: void
******************************************************/
void sh_command_ground(struct Shell *this_shell)
{

    char *amp_str = "&";
    int amLen = (unsigned)strlen(amp_str);

    char *rdStr = "<";
    char *wrStr = ">";

    if (this_shell->args_count == 3) {

        /*Background command*/
        if (strncmp(this_shell->arguments[2], amp_str, amLen) == 0) {
            /*Set ground to true*/
            this_shell->ground = 1;
        }
        else {
            /*Set ground to false*/
            this_shell->ground = 0;
        }
        /*Read command*/
        if (strncmp(this_shell->arguments[1], rdStr, amLen) == 0) {
            /*Set read to true*/
            this_shell->read = 1;
        }
        else {
            /*Set read to false*/
            this_shell->read = 0;
        }
        /*Write command*/
        if (strncmp(this_shell->arguments[1], wrStr, amLen) == 0) {
            /*Set write to true*/
            this_shell->write = 1;
        }
        else {
            /*Set write to false*/
            this_shell->write = 0;
        }

    }
    else {

        /*Command is foreground w no redirection*/
        this_shell->ground = 0;
        this_shell->read = 0;
        this_shell->write = 0;
    }

}

/******************************************************
#   Built-in Shell Commands
******************************************************/
/******************************************************
#   sh_change_directory
#   @desc: change directory based on user's path
#   @param: pointer to shell object
#   @return: void
******************************************************/
void sh_change_directory(struct Shell *this_shell)
{

    /*1 arg: Change to HOME*/
    if (this_shell->args_count == 1) {

        /*Check to ensure it worked*/
        if (chdir(getenv("HOME")) == -1) {
            fprintf(stdout, "cd unable to go home\n");
            fflush(stdout);
        }
    }

    /*2 args: Change to Path*/
    if (this_shell->args_count == 2) {

        printf("the path is: %s\n", this_shell->arguments[1]);

        /*Check to ensure it worked*/
        if (chdir(this_shell->arguments[1]) == -1) {
            fprintf(stdout, "cd unable to go to path: %s\n", this_shell->arguments[1]);
            fflush(stdout);
        }
    }

}

/******************************************************
#   sh_command_status
#   @desc: print the status of last command to console
#   @param: pointer to shell object
#   @return: void
******************************************************/
void sh_command_status(struct Shell *this_shell)
{

    /*Print the cstatus*/
    fprintf(stdout, " %s \n", g_last_status);
    fflush(stdout);

}
/******************************************************
#   exitShell
#   @desc: terminates bg processes and exits shell
#   @param: pointer to shell object
#   @return:
******************************************************/
void exitShell(struct Shell *this_shell) {

    /*Send kill signals to all child processes*/
    sh_kill_zombies(this_shell);

    /*Exit with 0 to stop loop*/
    exit(0);
}


/******************************************************
#   Command Execution via Processes
******************************************************/
/******************************************************
#   sh_other_command
#   @desc: determines whether command is foreground,
#       foreground with redirection, or background and
#       call appropriate function to execute command
#   @param: pointer to shell object
#   @return: void
******************************************************/
void sh_other_command(struct Shell *this_shell) {


    /*Determine type of command*/
    if (this_shell->ground == 0) {
        if (this_shell->read == 0 && this_shell->write == 0) {
            sh_reg_fg_process(this_shell);
        }
        else {
            sh_fg_process(this_shell);
        }
    }
    else {
        //printf("This is a background command\n");
        fflush(stdout);
        sh_bg_process(this_shell);
    }

}

/******************************************************
#   sh_execute_command
#   @desc: executes command stored in user's first argument
#       and prints to console if it fails
#   @param: pointer to shell object
#   @return: void
******************************************************/
void sh_execute_command(struct Shell *this_shell)
{

        /*execute command stored in first argument*/
    if (execvp(this_shell->arguments[0], this_shell->arguments)) {
        /*Command was not successful*/
        fflush(stdout);
        fprintf(stdout, "%s: no such file or directory\n", this_shell->arguments[0]);
        fflush(stdout);
        this_shell->status=1;
        /*Print to the formatted string*/
        sprintf(g_last_status, "exit value %d", this_shell->status);
    }

}

/******************************************************
#   sh_fg_process
#   @desc: starts foreground process with redirection and
#       waits for completion
#   @param: pointer to shell object
#   @return: void
******************************************************/
void sh_fg_process(struct Shell *this_shell)
{

    /*Child PID*/
    pid_t child_PID = -5;
    int wait;
    int status;
    /*File descriptors*/
    int file_desc_one;
    int file_desc_two;

    char **args = this_shell->arguments;

    /*Setup file descriptor for writing*/
    if (this_shell->write == 1) {

        /*Open */
        file_desc_one = open(this_shell->arguments[2], O_WRONLY | O_CREAT | O_TRUNC, 0664);

        /*File Open failed*/
        if (file_desc_one == -1) {


            fprintf(stdout, "simple_shell: cannot open %s for input\n", args[2]);
            fflush(stdout);
            this_shell->status=1;
            /*Print to the formatted string the failed exit value*/
            sprintf(g_last_status, "exit value %d", this_shell->status);

            /*Return to the command line*/
            return;
        }

    }

    /*Set up file descriptor for reading*/
    if (this_shell->read == 1) {

        if (args[2] == NULL) {

            file_desc_one = open("/dev/null", O_RDONLY);

        }
        else {

            file_desc_one = open(this_shell->arguments[2], O_RDONLY);

            /*File Open failed*/
            if (file_desc_one == -1) {
                fprintf(stdout, "simple_shell: cannot open %s for input\n", args[2]);
                fflush(stdout);

                this_shell->status = 1;
                /*Print to the formatted string the failed exit value*/
                sprintf(g_last_status, "exit value %d", this_shell->status);

                /*Return to the command line*/
                return;
            }
        }

    }



    /*Parent fork off child*/
    child_PID = fork();

    /*Regular command*/
    if (child_PID == 0) {

        /*In child process*/
        /*Set up redirection for write*/
        if (this_shell->write == 1) {
            /*Redirect stdout to file*/
            file_desc_two = dup2(file_desc_one, 1);
            this_shell->arguments[1] = '\0';
            this_shell->arguments[2] = '\0';
        }
        /*Set up redirection for read*/
        if (this_shell->read == 1) {
            /*Redirect stdin to file*/
            file_desc_two = dup2(file_desc_one, 0);
            this_shell->arguments[1] = '\0';
            this_shell->arguments[2] = '\0';

            /*redirection failed*/
            if(file_desc_one==-1){
                /*end child process*/
                _exit(1);
            }
        }
        /*Execute the user's command*/
        sh_execute_command(this_shell);

        /*Close 2nd file descriptor*/
        close(file_desc_two);
    }
    else if (child_PID == -1) {
        /*Fork failed*/
        fprintf(stdout, "Fork error \n");
        fflush(stdout);
        this_shell->status = 1;
        /*end process*/
        return;

    }
    else {
        /*Parent*/

        /*Close 1st file descriptor*/
        close(file_desc_one);

        if (child_PID > 0) {
            /*Wait for the child to complete*/
            do {
                wait = waitpid(child_PID, &status, 0);

            } while (!WIFEXITED(status) && !WIFSIGNALED(status));

        }

        /*Print formatted string*/
        sprintf(g_last_status, "exit value %d", status);
        //fprintf(stdout,"parent: waiting\n");
        //fprintf(stdout,"parent: child exited %d\n", status);

        fflush(stdout);
    }

}
/******************************************************
#   sh_bg_process
#   @desc: starts new background process, prints pid to
#       console, adds pid to user's PID array, and returns
#       control to the command line
#   @param: pointer to shell object
#   @return: void
******************************************************/
void sh_bg_process(struct Shell *this_shell)
{

    /*Child PID*/
    pid_t bg_child_PID=-10;

    /*Add signal handling to ignore*/
    struct sigaction action;
    action.sa_handler=SIG_IGN;
    action.sa_flags=0;
    sigfillset(&(action.sa_mask));
    sigaction(SIGINT, &action, NULL);

    /*Parent fork off child*/
    bg_child_PID=fork();

    //get rid of 3rd argument
    this_shell->arguments[2]=NULL;

    /*Regular command*/
    if (bg_child_PID == 0) {
        /*In child process*/

        /*execute command stored in first argument*/
        sh_execute_command(this_shell);
    }
    else if (bg_child_PID == -1) {
        printf("Fork failed!\n");
        fprintf(stdout, "Fork error \n");
        fflush(stdout);
        this_shell->status = 1;

    }
    else {
        /*Parent*/
        if (bg_child_PID > 0) {
            /*Print formatted string*/
            printf("background pid is %d\n", bg_child_PID);
            fflush(stdout);

            sh_catch_bg(this_shell);
            /*Keep track of PIDs in user*/
            int curNum=this_shell->bg_count;
            this_shell->bg_PIDs[curNum]=(int)bg_child_PID;
            /*increment Pid count*/
            this_shell->bg_count++;
            return;
        }
    }

}
/******************************************************
#   sh_reg_fg_process
#   @desc: starts foreground process w/out redirection and
#       waits for completion
#   @param: pointer to shell object
#   @return: void
******************************************************/
void sh_reg_fg_process(struct Shell *this_shell)
{

    /*Set flag to 0*/
    flag = 0;
    /*Set signal # to 0*/
    sig_num=0;
    /*Child PID*/
    pid_t fg_child_PID = -10;
    int wait;
    int status;
    int sign=0;

    char **args = this_shell->arguments;

    /*Catch the interrupt signal*/
    struct sigaction action;
    action.sa_handler = sh_catch_interr;
    action.sa_flags = 0;
    sigfillset(&(action.sa_mask));
    sigaction(SIGINT, &action, NULL);


     /*Fork the program*/
    fg_child_PID = fork();

    /*Regular command*/
    if (fg_child_PID == 0) {
        /*In child process*/

        /*execute command stored in first argument*/
        sh_execute_command(this_shell);
    }
    else if (fg_child_PID == -1) {
        printf("Fork failed!\n");
        fprintf(stdout, "Fork error \n");
        fflush(stdout);
        this_shell->status = 1;

    }
    else {
        /*Parent*/
        if (fg_child_PID > 0) {

            do {
                wait = waitpid(fg_child_PID, &status, 0);

            } while (!WIFEXITED(status) && !WIFSIGNALED(status));

            /*Child was terminated by a signal*/
            if(flag==1){
                sprintf(g_last_status, "terminated by signal %d", sig_num);
            }else{
                sprintf(g_last_status, "exit value %d", WEXITSTATUS(status));
            }

            /*Print statements used for debugging*/
            //printf("parent: waiting\n");
            //fflush(stdout);
            //printf("parent: child exited [%d]\n", status);
            //fflush(stdout);
        }
    }


}

/******************************************************
#   sh_catch_bg
#   @desc: catches BG processes that are done and prints
#       their status to the console
#   @param: pointer to shell object
#   @return: void
******************************************************/
void sh_catch_bg(struct Shell *this_shell)
{
    pid_t cur_PID;
    int status;
    cur_PID=waitpid(-1, &status, WNOHANG);

    int i;
    int bg_count=this_shell->bg_count;

    if(cur_PID>0){

            for(i=0; i < bg_count; i++){

                pid_t userPID=this_shell->bg_PIDs[i];

                if(cur_PID ==userPID){

                    /*Exit Status*/
                    if(WIFEXITED(status)){

                        fprintf(stdout,"background pid %d is done: exit value %d\n", cur_PID, WEXITSTATUS(status));
                        fflush(stdout);

                    }
                    /*Exit Signal*/
                    if(WIFSIGNALED(status)){
                        fprintf(stdout,"background pid %d is done: terminated by signal %d\n", cur_PID, WTERMSIG(status));
                        fflush(stdout);
                    }
                    /*Set item at that variable to 0*/
                    this_shell->bg_PIDs[i]=0;

            }

        }
    }

}

/******************************************************
#   sh_kill_zombies
#   @desc: loops thru array of BG PIDs and kills them
#   @param: pointer to shell object
#   @return: void
******************************************************/
void sh_kill_zombies(struct Shell *this_shell)
{
    int i=0;
    /*Kill all remaining processes*/
    for (i=0; i < this_shell->bg_count; i++)
    {
        /*skip PIDs set to 0*/
        if (this_shell->bg_PIDs[i]!=0){
            int zombie = this_shell->bg_PIDs[i];
            kill(zombie, 15);
        }


    }

}

/******************************************************
#   sh_catch_interr
#   @desc: catches interruption signal of child process,
#       updates signal flag and signal # global variable
#   @param: int signo
#   @return: void
******************************************************/
void sh_catch_interr(int signo)
{

    switch(signo){
        case SIGHUP:
            puts("terminated by signal 1");
            fflush(stdout);
            /*Set signal #*/
            sig_num=1;
            break;
        case SIGINT:
            puts("terminated by signal 2");
            fflush(stdout);
            sig_num=2;
            break;
        default:
            puts("terminated by other signal");
            return;
    }

    /*Set the flag to 1 to indicate child was terminated*/
    flag=1;

}
