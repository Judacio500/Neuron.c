#include"ani.h"
#include"neuron.h"

/*

A mi me parece extremadamente fascinante
ver como mi repertorio de librerias a crecido tanto que con proyectos tan complejos puedo volver a hacer
codigos con menos de 200 lineas

Cuando empece a trabajar con estructuras de datos los codigos cada vez eran mas largos y esto es
gratificante

*/

// Wrappers para ani, hacen match con el tipo behaviorFunc
// y convierten la red neuronal en un automata
void fStepWrapper(struct object *self, int step, void *params, void *env);
void bStepWrapper(struct object *self, int step, void *params, void *env);
void moveWindow(struct object *self, int step, void *params, void *env);
void checkEnd(struct object *self, int step, void *params, void *env);

F *generateLineBetween(float x1, float y1, float x2, float y2, float width);
void modelToObject(MODEL *network, void *env);

int main(int argc, char **argv)
{
    float inputs[] = 
    { 
        0, 0,
        0, 1,
        1, 0,
        1, 1 
    };

    float targets[] = 
    { 
        0, 
        1, 
        1, 
        0 
    };

    MODEL *m = createModel();

    addLayer(m, 2, sigmoid); 
    addLayer(m, 3, sigmoid); 
    addLayer(m, 1, sigmoid); 

    if(compileNetwork(m) != 0)
        return 1;
    
    m->baseInputs   = inputs;
    m->baseTargets  = targets;
    m->totalSamples = 4;
    m->maxEpochs    = 500; 
    m->learningRate = 2;
    m->currentSample   = 0;
    m->currentEpoch    = 0;
    m->epochErrorAccum = 0;
    
    ANI *ani = initAnimation();
    SCENE *scene = initScene(60, 45);
    
    // Objeto Controlador (Logico)
    OBJECT *controller = initObject("NeuralController", "LogicLayer", NULL, NULL);
    controller->custom = m; 

    // Configuracion de la FSM
    STATUS *stMove   = generateStatus(moveWindow, NULL, NULL); 
    STATUS *stFwd    = generateStatus(fStepWrapper, NULL, NULL);
    STATUS *stCheck  = generateStatus(checkEnd, NULL, NULL);
    STATUS *stBack   = generateStatus(bStepWrapper, NULL, NULL);

    stMove->params  = stFwd;
    stFwd->params   = stCheck;
    stCheck->params = stBack;
    stBack->params  = stMove;

    handleInsert(&controller->statusStack, stMove, 0, SIMPLE);
    controller->activeStatus = stMove;

    LIST *initialObjects = NULL;
    handleInsert(&initialObjects, controller, 0, SIMPLE);

    int numLayers = 3; 
    int stepsPerSample = (2 * numLayers) + 1; 
    int totalFrames = (m->maxEpochs * m->totalSamples * stepsPerSample) + 20;

    printf("Simulando %d frames...\n", totalFrames);

    animationSimple(ani, scene, initialObjects, totalFrames);

    startGraphicsLoop(ani, argc, argv, "XOR Neural Visualizer");

    return 0;
}

// Convertimos las funcion forwardStep y backwardStep, al formato behavior de Ani

// Aqui object guardara nuestro modelo en su campo custom, de ahi sacaremos la red
// step es irrelevante porque no implementamos su logica en este caso
// pero podriamos hacer que el modelo solo se redibuje cuando se cumplan cierto numero de steps
// params va a ser el estado en sentido contrario de la red y env sera la lista de objetos que no usaremos porque 
// este objeto no se va a dibujar lo que va a hacer es una traduccion del grafo a objetos de ani
// cosas que ani entiende

void fStepWrapper(struct object *self, int step, void *params, void *env)
{
    MODEL *network = (MODEL*)self->custom; 

    int sChange = stepForward(network);

    modelToObject(network,env);

    if(sChange)  // Hacemos el step forward y revisamos que no haya terminado 
    {   
        // Si termina sacamos el estado de backpropagation y lo metemos a la pila
        STATUS *sCheckEnd = (STATUS*)params;
        handleInsert(&self->statusStack,sCheckEnd,0,SIMPLE);    // cambiamos al estado de back propagation
        self->activeStatus = (STATUS*)self->statusStack->data;
    }
}

