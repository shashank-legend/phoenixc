/*
 * PhoenixC TAC optimizer
 *
 * A small, safe peephole optimizer that runs over the three-address code
 * produced by the ICG. Temporaries (t0, t1, ...) are assigned exactly once by
 * the generator, which lets us propagate their values freely.
 *
 * Passes (repeated until nothing changes):
 *   1. Constant folding        t0 = 2 * 3        ->  t0 = 6
 *   2. Algebraic identities    t1 = x + 0        ->  t1 = x
 *                              t2 = x * 0        ->  t2 = 0
 *   3. Copy/const propagation  t0 = 6; y = t0    ->  y = 6
 *   4. Dead temp elimination   removes temps that are never read
 */
#include "optimize.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int is_temp(const char *s) {
    if (!s || s[0] != 't' || !s[1]) return 0;
    for (const char *p = s + 1; *p; p++) {
        if (!isdigit((unsigned char)*p)) return 0;
    }
    return 1;
}

/* Integer literal (optionally negative). Floats are left alone on purpose. */
static int is_int_literal(const char *s, long *out) {
    if (!s || !*s) return 0;
    const char *p = s;
    if (*p == '-') p++;
    if (!*p) return 0;
    for (; *p; p++) {
        if (!isdigit((unsigned char)*p)) return 0;
    }
    if (out) *out = strtol(s, NULL, 10);
    return 1;
}

static int is_binary(TAC_OP op) {
    switch (op) {
        case TAC_ADD: case TAC_SUB: case TAC_MUL: case TAC_DIV: case TAC_MOD:
        case TAC_LT: case TAC_GT: case TAC_LE: case TAC_GE: case TAC_EQ: case TAC_NE:
            return 1;
        default:
            return 0;
    }
}

static void set_str(char **slot, const char *value) {
    char *copy = value ? strdup(value) : NULL;
    free(*slot);
    *slot = copy;
}

static void to_assign(TAC *t, const char *value) {
    t->op = TAC_ASSIGN;
    set_str(&t->arg1, value);
    set_str(&t->arg2, NULL);
}

static int fold(TAC *t) {
    long a, b;
    if (t->op == TAC_NEG && is_int_literal(t->arg1, &a)) {
        char buf[32];
        snprintf(buf, sizeof(buf), "%ld", -a);
        to_assign(t, buf);
        return 1;
    }
    if (!is_binary(t->op)) return 0;

    int la = is_int_literal(t->arg1, &a);
    int lb = is_int_literal(t->arg2, &b);

    if (la && lb) {
        long r;
        switch (t->op) {
            case TAC_ADD: r = a + b; break;
            case TAC_SUB: r = a - b; break;
            case TAC_MUL: r = a * b; break;
            case TAC_DIV: if (b == 0) return 0; r = a / b; break;
            case TAC_MOD: if (b == 0) return 0; r = a % b; break;
            case TAC_LT: r = a < b; break;
            case TAC_GT: r = a > b; break;
            case TAC_LE: r = a <= b; break;
            case TAC_GE: r = a >= b; break;
            case TAC_EQ: r = a == b; break;
            case TAC_NE: r = a != b; break;
            default: return 0;
        }
        char buf[32];
        snprintf(buf, sizeof(buf), "%ld", r);
        to_assign(t, buf);
        return 1;
    }

    /* Algebraic identities (one side constant) */
    if (lb && b == 0 && (t->op == TAC_ADD || t->op == TAC_SUB)) { to_assign(t, t->arg1); return 1; }
    if (la && a == 0 && t->op == TAC_ADD)                         { to_assign(t, t->arg2); return 1; }
    if (lb && b == 1 && (t->op == TAC_MUL || t->op == TAC_DIV))   { to_assign(t, t->arg1); return 1; }
    if (la && a == 1 && t->op == TAC_MUL)                         { to_assign(t, t->arg2); return 1; }
    if (((la && a == 0) || (lb && b == 0)) && t->op == TAC_MUL)   { to_assign(t, "0");     return 1; }
    return 0;
}

static int replace_uses(TAC *code, const char *name, const char *value) {
    int changed = 0;
    for (TAC *c = code; c; c = c->next) {
        if (c->arg1 && c->op != TAC_CALL && strcmp(c->arg1, name) == 0) {
            set_str(&c->arg1, value); changed = 1;
        }
        if (c->arg2 && strcmp(c->arg2, name) == 0 && c->op != TAC_CALL) {
            set_str(&c->arg2, value); changed = 1;
        }
        /* `param` and `return` keep their operand in `result` */
        if ((c->op == TAC_PARAM || c->op == TAC_RETURN) && c->result
            && strcmp(c->result, name) == 0) {
            set_str(&c->result, value); changed = 1;
        }
    }
    return changed;
}

static int is_used(TAC *code, const char *name) {
    for (TAC *c = code; c; c = c->next) {
        if (c->arg1 && strcmp(c->arg1, name) == 0) return 1;
        if (c->arg2 && strcmp(c->arg2, name) == 0) return 1;
        if ((c->op == TAC_PARAM || c->op == TAC_RETURN) && c->result
            && strcmp(c->result, name) == 0) return 1;
    }
    return 0;
}

TAC* optimize_tac(TAC *code, OptStats *stats) {
    int changed = 1;
    while (changed) {
        changed = 0;

        for (TAC *c = code; c; c = c->next) {
            if (fold(c)) { changed = 1; if (stats) stats->folded++; }
        }

        /* Propagate `tN = <literal|var>` into later uses */
        for (TAC *c = code; c; c = c->next) {
            if (c->op == TAC_ASSIGN && is_temp(c->result) && c->arg1) {
                char *name = strdup(c->result);
                char *value = strdup(c->arg1);
                if (replace_uses(c->next, name, value)) {
                    changed = 1;
                    if (stats) stats->propagated++;
                }
                free(name);
                free(value);
            }
        }

        /* Drop temp assignments whose result is never read */
        TAC *prev = NULL, *c = code;
        while (c) {
            TAC *next = c->next;
            int pure = c->op == TAC_ASSIGN || c->op == TAC_NEG || is_binary(c->op);
            if (pure && is_temp(c->result) && !is_used(code, c->result)) {
                if (prev) prev->next = next; else code = next;
                c->next = NULL;
                free_tac(c);
                changed = 1;
                if (stats) stats->eliminated++;
            } else {
                prev = c;
            }
            c = next;
        }
    }
    return code;
}
