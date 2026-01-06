#include <stdio.h>
#include <stdlib.h>

#ifdef _WIN32
#include <string.h>

static char buffer[2048];

// fake readline function
char* readline(char* prompt){
    fputs(prompt, stdout);
    fgets(buffer, 2048, stdin);
    char* cpy = malloc(strlen(buffer) + 1);
    strcpy(cpy, buffer);
    cpy[strlen(cpy) - 1] = '\0';
    return cpy;
}

// fake add_history function
void add_history(char* unused){}

// otherwise include the editline header
#else
#include <editline/readline.h>
#endif

int main(int argc, char** argv){
    // print version and exit information
    puts("Praxis Version 0.0.0.0.1");
    puts("Press Ctrl+c to Exit\n");

    // in a never endind loop
    while(1){

        // output our prompt and get input
        char* input = readline("praxis> ");
        
        // add input to history
        add_history(input);

        // echo input back to user
        printf("No you're a %s\n", input);

        // free retrieved input
        free(input);
    }
    return 0;
}
