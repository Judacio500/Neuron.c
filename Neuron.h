#ifndef NEURON_H
#define NEURON_H

#include<stdlib.h>
#include<math.h>
#include"graph.h"
#include"ani.h"
#include"list.h"

typedef float(Actv*)(float weightedSum);

typedef struct perceptron
{                           
    float output;
    float bias;            
    float gradient;
}PERCEPTRON;

typedef struct layer
{
    struct list *perceptrons;   // Layers have a list of all the perceptrons in them, is a differen way to acces
                                // the network, layers do not hold the struct perceptron but a NODE
                                // from graph.h
    Actv *func;                 // This is a general activation function, each layer architecture
                                // is decided beforehand and an activation function must be selected
}LAYER;

typedef struct network
{
    struct queue *layers;        // Double linked queue to acces all the layers
}NETWORK;

#endif