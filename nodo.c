#include <stdlib.h>
#include <pthread.h>
#include <stdio.h>
#include <semaphore.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#define N 3 // Número de nodos

int vector_peticiones[N];                                       // Cola de solicitudes por atender
int vector_atendidas[N];                                        // Cola de solicitudes atendidas
int token, token_lector, id, seccion_critica = 0, peticion = 0; // Testigos, ID del nodo, estado de la SC y número de petición
int quiere[3] = {0, 0, 0};

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

// Estructura para los mensajes de los clientes
struct msg_cliente
{
    long mtype;    // Tipo de mensaje, 1 -> pago, 2 -> anulación, 3 -> reserva, 4 -> administración, 5 -> consulta
    char mtext[1]; // Por definición, es necesario algún campo para almacenar datos, aunque no se quieran usar
};

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
void *receptor()
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

// Proceso de pagos
void *pagos(void *arg)
{
    int msgid = (int)(intptr_t)arg; // Obtenemos el ID de la cola de mensajes de los clientes
    struct msg_cliente msg_temp;    // Creamos un struct para almacenar temporalmente el mensaje del cliente

    // Bucle principal
    while (1)
    {
        // Esperamos a recibir un mensaje de pago
        msgrcv(msgid, &msg_temp, sizeof(msg_temp.mtext), 1, 0); // Esperamos mensaje de cliente de tipo 1

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

// Proceso de anulaciones
void *anulaciones(void *arg)
{
    int msgid = (int)(intptr_t)arg; // Obtenemos el ID de la cola de mensajes de los clientes
    struct msg_cliente msg_temp;    // Creamos un struct para almacenar temporalmente el mensaje del cliente

    // Bucle principal
    while (1)
    {
        // Esperamos a recibir un mensaje de anulación
        msgrcv(msgid, &msg_temp, sizeof(msg_temp.mtext), 2, 0); // Esperamos mensaje de cliente de tipo 2

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

// Proceso de reservas
void *reservas(void *arg)
{
    int msgid = (int)(intptr_t)arg; // Obtenemos el ID de la cola de mensajes de los clientes
    struct msg_cliente msg_temp;    // Creamos un struct para almacenar temporalmente el mensaje del cliente

    // Bucle principal
    while (1)
    {
        // Esperamos a recibir un mensaje de reserva
        msgrcv(msgid, &msg_temp, sizeof(msg_temp.mtext), 3, 0); // Esperamos mensaje de cliente de tipo 3

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

// Proceso de administración
void *administracion(void *arg)
{
    int msgid = (int)(intptr_t)arg; // Obtenemos el ID de la cola de mensajes de los clientes
    struct msg_cliente msg_temp;    // Creamos un struct para almacenar temporalmente el mensaje del cliente

    // Bucle principal
    while (1)
    {
        // Esperamos a recibir un mensaje de administración
        msgrcv(msgid, &msg_temp, sizeof(msg_temp.mtext), 4, 0); // Esperamos mensaje de cliente de tipo 4

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

// Proceso de consultas
void *consultas(void *arg)
{
    int msgid = (int)(intptr_t)arg; // Obtenemos el ID de la cola de mensajes de los clientes
    struct msg_cliente msg_temp;    // Creamos un struct para almacenar temporalmente el mensaje del cliente

    // Bucle principal
    while (1)
    {
        // Esperamos a recibir un mensaje de consulta
        msgrcv(msgid, &msg_temp, sizeof(msg_temp.mtext), 5, 0); // Esperamos mensaje de cliente de tipo 5

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

// Proceso principal
int main(int argc, char *argv[])
{
    id = atoi(argv[1]); // ID del nodo

    sem_init(&vector_atendidas_sem, 0, 1);
    sem_init(&vector_peticiones_sem, 0, 1);
    sem_init(&token_sem, 0, 1);
    sem_init(&seccion_critica_sem, 0, 1);

    // Le damos el token al nodo con ID = 0
    if (id == 0)
    {
        token = 1;
    }

    // Creamos la cola para recibir mensajes de los clientes
    int msgid = msgget(0x900 + id, 0666 | IPC_CREAT);
    if (msgid != -1)
    {
        msgctl(msgid, IPC_RMID, NULL);
    }

    // Creamos el hilo receptor, y un hilo para cada tipo de proceso
    pthread_t hilo_receptor;
    pthread_t hilo_pagos;
    pthread_t hilo_anulaciones;
    pthread_t hilo_reservas;
    pthread_t hilo_administracion;
    pthread_t hilo_consultas;
    pthread_create(&hilo_receptor, NULL, receptor, NULL);
    pthread_create(&hilo_pagos, NULL, pagos, (void *)(intptr_t)msgid);
    pthread_create(&hilo_anulaciones, NULL, anulaciones, (void *)(intptr_t)msgid);
    pthread_create(&hilo_reservas, NULL, reservas, (void *)(intptr_t)msgid);
    pthread_create(&hilo_administracion, NULL, administracion, (void *)(intptr_t)msgid);
    pthread_create(&hilo_consultas, NULL, consultas, (void *)(intptr_t)msgid);
}