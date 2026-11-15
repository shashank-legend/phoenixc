%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast/ast.h"
#include "error/error.h"

extern int yylex();
extern int yylineno;
extern int yycolumn;
extern FILE *yyin;

void yyerror(const char *s);

ASTNode *root = NULL;
%}

%union {
    char *str;
    struct ASTNode *node;
}

%token <str> IDENTIFIER INT_LITERAL FLOAT_LITERAL
%token INT_TYPE FLOAT_TYPE VOID_TYPE
%token RETURN IF ELSE WHILE FOR
%token PLUS MINUS MULTIPLY DIVIDE MODULO
%token ASSIGN EQ NE LT GT LE GE
%token LPAREN RPAREN LBRACE RBRACE SEMICOLON COMMA

%type <node> program function declaration statement statement_list
%type <node> expression assignment return_stmt if_stmt while_stmt block
%type <node> expr_stmt comparison_expr additive_expr multiplicative_expr
%type <node> for_stmt for_init for_update unary_expr opt_expression
%type <node> primary_expr param_list param arg_list
%type <str> type_specifier

%locations

%nonassoc LOWER_THAN_ELSE
%nonassoc ELSE

%start program

%%

program
    : function { 
        root = create_node(NODE_PROGRAM, "Program", NULL);
        add_child(root, $1);
        $$ = root;
    }
    | program function {
        add_child($1, $2);
        $$ = $1;
    }
    ;

type_specifier
    : INT_TYPE { $$ = strdup("int"); }
    | FLOAT_TYPE { $$ = strdup("float"); }
    | VOID_TYPE { $$ = strdup("void"); }
    ;

function
    : type_specifier IDENTIFIER LPAREN param_list RPAREN block {
        ASTNode *func = create_node(NODE_FUNCTION, "Function", $2);
        func->data_type = strcmp($1, "int") == 0 ? TYPE_INT : 
                         strcmp($1, "float") == 0 ? TYPE_FLOAT : TYPE_VOID;
        if ($4) add_child(func, $4);
        add_child(func, $6);
        $$ = func;
        free($1);
        free($2);
    }
    | type_specifier IDENTIFIER LPAREN RPAREN block {
        ASTNode *func = create_node(NODE_FUNCTION, "Function", $2);
        func->data_type = strcmp($1, "int") == 0 ? TYPE_INT : 
                         strcmp($1, "float") == 0 ? TYPE_FLOAT : TYPE_VOID;
        add_child(func, $5);
        $$ = func;
        free($1);
        free($2);
    }
    ;

param_list
    : param { $$ = $1; }
    | param_list COMMA param {
        if ($1 == NULL) {
            $$ = $3;
        } else {
            ASTNode *params = create_node(NODE_PARAM, "Params", NULL);
            add_child(params, $1);
            add_child(params, $3);
            $$ = params;
        }
    }
    ;

param
    : type_specifier IDENTIFIER {
        ASTNode *param = create_node(NODE_PARAM, "Param", $2);
        param->data_type = strcmp($1, "int") == 0 ? TYPE_INT : TYPE_FLOAT;
        $$ = param;
        free($1);
        free($2);
    }
    ;

block
    : LBRACE statement_list RBRACE { $$ = $2; }
    | LBRACE RBRACE { 
        $$ = create_node(NODE_BLOCK, "Block", NULL);
    }
    ;

statement_list
    : statement {
        ASTNode *block = create_node(NODE_BLOCK, "Block", NULL);
        if ($1) add_child(block, $1);
        $$ = block;
    }
    | statement_list statement {
        if ($2) add_child($1, $2);
        $$ = $1;
    }
    ;

statement
    : declaration { $$ = $1; }
    | assignment { $$ = $1; }
    | return_stmt { $$ = $1; }
    | if_stmt { $$ = $1; }
    | while_stmt { $$ = $1; }
    | for_stmt { $$ = $1; }
    | expr_stmt { $$ = $1; }
    | block { $$ = $1; }
    ;

declaration
    : type_specifier IDENTIFIER SEMICOLON {
        ASTNode *decl = create_node(NODE_DECLARATION, "Declaration", $1);
        ASTNode *id = create_identifier_node($2);
        add_child(decl, id);
        $$ = decl;
        free($1);
        free($2);
    }
    | type_specifier IDENTIFIER ASSIGN expression SEMICOLON {
        ASTNode *decl = create_node(NODE_DECLARATION, "Declaration", $1);
        ASTNode *id = create_identifier_node($2);
        add_child(decl, id);
        add_child(decl, $4);
        $$ = decl;
        free($1);
        free($2);
    }
    ;

assignment
    : IDENTIFIER ASSIGN expression SEMICOLON {
        ASTNode *assign = create_node(NODE_ASSIGNMENT, "Assignment", NULL);
        ASTNode *id = create_identifier_node($1);
        add_child(assign, id);
        add_child(assign, $3);
        $$ = assign;
        free($1);
    }
    ;

return_stmt
    : RETURN expression SEMICOLON {
        ASTNode *ret = create_node(NODE_RETURN, "Return", NULL);
        add_child(ret, $2);
        $$ = ret;
    }
    | RETURN SEMICOLON {
        $$ = create_node(NODE_RETURN, "Return", NULL);
    }
    ;

