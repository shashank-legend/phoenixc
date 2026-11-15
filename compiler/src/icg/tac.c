#include "tac.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int temp_count = 0;
static int label_count = 0;

TAC* create_tac(TAC_OP op, const char *result, const char *arg1, const char *arg2) {
    TAC *tac = (TAC*)malloc(sizeof(TAC));
    tac->op = op;
    tac->result = result ? strdup(result) : NULL;
    tac->arg1 = arg1 ? strdup(arg1) : NULL;
    tac->arg2 = arg2 ? strdup(arg2) : NULL;
    tac->next = NULL;
    return tac;
}

TAC* append_tac(TAC *list, TAC *new_tac) {
    if (!new_tac) return list;
    if (!list) return new_tac;
    
    TAC *curr = list;
    while (curr->next) {
        curr = curr->next;
    }
    curr->next = new_tac;
    return list;
}

char* new_temp() {
    char *temp = (char*)malloc(16);
    snprintf(temp, 16, "t%d", temp_count++);
    return temp;
}

char* new_label() {
    char *label = (char*)malloc(16);
    snprintf(label, 16, "L%d", label_count++);
    return label;
}

static const char* op_to_string(TAC_OP op) {
    switch(op) {
        case TAC_ADD: return "+";
        case TAC_SUB: return "-";
        case TAC_MUL: return "*";
        case TAC_DIV: return "/";
        case TAC_ASSIGN: return "=";
        case TAC_RETURN: return "return";
        case TAC_LABEL: return "label";
        case TAC_GOTO: return "goto";
        case TAC_IF_FALSE: return "if_false";
        case TAC_PARAM: return "param";
        case TAC_CALL: return "call";
        case TAC_LT: return "<";
        case TAC_GT: return ">";
        case TAC_LE: return "<=";
        case TAC_GE: return ">=";
        case TAC_EQ: return "==";
        case TAC_NE: return "!=";
        case TAC_MOD: return "%";
        case TAC_NEG: return "-";
        default: return "unknown";
    }
}

void fprint_tac(FILE *fp, TAC *code) {
    for (TAC *curr = code; curr; curr = curr->next) {
        switch (curr->op) {
            case TAC_FUNC_BEGIN:
                fprintf(fp, "func %s:\n", curr->result);
                break;
            case TAC_FUNC_END:
                fprintf(fp, "endfunc\n\n");
                break;
            case TAC_LABEL:
                fprintf(fp, "%s:\n", curr->result);
                break;
            case TAC_GOTO:
                fprintf(fp, "  goto %s\n", curr->result);
                break;
            case TAC_RETURN:
                if (curr->result) fprintf(fp, "  return %s\n", curr->result);
                else fprintf(fp, "  return\n");
                break;
            case TAC_ASSIGN:
                fprintf(fp, "  %s = %s\n", curr->result, curr->arg1);
                break;
            case TAC_NEG:
                fprintf(fp, "  %s = -%s\n", curr->result, curr->arg1);
                break;
            case TAC_IF_FALSE:
                fprintf(fp, "  if_false %s goto %s\n", curr->arg1, curr->result);
                break;
            case TAC_PARAM:
                fprintf(fp, "  param %s\n", curr->result);
                break;
            case TAC_CALL:
                if (curr->result) {
                    fprintf(fp, "  %s = call %s, %s\n", curr->result, curr->arg1, curr->arg2);
                } else {
                    fprintf(fp, "  call %s, %s\n", curr->arg1, curr->arg2);
                }
                break;
            default:
                fprintf(fp, "  %s = %s %s %s\n",
                        curr->result, curr->arg1, op_to_string(curr->op), curr->arg2);
                break;
        }
    }
}

void print_tac(TAC *code) {
    fprint_tac(stdout, code);
}

void write_tac_to_file(TAC *code, const char *filename) {
    FILE *fp = fopen(filename, "w");
    if (!fp) {
        fprintf(stderr, "Failed to open TAC output file: %s\n", filename);
        return;
    }
    fprint_tac(fp, code);
    fclose(fp);
}

int count_tac(TAC *code) {
    int n = 0;
    for (TAC *c = code; c; c = c->next) {
        if (c->op != TAC_LABEL && c->op != TAC_FUNC_BEGIN && c->op != TAC_FUNC_END) n++;
    }
    return n;
}

void free_tac(TAC *code) {
    TAC *curr = code;
    while (curr) {
        TAC *next = curr->next;
        free(curr->result);
        free(curr->arg1);
        free(curr->arg2);
        free(curr);
        curr = next;
    }
}