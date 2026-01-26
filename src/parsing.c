#include <stdio.h>
#include <stdlib.h>
#include "mpc.h"

/*
+---------------------------------------------------------+
| PORTABILITY: Platform-specific readline implementation  |
+---------------------------------------------------------+
*/

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

/*
+---------------------------------------------------------+
|                     TYPE DEFINITIONS                    |
+---------------------------------------------------------+
*/

struct lval;
struct lenv;

typedef struct lval lval;
typedef struct lenv lenv;

// Create Enumeration of Possible lval Types
enum {
    LVAL_ERR, LVAL_NUM, LVAL_SYM, LVAL_STR,
    LVAL_FUN, LVAL_SEXPR, LVAL_QEXPR
};

// Create Enumeration of Possible Error Types
enum { LERR_DIV_ZERO, LERR_BAD_OP, LERR_BAD_NUM };

typedef lval*(*lbuiltin)(lenv*, lval*);

/*
+---------------------------------------------------------+
|                  FORWARD DECLARATIONS                   |
+---------------------------------------------------------+
*/

lenv*   lenv_new(void);
void    lenv_del(lenv* e);
lenv*   lenv_copy(lenv* e);

lval*   lval_eval(lenv* e, lval* v);

lval*   lval_copy(lval* v);
void    lval_del(lval* v);
int     lval_eq(lval* x, lval* y);

lval*   builtin_eval(lenv* e, lval* a);
lval*   builtin_list(lenv* e, lval* a);

lval*   lval_read_str(mpc_ast_t* t);

/*
+---------------------------------------------------------+
|                   GLOBAL PARSER VARIABLES               |
+---------------------------------------------------------+
*/

mpc_parser_t* Number;
mpc_parser_t* Symbol;
mpc_parser_t* String;
mpc_parser_t* Comment;
mpc_parser_t* Sexpr;
mpc_parser_t* Qexpr;
mpc_parser_t* Expr;
mpc_parser_t* Praxis;

/*
+---------------------------------------------------------+
|                   STRUCTURE DEFINITIONS                 |
+---------------------------------------------------------+
*/

struct lenv{
    lenv*   par;
    int     count;
    char**  syms;
    lval**  vals;
};

struct lval {
    int         type;
    // basic
    long        num;
    char*       err;
    char*       sym;
    char*       str;
    // function
    lbuiltin    builtin;
    lenv*       env;
    lval*       formals;
    lval*       body;
    // expression
    int         count;
    struct lval** cell;
};

/*
+---------------------------------------------------------+
|                       CONSTRUCTORS                      |
+---------------------------------------------------------+
*/

lval* lval_num(long x){
    lval* v     = malloc(sizeof(lval));
    v->type     = LVAL_NUM;
    v->num      = x;
    return v;
}

lval* lval_fun(lbuiltin func){
    lval* v     = malloc(sizeof(lval));
    v->type     = LVAL_FUN;
    v->builtin      = func;
    return v;
}

lval* lval_err(char* fmt, ...){
    lval* v     = malloc(sizeof(lval));
    v->type     = LVAL_ERR;

    va_list va;
    va_start(va, fmt);

    v->err      = malloc(512);

    vsnprintf(v->err, 511, fmt, va);
    v->err      = realloc(v->err, strlen(v->err)+1);
    va_end(va);

    return v;
}

lval* lval_sym(char* s){
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_SYM;
    v->sym  = malloc(strlen(s) + 1);
    strcpy(v->sym, s);
    return v;
}

lval* lval_str(char* s){
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_STR;
    v->str  = malloc(strlen(s) + 1);
    strcpy(v->str, s);
    return v;
}

lval* lval_sexpr(void){
    lval* v     = malloc(sizeof(lval));
    v->type     = LVAL_SEXPR;
    v->count    = 0;
    v->cell     = NULL;
    return v;
}

lval* lval_qexpr(void){
    lval* v     = malloc(sizeof(lval));
    v->type     = LVAL_QEXPR;
    v->count    = 0;
    v->cell     = NULL;
    return v;
}

lval* lval_lambda(lval* formals, lval* body){
    lval* v     = malloc(sizeof(lval));
    v->type     = LVAL_FUN;
    v->builtin  = NULL;
    v->env      = lenv_new();
    v->formals  = formals;
    v->body     = body;
    return v;
}

