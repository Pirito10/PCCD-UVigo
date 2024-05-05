// Sin esta línea se rompe mi entorno, se puede suprimir en la entrega final pero la necesito para debuggear -Manu
#define _GNU_SOURCE

#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/msg.h>

#include "utils.h" // Archivo de cabecera con la definición de las funciones y estructura de los mensajes

#define MAX(a, b) (((a) > (b)) ? (a) : (b)) // Cálculo de valor máximo entre dos variables

int token, id, seccion_critica; // Testigo, ID del nodo y estado de la SC
int vector_peticiones[3][N];    // Cola de solicitudes por atender
int vector_atendidas[3][N];     // Cola de solicitudes atendidas
int cola_msg = 0;               // Cola de mensajes del nodo
int primero_t1 = 1;             // Está a 0 cuando los procesos T1 ya se han empezado a dar paso unos a otros

int quiere[3] = {0, 0, 0}; // Vector de procesos que quieren SC por cada prioridad

int espera_token = 0;       // Número de procesos a la espera de token
sem_t token_solicitado_sem; // Semáforo de paso para procesos en espera de token

int espera_t1;       // Número de procesos de T1 que están a la espera
sem_t espera_t1_sem; // Semáforo de paso para procesos T1 en espera por prioridad superior o en cola de SC

sem_t mutex_sc_sem; // Semáforo de exclusión mutua de SC

void *t0(void *args)
{
    while (1)
    {
        struct msg_nodo msg_cliente = (const struct msg_nodo){0};
        // Esperamos a recibir una solicitud de pago de un cliente
        msgrcv(cola_msg, &msg_cliente, sizeof(msg_cliente), PAGOS, 0);

        quiere[0]++;

        if (!token)
        {
            if (!peticion_activa(0))
            {
                broadcast(0);
            }
            if (espera_token)
            {
                espera_token++;
                sem_wait(&token_solicitado_sem);
            }
            else
            {
                espera_token++;
                struct msg_nodo msg_token = (const struct msg_nodo){0};
                // Recibir token
                msgrcv(cola_msg, &msg_token, sizeof(msg_token), TOKEN, 0);
                actualizar_atendidas(msg_token.vector_atendidas);
                token = 1;
                // Despertar a los procesos que esperaban el token
                for (int i = 1; i < espera_token; i++)
                {
                    sem_post(&token_solicitado_sem);
                }
                espera_token = 0;
            }
        }

        sem_wait(&mutex_sc_sem);
        seccion_critica = 1;
        // SECCIÓN CRÍTICA
        seccion_critica = 0;
        sem_post(&mutex_sc_sem);

        quiere[0]--;
        if (quiere[0] == 0)
        {
            vector_atendidas[0][id] = vector_peticiones[0][id];
            int nodo_siguiente = buscar_nodo_siguiente();
            if (nodo_siguiente > 0)
            {
                token = 0;
                enviar_token(nodo_siguiente);
            }
            else if (espera_t1 > 0)
            {
                espera_t1--;
                sem_post(&espera_t1_sem);
            }
        }
    }
}

