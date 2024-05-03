#include <stdlib.h>
#include <pthread.h>
#include <stdio.h>
#include <semaphore.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#define N 3       // Número de nodos
#define CLIENTE 0 // Clave para la cola de mensajes de clientes
#define NODO 1    // Clave para la cola de mensajes internodo

int vector_peticiones[3][N];                                                     // Cola de solicitudes por atender
int vector_atendidas[3][N];                                                      // Cola de solicitudes atendidas
int token, token_lector, id, seccion_critica = 0, sc_lectores = 0, peticion = 0; // Testigos, ID del nodo, estado de la SC y número de petición
int quiere[3] = {0, 0, 0};                                                       // Array con contadores de procesos esperando a la SC para cada tipo de prioridad

// Estructura de los mensajes
struct msg_solicitud
{
    long mtype;                   // Tipo de mensaje, 0 -> solicitud de un escritor, 1 -> solicitud de un lector, 2 -> devolución de token copia
    int id_nodo_origen;           // ID del nodo origen
    int num_peticion_nodo_origen; // Número de petición del nodo origen
    int prioridad_origen;         // Prioridad de la solicitud
};

// Estructura para los mensajes de token
struct msg_token
{
    long mtype;                    // 0 -> token original, 1 -> token copia
    int vector_atendidas_token[N]; // Vector de solicitudes atendidas
}

// Estructura para los mensajes de los clientes
struct msg_cliente
{
    long mtype;    // Tipo de mensaje, 1 -> pago, 2 -> anulación, 3 -> reserva, 4 -> administración, 5 -> consulta
    char mtext[1]; // Por definición, es necesario algún campo para almacenar datos, aunque no se quieran usar
};

// Función para enviar el testigo a otro nodo
void enviar_token(int id_nodo_destino, int tipo)
{
    // Creamos el mensaje
    struct msg_token msg_token;
    msg_token.mtype = tipo; // 0 -> token original, 1 -> token copia
    for (int j = 0; j < 3; j++)
    {
        for (int i = 0; i < N; i++)
        {
            // Introducimos el vector de atendidas en el mensaje
            msg_token.vector_atendidas_token[j][i] = vector_atendidas[j][i];
        }
    }

    // Actualizamos el estado del testigo en el nodo
    token = 0;

    // Enviamos el mensaje con el testigo
    int msgid = msgget(NODO + id_nodo_destino, 0666);
    msgsnd(msgid, &msg_token, sizeof(msg_token), 0);
}

// Función receptora de mensajes de solicitud
void *receptor()
{
    // Variables para almacenar la información recibida de los mensajes
    int id_nodo_origen, num_peticion_nodo_origen, prioridad_origen, tipo_origen;

    // Bucle receptor
    while (1)
    {
        // Esperamos a recibir una petición
        struct msg_solicitud msg_solicitud;
        int msgid = msgget(NODO + id, 0666);
        msgrcv(msgid, &msg_solicitud, sizeof(msg_solicitud), 0, 0);

        // Obtenemos los datos del mensaje
        tipo_origen = msg_peticion.mtype;                                 // Tipo de mensaje, 0 -> solicitud de un escritor, 1 -> solicitud de un lector, 2 -> devolución de token copia
        id_nodo_origen = msg_peticion.id_nodo_origen;                     // ID del nodo solicitante
        num_peticion_nodo_origen = msg_peticion.num_peticion_nodo_origen; // Número de petición de solicitud
        prioridad_origen = msg_peticion.prioridad_origen;                 // Prioridad de la solicitud

        // Actualizamos el vector de peticiones pendientes
        vector_peticiones[prioridad_origen][id_nodo_origen] = num_peticion_nodo_origen > vector_peticiones[prioridad_origen][id_nodo_origen] ? num_peticion_nodo_origen : vector_peticiones[prioridad_origen][id_nodo_origen];

        // Si lo que recibimos es la devolución de un token copia, eliminamos el nodo de la lista
        if (tipo_origen == 2)
        {
            eliminar_lista(id_nodo_origen);
        }
        // Si recibimos una solicitud de un lector, y ya hay un lector en sección crítica, enviamos un token copia, y añadimos el ID del nodo solicitante a la lista
        else if (tipo_origen == 1 && sc_lectores == 1)
        {
            anadir_lista(id_nodo_origen);
            enviar_token(id_nodo_origen, 1);
        }
        // Si tenemos el testigo, no estamos en la sección crítica y la petición es nueva, pasamos el testigo original
        else if (token && !seccion_critica && vector_peticiones[prioridad_origen][id_nodo_origen] > vector_atendidas[prioridad_origen][id_nodo_origen])
        {
            enviar_token(id_nodo_origen, 0);
        }
    }
}

