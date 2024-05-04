// Sin esta línea se rompe mi entorno, se puede suprimir en la entrega final pero la necesito para debuggear -Manu
#define _GNU_SOURCE

#include <stdlib.h>
#include <pthread.h>
#include <stdio.h>
#include <semaphore.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>

#define TOKEN 1
#define REQUEST 2
#define CLIENT 3

#define N 3                                   // Número de nodos
#define MAX(a,b) (((a)>(b))?(a):(b))

int token, id, seccion_critica, cola_msg;     // Testigos, ID del nodo, estado de la SC y la cola del nodo
int vector_peticiones[3][N];                  // Cola de solicitudes por atender
int vector_atendidas[3][N];                   // Cola de solicitudes atendidas

int quiere[3];                                // Vector de procesos que quieren por cada prioridad

// Estructura de los mensajes
struct msg_nodo
{
    long mtype;                   // Tipo de mensaje, 1 -> token, 2 -> peticion nodo, 3 -> peticion cliente
    int id_nodo_origen;           // ID del nodo origen
    int num_peticion_nodo_origen; // Número de petición del nodo origen
    int prioridad_origen;         // Prioridad de la solicitud
    int vector_atendidas[3][N];   // Vector atendidas
};

/**
 * Comprueba el vector quiere del nodo para determinar si hay procesos más prioritarios que el parámetro a la espera
 * @param prioridad prioridad a comprobar
 * @return devuelve 1 si la prioridad es más prioritaria que las que esperan en el nodo y 0 de lo contrario
*/
int prioridad_superior(int prioridad) {
    return NULL;
}

/**
 * Envía el token a otro nodo especificado
 * @param id_nodo id del nodo al que se envía el token
*/
void eviar_token(int id_nodo) {

}

void t0(int id_t0) {
    while(1) {
        struct msg_nodo msg_cliente;
        // Recibir peticion cliente
        msgrcv(cola_msg, &msg_cliente, sizeof(msg_cliente), CLIENT, 0);
        
    }
}

void receptor() {
    while(1) {
        struct msg_nodo msg_peticion;
        // Recibir peticion
        msgrcv(cola_msg, &msg_peticion, sizeof(msg_peticion), REQUEST, 0);
        // Actualizar vector peticiones
        vector_peticiones[msg_peticion.prioridad_origen][msg_peticion.id_nodo_origen] = MAX(vector_peticiones[msg_peticion.prioridad_origen][msg_peticion.id_nodo_origen], msg_peticion.num_peticion_nodo_origen);
        // Pasar token si procede
        if(token && !seccion_critica && prioridad_superior(msg_peticion.prioridad_origen)
        && (vector_peticiones[msg_peticion.prioridad_origen][msg_peticion.id_nodo_origen] > vector_atendidas[msg_peticion.prioridad_origen][msg_peticion.id_nodo_origen])) {
            enviar_token(msg_peticion.id_nodo_origen);
        }
    }
}

void main(int argc, char* argv[]) {
    id = atoi(argv[1]); // ID del nodo

    //Incializar cola nodo
    int msgid;
    msgid = msgget(id, 0666 | IPC_CREAT);
    if (msgid != -1)
    {
        msgctl(msgid, IPC_RMID, NULL);
    }
    
    pthread_t hilo_t0[10];
    for(int i = 0; i < 10; i++) {
        pthread_create(&hilo_t0[i], NULL, t0, i);
    }

    receptor();
}