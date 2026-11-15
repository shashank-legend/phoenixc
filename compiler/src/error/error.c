#include "error.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

ErrorInfo *error_list = NULL;
ErrorInfo *error_tail = NULL;
int error_count = 0;
int warning_count = 0;

void init_error_system() {
    error_list = NULL;
    error_tail = NULL;
    error_count = 0;
    warning_count = 0;
}

static const char* error_type_to_string(ErrorType type) {
    switch(type) {
        case ERROR_LEXICAL: return "LEXICAL_ERROR";
        case ERROR_SYNTAX: return "SYNTAX_ERROR";
        case ERROR_SEMANTIC: return "SEMANTIC_ERROR";
        case ERROR_TYPE: return "TYPE_ERROR";
        case ERROR_UNDECLARED: return "UNDECLARED_ERROR";
        case ERROR_REDECLARED: return "REDECLARED_ERROR";
        case ERROR_WARNING: return "WARNING";
        default: return "UNKNOWN_ERROR";
    }
}

void report_error(ErrorType type, int line, int column, const char *format, ...) {
    ErrorInfo *err = (ErrorInfo*)malloc(sizeof(ErrorInfo));
    err->type = type;
    err->line = line;
    err->column = column;
    err->next = NULL;
    
    char buffer[1024];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    err->message = strdup(buffer);
    err->code_snippet = NULL;
    
    if (error_tail) {
        error_tail->next = err;
        error_tail = err;
    } else {
        error_list = error_tail = err;
    }
    
    if (type != ERROR_WARNING) {
        error_count++;
    } else {
        warning_count++;
    }
    
    fprintf(stderr, "[%s] Line %d, Column %d: %s\n", 
            error_type_to_string(type), line, column, buffer);
}

void report_warning(int line, int column, const char *format, ...) {
    char buffer[1024];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    report_error(ERROR_WARNING, line, column, "%s", buffer);
}

static char* escape_json_string(const char *str) {
    if (!str) return strdup("");
    
    size_t len = strlen(str);
    char *escaped = (char*)malloc(len * 2 + 1);
    size_t j = 0;
    
    for (size_t i = 0; i < len; i++) {
        if (str[i] == '"' || str[i] == '\\') {
            escaped[j++] = '\\';
        }
        if (str[i] == '\n') {
            escaped[j++] = '\\';
            escaped[j++] = 'n';
            continue;
        }
        if (str[i] == '\t') {
            escaped[j++] = '\\';
            escaped[j++] = 't';
            continue;
        }
        escaped[j++] = str[i];
    }
    escaped[j] = '\0';
    return escaped;
}

void write_errors_to_json(const char *filename) {
    FILE *fp = fopen(filename, "w");
    if (!fp) {
        fprintf(stderr, "Failed to open error file: %s\n", filename);
        return;
    }
    
    fprintf(fp, "{\n");
    fprintf(fp, "  \"error_count\": %d,\n", error_count);
    fprintf(fp, "  \"warning_count\": %d,\n", warning_count);
    fprintf(fp, "  \"errors\": [\n");
    
    ErrorInfo *curr = error_list;
    int first = 1;
    
    while (curr) {
        if (!first) fprintf(fp, ",\n");
        first = 0;
        
        char *escaped_msg = escape_json_string(curr->message);
        char *escaped_snippet = escape_json_string(curr->code_snippet);
        
        fprintf(fp, "    {\n");
        fprintf(fp, "      \"type\": \"%s\",\n", error_type_to_string(curr->type));
        fprintf(fp, "      \"line\": %d,\n", curr->line);
        fprintf(fp, "      \"column\": %d,\n", curr->column);
        fprintf(fp, "      \"message\": \"%s\",\n", escaped_msg);
        fprintf(fp, "      \"code_snippet\": \"%s\"\n", escaped_snippet);
        fprintf(fp, "    }");
        
        free(escaped_msg);
        free(escaped_snippet);
        curr = curr->next;
    }
    
    fprintf(fp, "\n  ]\n");
    fprintf(fp, "}\n");
    
    fclose(fp);
}

void free_errors() {
    ErrorInfo *curr = error_list;
    while (curr) {
        ErrorInfo *next = curr->next;
        free(curr->message);
        free(curr->code_snippet);
        free(curr);
        curr = next;
    }
    error_list = error_tail = NULL;
    error_count = warning_count = 0;
}

int has_errors() {
    return error_count > 0;
}