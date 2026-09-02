/*****************************************
*  BDD Package (SAPPORO-1.94)   - Util  *
*  (C) Shin-ichi MINATO  (Apr. 19, 2022) *
*  Split from bddc.cc for modularity     *
******************************************/

#include "bddc_internal.h"
#include <string>
#include <new>

namespace sapporobdd {

/* ============================================================
 * Choice between the recursive and the iterative traversals
 *
 * The recursive versions descend one machine frame per level of the graph,
 * so they are used only while the remaining recursion budget covers the
 * number of variables -- b_recursion_fits() -- which with an untouched
 * budget is VarUsed < BDD_RecurLimit.  (The threshold used to be a second
 * copy of the limit's value, compared against VarUsed alone.)  The public
 * entry points bddsize(), bdddump() and bddexport() are called from the
 * top level, but the test holds wherever they are called from.
 * ============================================================ */
#define UTIL_USE_ITERATIVE() (!b_recursion_fits())

/* ============================================================
 * Stack structure for iterative traversal
 * ============================================================ */
#define UTIL_STACK_INIT_SIZE 256

struct UtilStackFrame {
    bddp f;           /* Current node */
    bddp c0;          /* Count from f0 (for count_iterative) */
    unsigned char state;  /* 0: init, 1: after f0, 2: after f1 */
};

/* top counts the frames in use; the sizes are bddp so that the doubling
   below cannot overflow a signed int on a graph over 2^31 levels, and the
   allocations go through the overflow-checked B_MALLOC/B_REALLOC. */
struct UtilStack {
    struct UtilStackFrame *frames;
    bddp top;
    bddp capacity;
};

static int util_stack_init(struct UtilStack *stack) {
    /* the malloc used to go unchecked, and the first frame write after a
       failure dereferenced the null pointer */
    stack->frames = B_MALLOC(struct UtilStackFrame, UTIL_STACK_INIT_SIZE);
    stack->top = 0;
    stack->capacity = stack->frames? UTIL_STACK_INIT_SIZE: 0;
    return stack->frames != 0;
}

static void util_stack_free(struct UtilStack *stack) {
    if (stack->frames) {
        free(stack->frames);
        stack->frames = 0;
    }
}

static int util_stack_push(struct UtilStack *stack) {
    if (stack->top >= stack->capacity) {
        bddp new_capacity = stack->capacity? stack->capacity * 2:
                                             UTIL_STACK_INIT_SIZE;
        struct UtilStackFrame *new_frames;
        if (new_capacity < stack->capacity) return 0; /* wrapped around */
        new_frames = B_REALLOC(stack->frames, struct UtilStackFrame,
                               new_capacity);
        if (!new_frames) return 0;
        stack->frames = new_frames;
        stack->capacity = new_capacity;
    }
    stack->top++;
    return 1;
}

static void util_stack_pop(struct UtilStack *stack) {
    if (stack->top > 0) stack->top--;
}

static struct UtilStackFrame *util_stack_current(struct UtilStack *stack) {
    if (stack->top > 0) return &stack->frames[stack->top - 1];
    return 0;
}

static struct UtilStackFrame *util_stack_parent(struct UtilStack *stack) {
    if (stack->top > 1) return &stack->frames[stack->top - 2];
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

    /* A failure used to make this return 0, which bddsize() answered as a
       normal node count of 0.  Nothing is marked yet at this point, so
       throwing here is clean; the push failures further down throw as well,
       and the callers' handlers clear the visit flags via reset_aborted(). */
    if (!util_stack_init(&stack))
        err("count_iterative: memory allocation failed", 0,
            ExceptionType::OutOfMemory);
    util_stack_push(&stack); /* capacity is fresh; cannot fail */
    frame = util_stack_current(&stack);
    frame->f = f;
    frame->state = 0;
    frame->c0 = 0;