if_stmt
    : IF LPAREN expression RPAREN statement %prec LOWER_THAN_ELSE {
        ASTNode *if_node = create_node(NODE_IF, "If", NULL);
        add_child(if_node, $3);
        add_child(if_node, $5);
        $$ = if_node;
    }
    | IF LPAREN expression RPAREN statement ELSE statement {
        ASTNode *if_node = create_node(NODE_IF, "If", NULL);
        add_child(if_node, $3);
        add_child(if_node, $5);
        add_child(if_node, $7);
        $$ = if_node;
    }
    ;

while_stmt
    : WHILE LPAREN expression RPAREN statement {
        ASTNode *while_node = create_node(NODE_WHILE, "While", NULL);
        add_child(while_node, $3);
        add_child(while_node, $5);
        $$ = while_node;
    }
    ;

for_stmt
    : FOR LPAREN for_init SEMICOLON opt_expression SEMICOLON for_update RPAREN statement {
        /* children: init, cond, update, body (any of the first three may be an empty Block) */
        ASTNode *for_node = create_node(NODE_FOR, "For", NULL);
        add_child(for_node, $3 ? $3 : create_node(NODE_BLOCK, "Block", NULL));
        add_child(for_node, $5 ? $5 : create_node(NODE_BLOCK, "Block", NULL));
        add_child(for_node, $7 ? $7 : create_node(NODE_BLOCK, "Block", NULL));
        add_child(for_node, $9);
        $$ = for_node;
    }
    ;

for_init
    : /* empty */ { $$ = NULL; }
    | type_specifier IDENTIFIER ASSIGN expression {
        ASTNode *decl = create_node(NODE_DECLARATION, "Declaration", $1);
        add_child(decl, create_identifier_node($2));
        add_child(decl, $4);
        $$ = decl;
        free($1);
        free($2);
    }
    | IDENTIFIER ASSIGN expression {
        ASTNode *assign = create_node(NODE_ASSIGNMENT, "Assignment", NULL);
        add_child(assign, create_identifier_node($1));
        add_child(assign, $3);
        $$ = assign;
        free($1);
    }
    ;

for_update
    : /* empty */ { $$ = NULL; }
    | IDENTIFIER ASSIGN expression {
        ASTNode *assign = create_node(NODE_ASSIGNMENT, "Assignment", NULL);
        add_child(assign, create_identifier_node($1));
        add_child(assign, $3);
        $$ = assign;
        free($1);
    }
    ;

opt_expression
    : /* empty */ { $$ = NULL; }
    | expression { $$ = $1; }
    ;

expr_stmt
    : expression SEMICOLON {
        ASTNode *stmt = create_node(NODE_EXPR_STMT, "ExprStmt", NULL);
        add_child(stmt, $1);
        $$ = stmt;
    }
    ;

expression
    : comparison_expr { $$ = $1; }
    ;

comparison_expr
    : additive_expr { $$ = $1; }
    | comparison_expr EQ additive_expr { $$ = create_binop_node("==", $1, $3); }
    | comparison_expr NE additive_expr { $$ = create_binop_node("!=", $1, $3); }
    | comparison_expr LT additive_expr { $$ = create_binop_node("<", $1, $3); }
    | comparison_expr GT additive_expr { $$ = create_binop_node(">", $1, $3); }
    | comparison_expr LE additive_expr { $$ = create_binop_node("<=", $1, $3); }
    | comparison_expr GE additive_expr { $$ = create_binop_node(">=", $1, $3); }
    ;

additive_expr
    : multiplicative_expr { $$ = $1; }
    | additive_expr PLUS multiplicative_expr { $$ = create_binop_node("+", $1, $3); }
    | additive_expr MINUS multiplicative_expr { $$ = create_binop_node("-", $1, $3); }
    ;

multiplicative_expr
    : unary_expr { $$ = $1; }
    | multiplicative_expr MULTIPLY unary_expr { $$ = create_binop_node("*", $1, $3); }
    | multiplicative_expr DIVIDE unary_expr { $$ = create_binop_node("/", $1, $3); }
    | multiplicative_expr MODULO unary_expr { $$ = create_binop_node("%", $1, $3); }
    ;

unary_expr
    : primary_expr { $$ = $1; }
    | MINUS unary_expr {
        ASTNode *neg = create_node(NODE_UNOP, "UnaryOp", "-");
        add_child(neg, $2);
        $$ = neg;
    }
    ;

primary_expr
    : IDENTIFIER { $$ = create_identifier_node($1); free($1); }
    | INT_LITERAL { $$ = create_literal_node($1); free($1); }
    | FLOAT_LITERAL { $$ = create_literal_node($1); free($1); }
    | LPAREN expression RPAREN { $$ = $2; }
    | IDENTIFIER LPAREN arg_list RPAREN {
        ASTNode *call = create_node(NODE_CALL, "Call", $1);
        if ($3) {
            for (int i = 0; i < $3->child_count; i++) {
                add_child(call, $3->children[i]);
            }
            free($3->children);
            free($3);
        }
        $$ = call;
        free($1);
    }
    | IDENTIFIER LPAREN RPAREN {
        ASTNode *call = create_node(NODE_CALL, "Call", $1);
        $$ = call;
        free($1);
    }
    ;

arg_list
    : expression {
        ASTNode *args = create_node(NODE_ARG, "Args", NULL);
        add_child(args, $1);
        $$ = args;
    }
    | arg_list COMMA expression {
        add_child($1, $3);
        $$ = $1;
    }
    ;

%%

void yyerror(const char *s) {
    report_error(ERROR_SYNTAX, yylineno, yycolumn, "%s", s);
}