/*****************************************
*  BDD Package (SAPPORO-1.94)   - Util  *
*  (C) Shin-ichi MINATO  (Apr. 19, 2022) *
*  Split from bddc.cc for modularity     *
******************************************/

#include "bddc_internal.h"
#include <string>

namespace sapporobdd {

/* ============================================================
 * Threshold for switching to iterative versions
 * Same as APPLY_RECURSION_THRESHOLD in bddc_apply_common.h
 * ============================================================ */
#define UTIL_RECURSION_THRESHOLD 8192

/* ============================================================
 * Stack structure for iterative traversal
 * ============================================================ */
#define UTIL_STACK_INIT_SIZE 256

struct UtilStackFrame {
    bddp f;           /* Current node */
    unsigned char state;  /* 0: init, 1: after f0, 2: after f1 */
    bddp c0;          /* Count from f0 (for count_iterative) */
};

struct UtilStack {
    struct UtilStackFrame *frames;
    int top;
    int capacity;
};

static void util_stack_init(struct UtilStack *stack) {
    stack->frames = (struct UtilStackFrame *)malloc(
        sizeof(struct UtilStackFrame) * UTIL_STACK_INIT_SIZE);
    stack->top = -1;
    stack->capacity = UTIL_STACK_INIT_SIZE;
}

static void util_stack_free(struct UtilStack *stack) {
    if (stack->frames) {
        free(stack->frames);
        stack->frames = 0;
    }
}

static int util_stack_push(struct UtilStack *stack) {
    stack->top++;
    if (stack->top >= stack->capacity) {
        int new_capacity = stack->capacity * 2;
        struct UtilStackFrame *new_frames = (struct UtilStackFrame *)realloc(
            stack->frames, sizeof(struct UtilStackFrame) * new_capacity);
        if (!new_frames) {
            stack->top--;
            return 0;
        }
        stack->frames = new_frames;
        stack->capacity = new_capacity;
    }
    return 1;
}

static void util_stack_pop(struct UtilStack *stack) {
    if (stack->top >= 0) stack->top--;
}

static struct UtilStackFrame *util_stack_current(struct UtilStack *stack) {
    if (stack->top >= 0) return &stack->frames[stack->top];
    return 0;
}

static struct UtilStackFrame *util_stack_parent(struct UtilStack *stack) {
    if (stack->top >= 1) return &stack->frames[stack->top - 1];
    return 0;
}

/* ============================================================
 * Iterative version of count()
 * ============================================================ */
static bddp count_iterative(bddp f)
{
    struct UtilStack stack;
    struct UtilStackFrame *frame, *parent;
    bddp final_result = 0;
    struct B_NodeTable *fp;
    bddp nx, f0, f1;

    if (B_CST(f)) return 0;

    util_stack_init(&stack);

    /* Push initial frame */
    if (!util_stack_push(&stack)) {
        util_stack_free(&stack);
        return 0;
    }
    frame = util_stack_current(&stack);
    frame->f = f;
    frame->state = 0;
    frame->c0 = 0;

    while (stack.top >= 0) {
        frame = util_stack_current(&stack);
        fp = B_NP(frame->f);

        switch (frame->state) {
        case 0: /* Initial state */
            /* Check visit flag */
            nx = B_GET_BDDP(fp->nx);
            if (nx & B_CST_MASK) {
                /* Already visited, return 0 */
                goto return_zero;
            }

            /* Set visit flag */
            B_SET_BDDP(fp->nx, nx | B_CST_MASK);

            /* Get f0 */
            f0 = B_GET_BDDP(fp->f0);
            if (B_CST(f0)) {
                /* f0 is constant, count is 0 */
                frame->c0 = 0;
                frame->state = 1;
                /* Fall through to state 1 */
            } else {
                /* Push frame for f0 */
                frame->state = 1;
                if (!util_stack_push(&stack)) {
                    util_stack_free(&stack);
                    return 0;
                }
                {
                    struct UtilStackFrame *child = util_stack_current(&stack);
                    child->f = f0;
                    child->state = 0;
                    child->c0 = 0;
                }
                break;
            }
            /* Fall through */

        case 1: /* After f0 */
            /* Get f1 */
            f1 = B_GET_BDDP(fp->f1);
            if (B_CST(f1)) {
                /* f1 is constant, count is 0 */
                /* Result = c0 + 0 + 1 */
                goto return_result;
            } else {
                /* Push frame for f1 */
                frame->state = 2;
                if (!util_stack_push(&stack)) {
                    util_stack_free(&stack);
                    return 0;
                }
                {
                    struct UtilStackFrame *child = util_stack_current(&stack);
                    child->f = f1;
                    child->state = 0;
                    child->c0 = 0;
                }
                break;
            }

        case 2: /* After f1 - result already computed by child */
            goto return_result;
        }
        continue;

    return_zero:
        /* Return 0 to parent */
        parent = util_stack_parent(&stack);
        if (parent) {
            if (parent->state == 1) {
                parent->c0 = 0;
            }
            /* For state 2, we add 0 to result which is handled in return_result */
        } else {
            final_result = 0;
        }
        util_stack_pop(&stack);
        continue;

    return_result:
        /* Return c0 + c1 + 1 to parent */
        /* c1 is in the child's return value (already added to parent->c0 or handled) */
        parent = util_stack_parent(&stack);
        if (parent) {
            if (parent->state == 1) {
                /* Parent was waiting for f0 result */
                parent->c0 = frame->c0 + 1;  /* This node's contribution */
            } else if (parent->state == 2) {
                /* Parent was waiting for f1 result */
                /* Add f1's count to final result: parent->c0 already has f0's count */
                /* We need to add c1 + 1 (this node) to parent's accumulated count */
                /* Actually, the current frame IS the child for f1 */
                /* parent->c0 has count from f0, we need to add count from f1 */
                /* But we're returning from f1, so we add to parent's c0 */
                parent->c0 += frame->c0 + 1;
            }
        } else {
            /* Root frame */
            final_result = frame->c0 + 1;
        }
        util_stack_pop(&stack);
    }

    util_stack_free(&stack);
    return final_result;
}

/* ============================================================
 * Iterative version of reset()
 * ============================================================ */
static void reset_iterative(bddp f)
{
    struct UtilStack stack;
    struct UtilStackFrame *frame;
    struct B_NodeTable *fp;
    bddp nx, f0, f1;

    if (B_CST(f)) return;

    util_stack_init(&stack);

    /* Push initial frame */
    if (!util_stack_push(&stack)) {
        util_stack_free(&stack);
        return;
    }
    frame = util_stack_current(&stack);
    frame->f = f;
    frame->state = 0;

    while (stack.top >= 0) {
        frame = util_stack_current(&stack);
        fp = B_NP(frame->f);

        switch (frame->state) {
        case 0: /* Initial state */
            /* Check visit flag */
            nx = B_GET_BDDP(fp->nx);
            if (!(nx & B_CST_MASK)) {
                /* Not visited, nothing to reset */
                goto pop_frame;
            }

            /* Reset visit flag */
            B_SET_BDDP(fp->nx, nx & ~B_CST_MASK);

            /* Get f0 */
            f0 = B_GET_BDDP(fp->f0);
            if (B_CST(f0)) {
                frame->state = 1;
                /* Fall through to state 1 */
            } else {
                /* Push frame for f0 */
                frame->state = 1;
                if (!util_stack_push(&stack)) {
                    util_stack_free(&stack);
                    return;
                }
                {
                    struct UtilStackFrame *child = util_stack_current(&stack);
                    child->f = f0;
                    child->state = 0;
                }
                break;
            }
            /* Fall through */

        case 1: /* After f0 */
            /* Get f1 */
            f1 = B_GET_BDDP(fp->f1);
            if (B_CST(f1)) {
                goto pop_frame;
            } else {
                /* Push frame for f1 */
                frame->state = 2;
                if (!util_stack_push(&stack)) {
                    util_stack_free(&stack);
                    return;
                }
                {
                    struct UtilStackFrame *child = util_stack_current(&stack);
                    child->f = f1;
                    child->state = 0;
                }
                break;
            }

        case 2: /* After f1 */
            goto pop_frame;
        }
        continue;

    pop_frame:
        util_stack_pop(&stack);
    }

    util_stack_free(&stack);
}

/* ============================================================
 * Iterative version of dump()
 * ============================================================ */
static void dump_iterative(bddp f)
{
    struct UtilStack stack;
    struct UtilStackFrame *frame;
    struct B_NodeTable *fp;
    bddp nx, f0, f1;
    bddvar v;

    if (B_CST(f)) return;

    util_stack_init(&stack);

    /* Push initial frame */
    if (!util_stack_push(&stack)) {
        util_stack_free(&stack);
        return;
    }
    frame = util_stack_current(&stack);
    frame->f = f;
    frame->state = 0;

    while (stack.top >= 0) {
        frame = util_stack_current(&stack);
        fp = B_NP(frame->f);

        switch (frame->state) {
        case 0: /* Initial state */
            /* Check visit flag */
            nx = B_GET_BDDP(fp->nx);
            if (nx & B_CST_MASK) {
                /* Already visited */
                goto pop_frame;
            }

            /* Set visit flag */
            B_SET_BDDP(fp->nx, nx | B_CST_MASK);

            /* Get f0 (absolute value for dump) */
            f0 = B_GET_BDDP(fp->f0);
            f0 = B_ABS(f0);
            if (B_CST(f0)) {
                frame->state = 1;
                /* Fall through to state 1 */
            } else {
                /* Push frame for f0 */
                frame->state = 1;
                if (!util_stack_push(&stack)) {
                    util_stack_free(&stack);
                    return;
                }
                {
                    struct UtilStackFrame *child = util_stack_current(&stack);
                    child->f = f0;
                    child->state = 0;
                }
                break;
            }
            /* Fall through */

        case 1: /* After f0 */
            /* Get f1 */
            f1 = B_GET_BDDP(fp->f1);
            if (B_CST(f1)) {
                frame->state = 2;
                /* Fall through to state 2 */
            } else {
                /* Push frame for f1 */
                frame->state = 2;
                if (!util_stack_push(&stack)) {
                    util_stack_free(&stack);
                    return;
                }
                {
                    struct UtilStackFrame *child = util_stack_current(&stack);
                    child->f = f1;
                    child->state = 0;
                }
                break;
            }
            /* Fall through */

        case 2: /* After f1 - now dump this node */
            /* Dump this node */
            v = B_VAR_NP(fp);
            f0 = B_GET_BDDP(fp->f0);
            f0 = B_ABS(f0);
            f1 = B_GET_BDDP(fp->f1);

            printf("N");
            printf(B_BDDP_FD, B_NDX(frame->f));
            printf(" = [V%d(%d), ", v, Var[v].lev);
            if(B_CST(f0)) printf(B_BDDP_FD, B_VAL(f0));
            else { printf("N"); printf(B_BDDP_FD, B_NDX(f0)); }
            printf(", ");
            if(B_NEG(f1)) putchar('~');
            if(B_CST(f1)) printf(B_BDDP_FD, B_ABS(B_VAL(f1)));
            else { printf("N"); printf(B_BDDP_FD, B_NDX(f1)); }
            printf("]");
            if(B_Z_NP(fp)) printf(" #Z");
            printf("\n");
            goto pop_frame;
        }
        continue;

    pop_frame:
        util_stack_pop(&stack);
    }

    util_stack_free(&stack);
}

bddp bddused() { return NodeUsed; }

bddp bddsize(bddp f)
/* Returns 0 for bddnull */
{
  bddp num;
  struct B_NodeTable *fp;

  if(f == bddnull) return 0;
  if(B_CST(f)) return 0; /* Constant */
  if((fp=B_NP(f))>=Node+NodeSpc || fp->varrfc == 0)
    err("bddsize: Invalid bddp", f, ExceptionType::InvalidBDDValue);

  num = count(f);
  reset(f);
  return num;
}

bddp bddvsize(bddp *p, int lim)
/* Returns 0 for bddnull */
{
  bddp num;
  struct B_NodeTable *fp;
  int n, i;

  /* Check operand */
  n = lim;
  for(i=0; i<n; i++)
  {
    if(p[i] == bddnull)
    {
      n = i;
      break;
    }
    if(!B_CST(p[i])&&
       ((fp=B_NP(p[i]))>=Node+NodeSpc || fp->varrfc==0))
      err("bddvsize: Invalid bddp", p[i], ExceptionType::InvalidBDDValue);
  }
  num = 0;
  for(i=0; i<n; i++) num += count(p[i]);
  for(i=0; i<n; i++) reset(p[i]);
  return num;
}

/* Recursive version of count (internal) */
static bddp count_recursive(bddp f)
{
  bddp nx;
  bddp c;
  struct B_NodeTable *fp;

  if(B_CST(f)) return 0; /* Constant */
  fp = B_NP(f);

  /* Check visit flag */
  nx = B_GET_BDDP(fp->nx);
  if(nx & B_CST_MASK) return 0;

  BDD_RECUR_INC;
  c = count_recursive(B_GET_BDDP(fp->f0)) + count_recursive(B_GET_BDDP(fp->f1)) + 1U ;
  BDD_RECUR_DEC;

  /* Set visit flag */
  B_SET_BDDP(fp->nx, nx | B_CST_MASK);

  return c;
}

bddp count(bddp f)
{
  /* Use iterative version when variable count exceeds threshold */
  if (VarUsed > UTIL_RECURSION_THRESHOLD) {
    return count_iterative(f);
  }
  return count_recursive(f);
}

/* Recursive version of dump (internal) */
static void dump_recursive(bddp f)
{
  bddp nx, f0, f1;
  bddvar v;
  struct B_NodeTable *fp;

  if(B_CST(f)) return; /* Constant */
  fp = B_NP(f);

  /* Check visit flag */
  nx = B_GET_BDDP(fp->nx);
  if(nx & B_CST_MASK) return;

  /* Set visit flag */
  B_SET_BDDP(fp->nx, nx | B_CST_MASK);

  /* Dump its subgraphs recursively */
  v = B_VAR_NP(fp);
  f0 = B_GET_BDDP(fp->f0);
  f0 = B_ABS(f0);
  f1 = B_GET_BDDP(fp->f1);
  BDD_RECUR_INC;
  dump_recursive(f0);
  dump_recursive(f1);
  BDD_RECUR_DEC;

  /* Dump this node */
  printf("N");
  printf(B_BDDP_FD, B_NDX(f));
  printf(" = [V%d(%d), ", v, Var[v].lev);
  if(B_CST(f0)) printf(B_BDDP_FD, B_VAL(f0));
  else { printf("N"); printf(B_BDDP_FD, B_NDX(f0)); }
  printf(", ");
  if(B_NEG(f1)) putchar('~');
  if(B_CST(f1)) printf(B_BDDP_FD, B_ABS(B_VAL(f1)));
  else { printf("N"); printf(B_BDDP_FD, B_NDX(f1)); }
  printf("]");
  if(B_Z_NP(fp)) printf(" #Z");
  printf("\n");
}

void dump(bddp f)
{
  /* Use iterative version when variable count exceeds threshold */
  if (VarUsed > UTIL_RECURSION_THRESHOLD) {
    dump_iterative(f);
    return;
  }
  dump_recursive(f);
}

/* Recursive version of reset (internal) */
static void reset_recursive(bddp f)
{
  bddp nx;
  struct B_NodeTable *fp;

  if(B_CST(f)) return; /* Constant */
  fp = B_NP(f);

  /* Check visit flag */
  nx = B_GET_BDDP(fp->nx);
  if(nx & B_CST_MASK)
  {
    /* Reset visit flag */
    B_SET_BDDP(fp->nx, nx & ~B_CST_MASK);
    BDD_RECUR_INC;
    reset_recursive(B_GET_BDDP(fp->f0));
    reset_recursive(B_GET_BDDP(fp->f1));
    BDD_RECUR_DEC;
  }
}

void reset(bddp f)
{
  /* Use iterative version when variable count exceeds threshold */
  if (VarUsed > UTIL_RECURSION_THRESHOLD) {
    reset_iterative(f);
    return;
  }
  reset_recursive(f);
}

int mp_add(struct B_MP *p, bddp ix)
{
  int len, i;
  bddp c, *wp;

  if(ix == B_MP_NULL) return 1;
  len = B_MP_LEN(ix);
  if(len) wp = mptable[len-1].word+(B_MP_VAL(ix)*len);
  else { wp = &ix; len = 1; }
  while(p->len < len) p->word[p->len++] = 0;

  c = 0;
  for(i=0; i<p->len; i++)
  {
    p->word[i] += c;
    c = (p->word[i] >= c)? 0: 1;
    if(i < len)
    {
      p->word[i] += wp[i];
      c = (p->word[i] >= wp[i])? c: 1;
    }
  }
  if(c)
  {
    if(p->len == B_MP_LMAX)
    {
      for(i=0; i<p->len; i++) p->word[i] = ~((bddp)0);
      return 1;
    }
    p->word[p->len++] = c;
  }
  return 0;
}

[[noreturn]] int err(const char *msg, bddp num, ExceptionType exType)
{
  const int msg_buf_size = 1024;
  char msg_buf[msg_buf_size];
  /* The whole message has to be built by a single snprintf: every snprintf
     call starts writing at the beginning of the buffer, so a sequence of
     calls (as in the fprintf(stderr, ...) chain this code was derived from)
     would leave only the last fragment and drop msg and num. */
  snprintf(msg_buf, msg_buf_size,
           "***** ERROR  %s ( " B_BDDP_FX " ) *****\n"
           " NodeLimit : " B_BDDP_FD "\t NodeSpc : " B_BDDP_FD
           "\t VarSpc : %d\n"
           " CacheSpc : " B_BDDP_FD "\t NodeUsed : " B_BDDP_FD
           "\t VarUsed : %d\n",
           msg, num, NodeLimit, NodeSpc, VarSpc, CacheSpc, NodeUsed, VarUsed);

  std::string errorMsg(msg_buf);

  // Throw appropriate exception based on exType
  switch (exType) {
    case ExceptionType::InvalidBDDValue:
      throw BDDInvalidBDDValueException(errorMsg, num);
    case ExceptionType::OutOfRange:
      throw BDDOutOfRangeException(errorMsg, num);
    case ExceptionType::OutOfMemory:
      throw BDDOutOfMemoryException(errorMsg, num);
    case ExceptionType::FileFormat:
      throw BDDFileFormatException(errorMsg, num);
    case ExceptionType::InternalError:
    default:
      throw BDDInternalErrorException(errorMsg, num);
  }
}

} // namespace sapporobdd
