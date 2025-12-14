#include "ani.h"

// Creacion de estructuras

/*

Esta seccion no esta comentada en su mayoria porque las decisiones del diseño ya estan en el .h
y la logica de estas funciones es sencilla, agarramos una bolsa del supermercado del tamaño que quieramos
metemos lo que vamos a comprar para esa bolsa y cerramos la bolsa

Basicamente...

Damos memoria a la estructura
Inicializamos con valores base y aprueba de crash

Esta seccion aunque simple se asegura de que si algo sale mal, nuestra libreria tenga las herramientas para hacer
el handling correcto

*/

// Necesarias porque display no recibe parametros
static ANI *g_anim = NULL;
static LIST *g_curr = NULL;
static int g_play = 0;
static int g_toggle = 0;

CRD *initCoord(float x, float y, float z)
{
    CRD *newC = (CRD*)malloc(sizeof(CRD));

    if(!newC)
        return NULL;
    
    newC->x = x;
    newC->y = y;
    newC->z = z;

    return newC;
}

DESIGN *initDesign(float r, float g, float b, float transparency)
{
    DESIGN *newD = (DESIGN*)malloc(sizeof(DESIGN));

    if(!newD)
        return NULL;

    newD->color = initCoord(r,g,b);
    newD->transparency = transparency;

    return newD;
}

F *initFigure(LIST *pointOffSet, DESIGN *des, CRD *localPosition, CRD *localRotation, enum figures f)
{
    F *newF = (F*)malloc(sizeof(F));
    
    if(!newF)
        return NULL;

    newF->offSet = pointOffSet;
    newF->f = f;
    if(des)
    {
        newF->color = des;
    }
    else
    {
        newF->color = initDesign(1,0,0,1); // Rojo solido por defecto
    }

    if (localPosition)
    {
        newF->relPos = localPosition;
    }
    else
    {
        newF->relPos = initCoord(0, 0, 0);
        if(!newF->relPos)
        {
            free(newF);
            return NULL;
        }
    }

    if (localRotation)
    {
        newF->localRot = localRotation;
    }
    else
    {
        newF->localRot = initCoord(0, 0, 0);
        if(!newF->localRot)
        {
            free(newF->relPos);
            free(newF);
            return NULL;
        }
    }

    return newF;
}

TRIGGER *initTrigger(Check check, STATUS *targetStatus)
{
    TRIGGER *newT = (TRIGGER*)malloc(sizeof(TRIGGER));
    
    if(!newT)
        return NULL;

    newT->check = check;
    newT->targetStatus = targetStatus;

    return newT;
}

TRANSFORM *initPhysics(F *colision, CRD *pos, CRD *scale, CRD *rotation)
{
    TRANSFORM *newT = (TRANSFORM*)malloc(sizeof(TRANSFORM));
    
    if(!newT)
        return NULL;

    newT->globalPos = pos;
    newT->scale = scale;
    newT->rotation = rotation;
    newT->effectArea = NULL;
    newT->colissionBox = colision;

    return newT;
}

OBJECT *initObject(char *objectName, char *layerName, TRANSFORM *initial, LIST *figures)
{
    OBJECT *newO = (OBJECT*)malloc(sizeof(OBJECT));

    if(!newO)
        return NULL;

    if(objectName)
        strncpy(newO->key, objectName, 29);
    else
        strncpy(newO->key, "unnamed", 29);
    newO->key[29] = '\0'; 

    if(layerName)
        strncpy(newO->layerKey, layerName, 29);
    else
        strncpy(newO->layerKey, "unnamed", 29);
    newO->layerKey[29] = '\0'; 

    if(!initial)
    {
        CRD *defPos   = initCoord(0, 0, 0);
        CRD *defScale = initCoord(1, 1, 1);
        CRD *defRot   = initCoord(0, 0, 0);

        if(!defPos || !defScale || !defRot)
        {
            free(newO);
            if(defPos) 
                free(defPos);
            if(defScale) 
                free(defScale);
            if(defRot)
                free(defRot);
            return NULL; 
        }

        newO->t = initPhysics(NULL, defPos, defScale, defRot); 

        if(!newO->t)
        {
            free(newO);
            free(defPos);
            free(defScale);
            free(defRot);
            return NULL;
        }
    }
    else
        newO->t = initial;
    
    if(!newO->t)
    {
        free(newO);
        return NULL;
    }

    newO->figures = figures;    
    calculateDimensions(newO);
    newO->statusStack = NULL;  // La pila de estados esta vacia 
    newO->activeStatus = NULL;
    newO->custom = NULL;
    newO->status = ALIVE; 

    return newO;
}

