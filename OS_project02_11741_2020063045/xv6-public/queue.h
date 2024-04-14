#include "types.h"
#include "param.h"

#define QUEUE_SIZE (NPROC + 1)

struct queue
{
    int idxs[QUEUE_SIZE];
    int size;
    int head;
    int tail;
};

int is_empty(struct queue *q);
int is_full(struct queue *q);
void init(struct queue *q);
void push(struct queue *q, int target);
void pop(struct queue *q);
int front(struct queue *q);
void delete(struct queue *q, int target);