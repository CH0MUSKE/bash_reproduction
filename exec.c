#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/param.h>
#include "prompt.h"
#include <stdbool.h>
#include "parser.h"
#include "utils.h"
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>



typedef struct dirent dirent;


char *old_pwd = NULL;

int extern_command(char **args){
    int pid = fork();
    if(pid==0){
        if(execvp(args[0], args) < 0){
            fprintf(stderr, "redirect_exec: %s\n", strerror(errno));
            free_tab(args);
            exit(1);
        }
    }
    else{
        int status;
        waitpid(pid, &status, 0);
        if (WIFEXITED(status)) {
            int exit_code = WEXITSTATUS(status);
            return exit_code;
        }
        if (WIFSIGNALED(status)) {
            int signal_number = WTERMSIG(status);
            return signal_number;
        }
    }
    return 0;
}

int exec_pwd(char **args){
    if (len_tab(args) > 1){
        char str[MAX_INPUT];
        strcat(str, "pwd: ");
        strcat(str, args[1]);
        strcat(str, ": invalid argument\n");
        write(STDERR_FILENO, str, strlen(str));
        return 1;
    }
    char *pwd = getcwd(NULL, MAX_INPUT);
    if(pwd == NULL){
        perror("pwd");
        return 1;
    }
    printf("%s\n",pwd);
    free(pwd);
    return 0;
}


int exec_cd(char **args){
    if (len_tab(args) > 2){
        char *str = "cd : too many arguments\n";
        write(STDERR_FILENO, str, strlen(str));
        return 1;
    }
    char *path = args[1];
    if(path == NULL){
        if(old_pwd != NULL){
            free(old_pwd);
        }
        old_pwd = getcwd(NULL, MAX_INPUT); 
        chdir(getenv("HOME"));
        return 0;
    }
    if(strcmp(path,"-") == 0){
        if(old_pwd == NULL){
            printf("bash: cd: OLDPWD not set");
            return 1;
        }
        else{
            char *tmp = strdup(old_pwd);
            free(old_pwd);
            old_pwd = getcwd(NULL, MAX_INPUT);
            chdir(tmp);
            free(tmp);
            return 0;
        }
    }
    char * tmp = getcwd(NULL, MAX_INPUT);
    if(chdir(path) == -1){
        perror("cd: No such file or directory");
        return 1;
    }
    if(old_pwd != NULL){
        free(old_pwd);
    }
    old_pwd = strdup(tmp);
    free(tmp);
    return 0;
}

int exec_ftype(char **args){
    char *ref = args[1];
    if(ref == NULL){
        printf("ftype needs a file\n");
        return 1;
    }
    struct stat st;
    if(lstat(ref,&st) == -1){
        perror("this file does not exist\n");
        return 1;
    }
    switch (st.st_mode & __S_IFMT){
        case __S_IFDIR : printf("directory\n"); break;;
        case __S_IFREG : printf("regular file\n"); break;
        case __S_IFIFO : printf("named pipe\n"); break;
        case __S_IFLNK : printf("symbolic link\n"); break;
        default: printf("other\n"); break;
    }
    return 0;
}


int exec_exit(char **args, int i){
    if (len_tab(args) > 2){
        char *str = "exit: too many arguments\n";
        write(STDERR_FILENO, str, strlen(str));
        return 1;
    }
    if (args[1] == NULL){
        free_tab(args);
        exit(i);
    }
    char c = atoi(args[1]);
    free_tab(args);
    exit(c);
}

int exec_if(char **parsed_input){
    int separator = separator_if(parsed_input);
    if(separator == -1){
        return 1;
    }
    char ***split = split_string_list_on(parsed_input, "{");   /*if [] { CMD } else { CMD } split[1] = { "CMD","}", "else", "{", "CMD", "}"}*/
    char **test = split[0];
    char **commands_else_commands = split[1];
    char ***split_else = split_string_list_on(commands_else_commands,"else");
    char **commands = split_else[0];
    char **else_commands = split_else[1];
    for (int i = 0; commands[i] != NULL; i++){
        if (commands[i+1] == NULL){
            free(commands[i]);
            commands[i] = NULL;
        }
    }
    int result = parse(test + 1);
    if(result != 0){
        if(else_commands[0] == NULL){
            free_tab_of_tab(split_else);
            free_tab_of_tab(split);
            return 0;
        }
        else{
            separator = separator_if(else_commands);
            if(separator == -1){
                free_tab_of_tab(split);
                free_tab_of_tab(split_else);
                return 0;
            }
            else{
                for (int i = 0; else_commands[i] != NULL; i++){
                    if (else_commands[i+1] == NULL){
                        free(else_commands[i]);
                        else_commands[i] = NULL;
                    }
                }
                result = parse(else_commands + 1);
                free_tab_of_tab(split_else);
                free_tab_of_tab(split);
                return result;
            }
        }
    }
    else{
        result = parse(commands);
        free_tab_of_tab(split_else);
        free_tab_of_tab(split);
        return result;
    }
}


