Linux Shell

Author: Varun Jayathirtha

The 'myShell.c' implements a 'Linux user shell'. It makes use of a parser provided by Vincent W. Freeh, Professor at NC State University. Its features includes:
- Execution of shell commands
- Piping of commands
- Input/Output redirection
- Implementation using system calls, the following commands : cd, setenv, unsetenv, echo, pwd, logout.
- All other commands are executed using 'execvp()' system call.


Usage:
```shell
$ make
$ ./ush
```
Once you are into the shell, you can run any supported shell commands.