// Proceso de pagos
void *pagos()
{
    int prioridad = 0;
    int msgid = msgget(CLIENTE + id, 0666 | IPC_CREAT); // Obtenemos el ID de la cola de mensajes de los clientes
    struct msg_cliente msg_temp;                        // Creamos un struct para almacenar temporalmente el mensaje del cliente

    // Bucle principal
    while (1)
    {
        // Esperamos a recibir un mensaje de cliente de tipo pago
        msgrcv(msgid, &msg_temp, sizeof(msg_temp), 1, 0);

        printf("Un proceso de pagos quiere entrar a la sección crítica...\n");

        // Apuntamos que un proceso de prioridad 0 quiere entrar a la sección crítica
        quiere[prioridad]++;

        // Si no tenemos el testigo, se envía una solicitud a cada nodo
        if (!token)
        {
            printf("Petición de token...\n");

            // Incrementamos el número de la solicitud
            peticion += 1;

            // Creamos el mensaje de solicitud
            struct msg_solicitud msg_solicitud;
            msg_solicitud.mtype = 0; // Solicitud de tipo escritor
            msg_solicitud.id_nodo_origen = id;
            msg_solicitud.num_peticion_nodo_origen = peticion;
            msg_solicitud.prioridad_origen = prioridad;

            // Lo enviamos a cada nodo
            for (int i = (id + 1) % N; i != id; i = (i + 1) % N)
            {
                int msgid = msgget(NODO + i, 0666);
                msgsnd(msgid, &msg_solicitud, sizeof(msg_solicitud), 0);
            }

            // Esperamos a recibir el testigo
            struct msg_token msg_token;
            int msgid = msgget(NODO + id, 0666);
            msgrcv(msgid, &msg_token, sizeof(msg_token), 0, 0);
            printf("Token recibido...\n");

            // Actualizamos el vector de solicitudes atendidas con el recibido en el mensaje
            for (int i = 0; i < N; i++)
            {
                vector_atendidas[prioridad][i] = msg_token.vector_atendidas_token[prioridad][i];
            }

            // Actualizamos el estado del testigo en el nodo
            token = 1;
        }

        // Actualizamos el estado de la sección crítica en el nodo
        seccion_critica = 1;

        // Sección crítica
        printf("Sección crítica...\n");
        sleep(2);
        printf("Saliendo de la sección crítica...\n");

        // Apuntamos que un proceso de prioridad 0 ya no quiere entrar a la sección crítica
        quiere[prioridad]--;
        // Apuntamos que la petición ya está atendida si no quedan más peticiones de igual prioridad en el nodo
        if (quiere[prioridad] == 0)
        {
            vector_atendidas[prioridad][id] = peticion;
        }

        // Actualizamos el estado de la sección crítica en el nodo
        seccion_critica = 0;

        int token_enviado = 0;
        // Buscamos si hay algún nodo esperando por el testigo, ordenando por prioridad
        for (int j = 0, j < 3; j++)
        {
            for (int i = (id + 1) % N; i != id; i = (i + 1) % N)
            {
                // Si lo hay, le pasamos el testigo
                if (vector_peticiones[j][i] > vector_atendidas[j][i])
                {
                    int token_enviado = 1;
                    enviar_token(i, 0);
                    break;
                }
            }
            if (token_enviado)
            {
                break;
            }
        }
    }
}

