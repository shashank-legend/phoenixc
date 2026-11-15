%{
#include "ast/ast.h"
#include "parser.tab.h"
#include "error/error.h"
#include <string.h>

int yycolumn = 1;

#define YY_USER_ACTION \
    yylloc.first_line = yylloc.last_line = yylineno; \
    yylloc.first_column = yycolumn; \
    yylloc.last_column = yycolumn + yyleng - 1; \
    yycolumn += yyleng;
%}

%option yylineno
%option noyywrap

DIGIT    [0-9]
LETTER   [a-zA-Z_]
ID       {LETTER}({LETTER}|{DIGIT})*
INT      {DIGIT}+
FLOAT    {DIGIT}+\.{DIGIT}+
WS       [ \t\r]+
NEWLINE  \n

%%

"int"       { return INT_TYPE; }
"float"     { return FLOAT_TYPE; }
"void"      { return VOID_TYPE; }
"return"    { return RETURN; }
"if"        { return IF; }
"else"      { return ELSE; }
"while"     { return WHILE; }
"for"       { return FOR; }

{ID}        { yylval.str = strdup(yytext); return IDENTIFIER; }
{INT}       { yylval.str = strdup(yytext); return INT_LITERAL; }
{FLOAT}     { yylval.str = strdup(yytext); return FLOAT_LITERAL; }

"+"         { return PLUS; }
"-"         { return MINUS; }
"*"         { return MULTIPLY; }
"/"         { return DIVIDE; }
"%"         { return MODULO; }
"="         { return ASSIGN; }
"=="        { return EQ; }
"!="        { return NE; }
"<"         { return LT; }
">"         { return GT; }
"<="        { return LE; }
">="        { return GE; }

"("         { return LPAREN; }
")"         { return RPAREN; }
"{"         { return LBRACE; }
"}"         { return RBRACE; }
";"         { return SEMICOLON; }
","         { return COMMA; }

{WS}        { /* skip whitespace */ }
{NEWLINE}   { yycolumn = 1; }

"//".*      { /* skip single-line comments */ }
"/*"        { 
    int c;
    while ((c = input()) != 0) {
        if (c == '\n') yycolumn = 1;
        else if (c == '*') {
            if ((c = input()) == '/') break;
            unput(c);
        }
    }
}

.           { 
    report_error(ERROR_LEXICAL, yylineno, yycolumn, 
                "Unexpected character: '%s'", yytext); 
}

%%