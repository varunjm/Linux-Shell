#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include "parse.h"

int *fds;
int pipeLength;

extern char **environ;

typedef enum {BASIC=0, CD, STENV, USTENV, ECHO, PWD} CommandType;

char * split(char *str)
{
    char *name = (char *)malloc(256);
    int i=0;
    while(str[i] != '=')
    {
        if(!str[i]) return NULL;
        name[i] = str[i];
        i++;
    }
    name[i]='\0';
    return name;
}

void execvpCMD(char *const cmd[], int choice)
{
    switch(choice)
    {
        case BASIC:                 // Basic commands
        {
            execvp(cmd[0], cmd);
            perror("Error in Command");
            exit(1);
            break;
        }
        case CD:
        {
            chdir(cmd[1]);
            break;
        }
        case STENV:
        {
            int params=0, i=0;
            while(cmd[params++]);
            
            if(params == 2)
            {
                while(environ[i]) printf("%s\n",environ[i++]);
            }
            if(params == 3)
            {
                int l1=0;
                
                l1 = strlen(cmd[1]);
                char *word = (char *)malloc(l1+2);
                strcpy(word,cmd[1]);
                strcat(word,"=");
                putenv(word);
            }
            if(params == 4)
            {
                int l1=0, l2=0;
                
                l1 = strlen(cmd[1]);
                l2 = strlen(cmd[2]);

                char *word = (char *)malloc(l1+l2+2);

                strcpy(word,cmd[1]);
                strcat(word,"=");
                strcat(word,cmd[2]);
                putenv(word);
            }
            break;
        }

        case USTENV:
        {
            int params=0;
            while(cmd[params++]);
            
            if(params == 3)
            {
                int count=0, pos = -1;
                char *temp;

                while(environ[count])
                {
                    temp = split(environ[count]);
                    if(!temp) 
                    {
                        count++;
                        continue;
                    }
                    if(!strcmp(temp,cmd[1]))
                        pos = count;
                    count++;
                }
                if(pos != -1)
                {
                    if(pos==count-1)
                    {
                        free(environ[pos]);
                        environ[pos] = NULL;
                    }
                    else
                    {
                        free(environ[pos]);
                        environ[pos] = environ[count-1];
                        environ[count-1] = NULL;
                    }
                }
            }
            else
            {
                printf("Missing parameter!\n");
            }
            break;
        }
        case ECHO:
        {
            int i = 1;
            char *c;
            while(cmd[i])
            {   
                if(cmd[i][0] == '$')
                {
                    c = &cmd[i++][1];
                    c = getenv(c);
                    if(c!=NULL)
                        printf("%s ",c);
                }
                else
                    printf("%s ",cmd[i++]);
            }
            printf("\n");
            break;
        }
        case PWD:
        {
            char cwd[1024];
            getcwd(cwd, sizeof(cwd));
            printf("%s\n",cwd);
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
    CommandType type = BASIC;

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
            type = CD;
        }
        if( strcmp(temp->args[0],"setenv") == 0)
        {
            type = STENV;
        }
        if( strcmp(temp->args[0],"unsetenv") == 0)
        {
            type = USTENV;
        }
        if( strcmp(temp->args[0],"echo") == 0)
        {
            type = ECHO;
        }
        if( strcmp(temp->args[0],"pwd") == 0)
        {
            type = PWD;
        }
        if( strcmp(temp->args[0],"logout") == 0)
        {
            exit(1);
        }
        if ( (pid = fork()) == -1) {
            perror("Error in fork");
            exit(1);
        }

        if ( pid == 0)
        {
            int val = 1; 

            if(temp->out == TpipeErr)
                val = 2;

            if ( i == 0 )                                    //  Input from file, only for first cmd in pipe
                inputRedirection(temp->in,temp->infile);

            if ( (i != 0) && (dup2(fds[2*i-2], 0) == -1) )   // redirect input from previous pipe    
            {     
                perror("Error in dup2");
                exit(1);
            }

            if( i == pipeLength)                             //  Output to file, only for last cmd in pipe
               outputRedirection(temp->out,temp->outfile);

            if ( (i != pipeLength) && dup2(fds[2*i + 1], val) == -1)      // redirect output to next pipe
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
  char host[256], user[256];

  gethostname(host, sizeof(host));
 

  while ( 1 ) {
    printf("%s%% ", host);
    p = parse();
    execute(p);
    freePipe(p);
  }
}