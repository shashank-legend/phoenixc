#ifndef ERROR_H
#define ERROR_H

#include <stdio.h>

typedef enum {
    ERROR_LEXICAL,
    ERROR_SYNTAX,
    ERROR_SEMANTIC,
    ERROR_TYPE,
    ERROR_UNDECLARED,
    ERROR_REDECLARED,
    ERROR_WARNING
} ErrorType;

typedef struct ErrorInfo {
    ErrorType type;
    int line;
    int column;
    char *message;
    char *code_snippet;
    struct ErrorInfo *next;
} ErrorInfo;

// Global error list
extern ErrorInfo *error_list;
extern int error_count;
extern int warning_count;

// Function declarations
void init_error_system();
void report_error(ErrorType type, int line, int column, const char *format, ...);
void report_warning(int line, int column, const char *format, ...);
void write_errors_to_json(const char *filename);
void free_errors();
int has_errors();

#endif // ERROR_H