SCENE *initScene(float width, float height)
{
    SCENE *newS = (SCENE*)malloc(sizeof(SCENE));
    if(!newS)
        return NULL;

    newS->width = width;
    newS->height = height;

    return newS;
}

LAYER *initLayer(char *layerName, Behavior initialBehavior)
{
    LAYER *newL = (LAYER*)malloc(sizeof(LAYER));
    if(!newL)
        return NULL;

    if(layerName)
        strncpy(newL->layerName, layerName, 29);
    else
        strncpy(newL->layerName, "unnamed_layer", 29);
    newL->layerName[29] = '\0';

    newL->objects = initHash(0); 

    newL->initialBehavior =  !initialBehavior ? Idle : initialBehavior;    // Si se define un estado inicial se aplica
                                                                           // si no se le da el estado Idle

    return newL;
}

PANEL *initPanel(SCENE *camera)
{
    PANEL *newP = (PANEL*)malloc(sizeof(PANEL));
    if(!newP)
        return NULL;

    newP->currentScene = camera;
    newP->layers = initHash(0);
    newP->allObjects = NULL;
    LAYER *backGround = initLayer("BACKGROUND", Static);    // Capa base para cualquier panel, BACKGROUND reservado con comportamiento Static
                                                            // Lo que quiere decir que toda estructuraa inicializada aqui no tiene movimiento ni interaccion con la fisica del mundo
    
    if(backGround)
        saveKey(&newP->layers, backGround->layerName, backGround);
    else
    {
        printf("Error initializing background layer");
        return NULL; // No se puede inicializar un panel sin capas es como querer caminar sobre nubes
    }

    return newP;
}

ANI *initAnimation()
{
    ANI *newA = (ANI*)malloc(sizeof(ANI));
    if(!newA)
        return NULL;

    newA->AS = PAUSE;   
    newA->speed = 1.0f; 
    newA->panels = createWrap(); 

    return newA;
}


// Insercion de estructuras
/*
    Estos serian los dummys de insercion para usuario/funciones "inteligentes" AKA Automatas

    Sirven para ensamblar la animacion manualmente especificando cada lista de objetos en un panel
    Cada capa y cada comportamiento 

    O para recibir las instrucciones de insercion de nuestras funciones inteligentes

*/

int addPanel(ANI *animation, PANEL *p)  // Esta funcion corresponde a la punta del iceberg
{                                       // Cuando un panel esta terminado lo añade a la cola de la animacion 
                                        // permitiendo siempre insertar al final de la animacion
    if(!animation || !p)
        return -1; // Panel no añadido

    return handleAppend(&animation->panels,p,1,DOUBLE); // Insercion en lista doblemente enlazada
}

int addObject(PANEL *p, LAYER *l, OBJECT *o)
{
    if(!p || !l || !o) return -1;
    
    saveKey(&l->objects, o->key, o);

    return handleInsert(&p->allObjects, o, 0, SIMPLE); 
}

int addLayer(PANEL *p, LAYER *l)
{
    if(!p || !l) 
        return -1;
    
        return saveKey(&p->layers, l->layerName, l);
}

/*

    Estas dos funciones son similares porque a diferencia de los paneles
    que son secuenciales, son muchos y son mas dificiles de describir

    Los objetos y las capas de un panel son contados, son especificos del panel
    y son unicos y relevantes para el contexto de la animacion

    por eso su implementacion esta hecha mediante hash, de esta forma
    se pueden buscar y modificar por como fueron insertados

*/

int addColission(OBJECT *o, F *colissionBox)
{
    if(!o || !o->t) 
        return -1;
    
    o->t->colissionBox = colissionBox;
    
    return 0;
}

LIST *rectangleOffSet(float width, float length)
{
    LIST *offSet = NULL;
    float halfX = length / 2.0f;
    float halfY = width / 2.0f;

    // Vertice 1: Esquina superior izquierda
    handleInsert(&offSet, initCoord(-halfX, halfY, 0), 0, SIMPLE);

    // Vertice 2: Esquina superior derecha
    handleInsert(&offSet, initCoord(halfX, halfY, 0), 0, SIMPLE);
    
    // Vertice 3: Esquina inferior derecha
    handleInsert(&offSet, initCoord(halfX, -halfY, 0), 0, SIMPLE);
    
    // Vertice 4: Esquina inferior izquierda
    handleInsert(&offSet, initCoord(-halfX, -halfY, 0), 0, SIMPLE);

    return offSet;
}

