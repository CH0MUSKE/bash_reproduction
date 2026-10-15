#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "exec.h"
#include <sys/param.h>
#include "utils.h"
#include <unistd.h>
#include <fcntl.h>

static int return_val = 0;
char *sepa[12] = {";", "|", "<", ">", ">|", ">>", "2>", "2>|", "2>>", "{", "}", "$"}; //par ordre de priorité

void update_return_val(int i){
    return_val = i;
}

int iscmd(char* str){
    if (str == NULL || strlen(str) == 0){
        return INT_MAX;
    }
    char *cmd[6] = {"pwd", "cd", "exit", "ftype", "if", "for"}; 
    for (int i = 0; i < 6; i++){
        if (strcmp(str, cmd[i]) == 0){
            return i;
        }
    }
    return INT_MAX;
}

int issepa(char* str){
    if (str == NULL || strlen(str) == 0){
        return INT_MAX;
    }
    for (int i = 0; i < 12; i++){
        if(strcmp(str, sepa[i]) == 0){
            return i;
        } 
    }
    return INT_MAX;
}

char * replace_pattern(char *original, char *pattern, char *replace) {
    // Calculate lengths
    size_t original_len = strlen(original);
    size_t pattern_len = strlen(pattern);
    size_t replacement_len = strlen(replace);

    // First, count occurrences of the pattern
    size_t count = 0;
    char *p = strstr(original, pattern);
    while (p != NULL){
        p = strstr(p + pattern_len, pattern);
        count++;
    }

    // Calculate the length of the new string
    size_t new_len = original_len + (replacement_len - pattern_len) * count;
    char *new_string = malloc(new_len + 1); // +1 for the null terminator
    if (!new_string) {
        fprintf(stderr, "Memory allocation failed\n");
        return NULL;
    }

    // Replace occurrences of the pattern
    char *current = original;
    char *new_current = new_string;
    while ((current = strstr(current, pattern)) != NULL) {
        // Copy everything before the pattern
        size_t prefix_len = current - original;
        strncpy(new_current, original, prefix_len);
        new_current += prefix_len;

        // Copy the replacement
        strcpy(new_current, replace);
        new_current += replacement_len;

        // Move past the pattern
        current += pattern_len;
        original = current;
    }

    // Copy the remaining part of the original string
    strcpy(new_current, original);

    return new_string;
}



void replace_in_list(char **lst, char *pattern, char *replace){ //utiliser pour les $
    for (int i = 0; lst[i] != NULL; i++){
        char *temp = replace_pattern(lst[i], pattern, replace);
        free(lst[i]);
        lst[i] = temp;
        if (lst[i] == NULL){
            perror("replace_in_list");
            exit(1);
        }
    }
}

char **parse_input(char *input){
    char *input_copy = (char *)calloc(strlen(input)+1, sizeof(char));


    strcpy(input_copy,input);   //strtok modifie le string

    char **arg = (char **)calloc(MAX_INPUT, sizeof(char *));
    if (arg == NULL){
        perror("Failed to allocate memory");
        free(input_copy);
        return NULL;
    }
    
    input_copy[strlen(input)] = '\0';
    char *token = strtok(input_copy, " ");

    int word_count = 0;
    while (token != NULL){
        char *word = strdup(token);
        arg[word_count++] = word;
        token = strtok(NULL," ");
    }
    arg[word_count] = NULL;
    free(input_copy);
    return arg;
}


char ***split_string_list_on(char **parsed_input, char *delimiter){
    char ***tab = calloc(3,sizeof(char**));
    if (tab == NULL){
        perror("Failed to allocate memory for tab");
        return NULL;
    }
    tab[0] = calloc(MAX_INPUT, sizeof(char *)); // si c'est vrai c'est très très grave
    if (tab[0] == NULL){
        perror("Failed to allocate memory for lst");
        free(tab);
        return NULL;
    }
    tab[1] = calloc(MAX_INPUT, sizeof(char *));
    if (tab[1] == NULL){
        perror("Failed to allocate memory for lst");
        free(tab[0]);
        free(tab);
        return NULL;
    }

    int i;
    for (i = 0; parsed_input[i] != NULL; i++){
        if (strcmp(parsed_input[i], delimiter) == 0){
            break;
        }
        tab[0][i] = strdup(parsed_input[i]);
    }
    free_tab_from_a_to_b(tab[0], i, MAX_INPUT);
    tab[0][i] = NULL;
    i++;
    int k;
    for (k = 0; parsed_input[i+k] != NULL; k++){
        tab[1][k] = strdup(parsed_input[i+k]);
    }
    free_tab_from_a_to_b(tab[1], k, MAX_INPUT);
    tab[1][k] = NULL;
    return tab;
}

