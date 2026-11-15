#include "icg.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static TAC_OP get_tac_op(const char *op) {
    if (strcmp(op, "+") == 0) return TAC_ADD;
    if (strcmp(op, "-") == 0) return TAC_SUB;
    if (strcmp(op, "*") == 0) return TAC_MUL;
    if (strcmp(op, "/") == 0) return TAC_DIV;
    if (strcmp(op, "%") == 0) return TAC_MOD;
    if (strcmp(op, "<") == 0) return TAC_LT;
    if (strcmp(op, ">") == 0) return TAC_GT;
    if (strcmp(op, "<=") == 0) return TAC_LE;
    if (strcmp(op, ">=") == 0) return TAC_GE;
    if (strcmp(op, "==") == 0) return TAC_EQ;
    if (strcmp(op, "!=") == 0) return TAC_NE;
    return TAC_ADD; // Default
}

TAC* gen_expr(ASTNode *node, char **result_var) {
    if (!node) {
        *result_var = NULL;
        return NULL;
    }
    
    TAC *code = NULL;
    
    switch (node->type) {
        case NODE_LITERAL:
            *result_var = strdup(node->value);
            break;
            
        case NODE_IDENTIFIER:
            *result_var = strdup(node->value);
            break;
            
        case NODE_BINOP: {
            if (node->child_count < 2) {
                *result_var = NULL;
                return NULL;
            }
            
            char *left_var = NULL;
            char *right_var = NULL;
            
            TAC *left_code = gen_expr(node->children[0], &left_var);
            TAC *right_code = gen_expr(node->children[1], &right_var);
            
            code = append_tac(left_code, right_code);
            
            if (left_var && right_var) {
                char *temp = new_temp();
                TAC_OP op = get_tac_op(node->value);
                TAC *op_tac = create_tac(op, temp, left_var, right_var);
                code = append_tac(code, op_tac);
                *result_var = temp;
            } else {
                *result_var = NULL;
            }
            
            free(left_var);
            free(right_var);
            break;
        }
        
        case NODE_UNOP: {
            char *operand = NULL;
            code = gen_expr(node->children[0], &operand);
            if (!operand) { *result_var = NULL; break; }
            /* Fold negative numeric literals directly */
            if (node->children[0]->type == NODE_LITERAL) {
                size_t n = strlen(operand) + 2;
                char *lit = (char*)malloc(n);
                snprintf(lit, n, "-%s", operand);
                *result_var = lit;
            } else {
                char *temp = new_temp();
                code = append_tac(code, create_tac(TAC_NEG, temp, operand, NULL));
                *result_var = temp;
            }
            free(operand);
            break;
        }

        case NODE_CALL: {
            // Function call
            char *func_name = node->value;
            int arg_count = node->child_count;
            
            // Generate code for arguments and create param instructions
            for (int i = 0; i < arg_count; i++) {
                char *arg_var = NULL;
                TAC *arg_code = gen_expr(node->children[i], &arg_var);
                code = append_tac(code, arg_code);
                if (arg_var) {
                    code = append_tac(code, create_tac(TAC_PARAM, arg_var, NULL, NULL));
                    free(arg_var);
                }
            }
            
            // Create call instruction
            char *temp = new_temp();
            char arg_count_str[16];
            snprintf(arg_count_str, sizeof(arg_count_str), "%d", arg_count);
            code = append_tac(code, create_tac(TAC_CALL, temp, func_name, arg_count_str));
            *result_var = temp;
            break;
        }
        
        default:
            *result_var = NULL;
            break;
    }
    
    return code;
}

