#ifndef ICG_H
#define ICG_H

#include "ast/ast.h"
#include "tac.h"


// Function declarations
TAC* generate_tac(ASTNode *root);
TAC* gen_expr(ASTNode *node, char **result_var);
TAC* gen_stmt(ASTNode *node);

#endif // ICG_H