LIST *polygonOffSet(int segments, float radius)
{
    LIST *offSet = NULL;

    if (segments < 3) segments = 3;

    float angleStep = (2.0f * M_PI) / segments;

    for(int i = 0; i < segments; i++)
    {
        float theta = i * angleStep;
        // Calculamos offSet (eliminado localX/Y para centrar en 0,0)
        float x = cosf(theta) * radius; 
        float y = sinf(theta) * radius;
        
        handleInsert(&offSet, initCoord(x, y, 0), 0, SIMPLE);
    }

    return offSet;
}

LIST *circleOffSet(int smoothness, float radius)
{
    LIST *offSet = NULL;

    if(smoothness <= 0)
        return NULL;

    int segments = smoothness * 70; // Tu logica de suavizado

    float angleStep = (2.0f * M_PI) / segments;

    for(int i = 0; i < segments; i++)
    {
        float theta = i * angleStep;
        // Calculamos offSet (eliminado localX/Y para centrar en 0,0)
        float x = cosf(theta) * radius; 
        float y = sinf(theta) * radius;
        
        handleInsert(&offSet, initCoord(x, y, 0), 0, SIMPLE);
    }

    return offSet;
}

LIST *lineOffSet(float length)
{
    LIST *offSet = NULL;
    float half = length / 2.0f;

    // Linea horizontal unitaria (Inicio)
    handleInsert(&offSet, initCoord(-half, 0, 0), 0, SIMPLE);

    // Linea horizontal unitaria (Fin)
    handleInsert(&offSet, initCoord(half, 0, 0), 0, SIMPLE);

    return offSet;
}

LIST *triangleOffSet(float base, float height)
{
    LIST *offSet = NULL;
    
    float halfBase = base / 2.0f;
    float halfHeight = height / 2.0f;

    // Vertice 1: Punta superior (Centrada)
    handleInsert(&offSet, initCoord(0, halfHeight, 0), 0, SIMPLE);

    // Vertice 2: Esquina inferior izquierda
    handleInsert(&offSet, initCoord(-halfBase, -halfHeight, 0), 0, SIMPLE);
    
    // Vertice 3: Esquina inferior derecha
    handleInsert(&offSet, initCoord(halfBase, -halfHeight, 0), 0, SIMPLE);

    return offSet;
}

F *generateFigure(enum figures figType, DESIGN *des, float arg1, float arg2, float localX, float localY, float zPriority, float rotX, float rotY, float rotZ)
{
    LIST *offSet = getOffSet(figType, arg1, arg2);
    
    if(!offSet)
        return NULL;

    CRD *pos = initCoord(localX, localY, zPriority);

    if(!pos)
        return NULL;

    CRD *rot = initCoord(rotX, rotY, rotZ);

    if(!rot)
    {
        free(pos);
        freeList(&offSet,destroyCoord);
        return NULL;
    }

    F *newF = initFigure(offSet,des,pos,rot,figType);

    return newF;
}

int destroyCoord(void *data)
{
    if(!data)
        return -1;

    CRD *toDest = (CRD*)data;

    free(toDest);

    return 0;
}

F *generateColission(enum figures figType, float arg1, float arg2)
{
    LIST *offSet = getOffSet(figType, arg1, arg2);

    if(!offSet)
        return NULL;

    CRD *pos = initCoord(0, 0, 0);

    if(!pos)
        return NULL;

    CRD *rot = initCoord(0, 0, 0);

    if(!rot)
    {
        free(pos);
        freeList(&offSet,destroyCoord);
        return NULL;
    }

    F *newF = initFigure(offSet,NULL,pos,rot,figType);

    return newF;
}

LIST *getOffSet(enum figures figType, float arg1, float arg2)
{
    switch(figType)
    {
        case TRIANGLE:
            return triangleOffSet(arg1, arg2);
        case RECTANGLE:
            return rectangleOffSet(arg1, arg2);
        case POLYGON:
            return polygonOffSet(arg1, arg2);
        case LINE:
            return lineOffSet(arg1);
        case CIRCLE:
            return circleOffSet(arg1, arg2);
        case OVAL:
            // Not yet implemented
            return NULL;
            //break;
        default:
            return NULL;
    }
}