void bStepWrapper(struct object *self, int step, void *params, void *env)
{
    // Se comporta igual que el forward pass pero cambiando a estado de forward pass al final

    MODEL *network = (MODEL*)self->custom; 

    int sChange = stepBackward(network);

    modelToObject(network,env);

    if(sChange)  // Hacemos el step forward y revisamos que no haya terminado 
    {   
        network->currentSample++;

        // Si termina sacamos el estado de backpropagation y lo metemos a la pila
        STATUS *sMoveWindow = (STATUS*)params;
        handleInsert(&self->statusStack,sMoveWindow,0,SIMPLE);    // cambiamos al estado de back propagation
        self->activeStatus = (STATUS*)self->statusStack->data;
    }
}

void moveWindow(struct object *self, int step, void *params, void *env)
{
    MODEL *m = (MODEL*)self->custom;
    
    if (m->currentSample >= m->totalSamples)
    {
        MLAYER *lastL = (MLAYER*)m->layers->last->data; 
        int outputCols = lastL->nNeurons;
        
        float mse = m->epochErrorAccum / (m->totalSamples * outputCols);
        printf("Epoch %d completed | MSE: %f\n", m->currentEpoch, mse);

        m->epochErrorAccum = 0;
        m->currentSample = 0; 
        m->currentEpoch++;

        if (m->currentEpoch >= m->maxEpochs) 
        {
            printf("Training Finished!\n");
            return;
        }
    }

    MLAYER *firstL = (MLAYER*)m->layers->first->data;
    MLAYER *lastL  = (MLAYER*)m->layers->last->data;
    
    int inputDim  = firstL->nNeurons;
    int outputDim = lastL->nNeurons;

    m->inputs  = m->baseInputs  + (m->currentSample * inputDim);
    m->targets = m->baseTargets + (m->currentSample * outputDim); 

    LIST *nodeList = firstL->perceptrons;
    int k = 0;

    while(nodeList) 
    {
        NODE *n = (NODE*)nodeList->data;
        PERCEPTRON *p = (PERCEPTRON*)n->data;
        
        p->output = m->inputs[k]; 
        
        nodeList = nodeList->next;
        k++;
    }

    m->currentLayer = m->layers->first;

    modelToObject(m,env);

    STATUS *sForward = (STATUS*)params; 
    handleInsert(&self->statusStack, sForward, 0, SIMPLE);
    self->activeStatus = (STATUS*)self->statusStack->data;
}

void checkEnd(struct object *self, int step, void *params, void *env)
{
    MODEL *m = (MODEL*)self->custom;
    
    MLAYER *lastL = (MLAYER*)m->layers->last->data;
    LIST *outNodes = lastL->perceptrons;
    int tIdx = 0;

    while(outNodes) 
    {
        NODE *n = (NODE*)outNodes->data;
        PERCEPTRON *p = (PERCEPTRON*)n->data;
        
        float error = m->targets[tIdx] - p->output;
        m->epochErrorAccum += (error * error);
        
        outNodes = outNodes->next;
        tIdx++;
    }

    modelToObject(m,env);

    STATUS *sBackward = (STATUS*)params;
    handleInsert(&self->statusStack, sBackward, 0, SIMPLE);
    self->activeStatus = (STATUS*)self->statusStack->data;
}

F *generateLineBetween(float x1, float y1, float x2, float y2, float width)
{
    float dx = x2 - x1;
    float dy = y2 - y1;
    float len = sqrtf(dx*dx + dy*dy);
    float angle = atan2f(dy, dx) * (180 / M_PI);

    float midX = (x1 + x2) / 2;
    float midY = (y1 + y2) / 2;

    DESIGN *lineColor = initDesign(0.3f, 0.3f, 0.3f, 0.5f); 
    
    F *line = generateFigure(LINE, lineColor, len, 0, midX, midY, -1, 0, 0, angle);
    return line;
}

F *generateLineBetween(float x1, float y1, float x2, float y2, DESIGN *des)
{
    float dx = x2 - x1;
    float dy = y2 - y1;
    float len = sqrtf(dx*dx + dy*dy);
    float angle = atan2f(dy, dx) * (180 / M_PI);

    float midX = (x1 + x2) / 2;
    float midY = (y1 + y2) / 2;

    F *line = generateFigure(LINE, des, len, 0, midX, midY, -1, 0, 0, angle);
    return line;
}

