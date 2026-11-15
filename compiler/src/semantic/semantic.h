#ifndef SEMANTIC_H
#define SEMANTIC_H

#include "ast/ast.h"

typedef struct SymbolEntry {
    char *name;
    DataType type;
    int line;
    int is_initialized;
    struct SymbolEntry *next;
} SymbolEntry;

typedef struct SymbolTable {
    SymbolEntry *head;
    struct SymbolTable *parent;
} SymbolTable;

// Global symbol table
extern SymbolTable *global_scope;
extern SymbolTable *current_scope;

// Function declarations
void init_semantic_analysis();
SymbolTable* create_scope(SymbolTable *parent);
void enter_scope();
void exit_scope();
void add_symbol(const char *name, DataType type, int line);
SymbolEntry* lookup_symbol(const char *name);
int semantic_check(ASTNode *root);
DataType check_expression(ASTNode *node);
void free_symbol_table(SymbolTable *table);
const char* type_name(DataType t);

#endif // SEMANTIC_H