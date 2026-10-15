#include <signal.h>
#include <stdlib.h>
#include "parser.h"
#include "prompt.h"
#include <readline/readline.h>
#include "utils.h"
#include<readline/history.h>

int main(int argc, char **argv) {
    int not_finished = 1;
    int return_val = 0;
    rl_outstream = stderr;

    struct sigaction act;
    sigset_t set;
    sigemptyset(&set);
    act.sa_handler = SIG_IGN;
    act.sa_flags = 0;
    act.sa_mask = set;
    sigaction(SIGINT, &act, NULL);
    sigaction(SIGTERM, &act, NULL);

    while (not_finished){
        rl_outstream = stderr;
        char *prmpt = prompt(return_val);
        char *buffer = readline(prmpt);
        add_history(buffer);
        
        free(prmpt);
        if (buffer == NULL){
            exit(return_val);
        }
        char **args = parse_input(buffer);
        free(buffer);
        return_val = parse(args);
        free_tab(args);
        update_return_val(return_val);
        
    }
    return 0;
}