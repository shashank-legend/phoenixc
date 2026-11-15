#include "semantic.h"
#include "../error/error.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

SymbolTable *global_scope = NULL;
SymbolTable *current_scope = NULL;

/* ---- Function table ---------------------------------------------------- */

typedef struct FunctionEntry {
    char *name;
    DataType return_type;
    int param_count;
    DataType *param_types;
    int line;
    struct FunctionEntry *next;
} FunctionEntry;

static FunctionEntry *function_table = NULL;
static FunctionEntry *current_function = NULL;

const char* type_name(DataType t) {
    switch (t) {
        case TYPE_INT: return "int";
        case TYPE_FLOAT: return "float";
        case TYPE_VOID: return "void";
        default: return "unknown";
    }
}

static FunctionEntry* lookup_function(const char *name) {
    for (FunctionEntry *f = function_table; f; f = f->next) {
        if (strcmp(f->name, name) == 0) return f;
    }
    return NULL;
}

/* Collect the flat list of Param nodes under a (possibly nested) Params node. */
static void collect_params(ASTNode *node, ASTNode **out, int *count, int max) {
    if (!node) return;
    if (node->type == NODE_PARAM && node->value) {
        if (*count < max) out[(*count)++] = node;
        return;
    }
    for (int i = 0; i < node->child_count; i++) {
        collect_params(node->children[i], out, count, max);
    }
}

#define MAX_PARAMS 64

/* Pass 1: register every function signature so calls can appear before definitions. */
static void register_function(ASTNode *func) {
    if (lookup_function(func->value)) {
        report_error(ERROR_REDECLARED, func->line, func->column,
                     "Function '%s' redefined", func->value);
        return;
    }
    ASTNode *params[MAX_PARAMS];
    int count = 0;
    if (func->child_count > 1) collect_params(func->children[0], params, &count, MAX_PARAMS);

    FunctionEntry *f = (FunctionEntry*)calloc(1, sizeof(FunctionEntry));
    f->name = strdup(func->value);
    f->return_type = func->data_type;
    f->param_count = count;
    f->param_types = count ? (DataType*)malloc(sizeof(DataType) * count) : NULL;
    for (int i = 0; i < count; i++) f->param_types[i] = params[i]->data_type;
    f->line = func->line;
    f->next = function_table;
    function_table = f;
}

static void free_function_table(void) {
    FunctionEntry *f = function_table;
    while (f) {
        FunctionEntry *next = f->next;
        free(f->name);
        free(f->param_types);
        free(f);
        f = next;
    }
    function_table = NULL;
    current_function = NULL;
}

/* int -> float widening is fine; anything else that differs is an error. */
static int is_assignable(DataType target, DataType value) {
    if (target == TYPE_UNKNOWN || value == TYPE_UNKNOWN) return 1;
    if (target == value) return 1;
    if (target == TYPE_FLOAT && value == TYPE_INT) return 1;
    return 0;
}

void init_semantic_analysis() {
    free_function_table();
    global_scope = create_scope(NULL);
    current_scope = global_scope;
}

SymbolTable* create_scope(SymbolTable *parent) {
    SymbolTable *scope = (SymbolTable*)malloc(sizeof(SymbolTable));
    scope->head = NULL;
    scope->parent = parent;
    return scope;
}

void enter_scope() {
    current_scope = create_scope(current_scope);
}

void exit_scope() {
    if (current_scope && current_scope->parent) {
        SymbolTable *old = current_scope;
        current_scope = current_scope->parent;
        free_symbol_table(old);
    }
}

void add_symbol(const char *name, DataType type, int line) {
    // Check for redeclaration in current scope
    SymbolEntry *curr = current_scope->head;
    while (curr) {
        if (strcmp(curr->name, name) == 0) {
            report_error(ERROR_REDECLARED, line, 0, 
                        "Variable '%s' redeclared (previously declared at line %d)", 
                        name, curr->line);
            return;
        }
        curr = curr->next;
    }
    
    SymbolEntry *entry = (SymbolEntry*)malloc(sizeof(SymbolEntry));
    entry->name = strdup(name);
    entry->type = type;
    entry->line = line;
    entry->is_initialized = 0;
    entry->next = current_scope->head;
    current_scope->head = entry;
}

SymbolEntry* lookup_symbol(const char *name) {
    SymbolTable *scope = current_scope;
    while (scope) {
        SymbolEntry *entry = scope->head;
        while (entry) {
            if (strcmp(entry->name, name) == 0) {
                return entry;
            }
            entry = entry->next;
        }
        scope = scope->parent;
    }
    return NULL;
}