char **for_parameters(char **args){
    char **param = calloc(5, sizeof(char*));
    char cmp[5] = {'A', 'r', 'e', 't', 'p'};
    for (int i = 0; args[i] != NULL; i++){
        for (int k = 0; k < 5; k++){
            if (args[i][0] == '-' && args[i][1] == cmp[k]){
                if (args[i+1] == NULL || (args[i+1] != NULL && args[i+1][0] == '-')){
                    param[k] = "";
                }
                else{
                    param[k] = args[i+1];
                }
            }
        }
    }
    return param;
}

int exec_directory(char **args, char **commands, char *dir_path, bool show_hidden, bool recursive, char* extension, char type){
    struct dirent *entry;
    DIR *dir = opendir(dir_path);

    if (dir == NULL) {
        perror("opendir");
        return 1;
    }
    int return_value = 0;
    
    while (((entry = readdir(dir)) != NULL)) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0){ //ignore . et ..
            continue;
        }
        if (entry->d_name[0] == '.' && !show_hidden) { //paramètre -A
            continue;
        }
        char *temp = calloc(MAX_INPUT, sizeof(char));
        strcpy(temp, dir_path);
        strcat(temp, "/");
        strcat(temp, entry->d_name);
        char *trunc = calloc(sizeof(char), strlen(temp) - strlen(extension) + 1);
        strncpy(trunc,temp, strlen(temp) - strlen(extension));
        

        if (recursive){
            struct stat st;
            if (stat(temp, &st) == 0){
                if (S_ISDIR(st.st_mode)){
                    exec_directory(args, commands, temp, show_hidden, recursive, extension, type);
                }
            }
        }
        free(temp);  
         if (strcmp(entry->d_name + strlen(entry->d_name) - strlen(extension), extension) != 0 || strlen(entry->d_name) == strlen(extension)){
            free(trunc);
            continue;
        }

        if (type == DT_UNKNOWN || (type != DT_UNKNOWN  && entry->d_type == type)){
            char val = args[1][0];
            char pattern[3];
            pattern[0] = '$';
            pattern[1] = val;
            pattern[2] = '\0';

            char **recall_cmd = calloc(sizeof(char), MAX_INPUT);
            for (int i = 0; commands[i] != NULL; i++){
                recall_cmd[i] = strdup(commands[i]);
            }
            replace_in_list(recall_cmd, pattern, trunc);
            int ret = parse(recall_cmd);
            if (ret > return_value){
                return_value = ret;
            }
            free_tab(recall_cmd);
        }


         
        free(trunc);
    }
    closedir(dir);
    return return_value;
}

int exec_for(char **args, char** commands){
    // int max = INT_MAX; //TODO -p MAX
    for (int i = 0; commands[i] != NULL; i++){
        if (commands[i+1] == NULL){
            free(commands[i]);
            commands[i] = NULL;
        }
    }
    char **param_values = for_parameters(args+4);

    char *extension = ""; // option -e EXT
    if (param_values[2] != NULL){
        int len = strlen(param_values[2]);
        extension = malloc(len + 2);
        strcpy(extension, ".");
        strcat(extension, param_values[2]);
    }

    bool show_hidden = false; // option -A
    if (param_values[0] != NULL && strcmp(param_values[0], "") == 0){
        show_hidden = true;
    }
    bool recursive = false; // option -r
    if (param_values[1] != NULL && strcmp(param_values[1], "") == 0){
        recursive = true;
    }

    char type = DT_UNKNOWN;
    if (param_values[3] != NULL){
        switch(param_values[3][0]){
            case 'f' : type = DT_REG; break;
            case 'd' : type = DT_DIR; break;
            case 'l' : type = DT_LNK; break;
            case 'p' : type = DT_FIFO; break;
            default : break;
        }
    }
    int ret = exec_directory(args, commands, args[3], show_hidden, recursive, extension, type);
    if (param_values[2] != NULL){
        free(extension);
    }
    free(param_values);
    return ret;

}

