#include <stdlib.h>
#include <pthread.h>
#include <stdio.h>
#include <semaphore.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

// Número de nodos
#define N 3

int vector_peticiones[N];           // Cola de solicitudes por atender
int vector_atendidas[N];            // Cola de solicitudes atendidas
int token, id, seccion_critica = 0; // Testigo, ID del nodo, y estado de la SC

// Semáforos de exclusión mutua
sem_t vector_atendidas_sem, vector_peticiones_sem, token_sem, seccion_critica_sem;

// Estructura de los mensajes
struct msgstruct
{
    long mtype;                    // Tipo de mensaje, 1 -> testigo, 2 -> petición
    int vector_atendidas_token[N]; // Datos del testigo
    int id_nodo_origen;            // ID del nodo origen
    int num_peticion_nodo_origen;  // Número de petición del nodo origen
};

// Función para crear las colas de mensajes
void inicializar_cola()
{
    // Creamos una cola de mensaje para cada nodo
    for (int i = 0; i < N; i++)
    {
        int msgid = msgget(0x900 + i, 0666);
        if (msgid != -1)
        {
            msgctl(msgid, IPC_RMID, NULL);
        }
        msgid = msgget(0x900 + i, 0666 | IPC_CREAT);
    }
}

// Función para enviar el testigo a otro nodo
void enviar_token(int id_nodo_destino)
{
    // Creamos el mensaje
    struct msgstruct msg_token;
    msg_token.mtype = 1;
    sem_wait(&vector_atendidas_sem);
    for (int i = 0; i < N; i++)
    {
        msg_token.vector_atendidas_token[i] = vector_atendidas[i];
    }
    sem_post(&vector_atendidas_sem);

    // Actualizamos el estado del testigo en el nodo
    sem_wait(&token_sem);
    token = 0;
    sem_post(&token_sem);

    // Enviamos el mensaje con el testigo
    int msgid = msgget(0x900 + id_nodo_destino, 0666);
    msgsnd(msgid, &msg_token, sizeof(msg_token), 0);
}

// Función receptora de mensajes de solicitud
void *receptor(void *arg)
{
    // Variables para almacenar la información recibida de los mensajes
    int id_nodo_origen, num_peticion_nodo_origen = 0;

    // Bucle receptor
    while (1)
    {
        // Esperamos a recibir una petición
        struct msgstruct msg_peticion;
        int msgid = msgget(0x900 + id, 0666);
        msgrcv(msgid, &msg_peticion, sizeof(msg_peticion), 2, 0);

        // Obtenemos los datos del mensaje
        id_nodo_origen = msg_peticion.id_nodo_origen;
        num_peticion_nodo_origen = msg_peticion.num_peticion_nodo_origen;

        // Actualizamos el vector de peticiones pendientes
        sem_wait(&vector_atendidas_sem);
        sem_wait(&vector_peticiones_sem);
        sem_wait(&seccion_critica_sem);
        sem_wait(&token_sem);
        vector_peticiones[id_nodo_origen] = num_peticion_nodo_origen > vector_peticiones[id_nodo_origen] ? num_peticion_nodo_origen : vector_peticiones[id_nodo_origen];

        // Si tenemos el testigo, no estamos en la sección crítica y la petición es nueva, pasamos el testigo
        if (token && !seccion_critica && vector_peticiones[id_nodo_origen] > vector_atendidas[id_nodo_origen])
        {
            sem_post(&vector_atendidas_sem);
            sem_post(&vector_peticiones_sem);
            sem_post(&seccion_critica_sem);
            sem_post(&token_sem);
            enviar_token(id_nodo_origen);
        }
        else
        {
            sem_post(&vector_atendidas_sem);
            sem_post(&vector_peticiones_sem);
            sem_post(&seccion_critica_sem);
            sem_post(&token_sem);
        }
    }
}

