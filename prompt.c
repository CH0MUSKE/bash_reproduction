#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#define GREEN "\001\033[32m\002"
#define RED "\001\033[91m\002"
#define BLUE "\001\033[34m\002"
#define YELLOW "\001\033[33m\002"
#define DEFAULT "\001\033[00m\002"
#define  PROMPT_SIZE 30

typedef struct dirent dirent;


char *current_rep_name(char *current){
    struct stat st;
    DIR *dir = opendir(current);
    if(dir == NULL){
        perror("opendir");
        return NULL;
    }
    if(stat(current,&st) == -1){
        perror("stat");
        closedir(dir);
        return NULL;
    }
    ino_t i = st.st_ino;
    dev_t d = st.st_dev;

    char path[PATH_MAX];
    snprintf(path,sizeof(path),"%s/..",current);
    DIR *dir2 = opendir(path);
    if(dir2 == NULL){
        perror("opendir");
        return NULL;
    }    
    dirent *entry;
    while((entry = readdir(dir2))){
        struct stat st2;
        char path2[PATH_MAX + 256]; // just to get rid of the warnings
        snprintf(path2,sizeof(path2),"%s/%s",path,entry->d_name);
        if(stat(path2,&st2) == -1){
            perror(path2);
            continue;
        }
        if(st2.st_ino == i && st2.st_dev == d){
            char *s = entry -> d_name;
            if((strcmp(".",s) == 0) || (strcmp("..",s) == 0)){
                closedir(dir);
                closedir(dir2);
                return "/";
            }
            else{
                char *result = strdup(entry->d_name);
                closedir(dir);
                closedir(dir2);
                return result;
            }
        }
    }
    closedir(dir);
    closedir(dir2);

    return NULL;
}

char *handle_big_path(char *path, int return_val){
    int len = strlen(path);
    int n = 5;
    if (return_val != 0){
        n += log10(return_val);
    }
    char *s = malloc(sizeof(char)*(PROMPT_SIZE - n + 1));
    if(len <= PROMPT_SIZE - 2 - n){
        strcpy(s,path);
        return s;
    }
    else{
        strcpy(s,"...");
        strncpy(s+3,path + len - (PROMPT_SIZE - 3 - n),(PROMPT_SIZE - 3 - n));
        s[PROMPT_SIZE - n] = '\0';
        return s;
    }
}

char *prompt(int return_val){
    char *pwd = getcwd(NULL, MAX_INPUT);
    if (pwd == NULL){
        perror("pwd");
        return NULL;
    }
    char *trunc_pwd = handle_big_path(pwd,return_val);
    free(pwd);
    if (trunc_pwd == NULL){
        perror("trunc_pwd");
        return NULL;
    }
    char *prompt = (char *)malloc(sizeof(char)*(MAX_INPUT));
    switch(return_val){
    case 0: sprintf(prompt,"%s[0]%s%s%s$ ",GREEN, BLUE,trunc_pwd,DEFAULT); break;
    default: sprintf(prompt,"%s[%d]%s%s%s$ ",RED,return_val,BLUE,trunc_pwd,DEFAULT); break;
    
    }
    free(trunc_pwd);
    return prompt;
}
