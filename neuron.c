#include"neuron.h"

MODEL *createModel()
{
    MODEL *newM = (MODEL*)malloc(sizeof(MODEL));

    if(!newM)
        return NULL;

    newM->layers = createWrap(); // Queue wrapper for the node list

    return newM; 
}

LAYER *createLayer(int nNeurons, Actv activation)
{
    LAYER *newL = (LAYER*)malloc(sizeof(LAYER));

    if(!newL)
        return NULL;

    newL->nNeurons = nNeurons;
    newL->activation = activation;
    newL->perceptrons = NULL;

    return newL;
}

PERCEPTRON *createPerceptron()
{
    PERCEPTRON *newP = (PERCEPTRON*)malloc(sizeof(PERCEPTRON));

    if(!newP)
        return NULL;

    newP->output = 0;
    newP->gradient = 0;
    newP->bias = randomWeight();

    return newP;
}

int addLayer(struct queue **layers, int nNeurons, Actv activation)
{
    LAYER *newL = createLayer(nNeurons,activation);

    if(!newL)
        return -1;

    for(int i=0; i<nNeurons; i++)
    {
        PERCEPTRON *newP = createPerceptron();
        
        if(!newP)
            return -1; 

        NODE *newN = createNode(NULL,NULL,newP);

        if(!newN)
            return -1;

        // If a perceptron or node is not created we will destroy the layer
        // in it's respective check, later though, when we are refining details

        handleInsert(&newL->perceptrons,newN,0,SIMPLE); // Añadimos el perceptron a la lista de neuronas de la capa
    }

    handleAppend(layers,newL,0,DOUBLE); // Añadimos la capa a la lista de capas

    return 0;
}


int compileNetwork(MODEL *network)
{
    LIST *iter = network->layers->first;

    while(iter && iter->next)
    {
        LAYER *A = (LAYER*)iter->data;
        LAYER *B = (LAYER*)iter->next->data;
        LIST *percA = A->perceptrons;   
        
        for(int i = 0; i<A->nNeurons; i++)
        {
            NODE *currentA = (NODE*)percA->data;
            LIST *percB = B->perceptrons; 
            for(int j = 0; j<B->nNeurons; j++)
            {
                NODE *currentB = (NODE*)percB->data;

                addEdgeThrough(currentA,currentB,randomWeight(),1);

                percB = percB->next;
            }
            percA = percA->next;
        }

        iter = iter->next;
    }

    return 0;
}

float randomWeight()
{
    return ((float)rand() / (float)RAND_MAX) * 2 - 1;
}