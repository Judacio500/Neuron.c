#ifndef NEURON_H
#define NEURON_H

#include<stdlib.h>
#include<math.h>
#include<time.h>
#include"graph.h"
#include"list.h"

 /*
    This library uses rand() for weight and bias initialization.
    Please call srand(time(NULL)) or srand(seed) in your main() 
    before compiling the network.
 */

typedef float(*Actv)(float weightedSum);
typedef int (*fitFunc)(struct model *network, float *inputData, float *targetData, int dataRows, int epochs, float learningRate); 

typedef struct perceptron
{                           
    float output;
    float bias;            
    float gradient;
}PERCEPTRON;

typedef struct layer
{
    int nNeurons;
    struct list *perceptrons;   // Layers have a list of all the perceptrons in them, is a differen way to acces
                                // the network, layers do not hold the struct perceptron but a NODE
                                // from graph.h
    Actv activation;           // This is a general activation function, each layer architecture
                                // is decided beforehand and an activation function must be selected
    Actv dActivation;          // the derivative of said activation (currently just featuring sigmoid)
                                // so it will be hard coded but adding this pointer makes it so the library 
                                // is scalable
}LAYER;

typedef struct model
{
    // This model though is implemented as a FSM
    // since it's gonna be used for visualization and pedagogic purposes

    // Right now this code just features MLP
    /*
        AKA Multilayer Perceptron, so this structure could not exist
        but since it would be interesting to implement different kinds of
        Neural networks, this structure is left as an Scalability decision   
    
    void *metadata; // related to the model we want to create

    */
    struct queue *layers;        // Double linked queue to acces all the layers
    struct list *currentLayer;
    float learningRate;

    // once we hit fit we will need:
    float *inputs;           // An input flattened array    
    float *targets;          // A target flattened array

    // Then there is this, interesting decission for this experimental library
    // fit is callback for 2 reasons, to simulate the classic modelo.fit with modelo->fit()
    // and because this allows us to create a fit function that works differently such as 
    // a straighter way to fit the model instead of the weird automaton thing we did in the .c

    fitFunc fit; 

}MODEL;

MODEL *createModel();
LAYER *createLayer(int nNeurons, Actv activation);
PERCEPTRON *createPerceptron();
int addLayer(MODEL *network, int nNeurons, Actv activation);
int compileNetwork(MODEL *network);
float randomWeight();
int stepForward(MODEL *network); 
int stepBackward(MODEL *network);
int train(MODEL *network, float *inputData, float *targetData, int dataRows, int epochs, float learningRate);
float sigmoid(float x);
float dSigmoid(float output);
float predict(MODEL *network, float *input); 

#endif