#ifndef EXEC_H
#define EXEC_H

int extern_command(char **);
int exec_cd(char **);
int exec_if(char **);
int exec_ftype(char **);
int exec_for(char **, char**);
int exec_pwd(char **);
int exec_exit(char **, int);
int redirection(char **, char**, int );
int semicolon(char **);
int pipeline(char **);
#endif
