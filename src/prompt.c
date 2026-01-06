#include <stdio.h>
#include <stdlib.h>

#include <editline/readline.h>
/* #include <editline/history.h> */

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

        // read a line of user input of maximum size 2048
        fgets(input, 2048, stdin);

        // echo input back to user
        printf("No you're a %s", input);

        // free retrieved input
        free(input);
    }
    return 0;
}
