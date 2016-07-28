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
char lastStatus[MAX_LEN];
volatile sig_atomic_t flag; /*0=not terminated 1=terminated*/
volatile sig_atomic_t sigNum; /*store signal term #*/

/*Data structure to house user's commands*/
typedef struct user user;
struct user {
    char *userInput; /*string to capture input*/
    int numArgs; /*number of arguments in command*/
    char **arguments; /*array of strings to contain each arg*/
    int ground; /*0 = fore, 1=back*/
    int status; 
    int read; /*0=false, 1=true*/
    int write; /*0=false, 1=true*/
    int bgPIDs[MAX_ARGS]; /*Array containing bg PIDs*/
    int numBG; /*Count of bg PIDs*/
};

/*User object prototypes*/
user* createUser();
void freeUser(struct user *thisUser);

/*Command line helper functions*/
void getCommands(struct user *thisUser);
void parseArgs(struct user *thisUser);
void identifyCommand(struct user *thisUser);
void typeOfCommand(struct user *thisUser);

/*Built-in Shell Commands*/
void changeDirectory(struct user *thisUser);
void statusCommand(struct user *thisUser);
void exitShell(struct user *thisUser);

/*Command execution via processes*/
void otherCommand(struct user *thisUser);
void executeCommand(struct user *thisUser);
void foreProcess(struct user *thisUser);
void regforeProcess(struct user *thisUser);
void backProcess(struct user *thisUser);

/*Program helper functions*/
void catchInt(int signo);
void killZombies(struct user *thisUser);
void catchDoneBG(struct user *thisUser);


int main()
{

    struct user *curUser = createUser();

    /*Catch signals to ignore CTRL-C*/
    struct sigaction ignoreC;
    ignoreC.sa_handler = SIG_IGN;
    ignoreC.sa_flags = 0;
    sigfillset(&(ignoreC.sa_mask));
    sigaction(SIGINT, &ignoreC, NULL);

    while (1) {
        catchDoneBG(curUser);
        getCommands(curUser);
        parseArgs(curUser);
        typeOfCommand(curUser);
        identifyCommand(curUser);
    }


    /*Clean up*/
    freeUser(curUser);

    exit(0);

}

/******************************************************
#   User Data Structure Functions
******************************************************/
/******************************************************
#   createUser
#   @desc: allocate memory for user data structure
#       and return pointer to user object
#   @param: n/a
#   @return: struct user *theUser
******************************************************/
user* createUser()
{

    /*Allocate memory for user and args*/
    user *theUser = malloc(sizeof(user));
    theUser->userInput = malloc(MAX_LEN* sizeof(char*));
    theUser->arguments = (char**)malloc(MAX_ARGS*sizeof(char*));
    theUser->numBG=0;

    int i = 0;

    /*Allocate memory for array of strings*/
    for (i = 0; i < MAX_ARGS; i++) 
    {
        theUser->arguments[i] = (char*)malloc(20 * sizeof(char));
    }

    return theUser;

}
/******************************************************
#   freeUser
#   @desc: Deallocate the memory used up by user
#   @param: pointer to user object
#   @return: void
******************************************************/
void freeUser(struct user *thisUser)
{
    free(thisUser);
}

/******************************************************
#   Command Line helper functions
******************************************************/
/******************************************************
#   getCommands
#   @desc:
#   @param: pointer to user object
#   @return:
******************************************************/
void getCommands(struct user *thisUser)
{
    /*Prompt user for commands*/
    fprintf(stdout, ": ");
    fflush(stdout);
    fgets(thisUser->userInput, 2048, stdin);

}

/******************************************************
#   parseArgs
#   @desc: parse the command stored in  user's userInput into
#       individual arguments stored in user's arguments
#   @param: pointer to user object
#   @return: void
******************************************************/
void parseArgs(struct user *thisUser)
{

    /*point to the user input*/
    char *buf = thisUser->userInput;
    char **args = thisUser->arguments;
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
    thisUser->numArgs = num;
    *args--;
    *args = NULL;
}

