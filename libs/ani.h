#ifndef ANI_H
#define ANI_H

#include<stdio.h>
#include<string.h> 
#include<math.h>
#include<GL/glut.h>
#include"list.h" 
#include"graph.h" 
#include"hash.h"  

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/*
    Para el desarrollo de esta API se tiene el siguiente planteamiento:

    La inspiracion de esta libreria vino a partir del modulo de animacion de matplotlib

    Una animacion es un tipo de dato interpretado por la API, tipo de dato que guarda
    todas las escenas que se deben mostrar, el contexto en el que se muestran
    las acomoda en profundidad, como, donde y cuando se dibujan
    en otras palabras, es el corazon de la libreria y lo que la hace utilizable y generica
    para imprimir algo en esta libreria se debe generar un tipo ANI (struct animation)
    pero de eso hablaremos mas tarde

    La jerarquia de las estructuras

    El tipo "struct animation" es la punta del ice berg, es el contenedor que recibe
    el usuario al generar una animacion y lo que reciben las funciones de impresion 
    de la libreria para mostrar las animaciones y tal vez generar gifs con ellas ()

    Animation
    |->Panel
       |->Layer
          |->Objects
          |->Scene

    El panel es la unidad de impresion de la libreria y de manera secuencial (COLAS)
    recibe la nueva posicion o contexto que debe dibujar conforme pasan los paneles
    como cada cosa en un panel se comporta distinto su dibujado tambien implica
    distintos parametros por eso...

    Tenemos la estructura layer, que define el comportamiento de un grupo de objetos en el panel
    puede o no definir una profundidad global para los objetos asi se podria definir un background por ejemplo

    Cada capa contiene:
    
    Scene: 
    Que simplemente es el cuadro a dibujar es decir "En este panel, donde esta mi camara y a donde apunta?"

    Lista de...
    Object:
    Un elemento a dibujar en el panel formado por figuras geometricas fundamentales

*/

typedef int (*Draw)(struct list *figures);  // El usuario debe definir sus draw para cada estado de su automata
typedef int (*Check)(struct object *self, void *environment);
typedef void (*Behavior)(struct object *self, int step, void *params, void *env);

typedef enum figures
{
    TRIANGLE,
    RECTANGLE,
    POLYGON,
    LINE,
    CIRCLE,
    OVAL
}FIG;

typedef enum animationStatus
{
    PAUSE,      // Animacion en pausa
    PLAYING,    // Animacion corriendo
    STEPS       // Animacion por pasos (permite explorarla con las flechas del teclado)
}aniStat;

typedef enum animationCycle 
{
    ALIVE,      // Sigue animándose normalmente
    FROZEN,     // Se queda quieto en su último frame dibujado
    HIDDEN,     // Deja de dibujarse 
    LOOP        // Reinicia su ciclo, para acciones rutinarias, como 4 frames de caminata/correr/saltar
}LIFE;

typedef struct animation
{
    enum animationStatus AS;
    float speed;            // Velocidad de reproduccion de la animación
    struct queue *panels;   // Cola de paneles a dibujar 
}ANI;

typedef struct panel
{
    struct scene *currentScene; // Cuadro a dibujar AKA Camara 
    struct hash *layers;        // Capas en este panel
    struct list *allObjects;    // Objetos totales del panel
}PANEL;

typedef struct layer
{
    char layerName[30];
    struct hash *objects;       // Lo que dibujamos
                                // Para modificaciones
    Behavior initialBehavior;   // Todos los objetos empiezan con un comportamiento IDLE
                                // que en la API se define como a la espera de un trigger
                                // ese comportamiento se puede cambiar en initialBehavior
                                // que hace override del behavior inicial para todos sus hijos
                                // de esta forma podemos inicializar un grupo de objetos que hacen algo en una capa
                                // por defecto esto se inicializa en IDLE
}LAYER;

typedef struct scene
{
    float width;                // lo que vemos en x
    float height;               // lo que vemos en y
                                // al definirlo como width y height
                                // nos ahorramos definir de una forma para 2D y otra para 3D
                                // ya que el 3D es calculable considerando un cono de vision
                                // Aunque lo mas probable es que no implemente 3D para esta entrega
}SCENE;

typedef struct object
{   
    char key[30];                    // clave hash del objeto
    char layerKey[30];               // Capa en la que se debe dibujar 
    struct transform *t;             // calculo de posicion del objeto
    struct list *figures;            // Componentes a dibujar para formar el objeto 
    struct list *statusStack;        // Comportamiento del objeto respecto al entorno
                                     // en forma de pila para poder hacer cambios acumulativos
                                     // Ejemplo, si detecto salto empujo en la pila esa accion
                                     // caigo al suelo, saco de la pila y puedo seguir corriendo
                                     // Al tener pares logicos/visuales podemos definir comportamientos
                                     // precargados, como solo moverse o solo saltar modificables por el entorno
    enum animationCycle status;      // Auxiliar para la generacion de animaciones
    struct statusPair *activeStatus; 
    struct node *currentFrame;
    float maxY;
    float maxX;
    void *custom;              
}OBJECT;

typedef struct statusPair
{
    Behavior func; // Como se comporta el objeto en el estado actual
    void *params;  // Parametros de comportamiento
    struct graph *animSequence; // Como se ve el objeto en el estado actual
}STATUS;