// {";", "|", "<", ">", ">|", ">>", "2>", "2>|", "2>>", "{", "}", "$"}
int *separator_place(char **parsed_input){ //renvoie l'index dans la liste ou INT_MAX si il n'y a pas de séparateur
    int *lst = calloc(sizeof(int), 2);
    int separator = INT_MAX;
    int index = INT_MAX;
    int parantheses = 0;
    for(int i = 0; parsed_input[i] != NULL; i++){
        int sepa = issepa(parsed_input[i]);
        if (sepa == 9){
            parantheses++;
        }
        else if (sepa == 10){
            parantheses--;
        }
        if (sepa == 9 || sepa == 10 || parantheses == 0){
            if (sepa < separator || (sepa > 1 && sepa < 9 && separator > 1)){
                separator = sepa;
                index = i;
            }
        }
    }
    lst[0] = separator;
    lst[1] = index;
    return lst;
}

int separator_if(char **parsed_input){ 
    int separator = INT_MAX;
    int parantheses = 0;
    for(int i = 0; parsed_input[i] != NULL; i++){
        int sepa = issepa(parsed_input[i]);
        if(sepa == 9 ){
            separator = sepa;
            parantheses ++;
        }
        if(sepa == 10){
            parantheses --;
            if(parantheses <= 0){
                break;
            }
        }
    }
    if(parantheses < 0 || separator == INT_MAX){
        return -1;
    }
    return separator;
}


int command_place(char **parsed_input){
    return MIN(iscmd(parsed_input[0]), INT_MAX);
}

int parse(char **parsed_input){
    if (parsed_input == NULL){
        perror("parse_input failed");
        return 1;
    }
    if (parsed_input[0] == NULL){
        return return_val;
    }

    if (strcmp(parsed_input[0], "") == 0){
        return return_val;
    }

    int *lst = separator_place(parsed_input); 
    int sepa_num = lst[0];
    free(lst);
    if (sepa_num == 10 || sepa_num == 11){  //impossible d'avoir } et $ sans { dans la chaine
        perror("wrong separator place number");
    }

    int cmd_num = command_place(parsed_input);
    char ***split = NULL;
    int result = 0;

    if(sepa_num != INT_MAX){
        split = split_string_list_on(parsed_input, sepa[sepa_num]);
    }

    
    if (sepa_num != INT_MAX && cmd_num != 4 && cmd_num != 5){ //différent si sepa_num est {, }, $ (9, 10, 11)  (11 utiliser replace_in_list)
        switch(sepa_num){
            case 0: result = semicolon(parsed_input); break;
            case 1: result = pipeline(parsed_input); break;
            case 2: case 3: case 4: case 5: case 6: case 7: case 8: result = redirection(split[0],split[1],sepa_num); break;
            default: perror("sepa_num incorrect");
        }
        free_tab_of_tab(split);
        return result;
    }
    
    if (sepa_num != INT_MAX){
        switch(cmd_num){
            case 4: free_tab_of_tab(split); return exec_if(parsed_input);
            case 5: result = exec_for(split[0], split[1]); free_tab_of_tab(split); return result;
            default: break;
        }
    }

    if (cmd_num == INT_MAX){
        return extern_command(parsed_input);
    }
    

    
    //donc commande interne
    switch(cmd_num){
        case 0: result = exec_pwd(parsed_input); break;
        case 1: result = exec_cd(parsed_input); break;
        case 2: result = exec_exit(parsed_input, return_val); break;
        case 3: result = exec_ftype(parsed_input); break;
        case 4: result = exec_if(parsed_input); break;
        case 5: result = exec_for(split[0], split[1]); break;
        default : break;
    }
    free_tab_of_tab(split);




    return result;
}