/******************************************************
#   identifyCommand
#   @desc: identify whether built in command, blank line
#       comment, or unix command
#   @param: pointer to user object
#   @return: void
******************************************************/
void identifyCommand(struct user *thisUser)
{

    char *cdStr = "cd";
    int cdLen = (unsigned)strlen(cdStr);
    char *exStr = "exit";
    int exLen = (unsigned)strlen(exStr);
    char *stStr = "status";
    int stLen = (unsigned)strlen(stStr);

    int cmLen = (unsigned)strlen(thisUser->userInput);
    /*Make copy of user input w/out \n */
    char args[cmLen - 1];
    strcpy(args, thisUser->userInput);

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
    else if (strncmp(args, cdStr, cdLen) == 0) {

        /*Send command to function*/
        changeDirectory(thisUser);
    }
    /*Command is to exit*/
    else if (strncmp(args, exStr, exLen) == 0) {
        //printf("User wants to exit\n");
        exitShell(thisUser);

    }
    /*Command is status*/
    else if (strncmp(args, stStr, stLen) == 0) {

        statusCommand(thisUser);
    }
    /*Command was unix command*/
    else {

        /*Send command to function*/
        otherCommand(thisUser);
    }

}

/******************************************************
#   typeOfCommand
#   @desc: identify whether unix command should be a
#       background command or foreground, and if foreground
#       if there should be redirection
#   @param: pointer to user object
#   @return: void
******************************************************/
void typeOfCommand(struct user *thisUser)
{

    char *ampStr = "&";
    int amLen = (unsigned)strlen(ampStr);

    char *rdStr = "<";
    char *wrStr = ">";

    if (thisUser->numArgs == 3) {

        /*Background command*/
        if (strncmp(thisUser->arguments[2], ampStr, amLen) == 0) {
            /*Set ground to true*/
            thisUser->ground = 1;
        }
        else {
            /*Set ground to false*/
            thisUser->ground = 0;
        }
        /*Read command*/
        if (strncmp(thisUser->arguments[1], rdStr, amLen) == 0) {
            /*Set read to true*/
            thisUser->read = 1;
        }
        else {
            /*Set read to false*/
            thisUser->read = 0;
        }
        /*Write command*/
        if (strncmp(thisUser->arguments[1], wrStr, amLen) == 0) {
            /*Set write to true*/
            thisUser->write = 1;
        }
        else {
            /*Set write to false*/
            thisUser->write = 0;
        }

    }
    else {

        /*Command is foreground w no redirection*/
        thisUser->ground = 0;
        thisUser->read = 0;
        thisUser->write = 0;
    }

}

/******************************************************
#   Built-in Shell Commands
******************************************************/
/******************************************************
#   changeDirectory
#   @desc: change directory based on user's path
#   @param: pointer to user object
#   @return: void
******************************************************/
void changeDirectory(struct user *thisUser)
{

    /*1 arg: Change to HOME*/
    if (thisUser->numArgs == 1) {

        /*Check to ensure it worked*/
        if (chdir(getenv("HOME")) == -1) {
            fprintf(stdout, "cd unable to go home\n");
            fflush(stdout);
        }
    }

    /*2 args: Change to Path*/
    if (thisUser->numArgs == 2) {

        printf("the path is: %s\n", thisUser->arguments[1]);

        /*Check to ensure it worked*/
        if (chdir(thisUser->arguments[1]) == -1) {
            fprintf(stdout, "cd unable to go to path: %s\n", thisUser->arguments[1]);
            fflush(stdout);
        }
    }

}

/******************************************************
#   statusCommand
#   @desc: print the status of last command to console
#   @param: pointer to user object
#   @return: void
******************************************************/
void statusCommand(struct user *thisUser)
{

    /*Print the cstatus*/
    fprintf(stdout, " %s \n", lastStatus);
    fflush(stdout);

}
/******************************************************
#   exitShell
#   @desc: terminates bg processes and exits shell
#   @param: pointer to user object
#   @return:
******************************************************/
void exitShell(struct user *thisUser) {

    /*Send kill signals to all child processes*/
    killZombies(thisUser);

    /*Exit with 0 to stop loop*/
    exit(0);
}


/******************************************************
#   Command Execution via Processes
******************************************************/
/******************************************************
#   otherCommand
#   @desc: determine whether command is foreground,
#       foreground with redirection, or background and
#       call appropriate function to execute command
#   @param: pointer to user object
#   @return: void
******************************************************/
void otherCommand(struct user *thisUser) {


    /*Determine type of command*/
    if (thisUser->ground == 0) {
        if (thisUser->read == 0 && thisUser->write == 0) {
            regforeProcess(thisUser);
        }
        else {
            foreProcess(thisUser);
        }
    }
    else {
        //printf("This is a background command\n");
        fflush(stdout);
        backProcess(thisUser);
    }

}

