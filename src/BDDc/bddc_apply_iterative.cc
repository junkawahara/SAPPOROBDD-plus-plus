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

} // namespace sapporobdd