DataType check_expression(ASTNode *node) {
    if (!node) return TYPE_UNKNOWN;
    
    switch (node->type) {
        case NODE_LITERAL:
            // Simple heuristic: if contains '.', it's float, else int
            if (node->value && strchr(node->value, '.')) {
                node->data_type = TYPE_FLOAT;
                return TYPE_FLOAT;
            }
            node->data_type = TYPE_INT;
            return TYPE_INT;
            
        case NODE_IDENTIFIER: {
            SymbolEntry *entry = lookup_symbol(node->value);
            if (!entry) {
                report_error(ERROR_UNDECLARED, node->line, node->column,
                           "Undeclared variable '%s'", node->value);
                return TYPE_UNKNOWN;
            }
            if (!entry->is_initialized) {
                report_warning(node->line, node->column,
                             "Variable '%s' used before initialization", node->value);
            }
            node->data_type = entry->type;
            return entry->type;
        }
        
        case NODE_BINOP: {
            if (node->child_count < 2) return TYPE_UNKNOWN;
            
            DataType left_type = check_expression(node->children[0]);
            DataType right_type = check_expression(node->children[1]);
            
            if (left_type == TYPE_UNKNOWN || right_type == TYPE_UNKNOWN) {
                return TYPE_UNKNOWN;
            }
            
            if (left_type == TYPE_VOID || right_type == TYPE_VOID) {
                report_error(ERROR_TYPE, node->line, node->column,
                             "Cannot use a void value in '%s' expression", node->value);
                return TYPE_UNKNOWN;
            }
            if (node->value && strcmp(node->value, "%") == 0
                && (left_type != TYPE_INT || right_type != TYPE_INT)) {
                report_error(ERROR_TYPE, node->line, node->column,
                             "Operator '%%' requires int operands (got %s and %s)",
                             type_name(left_type), type_name(right_type));
                return TYPE_UNKNOWN;
            }
            if (node->value && strcmp(node->value, "/") == 0 && right_type == TYPE_INT
                && node->children[1]->type == NODE_LITERAL
                && strcmp(node->children[1]->value, "0") == 0) {
                report_warning(node->line, node->column, "Division by zero");
            }
            /* Comparisons always yield int */
            if (node->value && strchr("<>=!", node->value[0])) {
                node->data_type = TYPE_INT;
                return TYPE_INT;
            }
            
            // Result type is the "larger" type
            DataType result = (left_type == TYPE_FLOAT || right_type == TYPE_FLOAT) 
                            ? TYPE_FLOAT : TYPE_INT;
            node->data_type = result;
            return result;
        }
        
        case NODE_UNOP: {
            if (node->child_count < 1) return TYPE_UNKNOWN;
            DataType t = check_expression(node->children[0]);
            node->data_type = t;
            return t;
        }

        case NODE_CALL: {
            FunctionEntry *f = lookup_function(node->value);
            /* Type-check arguments regardless, so undeclared vars inside are reported */
            DataType arg_types[MAX_PARAMS];
            for (int i = 0; i < node->child_count && i < MAX_PARAMS; i++) {
                arg_types[i] = check_expression(node->children[i]);
            }
            if (!f) {
                report_error(ERROR_UNDECLARED, node->line, node->column,
                             "Call to undeclared function '%s'", node->value);
                return TYPE_UNKNOWN;
            }
            if (node->child_count != f->param_count) {
                report_error(ERROR_SEMANTIC, node->line, node->column,
                             "Function '%s' expects %d argument(s) but %d given",
                             f->name, f->param_count, node->child_count);
            } else {
                for (int i = 0; i < f->param_count && i < MAX_PARAMS; i++) {
                    if (!is_assignable(f->param_types[i], arg_types[i])) {
                        report_error(ERROR_TYPE, node->line, node->column,
                                     "Argument %d of '%s' expects %s but got %s",
                                     i + 1, f->name, type_name(f->param_types[i]),
                                     type_name(arg_types[i]));
                    }
                }
            }
            node->data_type = f->return_type;
            return f->return_type;
        }
            
        default:
            return TYPE_UNKNOWN;
    }
}