/*
+---------------------------------------------------------+
|                   MEMORY MANAGEMENT                     |
+---------------------------------------------------------+
*/
static lval* lval_copy_fun(lval* v){
    lval* x = malloc(sizeof(lval));
    x->type = LVAL_FUN;

    if(v->builtin){
        x->builtin = v->builtin;
    } else {
        x->builtin  = NULL;
        x->env      = lenv_copy(v->env);
        x->formals  = lval_copy(v->formals);
        x->body     = lval_copy(v->body);
    }
    return x;
}

static lval* lval_copy_str(lval* v, int type){
    lval* x = malloc(sizeof(lval));
    x->type = type;

    char* src = NULL;
    char** dst = NULL;

    switch(type){
        case LVAL_ERR:
            src = v->err;
            dst = &x->err;
            break;
        case LVAL_SYM:
            src = v->sym;
            dst = &x->sym;
            break;
        case LVAL_STR:
            src = v->str;
            dst = &x->str;
            break;
    }

    *dst = malloc(strlen(src) + 1);
    strcpy(*dst, src);
    return x;
}

static lval* lval_copy_expr(lval* v){
    lval* x = malloc(sizeof(lval));
    x->type = v->type;
    x->count = v->count;
    x->cell = malloc(sizeof(lval*) * x->count);

    for (int i = 0; i < x->count; i++){
        x->cell[i] = lval_copy(v->cell[i]);
    }
    return x;
}

lval* lval_copy(lval* v){
    lval* x = malloc(sizeof(lval));
    x->type = v->type;

    switch (v->type){
        case LVAL_FUN:
            free(x);
            return lval_copy_fun(v);

        case LVAL_NUM:
            x->num = v->num;
            return x;

        case LVAL_ERR:
        case LVAL_SYM:
        case LVAL_STR:
            free(x);
            return lval_copy_str(v, v->type);

        case LVAL_SEXPR:
        case LVAL_QEXPR:
            free(x);
            return lval_copy_expr(v);
    }
    return x;
}

static void lval_del_fun(lval* v){
    if(!v->builtin){
        lenv_del(v->env);
        lval_del(v->formals);
        lval_del(v->body);
    }
}

static void lval_del_expr(lval* v){
    for (int i = 0; i < v->count; i++){
        lval_del(v->cell[i]);
    }
    free(v->cell);
}

void lval_del(lval* v){
    switch (v->type){
        case LVAL_NUM:
            break;

        case LVAL_ERR:
            free(v->err);
            break;

        case LVAL_SYM:
            free(v->sym);
            break;

        case LVAL_FUN:
            lval_del_fun(v);
            break;

        case LVAL_STR:
            free(v->str);
            break;

        case LVAL_QEXPR:
        case LVAL_SEXPR:
            lval_del_expr(v);
            break;
    }
    free(v);
}

/*
+---------------------------------------------------------+
|                       ENVIROMENT                        |
+---------------------------------------------------------+
*/

lenv* lenv_new(void){
    lenv* e     = malloc(sizeof(lenv));
    e->par      = NULL;
    e->count    = 0;
    e->syms     = NULL;
    e->vals     = NULL;
    return e;
}

void lenv_del(lenv* e){
    for (int i = 0; i<e->count; i++){
        free(e->syms[i]);
        lval_del(e->vals[i]);
    }
    free(e->syms);
    free(e->vals);
    free(e);
}

lval* lenv_get(lenv* e, lval* k){
    for(int i = 0; i < e->count; i++){
        if(strcmp(e->syms[i], k->sym) == 0){
            return lval_copy(e->vals[i]);
        }
    }

    if(e->par){
        return lenv_get(e->par, k);
    } else {
        return lval_err("Unbound symbol '%s'", k->sym);
    }
}

void lenv_put(lenv* e, lval* k, lval* v){
    for (int i = 0; i < e->count; i++){
        if(strcmp(e->syms[i], k->sym) == 0){
            lval_del(e->vals[i]);
            e->vals[i] = lval_copy(v);
            return;
        }
    }

    e->count++;
    e->vals = realloc(e->vals, sizeof(lval*) * e->count);
    e->syms = realloc(e->syms, sizeof(char*) * e->count);

    e->vals[e->count - 1] = lval_copy(v);
    e->syms[e->count - 1] = malloc(strlen(k->sym) + 1);
    strcpy(e->syms[e->count - 1], k->sym);
}

void lenv_def(lenv* e, lval* k, lval* v){
    while(e->par) { e = e->par; }
    lenv_put(e, k, v);
}

