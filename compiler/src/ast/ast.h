#ifndef AST_H
#define AST_H

typedef enum {
    NODE_PROGRAM,
    NODE_FUNCTION,
    NODE_DECLARATION,
    NODE_ASSIGNMENT,
    NODE_RETURN,
    NODE_IF,
    NODE_WHILE,
    NODE_FOR,
    NODE_BINOP,
    NODE_UNOP,
    NODE_IDENTIFIER,
    NODE_LITERAL,
    NODE_BLOCK,
    NODE_EXPR_STMT,
    NODE_CALL,
    NODE_PARAM,
    NODE_ARG
} NodeType;

typedef enum {
    TYPE_INT,
    TYPE_FLOAT,
    TYPE_VOID,
    TYPE_UNKNOWN
} DataType;

typedef struct ASTNode {
    NodeType type;
    char *label;
    char *value;
    DataType data_type;
    int line;
    int column;
    struct ASTNode **children;
    int child_count;
    int child_capacity;
} ASTNode;

// External declarations for bison
extern int yylineno;
extern int yycolumn;

// Function declarations
ASTNode* create_node(NodeType type, const char *label, const char *value);
void add_child(ASTNode *parent, ASTNode *child);
void print_ast(ASTNode *node, int depth);
void free_ast(ASTNode *node);
ASTNode* create_binop_node(const char *op, ASTNode *left, ASTNode *right);
ASTNode* create_identifier_node(const char *name);
ASTNode* create_literal_node(const char *value);

#endif // AST_H