// Proceso de anulaciones
void *anulaciones()
{
    int prioridad = 0;
    int msgid = msgget(CLIENTE + id, 0666 | IPC_CREAT); // Obtenemos el ID de la cola de mensajes de los clientes
    struct msg_cliente msg_temp;                        // Creamos un struct para almacenar temporalmente el mensaje del cliente

    // Bucle principal
    while (1)
    {
        // Esperamos a recibir un mensaje de cliente de tipo anulación
        msgrcv(msgid, &msg_temp, sizeof(msg_temp), 2, 0);

        printf("Un proceso de pagos quiere entrar a la sección crítica...\n");

        // Apuntamos que un proceso de prioridad 0 quiere entrar a la sección crítica
        quiere[prioridad]++;

        // Si no tenemos el testigo, se envía una solicitud a cada nodo
        if (!token)
        {
            printf("Petición de token...\n");

            // Incrementamos el número de la solicitud
            peticion += 1;

            // Creamos el mensaje de solicitud
            struct msg_solicitud msg_solicitud;
            msg_solicitud.mtype = 0; // Solicitud de tipo escritor
            msg_solicitud.id_nodo_origen = id;
            msg_solicitud.num_peticion_nodo_origen = peticion;
            msg_solicitud.prioridad_origen = prioridad;

            // Lo enviamos a cada nodo
            for (int i = (id + 1) % N; i != id; i = (i + 1) % N)
            {
                int msgid = msgget(NODO + i, 0666);
                msgsnd(msgid, &msg_solicitud, sizeof(msg_solicitud), 0);
            }

            // Esperamos a recibir el testigo
            struct msg_token msg_token;
            int msgid = msgget(NODO + id, 0666);
            msgrcv(msgid, &msg_token, sizeof(msg_token), 0, 0);
            printf("Token recibido...\n");

            // Actualizamos el vector de solicitudes atendidas con el recibido en el mensaje
            for (int i = 0; i < N; i++)
            {
                vector_atendidas[prioridad][i] = msg_token.vector_atendidas_token[prioridad][i];
            }

            // Actualizamos el estado del testigo en el nodo
            token = 1;
        }

        // Actualizamos el estado de la sección crítica en el nodo
        seccion_critica = 1;

        // Sección crítica
        printf("Sección crítica...\n");
        sleep(2);
        printf("Saliendo de la sección crítica...\n");

        // Apuntamos que un proceso de prioridad 0 ya no quiere entrar a la sección crítica
        quiere[prioridad]--;
        // Apuntamos que la petición ya está atendida si no quedan más peticiones de igual prioridad en el nodo
        if (quiere[prioridad] == 0)
        {
            vector_atendidas[prioridad][id] = peticion;
        }

        // Actualizamos el estado de la sección crítica en el nodo
        seccion_critica = 0;

        int token_enviado = 0;
        // Buscamos si hay algún nodo esperando por el testigo, ordenando por prioridad
        for (int j = 0, j < 3; j++)
        {
            for (int i = (id + 1) % N; i != id; i = (i + 1) % N)
            {
                // Si lo hay, le pasamos el testigo
                if (vector_peticiones[j][i] > vector_atendidas[j][i])
                {
                    int token_enviado = 1;
                    enviar_token(i, 0);
                    break;
                }
            }
            if (token_enviado)
            {
                break;
            }
        }
    }
}