/*

Funciones de construccion de secuencias de animacion para un objeto, construccion de objetos 

*/



STATUS *generateStatus(Behavior func, struct graph *animationSequence, void *params)
{
    STATUS *newStatus = (STATUS*)malloc(sizeof(STATUS));

    if(!newStatus)
        return NULL;

    newStatus->func = func;
    newStatus->animSequence = animationSequence;
    newStatus->params = params;

    return newStatus;
}

int pushFrame(QUEUE **sequence, OBJECT *frameObj)
{
    if(!sequence || !frameObj)
        return -1;
    
    return handleAppend(sequence, frameObj, 1.0f, SIMPLE);
}

int pushFigure(LIST **figureList, F *fig)
{
    if(!fig)
        return -1;

    return handleInsert(figureList, fig, 0, SIMPLE);
}

PANEL *generatePanelFromObjects(SCENE *camera, LIST *objects)
{
    PANEL *newP = initPanel(camera);
    if(!newP) return NULL;

    LIST *current = objects;
    while(current)
    {
        OBJECT *obj = instanceObject((OBJECT*)current->data); // Los paneles son instanciados
                                                              // todos los objetos insertados son instancias
                                                              // del objeto anterior, ninguno es el mismo
                                                              // el transform es unico porque referencia a la posicion del objeto
                                                              // en el panel de interes, como dibujamos desde esos datos
                                                              // no puede ser compartido
        
        LAYER *targetLayer = NULL;
        EHASH *found = hashing(newP->layers, obj->layerKey);

        printf("No es el hashing para %s", obj->key);

        if(found)
        {
            targetLayer = (LAYER*)found->pair;
        }
        else
        {
            targetLayer = initLayer(obj->layerKey, NULL);
            if(addLayer(newP, targetLayer) < 0)
            {
                return NULL;
            }
        }

        addObject(newP, targetLayer, obj);

        current = current->next;
    }

    return newP;
}

GRAPH *generateBluePrint(char *sequenceName, QUEUE *objectSequence, int type)
{
    if(!objectSequence || !objectSequence->first)
        return NULL;

    GRAPH *newSequence = createGraph(sequenceName, NULL);
    if(!newSequence) 
        return NULL;

    LIST *iter = objectSequence->first;
    int index = 0;
    char currentKey[50];

    NODE *prevNode = NULL, *firstNode = NULL;

    while(iter)
    {
        sprintf(currentKey, "%s_%d", sequenceName, index);
        
        void *currentFrameObj = iter->data;

        NODE *newNode = addNode(newSequence, currentKey, currentFrameObj); 

        if(!newNode)
            return NULL;

        if(index == 0)
            firstNode = newNode;

        if(prevNode)
        {
            addEdgeThrough(prevNode, newNode, 1.0f, 1); //
        }

        prevNode = newNode;
        iter = iter->next;
        index++;
    }

    if(type == 1 && prevNode && firstNode)
    {
        addEdgeThrough(prevNode, firstNode, 1.0f, 1);
    }

    return newSequence;
}

OBJECT *instanceObject(OBJECT *tmplt)
{
    /*
        Asi es, el bluePrint va aqui

        que nada te detenga de ponerle la animacion de un pollo asado bailando a una persona :D
    
        AKA Libertad creativa

    */

    OBJECT *newObj = initObject(tmplt->key, tmplt->layerKey, NULL, tmplt->figures); //Las figuras, el nombre y la capa se mantienen igual
    

    printf("\n\nPara %s\n", tmplt->key);
    if(newObj->key && newObj->layerKey)
        puts("Se copio la llave");
    else
        puts("No se copio la llave");


    if(!newObj) 
        return NULL;

    CRD *newPos = NULL, *newScale = NULL, *newRot = NULL;

    if(tmplt->t)
    {
        newPos   = initCoord(tmplt->t->globalPos->x, tmplt->t->globalPos->y, tmplt->t->globalPos->z);
        newScale = initCoord(tmplt->t->scale->x, tmplt->t->scale->y, tmplt->t->scale->z);
        newRot   = initCoord(tmplt->t->rotation->x, tmplt->t->rotation->y, tmplt->t->rotation->z);
        
        newObj->t = initPhysics(tmplt->t->colissionBox, newPos, newScale, newRot);
        
        newObj->t->effectArea = tmplt->t->effectArea;
    }

    LIST *previousStack = NULL;
    LIST *iter = tmplt->statusStack;

    // Volcar la pila
    while(iter)
    {
        handleInsert(&previousStack, iter->data, 0, SIMPLE);
        iter = iter->next;
    }

    // Rearmar la pila

    while(previousStack)
    {
        handleInsert(&newObj->statusStack,popData(&previousStack),0,SIMPLE);
    }

    // Utilizamos los mismos estados porque al final solo guardaremose el dibujado, y el dibujado es independiente del estado,
    // lo que iteraremos es la Cola paneles, no la animacion en si, utilizamos el mismo stack para ahorrar memoria
    // y mantener integridad en la animacion

    newObj->activeStatus = tmplt->activeStatus;
    newObj->currentFrame = tmplt->currentFrame;

    printf("\nSe copio bien la posicion\n");

    return newObj;
}

