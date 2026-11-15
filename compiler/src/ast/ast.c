#include "ast.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

ASTNode* create_node(NodeType type, const char *label, const char *value) {
    ASTNode *node = (ASTNode*)malloc(sizeof(ASTNode));
    node->type = type;
    node->label = label ? strdup(label) : NULL;
    node->value = value ? strdup(value) : NULL;
    node->data_type = TYPE_UNKNOWN;
    node->line = yylineno;
    node->column = yycolumn;
    node->children = NULL;
    node->child_count = 0;
    node->child_capacity = 0;
    return node;
}

void add_child(ASTNode *parent, ASTNode *child) {
    if (!parent || !child) return;
    
    if (parent->child_count >= parent->child_capacity) {
        parent->child_capacity = parent->child_capacity == 0 ? 4 : parent->child_capacity * 2;
        parent->children = (ASTNode**)realloc(parent->children, 
                                              parent->child_capacity * sizeof(ASTNode*));
    }
    
    parent->children[parent->child_count++] = child;
}

ASTNode* create_binop_node(const char *op, ASTNode *left, ASTNode *right) {
    ASTNode *node = create_node(NODE_BINOP, "BinaryOp", op);
    add_child(node, left);
    add_child(node, right);
    return node;
}

ASTNode* create_identifier_node(const char *name) {
    return create_node(NODE_IDENTIFIER, "Identifier", name);
}

ASTNode* create_literal_node(const char *value) {
    return create_node(NODE_LITERAL, "Literal", value);
}

static const char* node_type_to_string(NodeType type) {
    switch(type) {
        case NODE_PROGRAM: return "Program";
        case NODE_FUNCTION: return "Function";
        case NODE_DECLARATION: return "Declaration";
        case NODE_ASSIGNMENT: return "Assignment";
        case NODE_RETURN: return "Return";
        case NODE_IF: return "If";
        case NODE_WHILE: return "While";
        case NODE_FOR: return "For";
        case NODE_BINOP: return "BinaryOp";
        case NODE_UNOP: return "UnaryOp";
        case NODE_IDENTIFIER: return "Identifier";
        case NODE_LITERAL: return "Literal";
        case NODE_BLOCK: return "Block";
        case NODE_EXPR_STMT: return "ExprStmt";
        case NODE_CALL: return "Call";
        case NODE_PARAM: return "Param";
        case NODE_ARG: return "Arg";
        default: return "Unknown";
    }
}

void print_ast(ASTNode *node, int depth) {
    if (!node) return;
    
    for (int i = 0; i < depth; i++) printf("  ");
    
    printf("%s", node_type_to_string(node->type));
    if (node->label && strcmp(node->label, node_type_to_string(node->type)) != 0) {
        printf("[%s]", node->label);
    }
    if (node->value) {
        printf(": %s", node->value);
    }
    printf("\n");
    
    for (int i = 0; i < node->child_count; i++) {
        print_ast(node->children[i], depth + 1);
    }
}

void free_ast(ASTNode *node) {
    if (!node) return;
    
    for (int i = 0; i < node->child_count; i++) {
        free_ast(node->children[i]);
    }
    
    free(node->label);
    free(node->value);
    free(node->children);
    free(node);
}