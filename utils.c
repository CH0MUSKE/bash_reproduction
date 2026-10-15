
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>


void free_tab(char **tab){
    if(tab != NULL){
        size_t i = 0;
        while(tab[i] != NULL){
            free(tab[i]);
            i++;
        }
        free(tab);
    }
}

void free_tab_from_a_to_b(char **tab, size_t a, size_t b){  //free tab from a to b
    if (tab == NULL){
        return;
    }
    for (size_t i = a; i < b; i++){
        free(tab[i]);
    }
}

void free_tab_of_tab(char ***tab){
    if (tab != NULL){
        size_t i = 0;
        while(tab[i] != NULL){
            free_tab(tab[i]);
            i++;
        }
        free(tab);
    }
}

void print_tab(char **tab){
    if(tab != NULL){
        size_t i = 0;
        printf("[");
        while(tab[i] != NULL){
            printf(" %s,", tab[i]);
            i++;
        }
        printf("]\n");
    }
}

int len_tab(char **tab){
    if(tab != NULL){
        size_t i = 0;
        while(tab[i] != NULL){
            i++;
        }
        return i;
    }
    return 0;
}

int int_len(int n){
    int len = 1;
    n = n/10;
    while(n > 0){
        n = n/10;
        len++;
    }
    return len;
}
