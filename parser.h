#ifndef PARSER_H
#define PARSER_H

int update_return_val(int);
char **parse_input(char *input);
int parse(char **args);
void replace_in_list(char **lst, char *pattern, char *replace);
char *replace_pattern(char *original, char *pattern, char *replace);
char ***split_string_list_on(char **, char *);
int separator_if(char **);

#endif
