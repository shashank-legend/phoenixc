#ifndef OPTIMIZE_H
#define OPTIMIZE_H

#include "tac.h"

typedef struct {
    int folded;
    int propagated;
    int eliminated;
} OptStats;

/* Runs constant folding, propagation and dead-temp elimination. Returns the new list head. */
TAC* optimize_tac(TAC *code, OptStats *stats);

#endif // OPTIMIZE_H
