#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>

int *fds;
int pipeLength = 3;

void execvpCMD(char *const cmd[])
{
    execvp(cmd[0], cmd);
    perror("execvp");
    exit(1);
}

void closePipes()
{
    int i;

    for(i=0;i<pipeLength*2;i++)
    {
        char s[10];
        sprintf(s,"close %d",fds[i]);
        if (close(fds[i]) == -1) {
            perror(s);
            exit(1);
        }
    }
}


void runCommand(int fd, int *pp, char *const cmd[])
{
    if (dup2(pp[fd], fd) == -1) {
        perror("dup2");
        exit(1);
    }
    closePipes();
    execvpCMD(cmd);
}


void execute()
{
    char *const args[][5] = {
        { "ls", 0 },
        { "wc", 0 },
        { "grep", "7", NULL },
        { "wc", 0 },
    };

    int i;
    pid_t pid;

    fds = (int *)malloc(sizeof(int)*pipeLength*2);

    // Create pipes
    
    for(i=0;i<pipeLength*2;i+=2)
        if (pipe(fds+i) == -1) {
            perror("pipe1");
            exit(1);
        }
 

    for(i=0;i<=pipeLength;i++)
    {

        if ( (pid = fork()) == -1) {
            perror("fork2");
            exit(1);
        }

        if ( pid == 0)
        {
            if ( (i != 0) && (dup2(fds[2*i-2], 0) == -1) ) {            // redirect input from pipe1    
                perror("dup2");
                exit(1);
            }
            if ( (i != pipeLength) && dup2(fds[2*i + 1], 1) == -1) {            // redirect output to pipe2
                perror("dup2");
                exit(1);
            }
            closePipes();
            execvpCMD(args[i]);
        }   
    }

    closePipes();


    if (wait(NULL) == -1 && errno != ECHILD) {
        perror("wait");
        exit(1);
    }
}

int main()
{
    execute();

    return 0;
}