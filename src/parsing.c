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

// Create Enumeration of Possible lval Types
enum { LVAL_NUM, LVAL_ERR };

// Create Enumeration of Possible Error Types 
enum { LERR_DIV_ZERO, LERR_BAD_OP, LERR_BAD_NUM };

typedef struct {
    int type;
    long num;
    int err;
} lval;

// create a new number type lval
lval lval_num(long x){
    lval v;
    v.type  = LVAL_NUM;
    v.num   = x;
    return v;
}


// create a new error type val
lval lval_err(int x){
    lval v;
    v.type  = LVAL_ERR;
    v.err   = x;
    return v;
}

// print an "lval"
void lval_print(lval v){
    switch (v.type){
        // in the case the type is a number print it
        // then "break" out the switch
        case LVAL_NUM: 
            printf("%li", v.num); 
            break;
        case LVAL_ERR: 
            // check what type of error is it and print it
            if(v.err == LERR_DIV_ZERO){
                printf("ERROR: Division by ZERO");
            }
            if(v.err == LERR_BAD_OP){
                printf("ERROR: Invalid Operator");
            }
            if(v.err == LERR_BAD_NUM){
                printf("ERROR: Invalid Number");
            }
        break;
    }
}

void lval_println(lval v){
    lval_print(v);
    putchar('\n');
}

lval eval_op(lval x, char* op, lval y){
    if (strcmp(op, "+") == 0) {return lval_num(x.num + y.num);}
    if (strcmp(op, "-") == 0) {return lval_num(x.num - y.num);}
    if (strcmp(op, "*") == 0) {return lval_num(x.num * y.num);}
    if (strcmp(op, "/") == 0) {
        // if second operand is zero return error
        return y.num == 0
            ? lval_err(LERR_DIV_ZERO)
            : lval_num(x.num / y.num);
    }
    return lval_err(LERR_BAD_OP);
}

lval eval(mpc_ast_t* t){
    
    // if tagged as number, return it directly
    if (strstr(t->tag, "number")){
        errno = 0;
        long x = strtol(t->contents, NULL, 10);
        return errno != ERANGE ? lval_num(x) : lval_err(LERR_BAD_NUM);
    }
    // the operator is always second child
    char* op = t->children[1]->contents;

    // we store the third child in `x`
    lval x = eval(t->children[2]);

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

            lval result = eval(r.output);
            lval_println(result);
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
