#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include "ast/ast.h"
#include "semantic/semantic.h"
#include "icg/icg.h"
#include "icg/tac.h"
#include "icg/optimize.h"

#define PHOENIXC_VERSION "2.0.0"
#include "error/error.h"

extern FILE *yyin;
extern int yyparse();
extern ASTNode *root;

static void usage(FILE *out, const char *prog) {
    fprintf(out, "PhoenixC %s - a self-healing Mini C compiler\n\n", PHOENIXC_VERSION);
    fprintf(out, "Usage: %s <input_file> [options]\n\n", prog);
    fprintf(out, "Options:\n");
    fprintf(out, "  --print-ast        Print the Abstract Syntax Tree\n");
    fprintf(out, "  --print-tac        Print Three-Address Code\n");
    fprintf(out, "  -O, --optimize     Run the TAC optimizer (folding, propagation, DCE)\n");
    fprintf(out, "  --output <dir>     Output directory (default: .)\n");
    fprintf(out, "  -h, --help         Show this help\n");
    fprintf(out, "  -v, --version      Show version\n");
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        usage(stderr, argv[0]);
        return 1;
    }

    // Parse command line arguments
    char *input_file = NULL;
    int flag_print_ast = 0;
    int flag_print_tac = 0;
    int flag_optimize = 0;
    char *output_dir = ".";

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--print-ast") == 0) {
            flag_print_ast = 1;
        } else if (strcmp(argv[i], "--print-tac") == 0) {
            flag_print_tac = 1;
        } else if (strcmp(argv[i], "-O") == 0 || strcmp(argv[i], "--optimize") == 0) {
            flag_optimize = 1;
        } else if (strcmp(argv[i], "--output") == 0 && i + 1 < argc) {
            output_dir = argv[++i];
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            usage(stdout, argv[0]);
            return 0;
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--version") == 0) {
            printf("PhoenixC %s\n", PHOENIXC_VERSION);
            return 0;
        } else if (argv[i][0] == '-') {
            fprintf(stderr, "Unknown option: %s\n\n", argv[i]);
            usage(stderr, argv[0]);
            return 1;
        } else if (!input_file) {
            input_file = argv[i];
        }
    }

    if (!input_file) {
        usage(stderr, argv[0]);
        return 1;
    }
    
    if (mkdir(output_dir, 0755) != 0 && errno != EEXIST) {
        fprintf(stderr, "Error: Cannot create output directory '%s'\n", output_dir);
        return 1;
    }

    // Initialize error system
    init_error_system();
    
    // Open input file
    yyin = fopen(input_file, "r");
    if (!yyin) {
        fprintf(stderr, "Error: Cannot open file '%s'\n", input_file);
        return 1;
    }
    
    printf("=== PhoenixC %s ===\n", PHOENIXC_VERSION);
    printf("Input file: %s\n\n", input_file);
    
    // Phase 1: Lexical and Syntax Analysis
    printf("Phase 1: Lexical and Syntax Analysis...\n");
    int parse_result = yyparse();
    fclose(yyin);
    
    if (parse_result != 0 || has_errors()) {
        printf("✗ Parsing failed with %d error(s)\n\n", error_count);
        
        char error_file[512];
        snprintf(error_file, sizeof(error_file), "%s/errors.json", output_dir);
        write_errors_to_json(error_file);
        printf("Errors written to: %s\n", error_file);
        
        free_errors();
        return 1;
    }
    
    printf("✓ Parsing completed successfully\n\n");
    
    // Print AST if requested
    if (flag_print_ast) {
        printf("Abstract Syntax Tree:\n");
        print_ast(root, 0);
        printf("\n");
    }
    
    // Phase 2: Semantic Analysis
    printf("Phase 2: Semantic Analysis...\n");
    init_semantic_analysis();
    int semantic_ok = semantic_check(root);
    
    if (!semantic_ok || has_errors()) {
        printf("✗ Semantic analysis failed with %d error(s) and %d warning(s)\n\n", 
               error_count, warning_count);
        
        char error_file[512];
        snprintf(error_file, sizeof(error_file), "%s/errors.json", output_dir);
        write_errors_to_json(error_file);
        printf("Errors written to: %s\n", error_file);
        
        free_ast(root);
        free_symbol_table(global_scope);
        free_errors();
        return 1;
    }
    
    if (warning_count > 0) {
        printf("✓ Semantic analysis completed with %d warning(s)\n\n", warning_count);
    } else {
        printf("✓ Semantic analysis completed successfully\n\n");
    }
    
    // Phase 3: Intermediate Code Generation
    printf("Phase 3: Intermediate Code Generation...\n");
    TAC *tac_code = generate_tac(root);
    
    if (!tac_code) {
        printf("✗ TAC generation failed\n\n");
        
        char error_file[512];
        snprintf(error_file, sizeof(error_file), "%s/errors.json", output_dir);
        write_errors_to_json(error_file);
        printf("Errors written to: %s\n", error_file);
        
        free_ast(root);
        free_symbol_table(global_scope);
        free_errors();
        return 1;
    }
    
    printf("✓ TAC generation completed (%d instructions)\n\n", count_tac(tac_code));

    if (flag_optimize) {
        printf("Phase 4: Optimization...\n");
        int before = count_tac(tac_code);
        OptStats stats = {0, 0, 0};
        tac_code = optimize_tac(tac_code, &stats);
        int after = count_tac(tac_code);
        printf("✓ Optimized %d -> %d instructions "
               "(%d folded, %d propagated, %d eliminated)\n\n",
               before, after, stats.folded, stats.propagated, stats.eliminated);
    }
    
    // Print TAC if requested
    if (flag_print_tac) {
        printf("Three-Address Code:\n");
        print_tac(tac_code);
        printf("\n");
    }
    
    // Write TAC to file
    char tac_file[512];
    snprintf(tac_file, sizeof(tac_file), "%s/output.tac", output_dir);
    write_tac_to_file(tac_code, tac_file);
    printf("TAC written to: %s\n", tac_file);
    
    // Write success status (for Python healer)
    char status_file[512];
    snprintf(status_file, sizeof(status_file), "%s/compile_status.json", output_dir);
    FILE *status_fp = fopen(status_file, "w");
    if (status_fp) {
        fprintf(status_fp, "{\n");
        fprintf(status_fp, "  \"success\": true,\n");
        fprintf(status_fp, "  \"error_count\": 0,\n");
        fprintf(status_fp, "  \"warning_count\": %d,\n", warning_count);
        fprintf(status_fp, "  \"input_file\": \"%s\",\n", input_file);
        fprintf(status_fp, "  \"tac_file\": \"%s\"\n", tac_file);
        fprintf(status_fp, "}\n");
        fclose(status_fp);
    }
    
    printf("\n=== Compilation Successful ===\n");
    
    // Cleanup
    free_tac(tac_code);
    free_ast(root);
    free_symbol_table(global_scope);
    free_errors();
    
    return 0;
}