lenv* lenv_copy(lenv* e){
    lenv* n     = malloc(sizeof(lenv));
    n->par      = e->par;
    n->count    = e->count;
    n->syms     = malloc(sizeof(char*) * n->count);
    n->vals     = malloc(sizeof(lval*) * n->count);
    for (int i = 0; i < e->count; i++){
        n->syms[i] = malloc(strlen(e->syms[i]) + 1);
        strcpy(n->syms[i], e->syms[i]);
        n->vals[i] = lval_copy(e->vals[i]);
    }
    return n;
}

/*
+---------------------------------------------------------+
|                       UTILITIES                         |
+---------------------------------------------------------+
*/

static const char* LTYPE_NAMES[] = {
    [LVAL_ERR]      = "Error",
    [LVAL_NUM]      = "Number",
    [LVAL_SYM]      = "Symbol",
    [LVAL_STR]      = "String",
    [LVAL_FUN]      = "Function",
    [LVAL_SEXPR]    = "S-expression",
    [LVAL_QEXPR]    = "Q-expression"
};

char* ltype_name(int t){
    if(t >= 0 && t < 7){
        return (char*)LTYPE_NAMES[t];
    }
    return "Unknown";
}

lval* lval_pop(lval* v, int i){
    lval* x = v->cell[i];
    memmove(
        &v->cell[i],
        &v->cell[i+1],
        sizeof(lval*) * (v->count-i-1)
    );
    v->count--;
    v->cell = realloc(v->cell, sizeof(lval*) * v->count);
    return x;
}

lval* lval_take(lval* v, int i){
    lval* x = lval_pop(v, i);
    lval_del(v);
    return x;
}

lval* lval_add(lval* v, lval* x){
    v->count++;
    v->cell = realloc(v->cell, sizeof(lval*) * v->count);
    v->cell[v->count-1] = x;
    return v;
}

lval* lval_join(lval* x, lval* y){
    while(y->count){
        x = lval_add(x, lval_pop(y, 0));
    }
    lval_del(y);
    return x;
}

static int lval_eq_fun(lval* x, lval* y){
    if(x->builtin || y->builtin){
        return x->builtin == y->builtin;
    }
    return lval_eq(x->formals, y->formals) && lval_eq(x->body, y->body);
}

static int lval_eq_expr(lval* x, lval* y){
    if (x->count != y->count) {
        return 0;
    }

    for (int i = 0; i < x->count; i++){
        if(!lval_eq(x->cell[i], y->cell[i])) {
            return 0;
        }
    }
    return 1;
}

int lval_eq(lval* x, lval* y){
    if (x->type != y->type){
        return 0;
    }

    switch (x->type){
        case LVAL_NUM:
            return (x->num == y->num);

        case LVAL_ERR:
            return (strcmp(x->err, y->err) == 0);

        case LVAL_SYM:
            return (strcmp(x->sym, y->sym) == 0);

        case LVAL_STR:
            return (strcmp(x->str, y->str) == 0);

        case LVAL_FUN:
            return lval_eq_fun(x, y);

        case LVAL_QEXPR:
        case LVAL_SEXPR:
            return lval_eq_expr(x, y);
    }
    return 0;
}

/*
+---------------------------------------------------------+
|                       ASSERTION MACROS                  |
+---------------------------------------------------------+
*/