// Instanciador del comportamiento inicial IDLE
// sin secuencia de dibujo ni parametros extra
// (el usuario puede cambiar el IDLE de un objeto especifico si asi lo quiere)
STATUS *getBase(Behavior func)
{
    return generateStatus(func,NULL,NULL);
}

int checkGround(OBJECT *self, void *env)
{
    if(!self || !env) 
        return 0;
    
    PANEL *p = (PANEL*)env;
    
    float myY = self->t->globalPos->y;
    float myBottom = myY - self->maxY;
    float myX = self->t->globalPos->x;

    LIST *iter = p->allObjects; 
    while(iter)
    {
        OBJECT *other = (OBJECT*)iter->data;
        
        // Ignorar self y objetos sin colision
        if(other != self && other->t->colissionBox)
        {
            float otherX = other->t->globalPos->x;
            float otherMaxX = other->maxX; 
            
            if (fabs(myX - otherX) < (self->maxX + otherMaxX))
            {
                float otherTop = other->t->globalPos->y + other->maxY;
                float penetration = otherTop - myBottom;

                if (penetration >= 0 && penetration < 10) 
                {
                    self->t->globalPos->y = otherTop + self->maxY;
                    return 1; 
                }
            }
        }
        iter = iter->next;
    }
    return 0;
}

void calculateDimensions(OBJECT *obj)
{
    if (!obj || !obj->figures || !obj->t) 
    {

        if(obj && obj->t && obj->t->scale) 
        {
            obj->maxX = 0.5f * fabs(obj->t->scale->x);
            obj->maxY = 0.5f * fabs(obj->t->scale->y);
        }
        return;
    }

    float minX = 100000.0f, maxX = -100000.0f;
    float minY = 100000.0f, maxY = -100000.0f;
    int pointsFound = 0;

    LIST *figIter = obj->figures;
    while(figIter)
    {
        F *fig = (F*)figIter->data;
        if(fig && fig->offSet)
        {
            LIST *pIter = fig->offSet;
            while(pIter)
            {
                CRD *p = (CRD*)pIter->data;
                

                float effectiveX = p->x + fig->relPos->x;
                float effectiveY = p->y + fig->relPos->y;

                if(effectiveX < minX) minX = effectiveX;
                if(effectiveX > maxX) maxX = effectiveX;
                if(effectiveY < minY) minY = effectiveY;
                if(effectiveY > maxY) maxY = effectiveY;
                
                pointsFound = 1;
                pIter = pIter->next;
            }
        }
        figIter = figIter->next;
    }

    if(!pointsFound)
    {
        obj->maxX = 0.5f * fabs(obj->t->scale->x);
        obj->maxY = 0.5f * fabs(obj->t->scale->y);
    } 
    else 
    {
        float rawWidth = maxX - minX;
        float rawHeight = maxY - minY;
        
        obj->maxX = (rawWidth / 2.0f) * fabs(obj->t->scale->x);
        obj->maxY = (rawHeight / 2.0f) * fabs(obj->t->scale->y);
    }
}

