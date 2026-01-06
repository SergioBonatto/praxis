#include <stdio.h>
#include <stdlib.h>
#include "mpc.h"

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



long eval_op(long x, char* op, long y){
    if (strcmp(op, "+") == 0) {return x + y;}
    if (strcmp(op, "-") == 0) {return x - y;}
    if (strcmp(op, "*") == 0) {return x * y;}
    if (strcmp(op, "/") == 0) {return x / y;}
    return 0;
}

long eval(mpc_ast_t* t){
    
    // if tagged as number, return it directly
    if (strstr(t->tag, "number"))
        return atoi(t->contents);

    // the operator is always second child
    char* op = t->children[1]->contents;

    // we store the third child in `x`
    long x = eval(t->children[2]);

    // iterate the remainind children and combining
    int i = 3;
    while (strstr(t->children[i]->tag, "expr")){
        x = eval_op(x, op, eval(t->children[i]));
        i++;
    }
    return x;
}

int main(int argc, char** argv){

    // create some Parsers
    mpc_parser_t* Number    = mpc_new("number");
    mpc_parser_t* Operator  = mpc_new("operator");
    mpc_parser_t* Expr      = mpc_new("expr");
    mpc_parser_t* Praxis    = mpc_new("praxis");

    // define them with the following language
    mpca_lang(MPCA_LANG_DEFAULT,
        "                                                       \
        number      :   /-?[0-9]+/;                             \
        operator    :   '+' | '-' | '*' | '/';                  \
        expr        :   <number> | '(' <operator> <expr>+ ')';  \
        praxis      :   /^/ <operator> <expr>+ /$/;             \
        ",
        Number, Operator, Expr, Praxis
    );

    // print version and exit information
    puts("Praxis Version 0.0.0.0.1");
    puts("Press Ctrl+c to Exit\n");

    // in a never endind loop
    while(1){

        // output our prompt and get input
        char* input = readline("praxis > ");

        // add input to history
        add_history(input);

        // attempt to parse the user input
        mpc_result_t r;
        if (mpc_parse("<stdin>", input, Praxis, &r)){

            long result = eval(r.output);
            printf("%li\n", result);
            // on success print the AST
            /* mpc_ast_print(r.output); */
            mpc_ast_delete(r.output);
        } else {
            // otherwise print the error
            mpc_err_print(r.error);
            mpc_err_delete(r.error);
        }

        // free retrieved input
        free(input);
    }

    mpc_cleanup(4, Number, Operator, Expr, Praxis);

    return 0;
}
