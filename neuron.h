#ifndef NEURON_H
#define NEURON_H

#include<stdlib.h>
#include<math.h>
#include<time.h>
#include"graph.h"
#include"ani.h"
#include"list.h"

 /*
    This library uses rand() for weight and bias initialization.
    Please call srand(time(NULL)) or srand(seed) in your main() 
    before creating the network.
 */

typedef float(Actv*)(float weightedSum);

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
    Actv *activation;           // This is a general activation function, each layer architecture
                                // is decided beforehand and an activation function must be selected
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
}MODEL;

MODEL *createModel();
LAYER *createLayer(int nNeurons, Actv activation);
PERCEPTRON *createPerceptron();
int addLayer(struct queue **layers, int nNeurons, Actv activation);
int compileNetwork(MODEL *network);
float randomWeight();


#endif