TAC* gen_stmt(ASTNode *node) {
    if (!node) return NULL;
    
    TAC *code = NULL;
    
    switch (node->type) {
        case NODE_DECLARATION: {
            if (node->child_count >= 2) {
                char *var_name = node->children[0]->value;
                char *expr_var = NULL;
                TAC *expr_code = gen_expr(node->children[1], &expr_var);
                
                code = expr_code;
                if (expr_var) {
                    code = append_tac(code, create_tac(TAC_ASSIGN, var_name, expr_var, NULL));
                    free(expr_var);
                }
            }
            break;
        }
        
        case NODE_ASSIGNMENT: {
            if (node->child_count >= 2) {
                char *var_name = node->children[0]->value;
                char *expr_var = NULL;
                TAC *expr_code = gen_expr(node->children[1], &expr_var);
                
                code = expr_code;
                if (expr_var) {
                    code = append_tac(code, create_tac(TAC_ASSIGN, var_name, expr_var, NULL));
                    free(expr_var);
                }
            }
            break;
        }
        
        case NODE_RETURN: {
            if (node->child_count > 0) {
                char *expr_var = NULL;
                TAC *expr_code = gen_expr(node->children[0], &expr_var);
                code = expr_code;
                code = append_tac(code, create_tac(TAC_RETURN, expr_var, NULL, NULL));
                free(expr_var);
            } else {
                code = create_tac(TAC_RETURN, NULL, NULL, NULL);
            }
            break;
        }
        
        case NODE_IF: {
            char *cond_var = NULL;
            TAC *cond_code = gen_expr(node->children[0], &cond_var);
            
            char *else_label = new_label();
            char *end_label = new_label();
            
            code = cond_code;
            code = append_tac(code, create_tac(TAC_IF_FALSE, else_label, cond_var, NULL));
            free(cond_var);

            // Then branch
            if (node->child_count > 1) {
                code = append_tac(code, gen_stmt(node->children[1]));
            }
            code = append_tac(code, create_tac(TAC_GOTO, end_label, NULL, NULL));
            
            // Else branch
            code = append_tac(code, create_tac(TAC_LABEL, else_label, NULL, NULL));
            if (node->child_count > 2) {
                code = append_tac(code, gen_stmt(node->children[2]));
            }
            
            // End label
            code = append_tac(code, create_tac(TAC_LABEL, end_label, NULL, NULL));
            free(else_label);
            free(end_label);
            break;
        }
        
        case NODE_WHILE: {
            char *start_label = new_label();
            char *end_label = new_label();
            
            code = create_tac(TAC_LABEL, start_label, NULL, NULL);
            
            char *cond_var = NULL;
            TAC *cond_code = gen_expr(node->children[0], &cond_var);
            code = append_tac(code, cond_code);
            code = append_tac(code, create_tac(TAC_IF_FALSE, end_label, cond_var, NULL));
            free(cond_var);

            // Loop body
            if (node->child_count > 1) {
                code = append_tac(code, gen_stmt(node->children[1]));
            }
            
            code = append_tac(code, create_tac(TAC_GOTO, start_label, NULL, NULL));
            code = append_tac(code, create_tac(TAC_LABEL, end_label, NULL, NULL));
            free(start_label);
            free(end_label);
            break;
        }

        case NODE_FOR: {
            /* children: init, cond, update, body */
            char *start_label = new_label();
            char *end_label = new_label();

            code = gen_stmt(node->children[0]);
            code = append_tac(code, create_tac(TAC_LABEL, start_label, NULL, NULL));

            ASTNode *cond = node->children[1];
            if (cond && cond->type != NODE_BLOCK) {
                char *cond_var = NULL;
                code = append_tac(code, gen_expr(cond, &cond_var));
                code = append_tac(code, create_tac(TAC_IF_FALSE, end_label, cond_var, NULL));
                free(cond_var);
            }

            code = append_tac(code, gen_stmt(node->children[3]));
            code = append_tac(code, gen_stmt(node->children[2]));
            code = append_tac(code, create_tac(TAC_GOTO, start_label, NULL, NULL));
            code = append_tac(code, create_tac(TAC_LABEL, end_label, NULL, NULL));
            free(start_label);
            free(end_label);
            break;
        }

        case NODE_FUNCTION: {
            code = create_tac(TAC_FUNC_BEGIN, node->value, NULL, NULL);
            /* body is always the last child; params are bound by the caller's `param` */
            if (node->child_count > 0) {
                code = append_tac(code, gen_stmt(node->children[node->child_count - 1]));
            }
            code = append_tac(code, create_tac(TAC_FUNC_END, node->value, NULL, NULL));
            break;
        }
        
        case NODE_BLOCK: {
            for (int i = 0; i < node->child_count; i++) {
                code = append_tac(code, gen_stmt(node->children[i]));
            }
            break;
        }
        
        case NODE_EXPR_STMT: {
            if (node->child_count > 0) {
                char *result_var = NULL;
                code = gen_expr(node->children[0], &result_var);
                free(result_var);
            }
            break;
        }
        
        default:
            for (int i = 0; i < node->child_count; i++) {
                code = append_tac(code, gen_stmt(node->children[i]));
            }
            break;
    }
    
    return code;
}

TAC* generate_tac(ASTNode *root) {
    if (!root) return NULL;
    
    if (root->type == NODE_PROGRAM) {
        TAC *code = NULL;
        for (int i = 0; i < root->child_count; i++) {
            code = append_tac(code, gen_stmt(root->children[i]));
        }
        return code;
    }
    
    return gen_stmt(root);
}