void *t1(void *args)
{
    while (1)
    {
        struct msg_nodo msg_cliente = (const struct msg_nodo){0};
        // Recibir peticion cliente
        msgrcv(cola_msg, &msg_cliente, sizeof(msg_cliente), RESERVAS, 0);

        quiere[1]++;

        while (1)
        {
            if (!token)
            {
                if (!peticion_activa(1))
                {
                    broadcast(1);
                }
                if (espera_token)
                {
                    espera_token++;
                    sem_wait(&token_solicitado_sem);
                }
                else
                {
                    espera_token++;
                    struct msg_nodo msg_token = (const struct msg_nodo){0};
                    // Recibir token
                    msgrcv(cola_msg, &msg_token, sizeof(msg_token), TOKEN, 0);
                    actualizar_atendidas(msg_token.vector_atendidas);
                    token = 1;
                    // Despertar a los procesos que esperaban el token
                    for (int i = 1; i < espera_token; i++)
                    {
                        sem_post(&token_solicitado_sem);
                    }
                    espera_token = 0;
                }
            }

            // Los procesos T1 se suspenden aquí
            if (quiere[0] || !primero_t1)
            {
                espera_t1++;
                sem_wait(&espera_t1_sem);
            }
            // Si se tiene el token al ser despertado se pasa a obtener SC sinó se repite el proceso de intentar obtenerla
            if (token)
            {
                if (primero_t1)
                {
                    primero_t1 = 0;
                }
                break;
            }
        }

        sem_wait(&mutex_sc_sem);
        seccion_critica = 1;
        // SECCIÓN CRÍTICA
        seccion_critica = 0;
        sem_post(&mutex_sc_sem);

        quiere[1]--;
        if (quiere[1] == 0)
        {
            primero_t1 = 1;
            vector_atendidas[1][id] = vector_peticiones[1][id];
        }
        int nodo_siguiente = buscar_nodo_siguiente();
        if (nodo_siguiente > 0)
        {
            token = 0;
            primero_t1 = 1;
            enviar_token(nodo_siguiente);
            // Despertar a todos los T1 que esperaban en cola o a proceso prioritario para que esperen token
            for (int i = 0; i < espera_t1; i++)
            {
                sem_post(&espera_t1_sem);
            }
            espera_t1 = 0;
        }
        else if (!quiere[0])
        {
            espera_t1--;
            sem_post(&espera_t1_sem);
        }
    }
}

// Función receptora de mensajes de otros nodos
void receptor()
{
    while (1)
    {
        struct msg_nodo msg_peticion = (const struct msg_nodo){0};
        // Esperamos a recibir una solicitud de token
        msgrcv(cola_msg, &msg_peticion, sizeof(msg_peticion), REQUEST, 0);
        // Actualizamos el vector de peticiones
        vector_peticiones[msg_peticion.prioridad_origen][msg_peticion.id_nodo_origen] = MAX(vector_peticiones[msg_peticion.prioridad_origen][msg_peticion.id_nodo_origen], msg_peticion.num_peticion_nodo_origen);
        // Pasamos el token si procede
        if (token && !seccion_critica && prioridad_superior(msg_peticion.prioridad_origen) && (vector_peticiones[msg_peticion.prioridad_origen][msg_peticion.id_nodo_origen] > vector_atendidas[msg_peticion.prioridad_origen][msg_peticion.id_nodo_origen]))
        {
            token = 0;
            enviar_token(msg_peticion.id_nodo_origen);
        }
    }
}

int main(int argc, char *argv[])
{
    // Inicializamos los arrays a cero
    for (int i = 0; i < 3; i++)
    {
        for (int j = 0; j < N; j++)
        {
            vector_atendidas[i][j] = 0;
            vector_peticiones[i][j] = 0;
        }
    }

    id = atoi(argv[1]); // ID del nodo

    // Damos el token al nodo con ID = 0
    if (id == 0)
    {
        token = 1;
    }

    // Incializamos la cola de mensajes del nodo
    cola_msg = msgget(1000 + id, 0666 | IPC_CREAT);
    if (cola_msg != -1)
    {
        msgctl(cola_msg, IPC_RMID, NULL);
        cola_msg = msgget(1000 + id, 0666 | IPC_CREAT);
    }

    // Inicializamos los semáforos
    sem_init(&token_solicitado_sem, 0, 0);
    sem_init(&espera_t1_sem, 0, 0);
    sem_init(&mutex_sc_sem, 0, 1);

    // Creamos los hilos de prioridad T0
    pthread_t hilo_t0[3];
    for (int i = 0; i < 3; i++)
    {
        pthread_create(&hilo_t0[i], NULL, t0, NULL);
    }

    // Creamos los hilos de prioridad T1
    pthread_t hilo_t1[3];
    for (int i = 0; i < 3; i++)
    {
        pthread_create(&hilo_t1[i], NULL, t1, NULL);
    }

    // Ejecutamos el receptor de mensajes (bucle infinito)
    receptor();
}