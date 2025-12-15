#include"neuron.h"

int getXOR(int A, int B);

int main() {

    srand(time(NULL));

    // Flat input array, simulating the XOR table 
    float inputs[] = 
    { 
        0.0f, 0.0f,
        0.0f, 1.0f,
        1.0f, 0.0f,
        1.0f, 1.0f 
    };

    // Corresponding target for the inputs
    float targets[] = 
    { 
        0.0f, 
        1.0f, 
        1.0f, 
        0.0f 
    };


    MODEL *m = createModel(); 

    addLayer(m, 2, sigmoid); 
    addLayer(m, 3, sigmoid); 
    addLayer(m, 1, sigmoid); 

    compileNetwork(m);

    m->fit(m, inputs, targets, 4, 5000, 0.2);

    printf("\nResultado Final\n");
    
    for(int i=0; i<8; i+=2)
    {
        float prediction = predict(m,&inputs[i]);
        printf("\nInput: [%.0f,%.0f]\n Target: %f vs Prediction: %f\n",inputs[i],inputs[i+1],targets[(int)i/2],prediction);
    }

    int op = 0;
    do
    {
        printf("\n0.- Salir del Programa");
        printf("\n1.- Insertar valores a la red");
        printf("\nOption: ");
        scanf("%i",&op);

        if(op == 1)
        {
            float x1,x2;
            printf("\n\nX1 para la red neuronal:");
            scanf("%f",&x1);
            printf("\nX2 para la red neuronal:");
            scanf("%f",&x2);

            float arr[] = {x1,x2};
            float prediction = predict(m,arr);

            printf("\nInput: [%.0f,%.0f]\n Target: %i vs Prediction: %f\n",x1,x2,getXOR((int)x1,(int)x2),prediction);
        }
    }while(op);
    
    return 0;
}

int getXOR(int A, int B)
{
    // Expresion logica del XOR
    // A¬B + ¬AB
    return (A && !B) || (!A && B);
}