void modelToObject(MODEL *network, void *env)
{
    if(!network || !env) 
        return;

    PANEL *currentPanel = (PANEL*)env;

    OBJECT *visObj = initObject("Network", "GUI_LAYER", NULL, NULL);
    visObj->status = TEMPORAL; 

    DESIGN *textCol = initDesign(1, 1, 0, 1); // Amarillo

    char mseText[64];
    sprintf(mseText, "Epoch: %d | Samples: %d/%d", network->currentEpoch, network->currentSample, network->totalSamples);
    
    F *mseFig = generateText(textCol, mseText);
    
    if(mseFig && mseFig->relPos) 
    {
        mseFig->relPos->y = 13; // Bajamos un poco para asegurar visibilidad
        mseFig->relPos->x = -6; // Centrado aproximado
    }
    pushFigure(&visObj->figures, mseFig);

    float startX = -12;
    float layerGap = 12;
    float neuronGap = 5.0;

    LIST *layerIter = network->layers->first;
    int lIdx = 0;

    while(layerIter)
    {
        MLAYER *currL = (MLAYER*)layerIter->data;
        float currH = (currL->nNeurons - 1) * neuronGap;
        float currX = startX + (lIdx * layerGap);

        if(currL->perceptrons && layerIter->next) 
        {
            MLAYER *nextL = (MLAYER*)layerIter->next->data;
            float nextH = (nextL->nNeurons - 1) * neuronGap;
            float nextX = currX + layerGap;

            LIST *nList = currL->perceptrons;
            int nIdx = 0;
            
            while(nList)
            {
                NODE *n = (NODE*)nList->data;
                float y1 = (currH / 2) - (nIdx * neuronGap);
                
                EDGE *e = n->firstEdge;
                for(int j=0; j < nextL->nNeurons; j++)
                {
                    if(!e) break; 
                    float y2 = (nextH / 2) - (j * neuronGap);
                    
                    float w = e->weight;
                    float wAlpha = fabs(w);
                    if(wAlpha > 1) wAlpha = 1;
                    if(wAlpha < 0.1) wAlpha = 0.1;

                    DESIGN *lineCol;
                    if(w >= 0)
                        lineCol = initDesign(0, 1, 0.2, wAlpha); // Verde Neon
                    else
                        lineCol = initDesign(1, 0, 0.2, wAlpha); // Rojo Neon

                    F *line = generateLineBetween(currX, y1, nextX, y2, lineCol);
                    pushFigure(&visObj->figures, line);

                    e = e->nextNode;
                }
                nList = nList->next;
                nIdx++;
            }
        }

        LIST *nList = currL->perceptrons;
        int nIdx = 0;

        while(nList)
        {
            NODE *n = (NODE*)nList->data;
            PERCEPTRON *p = (PERCEPTRON*)n->data;
            
            float yPos = (currH / 2) - (nIdx * neuronGap);
            float alpha = p->output; 
            
            if(alpha < 0.3) alpha = 0.3; 
            if(alpha > 1) alpha = 1;

            // Circulo de la Neurona
            DESIGN *neuronCol = initDesign(0, 0.9, 1, alpha); // Cyan
            F *neuron = generateFigure(CIRCLE, neuronCol, 20, 1.2, currX, yPos, 0, 0, 0, 0);
            pushFigure(&visObj->figures, neuron);

            // Texto del Valor (Output)
            char valStr[16];
            sprintf(valStr, "%.2f", p->output);
            
            DESIGN *valCol = initDesign(1, 0, 1, 1); // Blanco solido
            F *valFig = generateText(valCol, valStr);
            
            if(valFig && valFig->relPos)
            {
                // Ajuste fino para centrar el texto sobre el circulo
                valFig->relPos->x = currX - 0.7; 
                valFig->relPos->y = yPos - 0.3; 
                valFig->relPos->z = 1; // Z positivo para que se pinte ENCIMA del circulo
            }
            pushFigure(&visObj->figures, valFig);
            
            nList = nList->next;
            nIdx++;
        }

        layerIter = layerIter->next;
        lIdx++;
    }

    LAYER *guiLayer = NULL;
    EHASH *found = hashing(currentPanel->layers, "GUI_LAYER");
    
    if(found) 
        guiLayer = (LAYER*)found->pair;
    else 
    {
        guiLayer = initLayer("GUI_LAYER", NULL);
        addLayer(currentPanel, guiLayer);
    }

    addObject(currentPanel, guiLayer, visObj);
}