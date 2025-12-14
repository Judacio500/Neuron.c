#ifndef LIST_H
#define LIST_H

#include<stdlib.h>

typedef int (*Dest)(void *data);
typedef int (*Comparator)(void *current, void *comparing);

typedef enum listType
{
    SIMPLE,
    DOUBLE
}LT;

typedef struct queue
{
    struct list *first;
    struct list *last;
}QUEUE;

typedef struct list
{
    void *data;             
    struct list *next;
    struct list *prev;
    float weight;   
}LIST;

LIST *createElement(void *data, float weight);
QUEUE *createWrap();
int insertList(LIST **l, void *data, float weight);
int append(QUEUE **r, void *data, float weight);
int orderedInsert(LIST **l, void *data, float weight);
LIST *pop(LIST **l);
void *popData(LIST **l);
LIST *dequeue(QUEUE *q);
void *dequeueData(QUEUE *q);
int freeList(LIST **l, Dest destroy);
void *extract(LIST **l, void *search, Comparator compare);
int handleInsert(LIST **l, void *data, float weight, enum listType t);
int handleAppend(QUEUE **l, void *data, float weight, enum listType t);
int insertDouble(LIST **l, void *data, float weight);
int appendDouble(QUEUE **l, void *data, float weight);
LIST *popDouble(LIST **l);
void *popDataDouble(LIST **l);
LIST *dequeueDouble(QUEUE *q);
void *dequeueDataDouble(QUEUE *q);


#endif