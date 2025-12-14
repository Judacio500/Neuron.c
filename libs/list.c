

#include"list.h"

LIST *createElement(void *data, float weight)
{
    LIST *newE = (LIST*)malloc(sizeof(LIST));

    if(!newE)
        return NULL;

    newE->data = data;
    newE->next = NULL;
    newE->prev = NULL;
    newE->weight = weight; // Aqui tenia  un 0

    return newE;
}

QUEUE *createWrap()
{
    QUEUE *newQ = (QUEUE*)malloc(sizeof(QUEUE));

    if(!newQ)
        return NULL;

    newQ->first = NULL;
    newQ->last = NULL;

    return newQ;
}

int insertList(LIST **l, void *data, float weight)
{
    LIST *newE = createElement(data,weight);

    if(!newE)
        return -1;

    newE->next = *l;
    *l = newE;
    
    return 0;
}

int append(QUEUE **q, void *data, float weight)
{
    if(!*q)
    {
        *q = createWrap();
        if(!*q)
            return -1;
    }

    LIST *newE = createElement(data,weight);

    if(!newE)
        return -1;

    if(!(*q)->first && !(*q)->last)
    {
        (*q)->first = newE;
        (*q)->last  = newE;
        return 0;
    } 

    (*q)->last->next = newE;
    (*q)->last = newE;

    return 0;
}

int orderedInsert(LIST **l, void *data, float weight)
{
    if(!l)
        return -1;
    
    while(*l && (*l)->weight < weight)
        l = &(*l)->next;

    insertList(l,data,weight);
    return 0;
}

LIST *pop(LIST **l)
{
    if(!l || !*l)
        return NULL;

    LIST *pElem = *l;
    *l = pElem->next;
    pElem->next = NULL;

    return pElem;
}

void *popData(LIST **l)
{
    if(!l || !*l)
        return NULL;

    LIST *pElem = *l;
    void *data = pElem->data;

    *l = pElem->next;

    free(pElem);

    return data;
}

LIST *dequeue(QUEUE *q)
{
    if(!q)
        return NULL;

    LIST *pElem = pop(&(q->first));

    if(!pElem)
        return NULL;

    if(!q->first)
        q->last = NULL;

    return pElem;
}

void *dequeueData(QUEUE *q)
{
    if(!q)
        return NULL;

    void *data = popData(&(q->first));

    if(!data)
        return NULL;

    if(!q->first)
        q->last = NULL;

    return data;
}

int freeList(LIST **l, Dest destroy)
{
    while(*l)
    {
        void *data = popData(l); // Eventualmente l->NULL

        if(destroy)              // Si no pasamos Dest los datos dentro de la lista no se destruyen
            destroy(data);       // solo la envoltura, esto es relevante porque hay datos que viven en otra
                                 // estructura y que pueden necesitarse en una lista solo temporalmente
    }

    return 0;
}

int freeQueue(QUEUE *q, Dest destroy)
{
    if(!q)
        return -1;

    freeList(&q->first, destroy); //Liberamos su lista
    free(q);

    return 0;
}

void *extract(LIST **l, void *search, Comparator compare)
{
    if(!l)
        return NULL;

    while(*l)
    {
        LIST *current = *l;
        
        if(compare(current->data,search))
           return popData(l);
        
        l = &(*l)->next;
    }
    return NULL;
}

int handleInsert(LIST **l, void *data, float weight, enum listType t)
{
    int returnCode;

    switch(t)
    {
        case SIMPLE:
            returnCode = insertList(l, data, weight);
            break;
        case DOUBLE:
            returnCode = insertDouble(l,data,weight);
    }
    
    return returnCode;
}

int handleAppend(QUEUE **q, void *data, float weight, enum listType t)
{
    int returnCode;

    switch(t)
    {
        case SIMPLE:
            returnCode = append(q, data, weight);
            break;
        case DOUBLE:
            returnCode = appendDouble(q,data,weight);
    }
    
    return returnCode;
}

int insertDouble(LIST **l, void *data, float weight)
{
    LIST *newE = createElement(data,weight);

    if(!newE)
        return -1;

    newE->next = *l;
    if(*l)
        (*l)->prev = newE;
    *l = newE;
    
    return 0;
}

int appendDouble(QUEUE **q, void *data, float weight)
{
        if(!*q)
    {
        *q = createWrap();
        if(!*q)
            return -1;
    }

    LIST *newE = createElement(data,weight);

    if(!newE)
        return -1;

    if(!(*q)->first && !(*q)->last)
    {
        (*q)->first = newE;
        (*q)->last  = newE;
        return 0;
    } 

    (*q)->last->next = newE;
    newE->prev = (*q)->last;
    (*q)->last = newE;

    return 0;
}

LIST *popDouble(LIST **l);  // La defino luego, es mas situacional que popData

void *popDataDouble(LIST **l)
{
    void *data = popData(l);
    
    if(!data)
        return NULL;

    if(*l)
        (*l)->prev = NULL;

    return data;

}

LIST *dequeueDouble(QUEUE *q); // Lo mismo que popDouble

void *dequeueDataDouble(QUEUE *q)
{
    void *data = dequeueData(q);

    if(!data)
        return NULL;

    if(q->first)                 // Como dequeue data llama a popData esencialmente
        q->first->prev = NULL;   // (*q)->first es lo mismo que (*l) en las listas
                                    // y por ende lo unico que hace falta desconectar es el prev
                                    // del inicio

    return data;
}

int copyList(LIST *src, LIST **dst, enum listType t) // No es deep copy para eso necesitariamos un callback de copia
{
    if(!dst)
        return NULL;

    while(src)
    {
        handleInsert(dst, src->data, 0, t);
        src = src->next;
    }

    return 0;
}