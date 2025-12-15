#include"neuron.h"

MODEL *createModel()
{
    MODEL *newM = (MODEL*)malloc(sizeof(MODEL));

    if(!newM)
        return NULL;

    newM->layers = createWrap(); // Queue wrapper for the node list
    newM->currentLayer = NULL;
    newM->learningRate = 0;
    newM->inputs = NULL;
    newM->targets = NULL;
    newM->fit = train;

    return newM; 
}

MLAYER *createLayer(int nNeurons, Actv activation)
{
    MLAYER *newL = (MLAYER*)malloc(sizeof(MLAYER));

    if(!newL)
        return NULL;

    newL->nNeurons = nNeurons;
    newL->activation = activation;
    newL->dActivation = dSigmoid;   // Hardcoded the activation derivative
                                    // this function will be assigned automatically when there is more functions
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

int addLayer(MODEL *network, int nNeurons, Actv activation)
{
    MLAYER *newL = createLayer(nNeurons,activation);

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
        MLAYER *A = (MLAYER*)iter->data;
        MLAYER *B = (MLAYER*)iter->next->data;
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

    MLAYER *currL = (MLAYER*)network->currentLayer->data;
    MLAYER *nextL = (MLAYER*)network->currentLayer->next->data;
    
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
            connections = connections->nextNode;
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

int stepBackward(MODEL *network) 
{
    if (!network->currentLayer) 
        return 1;

    MLAYER *currL = (MLAYER*)network->currentLayer->data;
    LIST *nodeList = currL->perceptrons;
    
    if (network->currentLayer->next == NULL) 
    { 
        int tIdx = 0;
        while(nodeList)
        {
            NODE *n = (NODE*)nodeList->data;
            PERCEPTRON *p = (PERCEPTRON*)n->data;


            // this is treated as a window, so targets is currently pointing
            // at the start of a certain "row" in the flattened array
            // each neuron represents what that neuron should be for each target column
            /*
                For example:
                Let's say we have 3 classes (this would be for a multi class problem)
                0 1 2 3 4 5 6 7 8
                1 0 0 0 0 1 0 1 0
                |   | |   | |   |
                
                The lines above define the dimensions of the array for each itteraation
                when we are sitting at position 0, we are defining that the neural network should (not must because crashes and segmentations are free)
                the same amount of output neurons as the amount of classes we have in the data set 
            */    
        
            float output = p->output;      // What we guessed
            float target = network->targets[tIdx];      // What it should be 

            
            // Gradient = Error * Derivative
            // Error = (target - output)
            // Derivative = dSigmoid(output)
            p->gradient = (target - output) * currL->dActivation(output); // output was already the sigmoid 
                                                                                  // value, hence we are just calling
                                                                                  // the derivative with the output of the neuron
            
            p->bias += network->learningRate * p->gradient;

            nodeList = nodeList->next;
            tIdx++;
        }
    } 
    
    else 
    {
        while(nodeList) 
        {
            NODE *n = (NODE*)nodeList->data;
            PERCEPTRON *p = (PERCEPTRON*)n->data;
            float output = p->output;
            
            float acumGradient = 0;

            EDGE *connection = n->firstEdge;
            while(connection) 
            {
                NODE *nextN = connection->destinationNode;  // extract the nodes from the next layer
                                                            // one at a time
                PERCEPTRON *nextP = (PERCEPTRON*)nextN->data;

                // Chain Rule
                // My fault = (Next Node's Fault) * (Strength of Connection)
                acumGradient += nextP->gradient * connection->weight; 
                
                // NewWeight = OldWeight + (LearningRate * Gradient * MyInput)
                // Basically 
                // The value i sent you (MyInput)
                // How much i want it to affect the gradient (LearningRate)
                // In which direction should we step to get it right (Gradient)
                
                // This is beautiful because we just depend from the node updated gradient
                // effectively propagating the error
                float delta = network->learningRate * nextP->gradient * output;
                connection->weight += delta;

                connection = connection->nextNode; // Next node i affected
            }
            
            // Now we know the total blame (acumGradient).
            // How much did i (the perceptron) affected that the result was not the expected
            // Multiply by my own derivative to save it for the Previous Layer.
            p->gradient = acumGradient * currL->dActivation(output);
            p->bias += network->learningRate * p->gradient;

            nodeList = nodeList->next;
        }
    }

    network->currentLayer = network->currentLayer->prev;

    if (network->currentLayer == NULL) 
        return 1; 
    return 0;
}

int train(MODEL *network, float *inputData, float *targetData, int dataRows, int epochs, float learningRate) 
{
    network->learningRate = learningRate;

    // The first layer defines the inputDim, the last layer defines the outputDim
    /*
        So if layer 1 has 4 neurons then the input in the flattened array has a length
        of 4 before starting the next row
    */
    MLAYER *firstLayer = (MLAYER*)network->layers->first->data;
    MLAYER *lastLayer  = (MLAYER*)network->layers->last->data;

    int inputCols  = firstLayer->nNeurons; 
    int outputCols = lastLayer->nNeurons; 

    printf("Training... (Epochs: %d, Samples: %d, Inputs: %d, Outputs: %d)\n", epochs, dataRows, inputCols, outputCols);

    for (int e = 0; e < epochs; e++) 
    {
        float totalError = 0;

        for (int i = 0; i < dataRows; i++) 
        {
            // This is were we move the window for the current row
            // since inputData is saving the original position of the array
            // X and y (inputs and targets)
            // once we reestart the loop, the position will be calculated correctly again
            // so to speak, network->inputs/targets is an iterator inside the dataframe
            // this window's end is defined by the amount of neurons in the input/output layer
            // for inputs and targets array respectively

            network->inputs  = inputData  + (i * inputCols); // Math to find the row in the flattened array 
            network->targets = targetData + (i * outputCols);

            LIST *nodeList = firstLayer->perceptrons;
            int k = 0;

            // Load the raw data in the neurons
            // stepForward already handles the logic to decide
            // wether the neuron activates or not
            while(nodeList) 
            {
                NODE *n = (NODE*)nodeList->data;
                PERCEPTRON *p = (PERCEPTRON*)n->data;
    
                p->output = network->inputs[k]; 
                
                nodeList = nodeList->next;
                k++;
            }

            network->currentLayer = network->layers->first; 
            
            // stepForward until the returned value is 1
            // which means that the pass forward state is finished
            while(!stepForward(network));

            // HARDCODED MSE 
            // the whole block can be changed for 
            // metric(params) 

            // We extract the error from the last layer
            LIST *outNodes = lastLayer->perceptrons;
            int tIdx = 0;
            
            while(outNodes) 
            {
                NODE *n = (NODE*)outNodes->data;
                PERCEPTRON *p = (PERCEPTRON*)n->data;
                
                // Difference between what it should be and what it actualy is
                float error = network->targets[tIdx] - p->output;
                
                // Squeared Error
                totalError += (error * error);
                
                outNodes = outNodes->next;
                tIdx++;
            }

            // stepBackwards until the returned value is 1
            // which means the backpropagation state is over
            while(stepBackward(network) == 0);

            if (e % 1 == 0) 
            {
                // MSE
                float mse = totalError / (dataRows * outputCols);
                printf("Epoch %d completed | MSE: %f\n", e, mse);
            }
            
        }
    }

    return 0;
}

float predict(MODEL *network, float *input)
{
    MLAYER *firstL = (MLAYER*)network->layers->first->data;
    LIST *nodeList = firstL->perceptrons;
    int k = 0;

    while(nodeList)
    {
        NODE *n = (NODE*)nodeList->data;            // Graph wrapper
        PERCEPTRON *p = (PERCEPTRON*)n->data;       // For each perceptron

        p->output = input[k];                       // Inject input value
        nodeList = nodeList->next;
        k++;
    }

    network->currentLayer = network->layers->first;

    while(!stepForward(network));

    MLAYER *lastL = (MLAYER*)network->layers->last->data;
    
    NODE *outNode = (NODE*)lastL->perceptrons->data;
    PERCEPTRON *outP = (PERCEPTRON*)outNode->data;

    return outP->output;
}

float sigmoid(float x) 
{
    return 1 / (1 + expf(-x));
}

float dSigmoid(float output) 
{
    return output * (1 - output);
}