int main(int argc, char *argv[])
{
    // Primer parámetro = 1 -> el nodo tiene el testigo
    // Primer parámetro = 0 -> el nodo no tiene el testigo
    token = atoi(argv[1]);
    // Segundo parámetro -> ID del nodo
    id = atoi(argv[2]);
    int peticion = 0; // Número de solicitud

    sem_init(&vector_atendidas_sem, 0, 1);
    sem_init(&vector_peticiones_sem, 0, 1);
    sem_init(&token_sem, 0, 1);
    sem_init(&seccion_critica_sem, 0, 1);

    // El nodo con el testigo es el encargado de crear las colas de mensajes
    if (token)
    {
        inicializar_cola();
    }

    // Creamos el hilo receptor de mensajes
    pthread_t hilo_receptor;
    pthread_create(&hilo_receptor, NULL, receptor, NULL);

    // Bucle principal
    while (1)
    {
        printf("Trabajando...\n");
        getchar();
        printf("Intentando entrar en la sección crítica...\n");
        sem_wait(&token_sem);
        // Si no tenemos el testigo, se envía una solicitud a cada nodo
        if (!token)
        {
            sem_post(&token_sem);
            printf("Petición de token...\n");

            // Establecemos el número de la solicitud
            peticion += 1;

            // Creamos el mensaje de solicitud
            struct msgstruct msg_peticion;
            msg_peticion.mtype = 2;
            msg_peticion.id_nodo_origen = id;
            msg_peticion.num_peticion_nodo_origen = peticion;

            // Lo enviamos a cada nodo
            for (int i = (id + 1) % N; i != id; i = (i + 1) % N)
            {
                int msgid = msgget(0x900 + i, 0666);
                msgsnd(msgid, &msg_peticion, sizeof(msg_peticion), 0);
            }
            // Esperamos a recibir el testigo
            struct msgstruct msg_token;
            int msgid = msgget(0x900 + id, 0666);
            msgrcv(msgid, &msg_token, sizeof(msg_token), 1, 0);
            printf("Token recibido...\n");

            // Actualizamos el vector de solicitudes atendidas con el recibido en el mensaje
            sem_wait(&vector_atendidas_sem);
            for (int i = 0; i < N; i++)
            {
                vector_atendidas[i] = msg_token.vector_atendidas_token[i];
            }
            sem_post(&vector_atendidas_sem);

            // Actualizamos el estado del testigo en el nodo
            sem_wait(&token_sem);
            token = 1;
            sem_post(&token_sem);
        }
        else
        {
            sem_post(&token_sem);
        }

        // Actualizamos el estado de la sección crítica en el nodo
        sem_wait(&seccion_critica_sem);
        seccion_critica = 1;
        sem_post(&seccion_critica_sem);

        // Sección crítica
        printf("Sección crítica...\n");
        getchar();
        printf("Saliendo de la sección crítica...\n");

        // Añadimos la petición creada a la lista de atendidas
        sem_wait(&vector_atendidas_sem);
        vector_atendidas[id] = peticion;
        sem_post(&vector_atendidas_sem);

        // Actualizamos el estado de la sección crítica en el nodo
        sem_wait(&seccion_critica_sem);
        seccion_critica = 0;
        sem_post(&seccion_critica_sem);

        // Buscamos si hay algún nodo esperando por el testigo
        sem_wait(&vector_atendidas_sem);
        sem_wait(&vector_peticiones_sem);
        int nodo_en_espera = 0;
        for (int i = (id + 1) % N; i != id; i = (i + 1) % N)
        {
            // Si lo hay, le pasamos el testigo
            if (vector_peticiones[i] > vector_atendidas[i])
            {
                sem_post(&vector_atendidas_sem);
                sem_post(&vector_peticiones_sem);
                nodo_en_espera = 1;
                enviar_token(i);
                break;
            }
        }
        if (!nodo_en_espera)
        {
            sem_post(&vector_atendidas_sem);
            sem_post(&vector_peticiones_sem);
        }
    }
}