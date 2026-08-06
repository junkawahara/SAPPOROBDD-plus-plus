/****************************************
 * BDD Graphic Viewer (SAPPORO-1.94)    *
 * (Variable sized element container)   *
 ****************************************/

#ifndef BDDXC_TRAIN_H
#define BDDXC_TRAIN_H

namespace sapporobdd {

/* Number of elements held by one container */
#define TRAIN_CONTAINER_SIZE 100

typedef char dummy;

struct container {
  dummy     *nodes;
  dummy     *tail;
  int       rest;
  container *next;
};

struct train {
  container *head;
  container *tail;
  int       size;
  int       bound;
};

/* Comparator given to TrainComp() */
typedef int (*TrainCompFunc)(const void *a, const void *b);

void  TrainReset(train *root, int size);
void  TrainFree(train *root);
void  TrainLoad(train *root, const void *node);
int   TrainCheck(train *root, const void *node);
int   TrainComp(train *root, const void *node, TrainCompFunc func);
void *TrainIndex(train *root, int x);
int   TrainBound(train *root);

} // namespace sapporobdd

#endif /* BDDXC_TRAIN_H */
