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
    newL->currentLayer = NULL;

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

int addLayer(MODEL *network, int nNeurons, Actv activation)
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

        handleInsert(&newL->perceptrons,newN,0,SIMPLE); // Adding perceptron to perceptron list for the layer
    }

    handleAppend(&network->layers,newL,0,DOUBLE); // Adding layer to layer list

    if(!network->currentLayer)
        network->currentLayer = network->layers->first; // setting the current layer to the first layer if there wasnt
                                                        // a current layer yet 

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

int stepForward(MODEL *network) 
{

    if (!network->currentLayer || !network->currentLayer->next) 
    {
        return 1;
    }

    LAYER *currL = (LAYER*)network->currentLayer->data;
    LAYER *nextL = (LAYER*)network->currentLayer->next->data;
    
    LIST *nodeList = nextL->perceptrons;

    while(nodeList) 
    {
        NODE *n = (NODE*)nodeList->data;
        PERCEPTRON *p = (PERCEPTRON*)n->data;

        p->output = p->bias;    // we initialize the output in a value with let's say
                                // "effect" since later the operations include a multiplication
                                // if we do not initialize this value we will end with a big 0
        nodeList = nodeList->next;
    }

    nodeList = currL->perceptrons;

    while(nodeList) 
    {
        /*
            Here lives the formula for the accumulated sum
            Each node in the current layer checks their adjacencies
        */

        NODE *currNode = (NODE*)nodeList->data;
        PERCEPTRON *currP = (PERCEPTRON*)currNode->data;
        float currOutput = currP->output; 

        EDGE *connections = currNode->firstEdge; 
        
        while(connections) 
        {
            NODE *destNode = connections->destinationNode;
            PERCEPTRON *destP = (PERCEPTRON*)destNode->data;
            destP->output += currOutput * connections->weight;
            connections = connections->next;
        }
        
        nodeList = nodeList->next;
    }

    // Check the activation for the next layer

    nodeList = nextL->perceptrons;
    
    while(nodeList) 
    {
        NODE *n = (NODE*)nodeList->data;            // Graph wrapper
        PERCEPTRON *p = (PERCEPTRON*)n->data;       // For each perceptron
        p->output = nextL->activation(p->output);   // we apply to it's output, the layer's activation function
                                                    // and save the result
        nodeList = nodeList->next;                  // then visit the next perceptron
    }

    // iterate the layer pointer
    network->currentLayer = network->currentLayer->next;

    // if there is no next layer, we are sitting at the output layer
    if (network->currentLayer->next == NULL) 
        return 1; 
    return 0; // "Keep dancing"
}

float sigmoid(float x) 
{
    return 1 / (1 + expf(-x));
}

float d_sigmoid(float output) 
{
    return output * (1 - output);
}