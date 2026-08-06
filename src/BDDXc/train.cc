/****************************************
 * BDD Graphic Viewer (SAPPORO-1.94)    *
 * (Variable sized element container)   *
 ****************************************/

#include "bddxc_internal.h"

namespace sapporobdd {

static container *NewContainer(int size)
{
  container *p = (container *)malloc(sizeof(container));
  if(p == 0)
    throw BDDOutOfMemoryException("BDDXc: cannot allocate a container",
                                  (bddp)sizeof(container));
  p->rest = TRAIN_CONTAINER_SIZE;
  p->next = 0;
  p->nodes = (dummy *)malloc((size_t)TRAIN_CONTAINER_SIZE * (size_t)size);
  if(p->nodes == 0)
  {
    free(p);
    throw BDDOutOfMemoryException("BDDXc: cannot allocate container elements",
                                  (bddp)((size_t)TRAIN_CONTAINER_SIZE
                                         * (size_t)size));
  }
  p->tail = p->nodes;
  return p;
}


void TrainReset(train *root, int size)
{
  container *p;

  root->size = size;
  p = NewContainer(size);
  root->head = p;
  root->tail = p;
  root->bound = 0;
}


void TrainFree(train *root)
{
  container *t, *tv;

  t = root->head;
  while(t != 0)
  {
    tv = t;
    t = t->next;
    free(tv->nodes);
    free(tv);
  }
  root->head = 0;
  root->tail = 0;
  root->size = 0;
  root->bound = 0;
}


static void AppendContainer(train *root)
{
  container *p = NewContainer(root->size);

  root->tail->next = p;
  root->tail = p;
}


void TrainLoad(train *root, const void *node)
{
  if(root->tail->rest == 0) AppendContainer(root);
  memcpy(root->tail->tail, node, (size_t)root->size);
  root->tail->tail += root->size;
  root->tail->rest--;
  root->bound++;
}


int TrainCheck(train *root, const void *node)
{
  container *t;
  int i, x;

  t = root->head;
  x = 0;
  while(t != 0)
  {
    for(i = 0; i < TRAIN_CONTAINER_SIZE - t->rest; i++)
    {
      if(memcmp(t->nodes + i * root->size, node, (size_t)root->size) == 0)
        return x + i;
    }
    t = t->next;
    x += TRAIN_CONTAINER_SIZE;
  }
  return EMPTY;
}


int TrainComp(train *root, const void *node, TrainCompFunc func)
{
  container *t;
  int i, x;

  t = root->head;
  x = 0;
  while(t != 0)
  {
    for(i = 0; i < TRAIN_CONTAINER_SIZE - t->rest; i++)
    {
      if((*func)(t->nodes + i * root->size, node)) return x + i;
    }
    t = t->next;
    x += TRAIN_CONTAINER_SIZE;
  }
  return EMPTY;
}


void *TrainIndex(train *root, int x)
{
  container *t;

  if(x < 0 || root->bound <= x)
    throw BDDOutOfRangeException("BDDXc: train index is out of range",
                                 (bddp)x);
  t = root->head;
  while(t != 0)
  {
    if(TRAIN_CONTAINER_SIZE - t->rest > x) return t->nodes + x * root->size;
    t = t->next;
    x -= TRAIN_CONTAINER_SIZE;
  }
  return 0;
}


int TrainBound(train *root)
{
  return root->bound;
}

} // namespace sapporobdd