typedef struct transform
{
    struct coordinates  *globalPos; // La posicion global del objeto en el mundo
    struct coordinates  *scale;     // La escala de z solo es relevante cuando se anima en 3D, al animar en 2D
                                    // guardada en una coordenada porque asi reciclamos estructuras
                                    // la funcion la ignorara porque puede romper la animación
    struct coordinates  *rotation;  // Las transformaciones rotacionales de nuestro objeto son posibles
                                    // esto nos permitiria mostrar en la animacion el perfil de algo
                                    // sin la necesidad de definir como se dibuja, aunque puede no verse bien
                                    // dependiendo del formato de dibujado

    // Estas estructuras deberian ser listas ya que un solo objeto puede tener multiples colisiones o areas 
    // de vision, de ruido, de lo que quiera el usuario definir como area + los parametros con los que lo defina
    // sin embargo por tiempo y solo para mostrar su funcionamiento se dejaron unicamente
    struct fig *colissionBox;       // Colision del objeto por si hay interaccion en la animacion
                                    // vive aqui para que las cajas de colision tambien escalen con el objeto
                                    // como ambos vienen de figuras que se calculan con offSet sera un escalado perfecto
    struct fig *effectArea;         // tecnicamente una colision tambien, separada para showcase
}TRANSFORM;

typedef struct trigger 
{
    Check check;                // El automata no sabe que acciones se deben tomar en un prefabricado
                                // pero se le puede decir de que estar pendiente
    statusPair *targetStatus;   // Un trigger debe siempre permitirnos hacer un cambio de estado
                                // asi targetStatus sea NULL podriamos hacer un pop a la pila
}TRIGGER;

typedef struct fig
{
    struct design *color;           // Parametros de diseño, actualmente solo color RGB + Transparencia
    struct list *offSet;            // Puntos a dibujar de la figura calculados mediante un offSet
                                    // definido por el struct coordenada   
    struct coordinates *relPos;     // Las figuras de un objeto necesitan una posicion relativa para dibujarse
    struct coordinates *localRot;   // Ademas para evitar funciones como "trianguloVolteado" o "rombo"
                                    // Podemos darle una rotacion inicial   
    enum figures f;                 // Figura a dibujar;
}F;

typedef struct design
{
    struct coordinates *color;      // RGB del color
    float transparency;             // 4to parametro de GLColor

    /*
        Aqui pueden ir mas cosas, como mi libreria de assets, pero eso se dejo fuera por tiempo 
    */

}DESIGN;

typedef struct coordinates
{
    float x, y, z;
}CRD;

typedef struct genericParams
{
    float speedX;       // Unidades por frame en X
    float speedY;       // Unidades por frame en Y
    float friction;         
    float gravity;

    int stepCounter;    // Cuenta cuántos frames lleva en este estado
    int durationLimit;  // Si llega a este numero, hace POP automático (para OneShots)
    struct list *triggers; // Lista de (TRIGGER*)    
}GP;

CRD *initCoord(float x, float y, float z);
F *initFigure(LIST *pointOffSet, CRD *localPosition, CRD *localRotation, enum figures f);
TRIGGER *initTrigger(Check check, STATUS *targetStatus);
TRANSFORM *initPhysics(F *colision, CRD *pos, CRD *scale, CRD *rotation);
OBJECT *initObject(char *objectName, char *layerName, TRANSFORM *initial, LIST *figures);
SCENE *initScene(float width, float height);
LAYER *initLayer(char *layerName, Behavior initialBehavior);
PANEL *initPanel(SCENE *camera);
DESIGN *initDesign(float r, float g, float b, float transparency);
ANI *initAnimation();
int destroyCoord(void *data);
int addPanel(ANI *animation, PANEL *p);
int addLayer(PANEL *p, LAYER *l);
int addObject(PANEL *p, LAYER *l, OBJECT *o);
int addColission(OBJECT *o, F *colissionBox);
F *generateFigure(enum figures f, DESIGN *des, float arg1, float arg2, float localX, float localY, float zPriority, float rotX, float rotY, float rotZ);
LIST *triangleOffSet(float base, float height);
LIST *lineOffSet(float length);
LIST *circleOffSet(int smoothness, float radius);
LIST *polygonOffSet(int segments, float radius);
LIST *rectangleOffSet(float width, float length);
LIST *getOffSet(enum figures figType, float arg1, float arg2);
F *generateColission(enum figures figType, float arg1, float arg2);
GRAPH *generateBluePrint(char *sequenceName, QUEUE *objectSequence, int type);
STATUS *generateStatus(Behavior func, struct graph *animationSequence, void *params);
int pushFrame(QUEUE *sequence, OBJECT *frameObj);
int pushFigure(LIST **figureList, F *fig);
PANEL *generatePanelFromObjects(SCENE *camera, LIST *objects);
OBJECT *instanceObject(OBJECT *tmplt);
STATUS *getBase(Behavior func);
void calculateDimensions(OBJECT *obj);
int animationSimple(ANI *toModify, SCENE *absolute, LIST *initialObjects, int frames);

// Cerebros, fisicas y triggers
void Static(struct object *self, int step, void *params, void *env);
void Idle(struct object *self, int step, void *params, void *env);
void Walk(struct object *self, int step, void *params, void *env);
void Jump(struct object *self, int step, void *params, void *env);
void Fall(struct object *self, int step, void *params, void *env);
int advanceAutomata(OBJECT *obj);
void physicsUpdate(OBJECT *self, GP *p);
int checkTriggers(OBJECT *self, GP *params, void *env);
int checkGround(OBJECT *self, void *env);

// OpenGL
void drawFigure(F *fig);
void drawObject(OBJECT *obj);
void display();
void reshape(int w, int h);
void timer(int v);
void keyboard(unsigned char key, int x, int y);
void special(int key, int x, int y);
void startGraphicsLoop(ANI *ani, int argc, char **argv, char *title);
int checkVision(OBJECT *self, void *env);

#endif