int checkTriggers(OBJECT *self, GP *params, void *env)
{
    if(!params || !params->triggers) // Un objeto sin triggers no puede hacer cambios
        return 0;

    LIST *triggers = params->triggers;

    while(triggers)
    {
        TRIGGER *t = (TRIGGER*)triggers->data;

        if(t->check(self,env))
        {
            STATUS *nextStatus = (STATUS*)t->targetStatus;

            if(!nextStatus) // Si no tenemos otro comportamiento en ese trigger entonces hacemos pop del comportamiento actual 
            {
                popData(&self->statusStack);
            }
            else
            {
                handleInsert(&self->statusStack,nextStatus,0,SIMPLE);
                self->activeStatus = nextStatus;
            }

            self->currentFrame = NULL;
            return 1;
        }
        triggers = triggers->next;
    }

    return 0;
}

void physicsUpdate(OBJECT *self, GP *p)
{
    if(!self || !self->t || !p) 
        return;

    self->t->globalPos->x += p->speedX;
    self->t->globalPos->y += p->speedY;

    p->speedY -= p->gravity;
    p->speedX *= p->friction; 
    // Importante aqui recalcar que p->friction es una multiplicacion
    // por lo que el frenado de la fricción es proporcional
    // lo que haria que nunca dejaramos de movernos
    // por eso:

    if(fabs(p->speedX) < 0.01) // Al llegar a cierta velocidad minima  
        p->speedX = 0;         // Matamos la velocidad por completo

    p->stepCounter++;
}

int advanceAutomata(OBJECT *obj)
{

    if(!obj || !obj->activeStatus || !obj->activeStatus->animSequence) // Nuestra libreria usa advanceAutomata responsablemente
        return -1;                                                     // pero al ser utilizable por terceros y poder implementar cerebros especificos
                                                                       // es mejor tener un check de seguridad aqui tambien

    GRAPH *currentClip = obj->activeStatus->animSequence;              // Nuestra secuencia actual

    if(!obj->currentFrame) 
    {
        char startKey[60];
        
        sprintf(startKey, "%s_0", currentClip->name); 

        NODE *firstFrame = hashNode(currentClip, startKey);
        
        if(firstFrame) 
        {
            obj->currentFrame = firstFrame;
            obj->currentFrame->cost = 0; // Reset del stepCounter interno del nodo
        } 
        else 
        {
            // Si no encuentra el frame 0, no podemos animar.
            return -1; 
        }
    }

    obj->currentFrame->cost += 1.0f;
    EDGE *path = obj->currentFrame->firstEdge; 
    
    if (path) 
    {
        if (obj->currentFrame->cost >= path->weight) 
        {
            obj->currentFrame->cost = 0;
            obj->currentFrame = path->destinationNode;
        }
    }
   
    return 0;
}

void Static(struct object *self, int step, void *params, void *env) // Estado para objetos inamovibles en la animacion
{                                                                   // No actualizan fisicas ni reciben triggers, solo existen en el
                                                                    // background
    if(self->activeStatus && self->activeStatus->animSequence)
        advanceAutomata(self);
}

void Idle(struct object *self, int step, void *params, void *env)
{
    GP *p = (GP*)params;

    // Revisamos si es necesario actualizar los estados
    // si sucede un push interrumpimos IDLE y pasamos al siguiente
    // estado
    if(checkTriggers(self, p, env))     // checkTriggers es parte de todos los cerebros base para darle al usuario la libertad 
        return;                         // de cambiar de estados a partir de triggers definidos por el usuario

    p->friction = 0.8f; // Si el ultimo estado era correr o caminar
                        // frenamos esa velocidad
    
    physicsUpdate(self, p); // Actualizamos las fisicas

    checkGround(self, env);
    
    // Y si nuestro idle tiene una animacion avanzamos el frame
    if(self->activeStatus && self->activeStatus->animSequence)
         advanceAutomata(self);
}

void Walk(struct object *self, int step, void *params, void *env)
{
    GP *p = (GP*)params;

    if(checkTriggers(self, p, env)) 
        return;

    // Aqui la velocidad X ya viene seteada en params por el usuario
    // actualizamos la friccion a 1, de esta forma no frenamos
    p->friction = 1.0f; 

    physicsUpdate(self, p);
    checkGround(self, env);

    if(self->activeStatus && self->activeStatus->animSequence)
         advanceAutomata(self);
}


