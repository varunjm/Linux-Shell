#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include "parse.h"

int *fds;
int pipeLength = 3;

void execvpCMD(char *const cmd[])
{
    execvp(cmd[0], cmd);
    perror("Error in Command");
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
        perror("Error in dup2");
        exit(1);
    }
    closePipes();
    execvpCMD(cmd);
}


void execute(Pipe p)
{
    
    Cmd temp = p->head;
    int i;
    pid_t pid;


    pipeLength = 0;
    if(p == NULL) return ;


    while(temp->next != NULL){
        temp = temp->next;
        pipeLength++;
    }   

    printf("In execute\n");
   
     
    if(pipeLength == 0)
    {
        temp = p->head;

        if ( (pid = fork()) == -1) {
            perror("Error in fork");
            exit(1);
        }
        if ( pid == 0)
        {
            execvpCMD(temp->args);
        }

        if (wait(NULL) == -1 && errno != ECHILD) {
            perror("Error in wait");
            exit(1);
        }
    }
    else
    {
        fds = (int *)malloc(sizeof(int)*pipeLength*2);
        
        for(i=0;i<pipeLength*2;i+=2)
            if (pipe(fds+i) == -1) {
                perror("Error in pipe");
                exit(1);
            }
     

        for(i=0,temp = p->head ; i<=pipeLength ; i++, temp = temp->next)
        {
            if ( (pid = fork()) == -1) {
                perror("Error in fork");
                exit(1);
            }

            if ( pid == 0)
            {
                if ( (i != 0) && (dup2(fds[2*i-2], 0) == -1) ) {     // redirect input from previous pipe    
                    perror("Error in dup2");
                    exit(1);
                }
                if ( (i != pipeLength) && dup2(fds[2*i + 1], 1) == -1) {    // redirect output to next pipe
                    perror("Error in dup2");
                    exit(1);
                }
                closePipes();
                execvpCMD(temp->args);
            }   
        }

        closePipes();
        free(fds);

        if (wait(NULL) == -1 && errno != ECHILD) {
            perror("Error in wait");
            exit(1);
        }

    }
    if (wait(NULL) == -1 && errno != ECHILD) {
            perror("Error in wait");
            exit(1);
        }
    // execute(p->next);
}

int main(int argc, char *argv[])
{
  Pipe p;
  char *host = "armadillo";

  while ( 1 ) {
    printf("%s%% ", host);
    p = parse();
    execute(p);
    freePipe(p);
  }
}