#define LASSERT(args, cond, fmt, ...)               \
    if (!(cond)){                                   \
        lval* err = lval_err(fmt, ##__VA_ARGS__);   \
        lval_del(args);                             \
        return err;                                 \
    }

#define LASSERT_TYPE(func, args, index, expect)                                         \
    LASSERT(args, args->cell[index]->type == expect,                                    \
        "Function '%s' passed incorrect type for argument %i. Got %s, Expected %s",     \
        func, index, ltype_name(args->cell[index]->type), ltype_name(expect)            \
    )

#define LASSERT_NUM(func, args, num)                                            \
  LASSERT(args, args->count == num,                                             \
    "Function '%s' passed incorrect number of arguments. Got %i, Expected %i.", \
    func, args->count, num)


#define LASSERT_NOT_EMPTY(func, args, index)                    \
    LASSERT(args, args->cell[index]->count != 0,                \
        "Function '%s' passed {} for argument %i.", func, index \
    )

/*
+---------------------------------------------------------+
|                       PARSING                           |
+---------------------------------------------------------+
*/

lval* lval_read_num(mpc_ast_t* t){
    errno = 0;
    long x = strtol(t->contents, NULL, 10);
    return errno != ERANGE
        ? lval_num(x)
        : lval_err("invalid number");
}

lval* lval_read_str(mpc_ast_t* t){
    t->contents[strlen(t->contents) - 1] = '\0';
    char* unescaped = malloc(strlen(t->contents+1)+1);
    strcpy(unescaped, t->contents+1);
    unescaped = mpcf_unescape(unescaped);
    lval* str = lval_str(unescaped);
    free(unescaped);
    return str;
}

lval* lval_read(mpc_ast_t* t){
    if (strstr(t->tag, "number")) { return lval_read_num(t); }
    if (strstr(t->tag, "symbol")) { return lval_sym(t->contents); }
    if (strstr(t->tag, "string")) { return lval_read_str(t); }

    lval* x = NULL;
    if(strcmp(t->tag, ">") == 0){x  = lval_sexpr();}
    if(strstr(t->tag, "sexpr")) {x  = lval_sexpr();}
    if(strstr(t->tag, "qexpr")) {x  = lval_qexpr();}

    for (int i = 0; i < t->children_num; i++){
        if(strcmp(t->children[i]->contents, "(")    == 0) { continue; }
        if(strcmp(t->children[i]->contents, ")")    == 0) { continue; }
        if(strcmp(t->children[i]->contents, "{")    == 0) { continue; }
        if(strcmp(t->children[i]->contents, "}")    == 0) { continue; }
        if (strcmp(t->children[i]->tag,  "regex")   == 0) { continue; }
        if (strstr(t->children[i]->tag, "comment")) { continue; }
        x = lval_add(x, lval_read(t->children[i]));
    }
    return x;
}

/*
+---------------------------------------------------------+
|                           PRINTING                      |
+---------------------------------------------------------+
*/

void lval_print(lval* v);

void lval_println(lval* v){
    lval_print(v);
    putchar('\n');
}

void lval_expr_print(lval* v, char open, char close){
    putchar(open);
    for (int i = 0; i < v->count; i++){
        lval_print(v->cell[i]);

        if (i != (v->count-1)){
            putchar(' ');
        }
    }
    putchar(close);
}

void lval_print_str(lval* v){
    char* escaped = malloc(strlen(v->str) + 1);
    strcpy(escaped, v->str);
    escaped = mpcf_escape(escaped);
    printf("\"%s\"", escaped );
    free(escaped);
}

static void lval_print_num(lval* v){
    printf("%li", v->num);
}

static void lval_print_err(lval* v){
    printf("ERROR: %s", v->err);
}

static void lval_print_fun(lval* v){
    if(v->builtin){
        printf("<builtin>");
    } else {
        printf("(\\ ");
        lval_print(v->formals);
        putchar(' ');
        lval_print(v->body);
        putchar(')');
    }
}

static void lval_print_sym(lval* v){
    printf("%s", v->sym);
}

static void lval_print_sexpr(lval* v){
    lval_expr_print(v, '(', ')');
}

static void lval_print_qexpr(lval* v){
    lval_expr_print(v, '{', '}');
}

void lval_print(lval* v){
    switch (v->type){
        case LVAL_NUM:
            lval_print_num(v);
            break;
        case LVAL_ERR:
            lval_print_err(v);
            break;
        case LVAL_FUN:
            lval_print_fun(v);
            break;
        case LVAL_SYM:
            lval_print_sym(v);
            break;
        case LVAL_STR:
            lval_print_str(v);
            break;
        case LVAL_SEXPR:
            lval_print_sexpr(v);
            break;
        case LVAL_QEXPR:
            lval_print_qexpr(v);
            break;
    }
}

/*
+---------------------------------------------------------+
|                       EVALUATION                        |
+---------------------------------------------------------+
*/

lval* lval_eval_sexpr(lenv* e, lval* v);
lval* lval_call(lenv* e, lval* f, lval* a);

lval* lval_eval(lenv* e, lval* v){
    if(v->type == LVAL_SYM){
        lval* x = lenv_get(e, v);
        lval_del(v);
        return x;
    }
    if(v->type == LVAL_SEXPR) {
        return lval_eval_sexpr(e, v);
    }
    return v;
}

lval* lval_eval_sexpr(lenv* e, lval* v){
    // evaluate children
    for (int i = 0; i < v->count; i++){
        v->cell[i] = lval_eval(e, v->cell[i]);
    }

    // error checking
    for (int i = 0; i < v->count; i++){
        if(v->cell[i]->type == LVAL_ERR){
            return lval_take(v, i);
        }
    }

    // empty expression
    if(v->count == 0 ) { return v; }

    // single expression
    if(v->count == 1) { return lval_take(v, 0);}

    // ensure first element is symbol
    lval* f = lval_pop(v, 0);
    if (f->type != LVAL_FUN){
        lval* err = lval_err(
                "S-expression starts whith incorrect type. "
                "Got %s, Expected %s",
                ltype_name(f->type),
                ltype_name(LVAL_FUN)
            );
        lval_del(f);
        lval_del(v);
        return err;
    }

    lval* result = lval_call(e, f, v);
    lval_del(f);
    return result;
}

static lval* validate_call_args(lval* f, lval* a, int given, int total){
    if (f->formals->count == 0){
        lval_del(a);
        return lval_err(
                "Function passed too many arguments. "
                "Got %i, Expected %i.", given, total
                );
    }
    return NULL;
}

static lval* handle_empty_varargs(lval* f){
    if (
        f->formals->count > 0 &&
        strcmp(f->formals->cell[0]->sym, "&") == 0 
       ) {
        if (f->formals->count != 2){
            return lval_err(
                    "Function format invalid. "
                    "Symbol '&' not followed by single symbol."
                    );
        }

        lval_del(lval_pop(f->formals, 0));
        
        lval* sym = lval_pop(f->formals, 0);
        lval* val = lval_qexpr();

        lenv_put(f->env, sym, val);
        lval_del(sym);
        lval_del(val);
    }
    return NULL;
}

static lval* handle_varargs(lenv* e, lval* f, lval* sym, lval* a){
    if(strcmp(sym->sym, "&") == 0){
        if(f->formals->count != 1){
            lval_del(a);
            return lval_err(
                    "Function format invalid. "
                    "Symbol '&' not followed by single symbol."
                    );
        }
        lval* nsym = lval_pop(f->formals, 0);
        lenv_put(f->env, nsym, builtin_list(e, a));
        lval_del(sym);
        lval_del(nsym);
        return lval_sexpr();
    }
    return NULL;
}

static lval* bind_formal_params(lenv* e, lval* f, lval* a, int given, int total){
    while (a->count){
        lval* err = validate_call_args(f, a, given, total);
        if(err) return err;

        lval* sym = lval_pop(f->formals, 0);
        lval* val = lval_pop(a, 0);

        lval* varargs_result = handle_varargs(e, f, sym, a);

        if(varargs_result){
            lval_del(val);
            return varargs_result;
        }

        lenv_put(f->env, sym, val);
        lval_del(sym);
        lval_del(val);
    }
    return NULL;
}

static lval* finalize_call(lenv* e, lval* f){
    if(f->formals->count == 0){
        f->env->par = e;
        return builtin_eval(
                f->env,
                lval_add(lval_sexpr(), lval_copy(f->body))
                );
    }
    return lval_copy(f);
}

lval* lval_call(lenv* e, lval* f, lval* a){
    if(f->builtin) { return f->builtin(e, a); }

    int given = a->count;
    int total = f->formals->count;

    lval* bind_result = bind_formal_params(e, f, a, given, total);
    if(bind_result && bind_result->type != LVAL_SEXPR){
        return bind_result;
    }

    lval_del(a);

    lval* empty_varargs_err = handle_empty_varargs(f);
    if(empty_varargs_err){
        return empty_varargs_err;
    }

    return finalize_call(e, f);
}

/*
+---------------------------------------------------------+
|               BUILTIN FUNCTIONS                         |
+---------------------------------------------------------+
*/


lval* builtin_head(lenv* e, lval* a){
    LASSERT_NUM("head", a, 1);
    LASSERT_TYPE("head", a, 0, LVAL_QEXPR);
    LASSERT_NOT_EMPTY("head", a, 0);

    lval* v = lval_take(a, 0);

    while(v->count > 1) { lval_del(lval_pop(v, 1)); }
    return v;
}

lval* builtin_tail(lenv* e, lval* a){
    LASSERT_NUM("tail", a, 1);
    LASSERT_TYPE("tail", a, 0, LVAL_QEXPR);
    LASSERT_NOT_EMPTY("tail", a, 0);

    // take first argument
    lval* v = lval_take(a, 0);

    // delete first element and return
    lval_del(lval_pop(v, 0));
    return v;
}

lval* builtin_list(lenv* e, lval* a){
    a->type = LVAL_QEXPR;
    return a;
}

lval* builtin_eval(lenv* e, lval* a){
    LASSERT_NUM("eval", a, 1);
    LASSERT_TYPE("eval", a, 0, LVAL_QEXPR);

    lval* x = lval_take(a, 0);
    x->type = LVAL_SEXPR;
    return lval_eval(e, x);
}

lval* builtin_join(lenv* e, lval* a){
    for (int i = 0; i<a->count; i++){
        LASSERT_TYPE("join", a, i, LVAL_QEXPR);
    }

    lval* x = lval_pop(a, 0);

    while (a->count){
        x = lval_join(x, lval_pop(a, 0));
    }

    lval_del(a);
    return x;

}


lval* builtin_op(lenv* e, lval* a, char* op);

lval* builtin_add(lenv* e, lval* a){
    return builtin_op(e, a, "+");
}

lval* builtin_sub(lenv* e, lval* a){
    return builtin_op(e, a, "-");
}

lval* builtin_mul(lenv* e, lval* a){
    return builtin_op(e, a, "*");
}

lval* builtin_div(lenv* e, lval* a){
    return builtin_op(e, a, "/");
}

static long apply_op(long x, long y, char* op){
    if(strcmp(op, "+") == 0) return x + y;
    if(strcmp(op, "-") == 0) return x - y;
    if(strcmp(op, "*") == 0) return x * y;
    if(strcmp(op, "/") == 0) return x / y;
    return 0;
}


lval* builtin_op(lenv* e, lval* a, char* op){
    // ensure all arguments are numbers
    for(int i = 0; i < a->count; i++ ){
        LASSERT_TYPE(op, a, i, LVAL_NUM);
    }

    // pop the first element
    lval* x = lval_pop(a, 0);

    // if no arguments and sub then perform unary negation
    if((strcmp(op, "-") == 0) && a->count == 0){
        x->num = -x->num;
    }

    // while there are still elements remaining
    while(a->count > 0){
        // pop the next element
        lval* y = lval_pop(a, 0);

        if(strcmp(op, "/") == 0 && y->num == 0) {
                lval_del(x);
                lval_del(y);
                lval_del(a);
                return lval_err("Division by ZERO");
        }

        x->num = apply_op(x->num, y->num, op);
        lval_del(y);
    }
    lval_del(a);
    return x;
}

/* Comparison Operations */

lval* builtin_ord(lenv* e, lval*a, char* op);
lval* builtin_cmp(lenv* e, lval* a, char* op);

lval* builtin_gt(lenv* e, lval* a){
    return builtin_ord(e, a, ">");
}

lval* builtin_lt(lenv* e, lval* a){
    return builtin_ord(e, a, "<");
}

lval* builtin_gte(lenv* e, lval* a){
    return builtin_ord(e, a, ">=");
}

lval* builtin_lte(lenv* e, lval* a){
    return builtin_ord(e, a, "<=");
}

lval* builtin_eq(lenv* e, lval* a){
    return builtin_cmp(e, a, "==");
}

lval* builtin_neq(lenv* e, lval* a){
    return builtin_cmp(e, a, "!=");
}

lval* builtin_ord(lenv* e, lval*a, char* op){
    LASSERT_NUM(op, a, 2);
    LASSERT_TYPE(op, a, 0, LVAL_NUM);
    LASSERT_TYPE(op, a, 1, LVAL_NUM);

    int r;
    if(strcmp(op, ">") == 0){
        r = (a->cell[0]->num > a->cell[1]->num);
    }
    if(strcmp(op, "<") == 0){
        r = (a->cell[0]->num < a->cell[1]->num);
    }
   if(strcmp(op, ">=") == 0){
        r = (a->cell[0]->num >= a->cell[1]->num);
    }
   if(strcmp(op, "<=") == 0){
        r = (a->cell[0]->num <= a->cell[1]->num);
    }
   lval_del(a);
   return lval_num(r);
}

lval* builtin_cmp(lenv* e, lval* a, char* op){
    LASSERT_NUM(op, a, 2);
    int r;
    if (strcmp(op, "==") == 0){
        r = lval_eq(a->cell[0], a->cell[1]);
    }
    if (strcmp(op, "!=") == 0){
        r = !lval_eq(a->cell[0], a->cell[1]);
    }
    lval_del(a);
    return lval_num(r);
}

/* Control Flow */

lval* builtin_if(lenv* e, lval* a){
    LASSERT_NUM("if", a, 3);
    LASSERT_TYPE("if", a, 0, LVAL_NUM);
    LASSERT_TYPE("if", a, 1, LVAL_QEXPR);
    LASSERT_TYPE("if", a, 2, LVAL_QEXPR);

    lval* x;
    a->cell[1]->type = LVAL_SEXPR;
    a->cell[2]->type = LVAL_SEXPR;

    if(a->cell[0]->num){
        x = lval_eval(e, lval_pop(a, 1));
    } else {
        x = lval_eval(e, lval_pop(a, 2));
    }

    lval_del(a);
    return x;
}

lval* builtin_lambda(lenv* e, lval* a){
    // check two arguments, each of which are q-expressions
    LASSERT_NUM("\\", a, 2);
    LASSERT_TYPE("\\", a, 0, LVAL_QEXPR);
    LASSERT_TYPE("\\", a, 1, LVAL_QEXPR);

    // check first Q-expression contains only symbols
    for (int i = 0; i< a->cell[0]->count; i++){
        LASSERT(a, (a->cell[0]->cell[i]->type == LVAL_SYM),
        "Cannot define non-symbol. Got %s, Expected %s",
        ltype_name(a->cell[0]->cell[i]->type), ltype_name(LVAL_SYM));
    }

    // pop first two arguments and pass them to lval_lambda
    lval* formals   = lval_pop(a, 0);
    lval* body      = lval_pop(a, 0);
    lval_del(a);

    return lval_lambda(formals, body);

}

/* Variable Operations */

lval* builtin_var(lenv* e, lval*a, char* func);

lval* builtin_def(lenv* e, lval* a){
    return builtin_var(e, a, "def");
}

lval* builtin_put(lenv* e, lval* a){
    return builtin_var(e, a , "=");
}

lval* builtin_var(lenv* e, lval*a, char* func){
    LASSERT_TYPE(func, a, 0, LVAL_QEXPR);

    lval* syms = a->cell[0];
    for(int i = 0; i < syms->count; i++){
        LASSERT(a, (syms->cell[i]->type == LVAL_SYM),
            "Function '%s' cannot define non-symbol. "
            "Got %s, expected %s.", func,
            ltype_name(syms->cell[i]->type),
            ltype_name(LVAL_SYM));
    }

    LASSERT(a, (syms->count == a->count-1),
            "Function '%s' passed to many arguments for symbols. "
            "Got %i. Expected %i.", func, syms->count, a->count-1);

    for (int i = 0; i < syms->count; i++){
        // if "def" define in globally.
        // if "put" define in locally
        if(strcmp(func, "def") == 0){
            lenv_def(e, syms->cell[i], a->cell[i+1]);
        }

        if(strcmp(func, "=") == 0){
            lenv_put(e, syms->cell[i], a->cell[i+1]);
        }
    }

    lval_del(a);
    return lval_sexpr();
}

/* I/O Operations */

lval* builtin_load(lenv* e, lval* a){
    LASSERT_NUM("load", a, 1);
    LASSERT_TYPE("load", a, 0, LVAL_STR);

    mpc_result_t r;

    if(mpc_parse_contents(a->cell[0]->str, Praxis, &r)){
        lval* expr = lval_read(r.output);
        mpc_ast_delete(r.output);

        while(expr->count){
            lval* to_eval = lval_pop(expr, 0);
            lval* x = lval_eval(e, to_eval);
            if(x->type == LVAL_ERR){ lval_println(x); }
            lval_del(x);
        }

        lval_del(expr);
        lval_del(a);
        return lval_sexpr();
    } else {
        char* err_msg = mpc_err_string(r.error);
        mpc_err_delete(r.error);

        lval* err = lval_err("Could not load Library %s", err_msg);
        free(err_msg);
        lval_del(a);

        return err;
    }
}

lval* builtin_print(lenv* e, lval* a){
    for (int i = 0; i < a->count; i++){
        lval_print(a->cell[i]);
        putchar(' ');
    }

    putchar('\n');
    lval_del(a);

    return lval_sexpr();
}

lval* builtin_error(lenv* e, lval* a){
    LASSERT_NUM("error", a, 1);
    LASSERT_TYPE("error", a, 0, LVAL_STR);

    lval* err = lval_err(a->cell[0]->str);

    lval_del(a);
    return err;
}

/*
+---------------------------------------------------------+
|                   ENVIROMENT SETUP                      |
+---------------------------------------------------------+
*/

void lenv_add_builtin(lenv* e, char* name, lbuiltin func){
    lval* k = lval_sym(name);
    lval* v = lval_fun(func);
    lenv_put(e, k, v);
    lval_del(k);
    lval_del(v);
}

void lenv_add_builtins(lenv* e){
    lenv_add_builtin(e, "list", builtin_list);
    lenv_add_builtin(e, "head", builtin_head);
    lenv_add_builtin(e, "tail", builtin_tail);
    lenv_add_builtin(e, "eval", builtin_eval);
    lenv_add_builtin(e, "join", builtin_join);
    lenv_add_builtin(e, "\\",   builtin_lambda);
    lenv_add_builtin(e, "def",  builtin_def);
    lenv_add_builtin(e, "=",    builtin_put);

    lenv_add_builtin(e, "if",   builtin_if);

    lenv_add_builtin(e, "==",   builtin_eq);
    lenv_add_builtin(e, "!=",   builtin_neq);
    lenv_add_builtin(e, ">",    builtin_gt);
    lenv_add_builtin(e, "<",    builtin_lt);
    lenv_add_builtin(e, ">=",   builtin_gte);
    lenv_add_builtin(e, "<=",   builtin_lte);

    lenv_add_builtin(e, "+", builtin_add);
    lenv_add_builtin(e, "-", builtin_sub);
    lenv_add_builtin(e, "*", builtin_mul);
    lenv_add_builtin(e, "/", builtin_div);

    lenv_add_builtin(e, "load", builtin_load);
    lenv_add_builtin(e, "print", builtin_print);
    lenv_add_builtin(e, "error", builtin_error);
}

/*
+---------------------------------------------------------+
|                           MAIN                          |
+---------------------------------------------------------+
*/

int main(int argc, char** argv){

    // create some Parsers
    Number    = mpc_new("number");
    Symbol    = mpc_new("symbol");
    String    = mpc_new("string");
    Comment   = mpc_new("comment");
    Sexpr     = mpc_new("sexpr");
    Qexpr     = mpc_new("qexpr");
    Expr      = mpc_new("expr");
    Praxis    = mpc_new("praxis");

    // define them with the following language
    mpca_lang(MPCA_LANG_DEFAULT,
        "                                                       \
        number      :   /-?[0-9]+/;                             \
        symbol      :   /[a-zA-Z0-9_+\\-*\\/\\\\=<>!&]+/;       \
        string      :   /\"(\\\\.|[^\"])*\"/ ;                  \
        comment     :   /;[^\\r\\n]*/;                          \
        sexpr       :   '(' <expr>* ')';                        \
        qexpr       :   '{' <expr>* '}';                        \
        expr        :   <number> | <symbol>| <string> |         \
                        <comment>| <sexpr> | <qexpr>;           \
        praxis      :   /^/ <expr>* /$/;                        \
        ",
        Number, Symbol, String, Comment, Sexpr, Qexpr, Expr, Praxis
    );

    lenv* e = lenv_new();
    lenv_add_builtins(e);

    if (argc >= 2) {
      for (int i = 1; i < argc; i++) {
        printf("Loading file: %s\n", argv[i]);
        lval* args = lval_add(lval_sexpr(), lval_str(argv[i]));

        lval* x = builtin_load(e, args);

        if (x->type == LVAL_ERR) { lval_println(x); }
        lval_del(x);
      }
    } else {
      puts("Praxis Version 0.0.1");
      puts("Press Ctrl+c to Exit\n");
    }

    while (1) {
        char* input = readline("praxis> ");
        add_history(input);

        mpc_result_t r;
        if (mpc_parse("<stdin>", input, Praxis, &r)) {
            lval* x = lval_eval(e, lval_read(r.output));
            lval_println(x);
            lval_del(x);
            mpc_ast_delete(r.output);
        } else {
            mpc_err_print(r.error);
            mpc_err_delete(r.error);
        }
    free(input);

    }
    mpc_cleanup(8, Number, Symbol, String, Comment, Sexpr, Qexpr, Expr, Praxis);
    return 0;
}
