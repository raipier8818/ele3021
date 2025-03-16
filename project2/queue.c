#include "queue.h"
#include "types.h"

int is_empty(struct queue *q)
{
    return q->size == 0;
}

int is_full(struct queue *q)
{
    return q->size == NPROC;
}

void init(struct queue *q)
{
    q->size = 0;
    q->head = 0;
    q->tail = 0;
}

void push(struct queue *q, int target)
{
    if (is_full(q))
    {
        return;
    }
    q->idxs[q->tail] = target;
    q->tail = (q->tail + 1) % (NPROC + 1);
    q->size++;
}

void pop(struct queue *q)
{
    if (is_empty(q))
    {
        return;
    }
    q->head = (q->head + 1) % (NPROC + 1);
    q->size--;
}

int front(struct queue *q)
{
    if (is_empty(q))
    {
        return -1;
    }
    return q->idxs[q->head];
}

void delete(struct queue *q, int target)
{
    if (is_empty(q))
    {
        return;
    }
    for (int i = q->head; i != q->tail; i = (i + 1) % (NPROC + 1))
    {
        if (q->idxs[i] == target)
        {
            for (int j = i; j != q->tail; j = (j + 1) % (NPROC + 1))
            {
                q->idxs[j] = q->idxs[(j + 1) % (NPROC + 1)];
            }
            q->tail = (q->tail - 1 + (NPROC + 1)) % (NPROC + 1);
            q->size--;
            return;
        }
    }
}