    while (stack.top > 0) {
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

            /* Get f0.  c0 is 0 from the frame's creation and stays so until
               the child for f0 reports its count. */
            f0 = B_GET_BDDP(fp->f0);
            frame->state = 1;
            if (!B_CST(f0)) {
                /* Push frame for f0 */
                if (!util_stack_push(&stack)) {
                    util_stack_free(&stack);
                    err("count_iterative: memory allocation failed", 0,
                        ExceptionType::OutOfMemory);
                }
                {
                    struct UtilStackFrame *child = util_stack_current(&stack);
                    child->f = f0;
                    child->state = 0;
                    child->c0 = 0;
                }
                break;
            }
            /* f0 is constant: nothing to count */
            /* fall through */

        case 1: /* After f0 */
            /* Get f1 */
            f1 = B_GET_BDDP(fp->f1);
            if (B_CST(f1)) {
                /* f1 is constant, count is 0: result = c0 + 0 + 1 */
                goto return_result;
            } else {
                /* Push frame for f1 */
                frame->state = 2;
                if (!util_stack_push(&stack)) {
                    util_stack_free(&stack);
                    err("count_iterative: memory allocation failed", 0,
                        ExceptionType::OutOfMemory);
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
        /* An already visited node contributes 0, which is what the parent's
           accumulator holds for this child anyway; nothing to add. */
        if (stack.top == 1) final_result = 0;
        util_stack_pop(&stack);
        continue;

    return_result:
        /* This node's count is c0 (the f0 subtree, added by that child) plus
           the f1 subtree (added into c0 by that child) plus 1 for itself.
           A parent in state 1 is waiting for its f0 count, one in state 2
           for its f1 count; both accumulate into the parent's c0. */
        parent = util_stack_parent(&stack);
        if (parent) {
            parent->c0 += frame->c0 + 1;
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

/* Last-resort clearing of the visit flags, used only when the heap stack of
   reset_iterative() cannot be (re)allocated.  Clearing the flags must not
   fail -- a flag left behind corrupts the node hash chains -- and it cannot
   allocate either, since it runs exactly when allocation fails.  It clears
   the flag of every node in the table: the flags are only ever set by one
   traversal at a time (count(), dump() or export), the whole of which is
   being reset, so no flag is lost that had to survive.  This replaces a
   recursion on the machine stack, whose depth was the length of the longest
   path and which crashed the process on a deep graph or a small stack. */
static void reset_fallback(void)
{
    struct B_NodeTable *fp;
    bddp nx;

    for (fp = Node; fp < Node + NodeSpc; fp++) {
        if (fp->varrfc == 0) continue;
        nx = B_GET_BDDP(fp->nx);
        if (nx & B_CST_MASK) {
            nx &= ~B_CST_MASK;
            B_SET_BDDP(fp->nx, nx);
        }
    }
}

static void reset_iterative(bddp f)
{
    struct UtilStack stack;
    struct UtilStackFrame *frame;
    struct B_NodeTable *fp;
    bddp nx, f0, f1;

    if (B_CST(f)) return;

    /* the failure used to be ignored (the malloc was not even checked), and
       the traversal silently stopped with flags still set */
    if (!util_stack_init(&stack)) {
        reset_fallback();
        return;
    }
    util_stack_push(&stack); /* capacity is fresh; cannot fail */
    frame = util_stack_current(&stack);
    frame->f = f;
    frame->state = 0;

    while (stack.top > 0) {
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
                    /* the stack cannot grow: clear everything that is left
                       with the last resort and stop */
                    util_stack_free(&stack);
                    reset_fallback();
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
                    reset_fallback();
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
 * Post-order traversal: visit(f, ctx) for every node reachable from f,
 * children first, each node once.  The visit flags are left set for the
 * caller to reset(); a visit that throws leaves them set as well, and
 * reset_aborted() clears them.  dump() and export_static() are the two
 * users; they used to be two copies of the same traversal, of which only
 * dump() had an iterative form, so a graph deeper than the recursion limit
 * could be counted and dumped but not exported.
 * ============================================================ */
static void traverse_postorder_iterative(bddp f, void (*visit)(bddp, void *),
                                         void *ctx)
{
    struct UtilStack stack;
    struct UtilStackFrame *frame;
    struct B_NodeTable *fp;
    bddp nx, f0, f1;

    if (B_CST(f)) return;

    /* a failure used to be silent, leaving a partial dump and, for the
       pushes below, visit flags still set */
    if (!util_stack_init(&stack))
        err("traverse_postorder: memory allocation failed", 0,
            ExceptionType::OutOfMemory);
    util_stack_push(&stack); /* capacity is fresh; cannot fail */
    frame = util_stack_current(&stack);
    frame->f = f;
    frame->state = 0;

    try {
    while (stack.top > 0) {
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

            /* Get f0 (absolute value: the inverter bit of a stored 0-edge
               is the ZDD flag, not an edge to a different node) */
            f0 = B_ABS(B_GET_BDDP(fp->f0));
            if (B_CST(f0)) {
                frame->state = 1;
                /* Fall through to state 1 */
            } else {
                /* Push frame for f0 */
                frame->state = 1;
                if (!util_stack_push(&stack))
                    err("traverse_postorder: memory allocation failed", 0,
                        ExceptionType::OutOfMemory);
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
                if (!util_stack_push(&stack))
                    err("traverse_postorder: memory allocation failed", 0,
                        ExceptionType::OutOfMemory);
                {
                    struct UtilStackFrame *child = util_stack_current(&stack);
                    child->f = f1;
                    child->state = 0;
                }
                break;
            }
            /* Fall through */

        case 2: /* After f1 - now visit this node */
            visit(frame->f, ctx);
            goto pop_frame;
        }
        continue;

    pop_frame:
        util_stack_pop(&stack);
    }
    }
    catch (...) {
        util_stack_free(&stack);
        throw;
    }

    util_stack_free(&stack);
}

static void traverse_postorder_recursive(bddp f, void (*visit)(bddp, void *),
                                         void *ctx)
{
  bddp nx, f0, f1;
  struct B_NodeTable *fp;

  if(B_CST(f)) return; /* Constant */
  fp = B_NP(f);

  /* Check visit flag */
  nx = B_GET_BDDP(fp->nx);
  if(nx & B_CST_MASK) return;

  /* Set visit flag */
  B_SET_BDDP(fp->nx, nx | B_CST_MASK);

  /* Visit its subgraphs recursively */
  f0 = B_ABS(B_GET_BDDP(fp->f0));
  f1 = B_GET_BDDP(fp->f1);
  BDD_RECUR_INC;
  traverse_postorder_recursive(f0, visit, ctx);
  traverse_postorder_recursive(f1, visit, ctx);
  BDD_RECUR_DEC;

  visit(f, ctx);
}

void traverse_postorder(bddp f, void (*visit)(bddp, void *), void *ctx)
{
  if (UTIL_USE_ITERATIVE()) traverse_postorder_iterative(f, visit, ctx);
  else traverse_postorder_recursive(f, visit, ctx);
}

/* ============================================================
 * Public observation functions
 * ============================================================ */

bddp bddused() { return NodeUsed; }

bddp bddsize(bddp f)
/* Returns 0 for bddnull */
{
  bddp num;
  int recur_count;

  if(f == bddnull) return 0;
  if(B_CST(f)) return 0; /* Constant */
  if(B_BAD_NODE(f))
    err("bddsize: Invalid bddp", f, ExceptionType::InvalidBDDValue);

  recur_count = BDD_RecurCount;
  try { num = count(f); }
  catch(...) { reset_aborted(&f, 1, recur_count); throw; }
  reset(f);
  return num;
}

bddp bddvsize(bddp *p, int lim)
/* The number of nodes shared by p[0..lim-1], up to the first bddnull.
   Returns 0 for bddnull. */
{
  bddp num;
  int n, i, recur_count;

  /* Check operands.  A negative lim used to be read as "no entries" and a
     null p with a positive lim was dereferenced. */
  if(lim < 0) err("bddvsize: Invalid lim", 0, ExceptionType::OutOfRange);
  if(p == 0 && lim > 0)
    err("bddvsize: Null array", 0, ExceptionType::InvalidBDDValue);
  n = lim;
  for(i=0; i<n; i++)
  {
    if(p[i] == bddnull)
    {
      n = i;
      break;
    }
    if(!B_CST(p[i]) && B_BAD_NODE(p[i]))
      err("bddvsize: Invalid bddp", p[i], ExceptionType::InvalidBDDValue);
  }
  num = 0;
  recur_count = BDD_RecurCount;
  try { for(i=0; i<n; i++) num += count(p[i]); }
  catch(...) { reset_aborted(p, n, recur_count); throw; }
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

  /* Set visit flag before descending, as export_static() and the iterative
     version do.  The graph is acyclic, so the subgraphs cannot reach this
     node again and the count is unchanged; marking first keeps the flagged
     region connected to the root, so that reset() starting at the root can
     still clear every flag when the traversal below is aborted. */
  B_SET_BDDP(fp->nx, nx | B_CST_MASK);

  BDD_RECUR_INC;
  c = count_recursive(B_GET_BDDP(fp->f0)) + count_recursive(B_GET_BDDP(fp->f1)) + 1U ;
  BDD_RECUR_DEC;

  return c;
}

bddp count(bddp f)
{
  if (UTIL_USE_ITERATIVE()) return count_iterative(f);
  return count_recursive(f);
}

/* Prints one node for dump().  The output goes to stdout unchecked; the
   public entry points test ferror(stdout) once the dump is complete. */
static void dump_node(bddp f, void *ctx)
{
  struct B_NodeTable *fp = B_NP(f);
  bddvar v = B_VAR_NP(fp);
  bddp f0 = B_ABS(B_GET_BDDP(fp->f0));
  bddp f1 = B_GET_BDDP(fp->f1);

  (void)ctx;
  printf("N");
  printf(B_BDDP_FD, B_NDX(f));
  printf(" = [V%u(%u), ", v, Var[v].lev);
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
  traverse_postorder(f, dump_node, 0);
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
  if (UTIL_USE_ITERATIVE()) reset_iterative(f);
  else reset_recursive(f);
}

void reset_aborted(bddp *p, int n, int recur_count)
/* Clears the visit flags of p[0..n-1] after a traversal (count(), dump() or
   export_static()) was aborted by an exception.  Those traversals borrow the
   nx field, which is the node hash chain pointer, as a visit flag and rely on
   reset() to restore it; leaving a flag behind corrupts the node table, so
   every caller has to run this on the way out. */
{
  int i;

  /* The unwound traversal frames never reached their BDD_RECUR_DEC, so the
     recursion counter is still as deep as the traversal got.  Put it back to
     the value the aborted call started from. */
  BDD_RecurCount = recur_count;

  /* The iterative version is used here whatever the variable count is.  It
     keeps its stack on the heap and never throws, so it can undo a traversal
     that stopped at the recursion limit -- reset_recursive() would hit that
     same limit before reaching the deepest flag, and an exception of its own
     would replace the one that aborted the traversal. */
  for(i=0; i<n; i++) if(p[i] != bddnull) reset_iterative(p[i]);
}

int mp_add(struct B_MP *p, bddp ix)
/* Adds the count ix (a plain number, or a reference into the
   multi-precision table) to p.  Returns 0 on success and 1 in two cases
   that the callers treat alike, as "the sum is not available": ix is
   B_MP_NULL (p is left untouched), or the sum does not fit in B_MP_LMAX
   words (p is left saturated at all ones and must not be read as a value).
   A reference that names no table entry is a corrupt handle and is reported
   as an internal error rather than read out of bounds. */
{
  int len, i;
  bddp c, *wp;

  if(ix == B_MP_NULL) return 1;
  len = B_MP_LEN(ix);
  if(len)
  {
    struct B_MPTable *mpt = &mptable[len-1];
    if(mpt->word == 0 || B_MP_VAL(ix) >= mpt->used)
      err("mp_add: invalid mp table reference", ix, ExceptionType::InternalError);
    wp = mpt->word+(B_MP_VAL(ix)*len);
  }
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
  /* The exception thrown below unwinds every library recursion frame between
     here and the caller, and none of those frames reaches its BDD_RECUR_DEC.
     Nothing inside the library resumes such a recursion (handlers like the
     one in bddsize() restore the counter they saved themselves before
     rethrowing), so once the exception reaches the user no library frame is
     left and the correct depth is 0.  Without this reset the counter stayed
     at the depth the aborted recursion had reached, and after a caught
     stack-overflow exception every later operation ran out of its recursion
     budget immediately -- permanently, since not even BDD_Init() reset it. */
  BDD_RecurCount = 0;

  const int msg_buf_size = 1024;
  char msg_buf[msg_buf_size];
  /* The whole message has to be built by a single snprintf: every snprintf
     call starts writing at the beginning of the buffer, so a sequence of
     calls (as in the fprintf(stderr, ...) chain this code was derived from)
     would leave only the last fragment and drop msg and num.  The message
     carries no trailing newline; that is the caller's to add when printing. */
  snprintf(msg_buf, msg_buf_size,
           "***** ERROR  %s ( " B_BDDP_FX " ) *****\n"
           " NodeLimit : " B_BDDP_FD "\t NodeSpc : " B_BDDP_FD
           "\t VarSpc : %u\n"
           " CacheSpc : " B_BDDP_FD "\t NodeUsed : " B_BDDP_FD
           "\t VarUsed : %u",
           msg, num, NodeLimit, NodeSpc, VarSpc, CacheSpc, NodeUsed, VarUsed);

  /* The exception object holds its message in a std::string, whose
     construction allocates -- and an out-of-memory error is raised exactly
     when the heap may be exhausted.  A std::bad_alloc from that construction
     would escape as an exception this library never promised; it is caught
     here and replaced by the library's own error, built from a message
     short enough to need no heap allocation. */
  try
  {
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
  catch(const std::bad_alloc&)
  {
    throw BDDOutOfMemoryException("out of memory", num);
  }
}

} // namespace sapporobdd