// Proceso de reservas
void *reservas()
{
    int prioridad = 1;
    int msgid = msgget(CLIENTE + id, 0666 | IPC_CREAT); // Obtenemos el ID de la cola de mensajes de los clientes
    struct msg_cliente msg_temp;                        // Creamos un struct para almacenar temporalmente el mensaje del cliente

    // Bucle principal
    while (1)
    {
        // Esperamos a recibir un mensaje de cliente de tipo reserva
        msgrcv(msgid, &msg_temp, sizeof(msg_temp), 3, 0);

        printf("Un proceso de pagos quiere entrar a la sección crítica...\n");

        // Apuntamos que un proceso de prioridad 1 quiere entrar a la sección crítica
        quiere[prioridad]++;

        // Si no tenemos el testigo, se envía una solicitud a cada nodo
        if (!token)
        {
            printf("Petición de token...\n");

            // Incrementamos el número de la solicitud
            peticion += 1;

            // Creamos el mensaje de solicitud
            struct msg_solicitud msg_solicitud;
            msg_solicitud.mtype = 0; // Solicitud de tipo escritor
            msg_solicitud.id_nodo_origen = id;
            msg_solicitud.num_peticion_nodo_origen = peticion;
            msg_solicitud.prioridad_origen = prioridad;

            // Lo enviamos a cada nodo
            for (int i = (id + 1) % N; i != id; i = (i + 1) % N)
            {
                int msgid = msgget(NODO + i, 0666);
                msgsnd(msgid, &msg_solicitud, sizeof(msg_solicitud), 0);
            }

            // Esperamos a recibir el testigo
            struct msg_token msg_token;
            int msgid = msgget(NODO + id, 0666);
            msgrcv(msgid, &msg_token, sizeof(msg_token), 0, 0);
            printf("Token recibido...\n");

            // Actualizamos el vector de solicitudes atendidas con el recibido en el mensaje
            for (int i = 0; i < N; i++)
            {
                vector_atendidas[prioridad][i] = msg_token.vector_atendidas_token[prioridad][i];
            }

            // Actualizamos el estado del testigo en el nodo
            token = 1;
        }

        // Actualizamos el estado de la sección crítica en el nodo
        seccion_critica = 1;

        // Sección crítica
        printf("Sección crítica...\n");
        sleep(2);
        printf("Saliendo de la sección crítica...\n");

        // Apuntamos que un proceso de prioridad 1 ya no quiere entrar a la sección crítica
        quiere[prioridad]--;
        // Apuntamos que la petición ya está atendida si no quedan más peticiones de igual prioridad en el nodo
        if (quiere[prioridad] == 0)
        {
            vector_atendidas[prioridad][id] = peticion;
        }

        // Actualizamos el estado de la sección crítica en el nodo
        seccion_critica = 0;

        int token_enviado = 0;
        // Buscamos si hay algún nodo esperando por el testigo, ordenando por prioridad
        for (int j = 0, j < 3; j++)
        {
            for (int i = (id + 1) % N; i != id; i = (i + 1) % N)
            {
                // Si lo hay, le pasamos el testigo
                if (vector_peticiones[j][i] > vector_atendidas[j][i])
                {
                    int token_enviado = 1;
                    enviar_token(i, 0);
                    break;
                }
            }
            if (token_enviado)
            {
                break;
            }
        }
    }
}

// Proceso de administración
void *administracion()
{
    int prioridad = 1;
    int msgid = msgget(CLIENTE + id, 0666 | IPC_CREAT); // Obtenemos el ID de la cola de mensajes de los clientes
    struct msg_cliente msg_temp;                        // Creamos un struct para almacenar temporalmente el mensaje del cliente

    // Bucle principal
    while (1)
    {
        // Esperamos a recibir un mensaje de cliente de tipo administración
        msgrcv(msgid, &msg_temp, sizeof(msg_temp), 4, 0);

        printf("Un proceso de pagos quiere entrar a la sección crítica...\n");

        // Apuntamos que un proceso de prioridad 1 quiere entrar a la sección crítica
        quiere[prioridad]++;

        // Si no tenemos el testigo, se envía una solicitud a cada nodo
        if (!token)
        {
            printf("Petición de token...\n");

            // Incrementamos el número de la solicitud
            peticion += 1;

            // Creamos el mensaje de solicitud
            struct msg_solicitud msg_solicitud;
            msg_solicitud.mtype = 0; // Solicitud de tipo escritor
            msg_solicitud.id_nodo_origen = id;
            msg_solicitud.num_peticion_nodo_origen = peticion;
            msg_solicitud.prioridad_origen = prioridad;

            // Lo enviamos a cada nodo
            for (int i = (id + 1) % N; i != id; i = (i + 1) % N)
            {
                int msgid = msgget(NODO + i, 0666);
                msgsnd(msgid, &msg_solicitud, sizeof(msg_solicitud), 0);
            }

            // Esperamos a recibir el testigo
            struct msg_token msg_token;
            int msgid = msgget(NODO + id, 0666);
            msgrcv(msgid, &msg_token, sizeof(msg_token), 0, 0);
            printf("Token recibido...\n");

            // Actualizamos el vector de solicitudes atendidas con el recibido en el mensaje
            for (int i = 0; i < N; i++)
            {
                vector_atendidas[prioridad][i] = msg_token.vector_atendidas_token[prioridad][i];
            }

            // Actualizamos el estado del testigo en el nodo
            token = 1;
        }

        // Actualizamos el estado de la sección crítica en el nodo
        seccion_critica = 1;

        // Sección crítica
        printf("Sección crítica...\n");
        sleep(2);
        printf("Saliendo de la sección crítica...\n");

        // Apuntamos que un proceso de prioridad 1 ya no quiere entrar a la sección crítica
        quiere[prioridad]--;
        // Apuntamos que la petición ya está atendida si no quedan más peticiones de igual prioridad en el nodo
        if (quiere[prioridad] == 0)
        {
            vector_atendidas[prioridad][id] = peticion;
        }

        // Actualizamos el estado de la sección crítica en el nodo
        seccion_critica = 0;

        int token_enviado = 0;
        // Buscamos si hay algún nodo esperando por el testigo, ordenando por prioridad
        for (int j = 0, j < 3; j++)
        {
            for (int i = (id + 1) % N; i != id; i = (i + 1) % N)
            {
                // Si lo hay, le pasamos el testigo
                if (vector_peticiones[j][i] > vector_atendidas[j][i])
                {
                    int token_enviado = 1;
                    enviar_token(i, 0);
                    break;
                }
            }
            if (token_enviado)
            {
                break;
            }
        }
    }
}

