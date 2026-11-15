#ifndef TAC_H
#define TAC_H

#include <stdio.h>

typedef enum {
    TAC_ADD,
    TAC_SUB,
    TAC_MUL,
    TAC_DIV,
    TAC_ASSIGN,
    TAC_RETURN,
    TAC_LABEL,
    TAC_GOTO,
    TAC_IF_FALSE,
    TAC_PARAM,
    TAC_CALL,
    TAC_LT,
    TAC_GT,
    TAC_LE,
    TAC_GE,
    TAC_EQ,
    TAC_NE,
    TAC_MOD,
    TAC_NEG,
    TAC_FUNC_BEGIN,
    TAC_FUNC_END
} TAC_OP;

typedef struct TAC {
    TAC_OP op;
    char *result;
    char *arg1;
    char *arg2;
    struct TAC *next;
} TAC;

// Function declarations
TAC* create_tac(TAC_OP op, const char *result, const char *arg1, const char *arg2);
TAC* append_tac(TAC *list, TAC *new_tac);
void print_tac(TAC *code);
void fprint_tac(FILE *fp, TAC *code);
int count_tac(TAC *code);
void write_tac_to_file(TAC *code, const char *filename);
void free_tac(TAC *code);
char* new_temp();
char* new_label();

#endif // TAC_H