void Jump(struct object *self, int step, void *params, void *env)
{
    GP *p = (GP*)params;

    if(checkTriggers(self, p, env)) 
        return;

    p->friction = 0.95f;

    physicsUpdate(self, p);

    if (p->speedY < 0) // Trigger especial para aterrizaje despues de un salto
    {
        if(checkGround(self, env)) 
        {
             p->speedY = 0;
             popData(&self->statusStack);

             if(self->statusStack) 
             {
                 self->activeStatus = (STATUS*)self->statusStack->data;
             } 
             else 
             {
                 self->activeStatus = NULL; 
             }

             self->currentFrame = NULL; 
             return;
        }
    }

    if(self->activeStatus && self->activeStatus->animSequence)
         advanceAutomata(self);
}

void Fall(OBJECT *self, int step, void *params, void *env)
{
    GP *p = (GP*)params;
    
    physicsUpdate(self, p);

    if(checkGround(self, env))
    {
        p->speedY = 0;
        
        STATUS *idleSt = getBase(Idle);
        handleInsert(&self->statusStack, idleSt, 0, SIMPLE);
        self->activeStatus = idleSt;
        return;
    }

    if(checkTriggers(self, p, env)) 
        return;

    if(self->activeStatus && self->activeStatus->animSequence)
         advanceAutomata(self);
}

int animationSimple(ANI *toModify, SCENE *absolute, LIST *initialObjects, int frames)
{
    if(!toModify || !initialObjects || !absolute)
        return -1;

    for(int i=0; i<frames; i++)
    {
        PANEL *nextPanel = generatePanelFromObjects(absolute,initialObjects);   // Crea efectivamente un nuevo panel
                                                                                // desde la lista de objetos
                                                                                // los paneles son instanciados, ergo, sus componentes son individuales
                                                                                // por lo que no hace falta copiar cada objeto, cada panel copia sus objetos despues de simular
                                                                                // vida

        if(!nextPanel)
            return -1;

        LIST *objects = initialObjects;

        while(objects)
        {
            OBJECT *current = (OBJECT*)objects->data;

            if(current->activeStatus && current->activeStatus->func)
                current->activeStatus->func(current,i,current->activeStatus->params,nextPanel->allObjects); // Simulamos vida para la lista global de objetos 
                                                                                                            // esto es hermoso porque cada panel copia su propio estado de los objetos
                                                                                                            // al final de la animacion la lista esta lista para seguir con los estados anteriores
                                                                                                            // pero a lo mejor añadiendo nuevos triggers y asi la lista de objetos cambia de manera global
            objects = objects->next;
        }                       

        addPanel(toModify,nextPanel);
    }

    return 0;
}

void drawFigure(F *fig)
{
    if (!fig)
        return;

    glPushMatrix(); // Empujamos cambios loccales para la figura, solo
                    // lo relativo a la figura se dibuja en la posicion de la figura

    if (fig->relPos)
        glTranslatef(fig->relPos->x, fig->relPos->y, fig->relPos->z);   // movemos a la posicion relativa de dibujado
                                                                        // si nuestro objeto es una persona y el centro del objeto esta en su estomago
                                                                        // no dibujamos la cabeza en su estomago, si no en la posicion relativa de la cabeza
                                                                        // con su estomago (el centro)

    if (fig->localRot)
        glRotatef(fig->localRot->z, 0, 0, 1); // Rotamos respecto al eje Z para variedad de figuras,
                                              // asi podemos hacer rombos y otras cosas

    if (fig->color && fig->color->color)
    {
        glColor4f(fig->color->color->x, fig->color->color->y, fig->color->color->z, fig->color->transparency); // Aplicamose l color que usaremos
    }
    else
    {
        glColor3f(1, 1, 1);
    }

    GLenum mode = GL_POLYGON; // Todo es un poligono al final para reducir el dibujado
                              // de esto me di cuenta cuando estaba implementando polygonOffSet
                              // y decidi dejarlo asi para esta entrega final

    if (fig->f == LINE) 
        mode = GL_LINES;

    glBegin(mode);

    LIST *pNode = fig->offSet;
    while (pNode)
    {
        CRD *p = (CRD*)pNode->data;
        glVertex3f(p->x, p->y, p->z);   // Dibujamos donde va, sin hacer ni un solo calculo para el offSet
        pNode = pNode->next;            // vivan las transformaciones matriciales
    }

    glEnd();

    glPopMatrix();
}