int get_umask(pid_t pid){
    int pidLen = int_len(pid);
    char *pidstr = calloc(pidLen + 1, sizeof(char));
    sprintf(pidstr, "%d", pid);
    char *path = calloc(14 + pidLen, sizeof(char));
    strcpy(path, "/proc/");
    strcat(path, pidstr);
    free(pidstr);
    strcat(path, "/status");
    int fd = open(path, O_RDONLY);
    free(path);
    while (1){
        char *line = calloc(sizeof(char), MAX_INPUT);
        if (read(fd, line, MAX_INPUT) == 0){
            free(line);
            break;
        }
        if (strstr(line, "Umask") != NULL){
            char *umask = calloc(sizeof(char), 5);
            strncpy(umask, line + 7, 4);
            int res = strtol(umask, NULL, 8);
            free(umask);
            free(line);
            return res;
        }
        free(line);
    }
    return 0;
}

int redirection(char **args, char** commands, int sepa_num){
    pid_t fshid = getppid();
    int umask = get_umask(fshid);
    int default_perm = 0666 & ~umask;
    int pipe_fd[2];
    pipe(pipe_fd); 
    pid_t pid = fork();

    if(pid == 0){
        close(pipe_fd[0]);
        char *file = commands[0];
        int fd;
        switch(sepa_num){
            case 2: fd = open(file,O_RDONLY); dup2(fd,0);break;
            case 3: fd = open(file, O_CREAT | O_WRONLY | O_TRUNC | O_EXCL, default_perm); dup2(fd,1); break;
            case 4: fd = open(file, O_CREAT | O_WRONLY | O_TRUNC, default_perm); dup2(fd,1); break;
            case 5: fd = open(file, O_CREAT | O_WRONLY | O_APPEND, default_perm); dup2(fd,1); break;
            case 6: fd = open(file, O_CREAT | O_WRONLY | O_TRUNC | O_EXCL, default_perm); dup2(fd,2); break;
            case 7: fd = open(file, O_CREAT | O_WRONLY | O_TRUNC, default_perm); dup2(fd,2); break;
            case 8: fd = open(file, O_CREAT | O_WRONLY | O_APPEND, default_perm); dup2(fd,2); break;
            default: perror("sepa_num incorrect");
        }
        if(fd < 0){
            close(pipe_fd[1]);
            perror("No such file or directory");
            exit(1);
        }
        if(strcmp(args[0],"exit") == 0){
            write(pipe_fd[1],"1",1);
            close(fd);
            close(pipe_fd[1]);
            exit(0);
        }
        close(fd);
        int result = parse(args);
        close(pipe_fd[1]);
        exit(result);
    }
    else{
        close(pipe_fd[1]);
        int status;
        waitpid(pid, &status, 0);
        if (WIFEXITED(status)) {
            char tmp[1];
            if(read(pipe_fd[0],tmp,1) == 1){
                int result = parse(args);
                close(pipe_fd[0]);
                return result;
            }
            else{
                close(pipe_fd[0]);
                return WEXITSTATUS(status); 
            }
        }
        close(pipe_fd[0]);
        return 0;
    }
}

int semicolon(char **parsed_input){
    char ***split = split_string_list_on(parsed_input, ";");
    parse(split[0]);
    int result2 = parse(split[1]);
    free_tab_of_tab(split);
    return result2;
    
}

int pipeline(char ** parsed_input){
    char ***split = split_string_list_on(parsed_input, "|");
    int pipe_fd[2];
    pipe(pipe_fd);
    pid_t pid = fork();

    if(pid == 0){
        close(pipe_fd[0]);
        dup2(pipe_fd[1],1);
        int result = parse(split[0]);
        free_tab_of_tab(split);
        close(pipe_fd[1]);
        exit(result);
    }
    else{
        close(pipe_fd[1]);
        pid_t pid2 = fork();
        if(pid2 == 0){
            dup2(pipe_fd[0],0);
            int result = parse(split[1]);
            free_tab_of_tab(split);
            close(pipe_fd[0]);
            exit(result);
        }
        else{
            free_tab_of_tab(split);
            close(pipe_fd[0]);
            int status;
            waitpid(pid2, &status, 0);
            if (WIFEXITED(status)) {
                return WEXITSTATUS(status); 
            }
            return 0;
        }
    }
}