/******************************************************
#   executeCommand
#   @desc: execute command stored in user's first argument
#       and print to console if it fails
#   @param: pointer to user object
#   @return: void
******************************************************/
void executeCommand(struct user *thisUser)
{

        /*execute command stored in first argument*/
    if (execvp(thisUser->arguments[0], thisUser->arguments)) {
        /*Command was not successful*/
        fflush(stdout);
        fprintf(stdout, "%s: no such file or directory\n", thisUser->arguments[0]);
        fflush(stdout);
        thisUser->status=1;
        /*Print to the formatted string*/
        sprintf(lastStatus, "exit value %d", thisUser->status);
    }

}

/******************************************************
#   foreProcess
#   @desc: start foreground process with redirection and
#       wait for completion
#   @param: pointer to user object
#   @return: void
******************************************************/
void foreProcess(struct user *thisUser)
{

    /*Child PID*/
    pid_t childPID = -5;
    int wait;
    int status;
    /*File descriptors*/
    int fdOne;
    int fdTwo;

    char **args = thisUser->arguments;

    /*Setup file descriptor for writing*/
    if (thisUser->write == 1) {

        /*Open */
        fdOne = open(thisUser->arguments[2], O_WRONLY | O_CREAT | O_TRUNC, 0664);

        /*File Open failed*/
        if (fdOne == -1) {


            fprintf(stdout, "smallsh: cannot open %s for input\n", args[2]);
            fflush(stdout);
            thisUser->status=1;
            /*Print to the formatted string the failed exit value*/
            sprintf(lastStatus, "exit value %d", thisUser->status);

            /*Return to the command line*/
            return;
        }

    }

    /*Set up file descriptor for reading*/
    if (thisUser->read == 1) {

        if (args[2] == NULL) {

            fdOne = open("/dev/null", O_RDONLY);

        }
        else {

            fdOne = open(thisUser->arguments[2], O_RDONLY);

            /*File Open failed*/
            if (fdOne == -1) {
                fprintf(stdout, "smallsh: cannot open %s for input\n", args[2]);
                fflush(stdout);

                thisUser->status = 1;
                /*Print to the formatted string the failed exit value*/
                sprintf(lastStatus, "exit value %d", thisUser->status);

                /*Return to the command line*/
                return;
            }
        }

    }



    /*Parent fork off child*/
    childPID = fork();

    /*Regular command*/
    if (childPID == 0) {

        /*In child process*/
        /*Set up redirection for write*/
        if (thisUser->write == 1) {
            /*Redirect stdout to file*/
            fdTwo = dup2(fdOne, 1);
            thisUser->arguments[1] = '\0';
            thisUser->arguments[2] = '\0';
        }
        /*Set up redirection for read*/
        if (thisUser->read == 1) {
            /*Redirect stdin to file*/
            fdTwo = dup2(fdOne, 0);
            thisUser->arguments[1] = '\0';
            thisUser->arguments[2] = '\0';

            /*redirection failed*/
            if(fdOne==-1){
                /*end child process*/
                _exit(1);
            }
        }
        /*Execute the user's command*/
        executeCommand(thisUser);

        /*Close 2nd file descriptor*/
        close(fdTwo);
    }
    else if (childPID == -1) {
        /*Fork failed*/
        fprintf(stdout, "Fork error \n");
        fflush(stdout);
        thisUser->status = 1;
        /*end process*/
        return;

    }
    else {
        /*Parent*/

        /*Close 1st file descriptor*/
        close(fdOne);

        if (childPID > 0) {
            /*Wait for the child to complete*/
            do {
                wait = waitpid(childPID, &status, 0);

            } while (!WIFEXITED(status) && !WIFSIGNALED(status));

        }

        /*Print formatted string*/
        sprintf(lastStatus, "exit value %d", status);
        //fprintf(stdout,"parent: waiting\n");
        //fprintf(stdout,"parent: child exited %d\n", status);

        fflush(stdout);
    }

}
/******************************************************
#   backProcess
#   @desc: start new background process, print pid to
#       console, add pid to user's PID array, and return
#       control to the command line
#   @param: pointer to user object
#   @return: void
******************************************************/
void backProcess(struct user *thisUser)
{

    /*Child PID*/
    pid_t backChildPID=-10;

    /*Add signal handling to ignore*/
    struct sigaction action;
    action.sa_handler=SIG_IGN;
    action.sa_flags=0;
    sigfillset(&(action.sa_mask));
    sigaction(SIGINT, &action, NULL);

    /*Parent fork off child*/
    backChildPID=fork();

    //get rid of 3rd argument
    thisUser->arguments[2]=NULL;

    /*Regular command*/
    if (backChildPID == 0) {
        /*In child process*/

        /*execute command stored in first argument*/
        executeCommand(thisUser);
    }
    else if (backChildPID == -1) {
        printf("Fork failed!\n");
        fprintf(stdout, "Fork error \n");
        fflush(stdout);
        thisUser->status = 1;

    }
    else {
        /*Parent*/
        if (backChildPID > 0) {
            /*Print formatted string*/
            printf("background pid is %d\n", backChildPID);
            fflush(stdout);

            catchDoneBG(thisUser);
            /*Keep track of PIDs in user*/
            int curNum=thisUser->numBG;
            thisUser->bgPIDs[curNum]=(int)backChildPID;
            /*increment Pid count*/
            thisUser->numBG++;
            return;
        }
    }

}
/******************************************************
#   regforeProcess
#   @desc: start foreground process w/out redirection and
#       wait for completion
#   @param: pointer to user object
#   @return: void
******************************************************/
void regforeProcess(struct user *thisUser)
{

    /*Set flag to 0*/
    flag = 0;
    /*Set signal # to 0*/
    sigNum=0;
    /*Child PID*/
    pid_t forechildPID = -10;
    int wait;
    int status;
    int sign=0;

    char **args = thisUser->arguments;

    /*Catch the interrupt signal*/
    struct sigaction action;
    action.sa_handler = catchInt;
    action.sa_flags = 0;
    sigfillset(&(action.sa_mask));
    sigaction(SIGINT, &action, NULL);


     /*Fork the program*/
    forechildPID = fork();

    /*Regular command*/
    if (forechildPID == 0) {
        /*In child process*/

        /*execute command stored in first argument*/
        executeCommand(thisUser);
    }
    else if (forechildPID == -1) {
        printf("Fork failed!\n");
        fprintf(stdout, "Fork error \n");
        fflush(stdout);
        thisUser->status = 1;

    }
    else {
        /*Parent*/
        if (forechildPID > 0) {

            do {
                wait = waitpid(forechildPID, &status, 0);

            } while (!WIFEXITED(status) && !WIFSIGNALED(status));

            /*Child was terminated by a signal*/
            if(flag==1){
                sprintf(lastStatus, "terminated by signal %d", sigNum);
            }else{
                sprintf(lastStatus, "exit value %d", WEXITSTATUS(status));
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
#   catchDoneBG
#   @desc: catch BG processes that are done and print
#       their status to the console
#   @param: pointer to user object
#   @return: void
******************************************************/
void catchDoneBG(struct user *thisUser)
{
    pid_t curPID;
    int status;
    curPID=waitpid(-1, &status, WNOHANG);

    int i;
    int bgCount=thisUser->numBG;

    if(curPID>0){

            for(i=0; i < bgCount; i++){

                pid_t userPID=thisUser->bgPIDs[i];

                if(curPID ==userPID){

                    /*Exit Status*/
                    if(WIFEXITED(status)){

                        fprintf(stdout,"background pid %d is done: exit value %d\n", curPID, WEXITSTATUS(status));
                        fflush(stdout);

                    }
                    /*Exit Signal*/
                    if(WIFSIGNALED(status)){
                        fprintf(stdout,"background pid %d is done: terminated by signal %d\n", curPID, WTERMSIG(status));
                        fflush(stdout);
                    }
                    /*Set item at that variable to 0*/
                    thisUser->bgPIDs[i]=0;

            }

        }
    }

}

/******************************************************
#   killZombies
#   @desc: loop thru array of BG PIDs and kill them
#   @param: pointer to user object
#   @return: void
******************************************************/
void killZombies(struct user *thisUser)
{
    int i=0;
    /*Kill all remaining processes*/
    for (i=0; i < thisUser->numBG; i++)
    {
        /*skip PIDs set to 0*/
        if (thisUser->bgPIDs[i]!=0){
            int zombie = thisUser->bgPIDs[i];
            kill(zombie, 15);
        }


    }

}

/******************************************************
#   catchInt
#   @desc: catch interruption signal of child process,
#       update signal flag and signal # global variable
#   @param: int signo
#   @return: void
******************************************************/
void catchInt(int signo)
{

    switch(signo){
        case SIGHUP:
            puts("terminated by signal 1");
            fflush(stdout);
            /*Set signal #*/
            sigNum=1;
            break;
        case SIGINT:
            puts("terminated by signal 2");
            fflush(stdout);
            sigNum=2;
            break;
        default:
            puts("terminated by other signal");
            return;
    }

    /*Set the flag to 1 to indicate child was terminated*/
    flag=1;

}