void drawObject(OBJECT *obj)
{
    if (!obj || !obj->t)
        return;

    glPushMatrix();

    // Estas 3 lineas por cada push/pop lo que nos dicen es basicamente
    // Quiero dibujar mi objeto en X,Y (Translate)
    // Quiero dibujarlo a N* de su posicion normal
    // Quiero que tenga un tamaño de n*escala donde n es la escala inicial del objeto

    glTranslatef(obj->t->globalPos->x, obj->t->globalPos->y, obj->t->globalPos->z); // Trasladamos
    glRotatef(obj->t->rotation->z, 0, 0, 1);                                        // Rotamos      // Este orden es importante porque la combinatoria de estas operaciones da resultados
    glScalef(obj->t->scale->x, obj->t->scale->y, 1);                                // Escalamos    // completamente distintos en cada caso, por algo es COMBINATORIA

    if(obj->t->colissionBox && g_toggle)
        drawFigure(obj->t->colissionBox);

    if(obj->t->effectArea && g_toggle)
        drawFigure(obj->t->effectArea);

    LIST *fNode = obj->figures;
    while (fNode)
    {
        drawFigure((F*)fNode->data);
        fNode = fNode->next;
    }

    glPopMatrix(); // Pop porque los otros objetos tienen su dibujado propio
}

void display()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (!g_anim || !g_curr)
        return;

    PANEL *p = (PANEL*)g_curr->data;
    SCENE *s = p->currentScene;

    if (s)
    {
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(-s->width / 2, s->width / 2,-s->height / 2, s->height / 2, -1000, 1000);

        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        gluLookAt(0, 0, 1,
                  0, 0, 0,
                  0, 1, 0);
    }

    LIST *oNode = p->allObjects;
    while (oNode)
    {
        drawObject((OBJECT*)oNode->data);
        oNode = oNode->next;
    }

    glutSwapBuffers();
}

void reshape(int w, int h)
{
    if (h == 0)
        h = 1;

    glViewport(0, 0, w, h);
}

void timer(int v)
{
    if (g_play && g_anim)
    {
        if (g_curr && g_curr->next)
        {
            g_curr = g_curr->next;
        }
        else
        {
            g_curr = g_anim->panels->first;
        }
    }

    glutPostRedisplay();
    glutTimerFunc(16, timer, 0);
}

void keyboard(unsigned char key, int x, int y)
{
    switch(key)
    {
        case 32:
            g_play = !g_play;
            break;
        
        case 't':
        case 'T':
            g_toggle = !g_toggle;
            break;

    }
}

void special(int key, int x, int y)
{
    if (!g_play && g_curr)
    {
        if (key == GLUT_KEY_RIGHT && g_curr->next)
        {
            g_curr = g_curr->next;
        }
        if (key == GLUT_KEY_LEFT && g_curr->prev)
        {
            g_curr = g_curr->prev;
        }
        glutPostRedisplay();
    }
}

void startGraphicsLoop(ANI *ani, int argc, char **argv, char *title)
{
    if (!ani || !ani->panels || !ani->panels->first)
        return;

    g_anim = ani;
    g_curr = ani->panels->first;

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(800, 600);
    glutCreateWindow(title);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(special);
    glutTimerFunc(0, timer, 0);

    glutMainLoop();
}

int checkVision(OBJECT *self, void *env)
{
    if (!self || !self->t || !env)
        return 0;

    // Casteo a lista porque animationSimple pasa la lista de objetos vivos
    LIST *iter = (LIST*)env;

    // Definimos el rango de vision hacia la derecha (X positivo)
    // 5.0f es la distancia de vision (ajustable)
    float visionRange = 6.0f;
    float myX = self->t->globalPos->x;
    float myY = self->t->globalPos->y;

    while (iter)
    {
        OBJECT *other = (OBJECT*)iter->data;

        // Ignorar a uno mismo y objetos sin cuerpo fisico
        if (other != self && other->t->colissionBox)
        {
            float otherX = other->t->globalPos->x;
            float otherY = other->t->globalPos->y;

            // 1. Chequeo Vertical: ¿Esta en mi carril? (Misma altura aprox)
            if (fabs(myY - otherY) < (self->maxY + other->maxY))
            {
                // 2. Chequeo Horizontal: ¿Esta delante de mi?
                float dist = otherX - myX;

                // Si esta adelante (dist > 0) y dentro del rango
                if (dist > 0 && dist < visionRange)
                {
                    return 1; // Detectado
                }
            }
        }
        iter = iter->next;
    }

    return 0;
}