static void check_statement(ASTNode *node) {
    if (!node) return;
    
    switch (node->type) {
        case NODE_DECLARATION: {
            if (node->child_count >= 1) {
                ASTNode *id_node = node->children[0];
                if (id_node && id_node->value) {
                    DataType var_type = TYPE_INT; // Default to int
                    if (node->value) {
                        if (strcmp(node->value, "int") == 0) var_type = TYPE_INT;
                        else if (strcmp(node->value, "float") == 0) var_type = TYPE_FLOAT;
                    }
                    
                    add_symbol(id_node->value, var_type, node->line);
                    
                    if (node->child_count >= 2) {
                        DataType expr_type = check_expression(node->children[1]);
                        if (!is_assignable(var_type, expr_type)) {
                            report_error(ERROR_TYPE, node->line, node->column,
                                         "Cannot initialize %s variable '%s' with a %s value "
                                         "(implicit narrowing)",
                                         type_name(var_type), id_node->value,
                                         type_name(expr_type));
                        }
                        
                        SymbolEntry *entry = lookup_symbol(id_node->value);
                        if (entry) entry->is_initialized = 1;
                    }
                }
            }
            break;
        }
        
        case NODE_ASSIGNMENT: {
            if (node->child_count >= 2) {
                ASTNode *id_node = node->children[0];
                if (id_node && id_node->value) {
                    SymbolEntry *entry = lookup_symbol(id_node->value);
                    if (!entry) {
                        report_error(ERROR_UNDECLARED, node->line, node->column,
                                   "Assignment to undeclared variable '%s'", id_node->value);
                    } else {
                        DataType expr_type = check_expression(node->children[1]);
                        if (!is_assignable(entry->type, expr_type)) {
                            report_error(ERROR_TYPE, node->line, node->column,
                                         "Cannot assign a %s value to %s variable '%s' "
                                         "(implicit narrowing)",
                                         type_name(expr_type), type_name(entry->type),
                                         id_node->value);
                        }
                        entry->is_initialized = 1;
                    }
                }
            }
            break;
        }
        
        case NODE_RETURN: {
            DataType ret = TYPE_VOID;
            if (node->child_count > 0) {
                ret = check_expression(node->children[0]);
            }
            if (current_function) {
                DataType expected = current_function->return_type;
                if (expected == TYPE_VOID && node->child_count > 0) {
                    report_error(ERROR_TYPE, node->line, node->column,
                                 "void function '%s' should not return a value",
                                 current_function->name);
                } else if (expected != TYPE_VOID && node->child_count == 0) {
                    report_error(ERROR_TYPE, node->line, node->column,
                                 "Function '%s' must return a %s value",
                                 current_function->name, type_name(expected));
                } else if (expected != TYPE_VOID && !is_assignable(expected, ret)) {
                    report_error(ERROR_TYPE, node->line, node->column,
                                 "Function '%s' returns %s but declared %s",
                                 current_function->name, type_name(ret), type_name(expected));
                }
            }
            break;
        }

        case NODE_FUNCTION: {
            current_function = lookup_function(node->value);
            enter_scope();
            ASTNode *params[MAX_PARAMS];
            int count = 0;
            if (node->child_count > 1) collect_params(node->children[0], params, &count, MAX_PARAMS);
            for (int i = 0; i < count; i++) {
                add_symbol(params[i]->value, params[i]->data_type, params[i]->line);
                SymbolEntry *e = lookup_symbol(params[i]->value);
                if (e) e->is_initialized = 1;
            }
            /* body is always the last child */
            if (node->child_count > 0) {
                check_statement(node->children[node->child_count - 1]);
            }
            exit_scope();
            current_function = NULL;
            break;
        }

        case NODE_FOR:
            /* children: init, cond, update, body */
            enter_scope();
            if (node->child_count > 0) check_statement(node->children[0]);
            if (node->child_count > 1 && node->children[1]->type != NODE_BLOCK)
                check_expression(node->children[1]);
            if (node->child_count > 2) check_statement(node->children[2]);
            if (node->child_count > 3) check_statement(node->children[3]);
            exit_scope();
            break;
            
        case NODE_IF:
        case NODE_WHILE:
            if (node->child_count > 0) {
                check_expression(node->children[0]);
            }
            if (node->child_count > 1) {
                check_statement(node->children[1]);
            }
            if (node->child_count > 2) {
                check_statement(node->children[2]);
            }
            break;
            
        case NODE_BLOCK:
            enter_scope();
            for (int i = 0; i < node->child_count; i++) {
                check_statement(node->children[i]);
            }
            exit_scope();
            break;
            
        case NODE_EXPR_STMT:
            if (node->child_count > 0) {
                check_expression(node->children[0]);
            }
            break;
            
        default:
            for (int i = 0; i < node->child_count; i++) {
                check_statement(node->children[i]);
            }
            break;
    }
}

int semantic_check(ASTNode *root) {
    if (!root) return 0;
    
    if (root->type == NODE_PROGRAM) {
        for (int i = 0; i < root->child_count; i++) {
            if (root->children[i]->type == NODE_FUNCTION) register_function(root->children[i]);
        }
        if (!lookup_function("main")) {
            report_warning(1, 0, "No 'main' function defined");
        }
        for (int i = 0; i < root->child_count; i++) {
            check_statement(root->children[i]);
        }
    } else {
        check_statement(root);
    }
    
    return !has_errors();
}

void free_symbol_table(SymbolTable *table) {
    if (!table) return;
    
    SymbolEntry *curr = table->head;
    while (curr) {
        SymbolEntry *next = curr->next;
        free(curr->name);
        free(curr);
        curr = next;
    }
    free(table);
}