// Proceso de consultas
void *consultas()
{
    int prioridad = 2;
    int msgid = msgget(CLIENTE + id, 0666 | IPC_CREAT); // Obtenemos el ID de la cola de mensajes de los clientes
    struct msg_cliente msg_temp;                        // Creamos un struct para almacenar temporalmente el mensaje del cliente

    // Bucle principal
    while (1)
    {
        // Esperamos a recibir un mensaje de cliente de tipo consulta
        msgrcv(msgid, &msg_temp, sizeof(msg_temp), 5, 0);

        printf("Trabajando...\n");
        getchar();
        printf("Intentando entrar en la sección crítica...\n");
        // Si no tenemos el testigo, se envía una solicitud a cada nodo
        if (!token)
        {
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
            for (int i = 0; i < N; i++)
            {
                vector_atendidas[i] = msg_token.vector_atendidas_token[i];
            }

            // Actualizamos el estado del testigo en el nodo
            token = 1;
        }

        // Actualizamos el estado de la sección crítica en el nodo
        seccion_critica = 1;

        // Sección crítica
        printf("Sección crítica...\n");
        getchar();
        printf("Saliendo de la sección crítica...\n");

        // Añadimos la petición creada a la lista de atendidas
        vector_atendidas[id] = peticion;

        // Actualizamos el estado de la sección crítica en el nodo
        seccion_critica = 0;

        // Buscamos si hay algún nodo esperando por el testigo
        for (int i = (id + 1) % N; i != id; i = (i + 1) % N)
        {
            // Si lo hay, le pasamos el testigo
            if (vector_peticiones[i] > vector_atendidas[i])
            {
                enviar_token(i);
                break;
            }
        }
    }
}

// Proceso principal
int main(int argc, char *argv[])
{
    id = atoi(argv[1]); // ID del nodo

    // Le damos el token al nodo con ID = 0
    if (id == 0)
    {
        token = 1;
    }

    int msgid;

    // Creamos la cola para recibir mensajes de los clientes
    msgid = msgget(CLIENTE + id, 0666 | IPC_CREAT);
    if (msgid != -1)
    {
        msgctl(msgid, IPC_RMID, NULL);
    }

    // Creamos la cola para recibir mensajes de los nodos
    msgid = msgget(NODO + id, 0666 | IPC_CREAT);
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
    pthread_create(&hilo_pagos, NULL, pagos, NULL);
    pthread_create(&hilo_anulaciones, NULL, anulaciones, NULL);
    pthread_create(&hilo_reservas, NULL, reservas, NULL);
    pthread_create(&hilo_administracion, NULL, administracion, NULL);
    pthread_create(&hilo_consultas, NULL, consultas, NULL);
}