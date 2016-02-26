#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include "parse.h"

int *fds;
int pipeLength;

typedef enum {basic=0, cd} CommandType;

void execvpCMD(char *const cmd[], int choice)
{
    switch(choice)
    {
        case 0:
        {
            execvp(cmd[0], cmd);
            perror("Error in Command");
            exit(1);
            break;
        }
        case 1:
        {
            chdir(cmd[1]);
            break;
        }
    }
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

void inputRedirection(int flag, char f[])
{
    if(flag == Tin)
    {
        int inFile = open(f,O_RDONLY);
        if( dup2(inFile, 0) == -1 )
        {
            perror("Error in dup2");
            exit(1); 
        }
    }
}

void outputRedirection(int flag, char f[])
{
    switch(flag)
    {
        case Tout:
        {
            int outFile = open(f,O_WRONLY | O_CREAT, S_IRWXU | S_IRWXG);
            if( dup2(outFile, 1) == -1 )
            {
                perror("Error in dup2");
                exit(1); 
            }
            break;
        }
        case Tapp:
        {
            int outFile = open(f,O_WRONLY | O_CREAT | O_APPEND, S_IRWXU | S_IRWXG);
            if( dup2(outFile, 1) == -1 )
            {
                perror("Error in dup2");
                exit(1); 
            }
            break;
        }
        case ToutErr:
        {
            int outFile = open(f,O_WRONLY | O_CREAT, S_IRWXU | S_IRWXG);
            if( dup2(outFile, 2) == -1 )
            {
                perror("Error in dup2");
                exit(1); 
            }
            break;
        }
        case TappErr:
        {
            int outFile = open(f,O_WRONLY | O_CREAT | O_APPEND, S_IRWXU | S_IRWXG);
            if( dup2(outFile, 2) == -1 )
            {
                perror("Error in dup2");
                exit(1); 
            }
            break;
        }
    }
}

void execute(Pipe p)
{
    if(p == NULL)	return ;
    
    Cmd temp = p->head;
    int i;
    pid_t pid;
    pipeLength = 0;
    CommandType type = basic;

    while(temp->next != NULL){
        temp = temp->next;
        pipeLength++;
    }   

    fds = (int *)malloc(sizeof(int)*pipeLength*2);
    
    for(i=0;i<pipeLength*2;i+=2)
        if (pipe(fds+i) == -1) {
            perror("Error in pipe");
            exit(1);
        }
 

    for(i=0,temp = p->head ; i<=pipeLength ; i++, temp = temp->next)
    {
        if( strcmp(temp->args[0],"cd") == 0)
        {
            type = cd;
        }
        if ( (pid = fork()) == -1) {
            perror("Error in fork");
            exit(1);
        }

        if ( pid == 0)
        {
            if ( i == 0 )                                    //  Input from file, only for first cmd in pipe
                inputRedirection(temp->in,temp->infile);

            if ( (i != 0) && (dup2(fds[2*i-2], 0) == -1) )   // redirect input from previous pipe    
            {     
                perror("Error in dup2");
                exit(1);
            }

            if( i == pipeLength)                            //  Output to file, only for last cmd in pipe
               outputRedirection(temp->out,temp->outfile);

            if ( (i != pipeLength) && dup2(fds[2*i + 1], 1) == -1)      // redirect output to next pipe
            {    
                perror("Error in dup2");
                exit(1);
            }
            closePipes();
            execvpCMD(temp->args,type);
        }   
    }

    closePipes();
    free(fds);

    for (i = 0; i <= pipeLength ; i++)
    {
        if (wait(NULL) == -1 && errno != ECHILD) {
            perror("Error in wait");
            exit(1);
        }
    } 
    execute(p->next);
}

int main(int argc, char *argv[])
{
  Pipe p;
  char *host = "varun_shell";

  while ( 1 ) {
    printf("%s%% ", host);
    p = parse();
    execute(p);
    freePipe(p);
  }
}