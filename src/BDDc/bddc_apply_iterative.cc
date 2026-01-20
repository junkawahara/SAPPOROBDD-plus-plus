/*****************************************
*  BDD Package (SAPPORO-1.94)   - Apply Iterative *
*  (C) Shin-ichi MINATO  (Apr. 19, 2022)          *
*  Non-recursive apply using explicit stack       *
*  Used when VarUsed > APPLY_RECURSION_THRESHOLD  *
******************************************/

#include "bddc_apply_common.h"
#include <stdlib.h>

namespace sapporobdd {

/* Stack management functions */
static void stack_init(struct ApplyStack *stack) {
    stack->frames = (struct ApplyStackFrame *)malloc(
        sizeof(struct ApplyStackFrame) * APPLY_STACK_INIT_SIZE);
    stack->top = -1;
    stack->capacity = APPLY_STACK_INIT_SIZE;
}

static void stack_free(struct ApplyStack *stack) {
    if (stack->frames) {
        free(stack->frames);
        stack->frames = 0;
    }
}

static int stack_push(struct ApplyStack *stack) {
    stack->top++;
    if (stack->top >= stack->capacity) {
        int new_capacity = stack->capacity * 2;
        struct ApplyStackFrame *new_frames = (struct ApplyStackFrame *)realloc(
            stack->frames, sizeof(struct ApplyStackFrame) * new_capacity);
        if (!new_frames) {
            stack->top--;
            return 0; /* allocation failed */
        }
        stack->frames = new_frames;
        stack->capacity = new_capacity;
    }
    return 1; /* success */
}

static void stack_pop(struct ApplyStack *stack) {
    if (stack->top >= 0) stack->top--;
}

static struct ApplyStackFrame *stack_current(struct ApplyStack *stack) {
    if (stack->top >= 0) return &stack->frames[stack->top];
    return 0;
}

static struct ApplyStackFrame *stack_parent(struct ApplyStack *stack) {
    if (stack->top >= 1) return &stack->frames[stack->top - 1];
    return 0;
}

/*
 * Check terminal cases for binary operations
 * Returns 1 if terminal case found (result in *result), 0 otherwise
 * May modify f and g (operand swap, negation handling)
 */
static int check_terminal_binary(bddp *f, bddp *g, unsigned char op,
                                  unsigned char skip, bddp *result,
                                  int *need_negate) {
    struct B_NodeTable *fp;
    bddp h;

    *need_negate = 0;

    if (skip) return 0;

    switch(op) {
    case BC_AND:
        if(*f == bddfalse || *g == bddfalse || *f == B_NOT(*g)) {
            *result = bddfalse;
            return 1;
        }
        if(*f == *g) {
            if(*f != bddtrue) { fp = B_NP(*f); B_RFC_INC_NP(fp); }
            *result = *f;
            return 1;
        }
        if(*f == bddtrue) { fp = B_NP(*g); B_RFC_INC_NP(fp); *result = *g; return 1; }
        if(*g == bddtrue) { fp = B_NP(*f); B_RFC_INC_NP(fp); *result = *f; return 1; }
        if(*f < *g) { h = *f; *f = *g; *g = h; }
        break;

    case BC_XOR:
        if(*f == *g) { *result = bddfalse; return 1; }
        if(*f == B_NOT(*g)) { *result = bddtrue; return 1; }
        if(*f == bddfalse) { fp = B_NP(*g); B_RFC_INC_NP(fp); *result = *g; return 1; }
        if(*g == bddfalse) { fp = B_NP(*f); B_RFC_INC_NP(fp); *result = *f; return 1; }
        if(*f == bddtrue) { fp = B_NP(*g); B_RFC_INC_NP(fp); *result = B_NOT(*g); return 1; }
        if(*g == bddtrue) { fp = B_NP(*f); B_RFC_INC_NP(fp); *result = B_NOT(*f); return 1; }
        if(B_NEG(*f) && B_NEG(*g)) { *f = B_NOT(*f); *g = B_NOT(*g); }
        else if(B_NEG(*f) || B_NEG(*g)) {
            *f = B_ABS(*f); *g = B_ABS(*g);
            if(*f < *g) { h = *f; *f = *g; *g = h; }
            *need_negate = 1;
            return 0; /* Continue with recursion, but negate result */
        }
        if(*f < *g) { h = *f; *f = *g; *g = h; }
        break;

    case BC_INTERSEC:
        if(*f == bddfalse || *g == bddfalse) { *result = bddfalse; return 1; }
        if(*f == bddtrue) { *result = B_NEG(*g)? bddtrue: bddfalse; return 1; }
        if(*g == bddtrue) { *result = B_NEG(*f)? bddtrue: bddfalse; return 1; }
        if(*f == *g) { fp = B_NP(*f); B_RFC_INC_NP(fp); *result = *f; return 1; }
        if(*f == B_NOT(*g)) { fp = B_NP(*f); B_RFC_INC_NP(fp); *result = B_ABS(*f); return 1; }
        if(*f < *g) { h = *f; *f = *g; *g = h; }
        break;

    case BC_UNION:
        if(*f == bddfalse) {
            if(!B_CST(*g)) { fp = B_NP(*g); B_RFC_INC_NP(fp); }
            *result = *g;
            return 1;
        }
        if(*f == bddtrue) {
            if(!B_CST(*g)) { fp = B_NP(*g); B_RFC_INC_NP(fp); }
            *result = B_NEG(*g)? *g: B_NOT(*g);
            return 1;
        }
        if(*g == bddfalse || *f == *g) {
            fp = B_NP(*f); B_RFC_INC_NP(fp);
            *result = *f;
            return 1;
        }
        if(*g == bddtrue || *f == B_NOT(*g)) {
            fp = B_NP(*f); B_RFC_INC_NP(fp);
            *result = B_NEG(*f)? *f: B_NOT(*f);
            return 1;
        }
        if(*f < *g) { h = *f; *f = *g; *g = h; }
        break;

    case BC_SUBTRACT:
        if(*f == bddfalse || *f == *g) { *result = bddfalse; return 1; }
        if(*f == bddtrue || *f == B_NOT(*g)) {
            *result = B_NEG(*g)? bddfalse: bddtrue;
            return 1;
        }
        if(*g == bddfalse) { fp = B_NP(*f); B_RFC_INC_NP(fp); *result = *f; return 1; }
        if(*g == bddtrue) { fp = B_NP(*f); B_RFC_INC_NP(fp); *result = B_ABS(*f); return 1; }
        break;

    default:
        break;
    }

    return 0;
}

/*
 * Check cache for binary operations
 * Returns 1 if cache hit (result in *result), 0 otherwise
 */
static int check_cache_binary(bddp f, bddp g, unsigned char op,
                               bddp *key, bddp *result) {
    struct B_NodeTable *fp;
    struct B_CacheTable *cachep;

    if((B_CST(f) || B_RFC_ONE_NP(B_NP(f))) &&
       (B_CST(g) || B_RFC_ONE_NP(B_NP(g)))) {
        *key = bddnull;
        return 0;
    }

    *key = B_CACHEKEY(op, f, g);
    cachep = Cache + *key;
    if(cachep->op == op &&
       f == B_GET_BDDP(cachep->f) &&
       g == B_GET_BDDP(cachep->g)) {
        *result = B_GET_BDDP(cachep->h);
        if(!B_CST(*result) && *result != bddnull) {
            fp = B_NP(*result);
            B_RFC_INC_NP(fp);
        }
        return 1;
    }

    return 0;
}

/*
 * Store result in cache
 */
static void store_cache(bddp key, unsigned char op, bddp f, bddp g, bddp h) {
    struct B_CacheTable *cachep;
    if(key != bddnull) {
        cachep = Cache + key;
        cachep->op = op;
        B_SET_BDDP(cachep->f, f);
        B_SET_BDDP(cachep->g, g);
        B_SET_BDDP(cachep->h, h);
    }
}

/*
 * Extract child nodes for binary operations
 */
static void get_children_binary(bddp f, bddp g,
                                 bddp *f0, bddp *f1, bddp *g0, bddp *g1,
                                 bddvar *v, char *z) {
    struct B_NodeTable *fp, *gp;
    bddvar flev, glev;

    *z = 0;
    fp = B_NP(f);
    flev = B_CST(f)? 0: Var[B_VAR_NP(fp)].lev;
    gp = B_NP(g);
    glev = B_CST(g)? 0: Var[B_VAR_NP(gp)].lev;
    *f0 = f; *f1 = f;
    *g0 = g; *g1 = g;

    if(flev <= glev) {
        *v = B_VAR_NP(gp);
        if(B_Z_NP(gp)) {
            *z = 1;
            if(flev < glev) *f1 = bddfalse;
        }
        *g0 = B_GET_BDDP(gp->f0);
        *g1 = B_GET_BDDP(gp->f1);
        if(B_NEG(g)^B_NEG(*g0)) *g0 = B_NOT(*g0);
        if(B_NEG(g) && !*z) *g1 = B_NOT(*g1);
    }

    if(flev >= glev) {
        *v = B_VAR_NP(fp);
        if(B_Z_NP(fp)) {
            *z = 1;
            if(flev > glev) *g1 = bddfalse;
        }
        *f0 = B_GET_BDDP(fp->f0);
        *f1 = B_GET_BDDP(fp->f1);
        if(B_NEG(f)^B_NEG(*f0)) *f0 = B_NOT(*f0);
        if(B_NEG(f) && !*z) *f1 = B_NOT(*f1);
    }
}

/*
 * Iterative version of apply_binary
 * Uses explicit stack instead of recursion
 */
bddp apply_binary_iterative(bddp f, bddp g, unsigned char op, unsigned char skip)
{
    struct ApplyStack stack;
    struct ApplyStackFrame *frame, *parent;
    bddp result;
    int need_negate;
    bddp final_result = bddnull;

    stack_init(&stack);

    /* Push initial frame */
    if (!stack_push(&stack)) {
        stack_free(&stack);
        return bddnull;
    }
    frame = stack_current(&stack);
    frame->f = f;
    frame->g = g;
    frame->op = op;
    frame->skip = skip;
    frame->state = 0;
    frame->h0 = bddnull;
    frame->h1 = bddnull;
    frame->result = bddnull;

    while (stack.top >= 0) {
        frame = stack_current(&stack);

        switch (frame->state) {
        case 0: /* Initial state */
            /* Check terminal cases */
            if (check_terminal_binary(&frame->f, &frame->g, frame->op,
                                       frame->skip, &result, &need_negate)) {
                frame->result = result;
                goto pop_frame;
            }

            /* XOR special case: need to negate final result */
            if (need_negate) {
                /* Push new frame for the non-negated computation */
                frame->state = 3; /* State 3: waiting for XOR negate result */
                if (!stack_push(&stack)) {
                    frame->result = bddnull;
                    goto pop_frame;
                }
                {
                    struct ApplyStackFrame *child = stack_current(&stack);
                    child->f = frame->f;
                    child->g = frame->g;
                    child->op = frame->op;
                    child->skip = 1; /* skip=1 to avoid infinite loop */
                    child->state = 0;
                    child->h0 = bddnull;
                    child->h1 = bddnull;
                    child->result = bddnull;
                }
                break;
            }

            /* Check cache */
            if (check_cache_binary(frame->f, frame->g, frame->op,
                                    &frame->key, &result)) {
                frame->result = result;
                goto pop_frame;
            }

            /* Extract children */
            get_children_binary(frame->f, frame->g,
                                &frame->f0, &frame->f1, &frame->g0, &frame->g1,
                                &frame->v, &frame->z);

            /* Push frame for first recursive call: apply(f0, g0, op, 0) */
            frame->state = 1;
            if (!stack_push(&stack)) {
                frame->result = bddnull;
                goto pop_frame;
            }
            {
                struct ApplyStackFrame *child = stack_current(&stack);
                child->f = frame->f0;
                child->g = frame->g0;
                child->op = frame->op;
                child->skip = 0;
                child->state = 0;
                child->h0 = bddnull;
                child->h1 = bddnull;
                child->result = bddnull;
            }
            break;

        case 1: /* After h0 computation */
            /* h0 is in the child frame's result (already popped, stored in frame->h0) */
            if (frame->h0 == bddnull) {
                frame->result = bddnull;
                goto pop_frame;
            }

            /* Push frame for second recursive call: apply(f1, g1, op, 0) */
            frame->state = 2;
            if (!stack_push(&stack)) {
                bddfree(frame->h0);
                frame->result = bddnull;
                goto pop_frame;
            }
            {
                struct ApplyStackFrame *child = stack_current(&stack);
                child->f = frame->f1;
                child->g = frame->g1;
                child->op = frame->op;
                child->skip = 0;
                child->state = 0;
                child->h0 = bddnull;
                child->h1 = bddnull;
                child->result = bddnull;
            }
            break;

        case 2: /* After h1 computation */
            /* h1 is in frame->h1 (set when child was popped) */
            if (frame->h1 == bddnull) {
                bddfree(frame->h0);
                frame->result = bddnull;
                goto pop_frame;
            }

            /* Combine h0 and h1 */
            frame->result = frame->z ?
                getzddp(frame->v, frame->h0, frame->h1) :
                getbddp(frame->v, frame->h0, frame->h1);

            if (frame->result == bddnull) {
                bddfree(frame->h0);
                bddfree(frame->h1);
            }

            /* Store in cache */
            store_cache(frame->key, frame->op, frame->f, frame->g, frame->result);
            goto pop_frame;

        case 3: /* XOR negate: waiting for child result */
            /* Child result is in frame->h0 */
            if (frame->h0 == bddnull) {
                frame->result = bddnull;
            } else {
                frame->result = B_NOT(frame->h0);
            }
            goto pop_frame;
        }

        continue;

    pop_frame:
        /* Pass result to parent frame */
        parent = stack_parent(&stack);
        if (parent) {
            if (parent->state == 1) {
                /* Parent was waiting for h0 */
                parent->h0 = frame->result;
            } else if (parent->state == 2) {
                /* Parent was waiting for h1 */
                parent->h1 = frame->result;
            } else if (parent->state == 3) {
                /* Parent was waiting for XOR negate result */
                parent->h0 = frame->result;
            }
        } else {
            /* This is the root frame, save final result */
            final_result = frame->result;
        }
        stack_pop(&stack);
    }

    stack_free(&stack);
    return final_result;
}

/* ============================================================
 * Iterative version of apply_unary
 * ============================================================ */

/*
 * Check terminal cases for unary operations
 * Returns 1 if terminal case found (result in *result), 0 otherwise
 * Returns 2 if need to recurse with negated f (for AT0/AT1/OFFSET/LSHIFT/RSHIFT)
 */
static int check_terminal_unary(bddp *f, bddp g, unsigned char op,
                                 unsigned char skip, bddp *result) {
    struct B_NodeTable *fp;
    bddp h, h0, h1;
    bddvar flev, glev;

    if (skip) return 0;

    switch(op) {
    case BC_AT0:
    case BC_AT1:
    case BC_OFFSET:
        if(B_CST(*f)) { *result = *f; return 1; }
        fp = B_NP(*f); flev = Var[B_VAR_NP(fp)].lev;
        glev = Var[(bddvar)g].lev;
        if(flev < glev) { B_RFC_INC_NP(fp); *result = *f; return 1; }
        if(flev == glev) {
            if(op != BC_AT1) {
                h = B_GET_BDDP(fp->f0);
                if(B_NEG(*f)^B_NEG(h)) h = B_NOT(h);
            } else {
                h = B_GET_BDDP(fp->f1);
                if(B_NEG(*f)) h = B_NOT(h);
            }
            if(!B_CST(h)) { fp = B_NP(h); B_RFC_INC_NP(fp); }
            *result = h;
            return 1;
        }
        if(B_NEG(*f)) {
            *f = B_NOT(*f);
            return 2; /* Need to recurse with negated f, then negate result */
        }
        break;

    case BC_ONSET:
        if(B_CST(*f)) { *result = bddfalse; return 1; }
        fp = B_NP(*f); flev = Var[B_VAR_NP(fp)].lev;
        glev = Var[(bddvar)g].lev;
        if(flev < glev) { *result = bddfalse; return 1; }
        if(flev == glev) {
            h = B_GET_BDDP(fp->f1);
            if(!B_CST(h)) { fp = B_NP(h); B_RFC_INC_NP(fp); }
            *result = h;
            return 1;
        }
        if(B_NEG(*f)) *f = B_NOT(*f);
        break;

    case BC_CHANGE:
        if(*f == bddfalse) { *result = *f; return 1; }
        if(B_CST(*f)) {
            *result = getzddp((bddvar)g, bddfalse, *f);
            return 1;
        }
        fp = B_NP(*f); flev = Var[B_VAR_NP(fp)].lev;
        glev = Var[(bddvar)g].lev;
        if(flev < glev) {
            B_RFC_INC_NP(fp);
            h = getzddp((bddvar)g, bddfalse, *f);
            if(h == bddnull) bddfree(*f);
            *result = h;
            return 1;
        }
        if(flev == glev) {
            h0 = B_GET_BDDP(fp->f1);
            h1 = B_GET_BDDP(fp->f0);
            if(B_NEG(*f)^B_NEG(h1)) h1 = B_NOT(h1);
            if(!B_CST(h0)) { fp = B_NP(h0); B_RFC_INC_NP(fp); }
            if(!B_CST(h1)) { fp = B_NP(h1); B_RFC_INC_NP(fp); }
            h = getzddp((bddvar)g, h0, h1);
            if(h == bddnull) { bddfree(h0); bddfree(h1); }
            *result = h;
            return 1;
        }
        break;

    case BC_LSHIFT:
    case BC_RSHIFT:
        if(B_CST(*f)) { *result = *f; return 1; }
        if(B_NEG(*f)) {
            *f = B_NOT(*f);
            return 2; /* Need to recurse with negated f, then negate result */
        }
        break;

    default:
        break;
    }

    return 0;
}

/*
 * Check cache for unary operations
 */
static int check_cache_unary(bddp f, bddp g, unsigned char op,
                              bddp *key, bddp *result) {
    struct B_NodeTable *fp;
    struct B_CacheTable *cachep;

    fp = B_NP(f);
    if(B_RFC_ONE_NP(fp)) {
        *key = bddnull;
        return 0;
    }

    *key = B_CACHEKEY(op, f, g);
    cachep = Cache + *key;
    if(cachep->op == op &&
       f == B_GET_BDDP(cachep->f) &&
       g == B_GET_BDDP(cachep->g)) {
        *result = B_GET_BDDP(cachep->h);
        if(!B_CST(*result) && *result != bddnull) {
            fp = B_NP(*result);
            B_RFC_INC_NP(fp);
        }
        return 1;
    }

    return 0;
}

/*
 * Store cache with additional entries for related operations
 */
static void store_cache_unary(bddp key, unsigned char op, bddp f, bddp g, bddp h) {
    struct B_CacheTable *cachep;
    bddp key2;

    if(key == bddnull) return;

    cachep = Cache + key;
    cachep->op = op;
    B_SET_BDDP(cachep->f, f);
    B_SET_BDDP(cachep->g, g);
    B_SET_BDDP(cachep->h, h);

    /* Additional cache entries for related operations */
    if(h == f) switch(op) {
    case BC_AT0:
        key2 = B_CACHEKEY(BC_AT1, f, g);
        cachep = Cache + key2;
        cachep->op = BC_AT1;
        B_SET_BDDP(cachep->f, f);
        B_SET_BDDP(cachep->g, g);
        B_SET_BDDP(cachep->h, h);
        break;
    case BC_AT1:
        key2 = B_CACHEKEY(BC_AT0, f, g);
        cachep = Cache + key2;
        cachep->op = BC_AT0;
        B_SET_BDDP(cachep->f, f);
        B_SET_BDDP(cachep->g, g);
        B_SET_BDDP(cachep->h, h);
        break;
    case BC_OFFSET:
        key2 = B_CACHEKEY(BC_ONSET, f, g);
        cachep = Cache + key2;
        cachep->op = BC_ONSET;
        B_SET_BDDP(cachep->f, f);
        B_SET_BDDP(cachep->g, g);
        B_SET_BDDP(cachep->h, bddfalse);
        break;
    default:
        break;
    }
    if(h == bddfalse && op == BC_ONSET) {
        key2 = B_CACHEKEY(BC_OFFSET, f, g);
        cachep = Cache + key2;
        cachep->op = BC_OFFSET;
        B_SET_BDDP(cachep->f, f);
        B_SET_BDDP(cachep->g, g);
        B_SET_BDDP(cachep->h, f);
    }
}

/*
 * Extract child nodes for unary operations
 */
static void get_children_unary(bddp f, bddp *f0, bddp *f1, bddvar *v, char *z) {
    struct B_NodeTable *fp;

    fp = B_NP(f);
    *v = B_VAR_NP(fp);
    *z = B_Z_NP(fp)? 1: 0;
    *f0 = B_GET_BDDP(fp->f0);
    *f1 = B_GET_BDDP(fp->f1);
    if(B_NEG(f)^B_NEG(*f0)) *f0 = B_NOT(*f0);
    if(B_NEG(f) && !*z) *f1 = B_NOT(*f1);
}

/*
 * Iterative version of apply_unary
 */
bddp apply_unary_iterative(bddp f, bddp g, unsigned char op, unsigned char skip)
{
    struct ApplyStack stack;
    struct ApplyStackFrame *frame, *parent;
    bddp result;
    int term_result;
    bddp final_result = bddnull;

    stack_init(&stack);

    /* Push initial frame */
    if (!stack_push(&stack)) {
        stack_free(&stack);
        return bddnull;
    }
    frame = stack_current(&stack);
    frame->f = f;
    frame->g = g;
    frame->op = op;
    frame->skip = skip;
    frame->state = 0;
    frame->h0 = bddnull;
    frame->h1 = bddnull;
    frame->result = bddnull;

    while (stack.top >= 0) {
        frame = stack_current(&stack);

        switch (frame->state) {
        case 0: /* Initial state */
            /* Check terminal cases */
            term_result = check_terminal_unary(&frame->f, frame->g, frame->op,
                                                frame->skip, &result);
            if (term_result == 1) {
                frame->result = result;
                goto pop_frame;
            }
            if (term_result == 2) {
                /* Need to recurse with negated f, then negate result */
                frame->state = 3; /* State 3: waiting for negated recursion */
                if (!stack_push(&stack)) {
                    frame->result = bddnull;
                    goto pop_frame;
                }
                {
                    struct ApplyStackFrame *child = stack_current(&stack);
                    child->f = frame->f;
                    child->g = frame->g;
                    child->op = frame->op;
                    child->skip = 1;
                    child->state = 0;
                    child->h0 = bddnull;
                    child->h1 = bddnull;
                    child->result = bddnull;
                }
                break;
            }

            /* Check cache */
            if (check_cache_unary(frame->f, frame->g, frame->op,
                                   &frame->key, &result)) {
                frame->result = result;
                goto pop_frame;
            }

            /* Extract children */
            get_children_unary(frame->f, &frame->f0, &frame->f1, &frame->v, &frame->z);

            /* For LSHIFT/RSHIFT, compute new variable */
            if (frame->op == BC_LSHIFT || frame->op == BC_RSHIFT) {
                bddvar flev = bddlevofvar(frame->v);
                bddvar newlev;
                if (frame->op == BC_LSHIFT) {
                    newlev = flev + (bddvar)frame->g;
                    if (newlev > VarUsed || newlev < flev) {
                        err("apply: Invalid shift", newlev, ExceptionType::OutOfRange);
                    }
                } else {
                    newlev = flev - (bddvar)frame->g;
                    if (newlev == 0 || newlev > flev) {
                        err("apply: Invalid shift", newlev, ExceptionType::OutOfRange);
                    }
                }
                frame->v = bddvaroflev(newlev);
            }

            /* Push frame for first recursive call: apply(f0, g, op, 0) */
            frame->state = 1;
            if (!stack_push(&stack)) {
                frame->result = bddnull;
                goto pop_frame;
            }
            {
                struct ApplyStackFrame *child = stack_current(&stack);
                child->f = frame->f0;
                child->g = frame->g;
                child->op = frame->op;
                child->skip = 0;
                child->state = 0;
                child->h0 = bddnull;
                child->h1 = bddnull;
                child->result = bddnull;
            }
            break;

        case 1: /* After h0 computation */
            if (frame->h0 == bddnull) {
                frame->result = bddnull;
                goto pop_frame;
            }

            /* Push frame for second recursive call: apply(f1, g, op, 0) */
            frame->state = 2;
            if (!stack_push(&stack)) {
                bddfree(frame->h0);
                frame->result = bddnull;
                goto pop_frame;
            }
            {
                struct ApplyStackFrame *child = stack_current(&stack);
                child->f = frame->f1;
                child->g = frame->g;
                child->op = frame->op;
                child->skip = 0;
                child->state = 0;
                child->h0 = bddnull;
                child->h1 = bddnull;
                child->result = bddnull;
            }
            break;

        case 2: /* After h1 computation */
            if (frame->h1 == bddnull) {
                bddfree(frame->h0);
                frame->result = bddnull;
                goto pop_frame;
            }

            /* Combine h0 and h1 */
            frame->result = frame->z ?
                getzddp(frame->v, frame->h0, frame->h1) :
                getbddp(frame->v, frame->h0, frame->h1);

            if (frame->result == bddnull) {
                bddfree(frame->h0);
                bddfree(frame->h1);
            }

            /* Store in cache */
            store_cache_unary(frame->key, frame->op, frame->f, frame->g, frame->result);
            goto pop_frame;

        case 3: /* Negated recursion: waiting for child result */
            if (frame->h0 == bddnull) {
                frame->result = bddnull;
            } else {
                frame->result = B_NOT(frame->h0);
            }
            goto pop_frame;
        }

        continue;

    pop_frame:
        parent = stack_parent(&stack);
        if (parent) {
            if (parent->state == 1) {
                parent->h0 = frame->result;
            } else if (parent->state == 2) {
                parent->h1 = frame->result;
            } else if (parent->state == 3) {
                parent->h0 = frame->result;
            }
        } else {
            final_result = frame->result;
        }
        stack_pop(&stack);
    }

    stack_free(&stack);
    return final_result;
}

/* ============================================================
 * Iterative version of apply_count
 * ============================================================ */

/*
 * Check terminal cases for count operations
 * Returns 1 if terminal case found (result in *result), 0 otherwise
 * Returns 2 if need special handling (negation removal for CARD)
 */
static int check_terminal_count(bddp *f, unsigned char op,
                                 unsigned char skip, bddp *result) {
    if (skip) return 0;

    switch(op) {
    case BC_SUPPORT:
        if(B_CST(*f)) { *result = bddfalse; return 1; }
        if(B_NEG(*f)) *f = B_NOT(*f);
        break;

    case BC_CARD:
        if(B_CST(*f)) { *result = (*f == bddfalse)? 0: 1; return 1; }
        if(B_NEG(*f)) {
            *f = B_NOT(*f);
            return 2; /* Need special handling: result + 1 */
        }
        break;

    case BC_CARD2:
        if(B_CST(*f)) { *result = (*f == bddfalse)? 0: 1; return 1; }
        break;

    case BC_LIT:
        if(B_CST(*f)) { *result = 0; return 1; }
        if(B_NEG(*f)) *f = B_NOT(*f);
        break;

    case BC_LEN:
        if(B_CST(*f)) { *result = 0; return 1; }
        if(B_NEG(*f)) *f = B_NOT(*f);
        break;

    default:
        break;
    }

    return 0;
}

/*
 * Check cache for count operations
 */
static int check_cache_count(bddp f, unsigned char op, bddp *key, bddp *result) {
    struct B_NodeTable *fp;
    struct B_CacheTable *cachep;
    unsigned char cache_op = (op == BC_CARD2) ? BC_CARD : op;
    (void)cache_op; /* Used below */

    fp = B_NP(f);
    if(B_RFC_ONE_NP(fp)) {
        *key = bddnull;
        return 0;
    }

    if (op == BC_SUPPORT) {
        *key = B_CACHEKEY(op, f, bddfalse);
    } else {
        *key = B_CACHEKEY(cache_op, f, bddfalse);
    }
    cachep = Cache + *key;

    if (op == BC_SUPPORT) {
        if(cachep->op == op &&
           f == B_GET_BDDP(cachep->f) &&
           bddfalse == B_GET_BDDP(cachep->g)) {
            *result = B_GET_BDDP(cachep->h);
            if(!B_CST(*result) && *result != bddnull) {
                fp = B_NP(*result);
                B_RFC_INC_NP(fp);
            }
            return 1;
        }
    } else if (op == BC_CARD2) {
        if(cachep->op == BC_CARD &&
           f == B_GET_BDDP(cachep->f) &&
           bddfalse == B_GET_BDDP(cachep->g)) {
            *result = B_GET_BDDP(cachep->h);
            if(*result != bddnull) return 1;
        }
    } else {
        if(cachep->op == op &&
           f == B_GET_BDDP(cachep->f) &&
           bddfalse == B_GET_BDDP(cachep->g)) {
            *result = B_GET_BDDP(cachep->h);
            return 1;
        }
    }

    return 0;
}

/*
 * Extract child nodes for count operations
 */
static void get_children_count(bddp f, bddp *f0, bddp *f1, bddvar *v, char *z) {
    struct B_NodeTable *fp;

    fp = B_NP(f);
    *v = B_VAR_NP(fp);
    *z = B_Z_NP(fp)? 1: 0;
    *f0 = B_GET_BDDP(fp->f0);
    *f1 = B_GET_BDDP(fp->f1);
    if(B_NEG(f)^B_NEG(*f0)) *f0 = B_NOT(*f0);
    /* Note: for count operations, f1 is not negated */
}

/*
 * Iterative version of apply_count
 * Note: BC_SUPPORT and BC_LIT have nested apply calls, so we use
 * the main apply() dispatcher for those (which routes to iterative
 * versions when appropriate)
 */
bddp apply_count_iterative(bddp f, bddp g, unsigned char op, unsigned char skip)
{
    struct ApplyStack stack;
    struct ApplyStackFrame *frame, *parent;
    bddp result;
    int term_result;
    bddp final_result = bddnull;

    stack_init(&stack);

    /* Push initial frame */
    if (!stack_push(&stack)) {
        stack_free(&stack);
        return bddnull;
    }
    frame = stack_current(&stack);
    frame->f = f;
    frame->g = g;
    frame->op = op;
    frame->skip = skip;
    frame->state = 0;
    frame->h0 = bddnull;
    frame->h1 = bddnull;
    frame->result = bddnull;

    while (stack.top >= 0) {
        frame = stack_current(&stack);

        switch (frame->state) {
        case 0: /* Initial state */
            /* Check terminal cases */
            term_result = check_terminal_count(&frame->f, frame->op,
                                                frame->skip, &result);
            if (term_result == 1) {
                frame->result = result;
                goto pop_frame;
            }
            if (term_result == 2) {
                /* BC_CARD with negated f: need result + 1 */
                frame->state = 3;
                if (!stack_push(&stack)) {
                    frame->result = bddnull;
                    goto pop_frame;
                }
                {
                    struct ApplyStackFrame *child = stack_current(&stack);
                    child->f = frame->f;
                    child->g = bddfalse;
                    child->op = BC_CARD;
                    child->skip = 1;
                    child->state = 0;
                    child->h0 = bddnull;
                    child->h1 = bddnull;
                    child->result = bddnull;
                }
                break;
            }

            /* Check cache */
            if (check_cache_count(frame->f, frame->op, &frame->key, &result)) {
                frame->result = result;
                goto pop_frame;
            }

            /* Extract children */
            if (frame->op == BC_SUPPORT) {
                get_children_unary(frame->f, &frame->f0, &frame->f1, &frame->v, &frame->z);
            } else {
                get_children_count(frame->f, &frame->f0, &frame->f1, &frame->v, &frame->z);
            }

            /* Push frame for first recursive call */
            frame->state = 1;
            if (!stack_push(&stack)) {
                frame->result = bddnull;
                goto pop_frame;
            }
            {
                struct ApplyStackFrame *child = stack_current(&stack);
                if (frame->op == BC_CARD2) {
                    child->f = B_ABS(frame->f0);
                } else {
                    child->f = frame->f0;
                }
                child->g = bddfalse;
                child->op = frame->op;
                child->skip = 0;
                child->state = 0;
                child->h0 = bddnull;
                child->h1 = bddnull;
                child->result = bddnull;
            }
            break;

        case 1: /* After h0 computation */
            /* BC_CARD2 uses B_MP_NULL for errors, not bddnull */
            if (frame->op == BC_CARD2) {
                if (frame->h0 == B_MP_NULL) {
                    frame->result = B_MP_NULL;
                    goto pop_frame;
                }
            } else if (frame->h0 == bddnull && frame->op != BC_LEN) {
                frame->result = bddnull;
                goto pop_frame;
            }

            /* Push frame for second recursive call */
            frame->state = 2;
            if (!stack_push(&stack)) {
                frame->result = bddnull;
                goto pop_frame;
            }
            {
                struct ApplyStackFrame *child = stack_current(&stack);
                if (frame->op == BC_CARD2) {
                    child->f = B_ABS(frame->f1);
                } else {
                    child->f = frame->f1;
                }
                child->g = bddfalse;
                child->op = frame->op;
                child->skip = 0;
                child->state = 0;
                child->h0 = bddnull;
                child->h1 = bddnull;
                child->result = bddnull;
            }
            break;

        case 2: /* After h1 computation */
            /* Compute final result based on operation */
            switch (frame->op) {
            case BC_SUPPORT:
                if (frame->h1 == bddnull) {
                    bddfree(frame->h0);
                    frame->result = bddnull;
                    goto pop_frame;
                }
                /* Need nested apply call for UNION/AND */
                {
                    bddp h;
                    if (frame->z) {
                        h = apply(frame->h0, frame->h1, BC_UNION, 0);
                    } else {
                        h = apply(B_NOT(frame->h0), B_NOT(frame->h1), BC_AND, 0);
                    }
                    bddfree(frame->h0);
                    bddfree(frame->h1);
                    if (h == bddnull) {
                        frame->result = bddnull;
                        goto pop_frame;
                    }
                    frame->h0 = h;
                    frame->result = frame->z ?
                        getzddp(frame->v, frame->h0, bddtrue) :
                        getbddp(frame->v, B_NOT(frame->h0), bddtrue);
                    if (frame->result == bddnull) bddfree(frame->h0);
                }
                break;

            case BC_CARD:
                frame->result = frame->h0 + frame->h1;
                if (frame->result >= bddnull) frame->result = bddnull;
                break;

            case BC_CARD2:
                if (frame->h1 == B_MP_NULL) {
                    frame->result = B_MP_NULL;
                    goto pop_frame;
                }
                /* Complex MP handling - use original recursive version for this */
                {
                    struct B_MP mp;
                    struct B_MPTable *mpt;
                    bddp i, size2;
                    bddp *wp;

                    mp.len = 1;
                    mp.word[0] = 0;
                    if(B_NEG(frame->f0)) mp.word[0]++;
                    if(B_NEG(frame->f1)) mp.word[0]++;
                    mp_add(&mp, frame->h0);
                    mp_add(&mp, frame->h1);
                    if(mp.len == 1 && mp.word[0] <= bddnull) {
                        frame->result = mp.word[0];
                        break;
                    }
                    mpt = mptable + mp.len-1;
                    if(mpt->word == 0) {
                        mpt->size = 16;
                        mpt->used = 0;
                        mpt->word = B_MALLOC(bddp, mp.len * mpt->size);
                        if (!mpt->word) {
                            err("apply: not enough memory for mp table",
                                sizeof(bddp) * mp.len * mpt->size,
                                ExceptionType::OutOfMemory);
                        }
                    }
                    if(mpt->size == mpt->used) {
                        size2 = mpt->size << 1;
                        if(size2 > (B_CST_MASK>>B_MP_LWID)) {
                            frame->result = B_MP_NULL;
                            break;
                        }
                        wp = 0;
                        wp = B_MALLOC(bddp, mp.len * size2);
                        if(!wp) {
                            err("apply: not enough memory for mp table",
                                sizeof(bddp) * mp.len * size2,
                                ExceptionType::OutOfMemory);
                        }
                        for(i=0; i<mp.len*(mpt->size); i++) wp[i] = mpt->word[i];
                        mpt->size = size2;
                        free(mpt->word);
                        mpt->word = wp;
                    }
                    wp = mpt->word;
                    for(i=0; i<(bddp)mp.len; i++) wp[mp.len*(mpt->used)+i] = mp.word[i];
                    frame->result = (((bddp)mp.len-1)<<B_MP_LPOS) + B_CST_MASK + (mpt->used++);
                }
                break;

            case BC_LIT:
                frame->result = frame->h0 + frame->h1;
                if (frame->result >= bddnull) frame->result = bddnull;
                /* Add card of f1 */
                {
                    bddp card = apply(frame->f1, bddfalse, BC_CARD, 0);
                    frame->result += card;
                    if (frame->result >= bddnull) frame->result = bddnull;
                }
                break;

            case BC_LEN:
                {
                    bddp h1_plus = frame->h1 + 1;
                    frame->result = (frame->h0 < h1_plus) ? h1_plus : frame->h0;
                }
                break;

            default:
                frame->result = bddnull;
                break;
            }

            /* Store in cache */
            if (frame->key != bddnull) {
                struct B_CacheTable *cachep = Cache + frame->key;
                if (frame->op == BC_CARD2)
                    cachep->op = BC_CARD;
                else
                    cachep->op = frame->op;
                B_SET_BDDP(cachep->f, frame->f);
                B_SET_BDDP(cachep->g, bddfalse);
                B_SET_BDDP(cachep->h, frame->result);
            }
            goto pop_frame;

        case 3: /* BC_CARD negated: waiting for child result */
            if (frame->h0 >= bddnull) {
                frame->result = bddnull;
            } else {
                frame->result = frame->h0 + 1;
                if (frame->result >= bddnull) frame->result = bddnull;
            }
            goto pop_frame;
        }

        continue;

    pop_frame:
        parent = stack_parent(&stack);
        if (parent) {
            if (parent->state == 1) {
                parent->h0 = frame->result;
            } else if (parent->state == 2) {
                parent->h1 = frame->result;
            } else if (parent->state == 3) {
                parent->h0 = frame->result;
            }
        } else {
            final_result = frame->result;
        }
        stack_pop(&stack);
    }

    stack_free(&stack);
    return final_result;
}

/* ============================================================
 * Iterative version of apply_special
 * ============================================================ */

/*
 * Check terminal cases for special operations
 */
static int check_terminal_special(bddp f, bddp *g, unsigned char op,
                                   unsigned char skip, bddp *result) {
    struct B_NodeTable *fp;

    if (skip) return 0;

    switch(op) {
    case BC_COFACTOR:
        if(B_CST(f)) { *result = f; return 1; }
        if(*g == bddfalse || f == B_NOT(*g)) { *result = bddfalse; return 1; }
        if(f == *g) { *result = bddtrue; return 1; }
        if(*g == bddtrue) { fp = B_NP(f); B_RFC_INC_NP(fp); *result = f; return 1; }
        break;

    case BC_UNIV:
        if(B_CST(f)) { *result = f; return 1; }
        if(B_CST(*g)) { fp = B_NP(f); B_RFC_INC_NP(fp); *result = f; return 1; }
        if(B_NEG(*g)) *g = B_NOT(*g);
        break;

    default:
        break;
    }

    return 0;
}

/*
 * Iterative version of apply_special
 */
bddp apply_special_iterative(bddp f, bddp g, unsigned char op, unsigned char skip)
{
    struct ApplyStack stack;
    struct ApplyStackFrame *frame, *parent;
    bddp result;
    bddp final_result = bddnull;

    stack_init(&stack);

    /* Push initial frame */
    if (!stack_push(&stack)) {
        stack_free(&stack);
        return bddnull;
    }
    frame = stack_current(&stack);
    frame->f = f;
    frame->g = g;
    frame->op = op;
    frame->skip = skip;
    frame->state = 0;
    frame->h0 = bddnull;
    frame->h1 = bddnull;
    frame->result = bddnull;

    while (stack.top >= 0) {
        frame = stack_current(&stack);

        switch (frame->state) {
        case 0: /* Initial state */
            /* Check terminal cases */
            if (check_terminal_special(frame->f, &frame->g, frame->op,
                                        frame->skip, &result)) {
                frame->result = result;
                goto pop_frame;
            }

            /* Check cache */
            if (check_cache_binary(frame->f, frame->g, frame->op,
                                    &frame->key, &result)) {
                frame->result = result;
                goto pop_frame;
            }

            /* Extract children */
            get_children_binary(frame->f, frame->g,
                                &frame->f0, &frame->f1, &frame->g0, &frame->g1,
                                &frame->v, &frame->z);

            /* For COFACTOR, check for single-side recursion */
            if (frame->op == BC_COFACTOR) {
                if (frame->g0 == bddfalse && frame->g1 != bddfalse) {
                    /* Only recurse on f1, g1 */
                    frame->state = 4; /* Special state for single recursion */
                    if (!stack_push(&stack)) {
                        frame->result = bddnull;
                        goto pop_frame;
                    }
                    {
                        struct ApplyStackFrame *child = stack_current(&stack);
                        child->f = frame->f1;
                        child->g = frame->g1;
                        child->op = frame->op;
                        child->skip = 0;
                        child->state = 0;
                        child->h0 = bddnull;
                        child->h1 = bddnull;
                        child->result = bddnull;
                    }
                    break;
                }
                if (frame->g1 == bddfalse && frame->g0 != bddfalse) {
                    /* Only recurse on f0, g0 */
                    frame->state = 4; /* Special state for single recursion */
                    if (!stack_push(&stack)) {
                        frame->result = bddnull;
                        goto pop_frame;
                    }
                    {
                        struct ApplyStackFrame *child = stack_current(&stack);
                        child->f = frame->f0;
                        child->g = frame->g0;
                        child->op = frame->op;
                        child->skip = 0;
                        child->state = 0;
                        child->h0 = bddnull;
                        child->h1 = bddnull;
                        child->result = bddnull;
                    }
                    break;
                }
            }

            /* Push frame for first recursive call */
            frame->state = 1;
            if (!stack_push(&stack)) {
                frame->result = bddnull;
                goto pop_frame;
            }
            {
                struct ApplyStackFrame *child = stack_current(&stack);
                child->f = frame->f0;
                child->g = frame->g0;
                child->op = frame->op;
                child->skip = 0;
                child->state = 0;
                child->h0 = bddnull;
                child->h1 = bddnull;
                child->result = bddnull;
            }
            break;

        case 1: /* After h0 computation */
            if (frame->h0 == bddnull) {
                frame->result = bddnull;
                goto pop_frame;
            }

            /* Push frame for second recursive call */
            frame->state = 2;
            if (!stack_push(&stack)) {
                bddfree(frame->h0);
                frame->result = bddnull;
                goto pop_frame;
            }
            {
                struct ApplyStackFrame *child = stack_current(&stack);
                child->f = frame->f1;
                /* For UNIV, second call uses g0, not g1 */
                child->g = (frame->op == BC_UNIV) ? frame->g0 : frame->g1;
                child->op = frame->op;
                child->skip = 0;
                child->state = 0;
                child->h0 = bddnull;
                child->h1 = bddnull;
                child->result = bddnull;
            }
            break;

        case 2: /* After h1 computation */
            if (frame->h1 == bddnull) {
                bddfree(frame->h0);
                frame->result = bddnull;
                goto pop_frame;
            }

            /* Combine results based on operation */
            if (frame->op == BC_COFACTOR) {
                frame->result = getbddp(frame->v, frame->h0, frame->h1);
                if (frame->result == bddnull) {
                    bddfree(frame->h0);
                    bddfree(frame->h1);
                }
            } else if (frame->op == BC_UNIV) {
                if (frame->g0 != frame->g1) {
                    /* Need nested AND operation */
                    frame->result = apply(frame->h0, frame->h1, BC_AND, 0);
                    bddfree(frame->h0);
                    bddfree(frame->h1);
                } else {
                    frame->result = getbddp(frame->v, frame->h0, frame->h1);
                    if (frame->result == bddnull) {
                        bddfree(frame->h0);
                        bddfree(frame->h1);
                    }
                }
            }

            /* Store in cache */
            store_cache(frame->key, frame->op, frame->f, frame->g, frame->result);
            goto pop_frame;

        case 4: /* COFACTOR single recursion: result is direct */
            frame->result = frame->h0;
            /* Store in cache */
            store_cache(frame->key, frame->op, frame->f, frame->g, frame->result);
            goto pop_frame;
        }

        continue;

    pop_frame:
        parent = stack_parent(&stack);
        if (parent) {
            if (parent->state == 1) {
                parent->h0 = frame->result;
            } else if (parent->state == 2) {
                parent->h1 = frame->result;
            } else if (parent->state == 4) {
                parent->h0 = frame->result;
            }
        } else {
            final_result = frame->result;
        }
        stack_pop(&stack);
    }

    stack_free(&stack);
    return final_result;
}

} // namespace sapporobdd
