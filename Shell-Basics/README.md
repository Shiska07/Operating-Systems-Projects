# Shell-Basics
This program implements basic functionality of a shell using system commands such as fork, wait and exec.
The code can be found in msh.c file and demonstrates how new processes are created and executed using the above listed system calls.
The program is capable of executing the following commands:

1. all commands stored in /usr/local/bin /usr/bin /bin and current working directory
2. cd
3. ls
4. exit/quit
5. showpids (lists PIDs corresponding to last 15 processes)
6. history (list last 15 commands executed)
7. !n (re-run the nth command in history)

The shell supports up to 10 command line parameters in addition to the command itself(this can be easily modified within the code). Parameters can be used separately or combined (eg:
-a -l -e or -ale).
