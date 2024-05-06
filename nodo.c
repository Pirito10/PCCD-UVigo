// Sin esta línea se rompe mi entorno, se puede suprimir en la entrega final pero la necesito para debuggear -Manu
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <sys/msg.h>
#include <string.h>

#include "utils.h" // Archivo de cabecera con la definición de las funciones y estructura de los mensajes

int token, token_consulta, id, seccion_critica = 0; // Testigo, testigo consulta, ID del nodo y estado de la SC
int vector_peticiones[3][N];                        // Cola de solicitudes por atender
int vector_atendidas[3][N];                         // Cola de solicitudes atendidas
int cola_msg = 0;                                   // Cola de mensajes del nodo
int nodo_activo = 0;                                // A 1 cuando al menos un proceso no está a la espera de peticion de un cliente
int paso_consultas = 0;                             // A 1 cuando las consultas pueden entrar en SC
int primera_consulta = 1;                           // A 1 cuando no hay ninguna consulta en SC ni tokens de consulta distribuidos
int token_consulta_origen = -1;                     // ID del nodo del que se ha recibido el token consulta
int consultas_sc = 0;                               // Contador de numero de consultas en SC

int cola_t0, cola_t1, cola_t2 = 0; // Contadores para los procesos a la espera de cada prioridad
sem_t cola_t0_sem, cola_t1_sem, cola_t2_sem;

sem_t lista_vacia_sem; // Semaforo para la sync entre el ultipo lector y el receptor

int quiere[3] = {0, 0, 0}; // Vector de procesos que quieren SC por cada prioridad

sem_t mutex_sc_sem; // Semáforo de exclusión mutua de SC

// Hilo de tipo PAGOS/ANULACIONES
void *t0(void *args)
{
    struct thread_info *info = args;

    printf("[NODO %d][%s %d] -> proceso creado\n", id, info->nombre, info->thread_num);
    while (1)
    {
        struct msg_nodo msg_cliente = (const struct msg_nodo){0};
        // Esperamos a recibir una solicitud de pago/anulacion de un cliente
        msgrcv(cola_msg, &msg_cliente, sizeof(msg_cliente), info->tipo, 0);

        quiere[0]++;

        if ((!token || !lista_vacia()) && !peticion_activa(0))
        {
            printf("[NODO %d][%s %d] -> solicitando token\n", id, info->nombre, info->thread_num);
            broadcast(0);
        }

        if (nodo_activo)
        {
            printf("[NODO %d][%s %d] -> nodo activo, esperando ser atendido\n", id, info->nombre, info->thread_num);
            cola_t0++;
            sem_wait(&cola_t0_sem);
            printf("[NODO %d][%s %d] -> proceso despertado, intentando entrar\n", id, info->nombre, info->thread_num);
        }
        else
        {
            printf("[NODO %d][%s %d] -> nodo inactivo, intentando entrar\n", id, info->nombre, info->thread_num);
            nodo_activo = 1;
        }

        if (!token)
        {
            printf("[NODO %d][%s %d] -> no hay token, realizando solicitud\n", id, info->nombre, info->thread_num);
            struct msg_nodo msg_token = (const struct msg_nodo){0};
            // Recibir token
            msgrcv(cola_msg, &msg_token, sizeof(msg_token), TOKEN, 0);
            actualizar_atendidas(msg_token.vector_atendidas);
            token = 1;
            printf("[NODO %d][%s %d] -> token recibido\n", id, info->nombre, info->thread_num);
        }

        sem_wait(&mutex_sc_sem);
        seccion_critica = 1;
        printf("[NODO %d][%s %d] -> seccion critica\n", id, info->nombre, info->thread_num);
        // SECCIÓN CRÍTICA
        sleep(10);
        seccion_critica = 0;
        sem_post(&mutex_sc_sem);

        quiere[0]--;
        printf("[NODO %d][%s %d] -> ha salido seccion critica\n", id, info->nombre, info->thread_num);
        if (quiere[0] == 0)
        {
            printf("[NODO %d][%s %d] -> ultimo proceso ha salido, actualizando atendidas\n", id, info->nombre, info->thread_num);
            vector_atendidas[0][id] = vector_peticiones[0][id];
        }
        int nodo_siguiente = buscar_nodo_siguiente();
        if (nodo_siguiente >= 0)
        {
            printf("[NODO %d][%s %d] -> peticion prioritaria en nodo %d, enviando token\n", id, info->nombre, info->thread_num, nodo_siguiente);
            token = 0;
            enviar_token(nodo_siguiente);
        }
        if (procesos_quieren())
        {
            if (nodo_siguiente >= 0)
            {
                hacer_peticiones();
            }
            printf("[NODO %d][%s %d] -> despertando siguiente\n", id, info->nombre, info->thread_num);
            despertar_siguiente();
        }
        else
        {
            printf("[NODO %d][%s %d] -> nodo en espera\n", id, info->nombre, info->thread_num);
            nodo_activo = 0;
        }
    }
}

// Hilo de tipo RESERVAS/ADMINISTRACION
void *t1(void *args)
{
    struct thread_info *info = args;

    printf("[NODO %d][%s %d] -> proceso creado\n", id, info->nombre, info->thread_num);
    while (1)
    {
        struct msg_nodo msg_cliente = (const struct msg_nodo){0};
        // Recibir peticion cliente
        msgrcv(cola_msg, &msg_cliente, sizeof(msg_cliente), info->tipo, 0);

        quiere[1]++;

        int proceso_despertado;
        do
        {
            proceso_despertado = 0;
            if ((!token || !lista_vacia()) && !peticion_activa(1))
            {
                printf("[NODO %d][%s %d] -> solicitando token\n", id, info->nombre, info->thread_num);
                broadcast(1);
            }

            if (nodo_activo)
            {
                printf("[NODO %d][%s %d] -> nodo activo, esperando ser atendido\n", id, info->nombre, info->thread_num);
                cola_t1++;
                sem_wait(&cola_t1_sem);
                printf("[NODO %d][%s %d] -> proceso despertado, intentando entrar\n", id, info->nombre, info->thread_num);
            }
            else
            {
                printf("[NODO %d][%s %d] -> nodo inactivo, intentando entrar\n", id, info->nombre, info->thread_num);
                nodo_activo = 1;
            }

            if (!token)
            {
                printf("[NODO %d][%s %d] -> no hay token, realizando solicitud\n", id, info->nombre, info->thread_num);
                struct msg_nodo msg_token = (const struct msg_nodo){0};
                // Recibir token
                msgrcv(cola_msg, &msg_token, sizeof(msg_token), TOKEN, 0);
                actualizar_atendidas(msg_token.vector_atendidas);
                token = 1;
                printf("[NODO %d][%s %d] -> token recibido\n", id, info->nombre, info->thread_num);
            }
            if (quiere[0] > 0)
            {
                printf("[NODO %d][%s %d] -> proceso prioritario a la espera, dando paso\n", id, info->nombre, info->thread_num);
                proceso_despertado = 1;
                despertar_siguiente();
            }
        } while (proceso_despertado);

        sem_wait(&mutex_sc_sem);
        seccion_critica = 1;
        printf("[NODO %d][%s %d] -> seccion critica\n", id, info->nombre, info->thread_num);
        // SECCIÓN CRÍTICA
        sleep(10);
        seccion_critica = 0;
        sem_post(&mutex_sc_sem);

        quiere[1]--;
        printf("[NODO %d][%s %d] -> ha salido seccion critica\n", id, info->nombre, info->thread_num);
        if (quiere[1] == 0)
        {
            printf("[NODO %d][%s %d] -> ultimo proceso ha salido, actualizando atendidas\n", id, info->nombre, info->thread_num);
            vector_atendidas[1][id] = vector_peticiones[1][id];
        }
        int nodo_siguiente = buscar_nodo_siguiente();
        if (nodo_siguiente >= 0)
        {
            printf("[NODO %d][%s %d] -> peticion prioritaria en nodo %d, enviando token\n", id, info->nombre, info->thread_num, nodo_siguiente);
            token = 0;
            enviar_token(nodo_siguiente);
        }
        if (procesos_quieren())
        {
            if (nodo_siguiente >= 0)
            {
                hacer_peticiones();
            }
            printf("[NODO %d][%s %d] -> despertando siguiente\n", id, info->nombre, info->thread_num);
            despertar_siguiente();
        }
        else
        {
            printf("[NODO %d][%s %d] -> nodo en espera\n", id, info->nombre, info->thread_num);
            nodo_activo = 0;
        }
    }
}

// Hilo de tipo CONSULTAS
void *t2(void *args)
{

    struct thread_info *info = args;

    printf("[NODO %d][%s %d] -> proceso creado\n", id, info->nombre, info->thread_num);
    while (1)
    {
        struct msg_nodo msg_cliente = (const struct msg_nodo){0};
        // Recibir peticion cliente
        msgrcv(cola_msg, &msg_cliente, sizeof(msg_cliente), CONSULTAS, 0);

        quiere[2]++;

        int proceso_despertado;
        do
        {
            proceso_despertado = 0;
            if ((!(token || token_consulta)) && !peticion_activa(2))
            {
                printf("[NODO %d][%s %d] -> solicitando token\n", id, info->nombre, info->thread_num);
                broadcast(2);
            }

            if (nodo_activo)
            {
                if (!paso_consultas)
                {
                    printf("[NODO %d][%s %d] -> nodo activo, esperando ser atendido\n", id, info->nombre, info->thread_num);
                    cola_t2++;
                    sem_wait(&cola_t2_sem);
                }
            }
            else
            {
                printf("[NODO %d][%s %d] -> nodo inactivo, intentando entrar\n", id, info->nombre, info->thread_num);
                nodo_activo = 1;
            }

            if (!(token || token_consulta))
            {
                printf("[NODO %d][%s %d] -> no hay token, realizando solicitud\n", id, info->nombre, info->thread_num);
                struct msg_nodo msg_token = (const struct msg_nodo){0};
                // Recibir token
                msgrcv(cola_msg, &msg_token, sizeof(msg_token), TOKEN, 0);
                actualizar_atendidas(msg_token.vector_atendidas);
                if (msg_token.consulta)
                {
                    printf("[NODO %d][%s %d] -> token consulta recibido\n", id, info->nombre, info->thread_num);
                    token_consulta = 1;
                    token_consulta_origen = msg_token.id_nodo_origen;
                }
                else
                {
                    printf("[NODO %d][%s %d] -> token recibido\n", id, info->nombre, info->thread_num);
                    token = 1;
                }
            }
            if (quiere[0] > 0 || quiere[1] > 0)
            {
                printf("[NODO %d][%s %d] -> proceso prioritario a la espera, dando paso\n", id, info->nombre, info->thread_num);
                if (token_consulta)
                {
                    printf("[NODO %d][%s %d] -> devolviendo token consulta\n", id, info->nombre, info->thread_num);
                    token_consulta = 0;
                    devolver_token_consulta();
                }
                proceso_despertado = 1;
                despertar_siguiente();
            }
        } while (proceso_despertado);

        if (primera_consulta)
        {
            printf("[NODO %d][%s %d] -> primera consulta, obteniendo SC y dando paso a consultas\n", id, info->nombre, info->thread_num);
            primera_consulta = 0;
            sem_wait(&mutex_sc_sem);
            seccion_critica = 1;
            paso_consultas = 1;
            for (int i = 0; i < cola_t2; i++)
            {
                sem_post(&cola_t2_sem);
            }
        }

        consultas_sc++;
        printf("[NODO %d][%s %d] -> seccion critica\n", id, info->nombre, info->thread_num);
        // SECCIÓN CRÍTICA
        sleep(10);
        consultas_sc--;

        quiere[2]--;
        printf("[NODO %d][%s %d] -> ha salido seccion critica\n", id, info->nombre, info->thread_num);
        if (quiere[2] == 0)
        {
            printf("[NODO %d][%s %d] -> ultimo proceso ha salido, actualizando atendidas\n", id, info->nombre, info->thread_num);
            vector_atendidas[2][id] = vector_peticiones[2][id];
        }
        int nodo_siguiente = buscar_nodo_siguiente();
        if (nodo_siguiente >= 0 || quiere[0] || quiere[1])
        {
            paso_consultas = 0;
            printf("[NODO %d][%s %d] -> proceso prioritario quiere, parando consultas\n", id, info->nombre, info->thread_num, nodo_siguiente);
            if (consultas_sc == 0)
            {
                if (token_consulta)
                {
                    printf("[NODO %d][%s %d] -> ultima consulta, devolviendo token consulta\n", id, info->nombre, info->thread_num, nodo_siguiente);
                    token_consulta = 0;
                    devolver_token_consulta();
                }
                else
                {
                    if (!lista_vacia())
                    {
                        printf("[NODO %d][%s %d] -> ultima consulta, esperando devolucion\n", id, info->nombre, info->thread_num, nodo_siguiente);
                        sem_wait(&lista_vacia_sem);
                    }
                }
                printf("[NODO %d][%s %d] -> ultima consulta, saliendo de SC\n", id, info->nombre, info->thread_num, nodo_siguiente);
                seccion_critica = 0;
                sem_post(&mutex_sc_sem);
                primera_consulta = 1;

                nodo_siguiente = buscar_nodo_siguiente();
                if (token && nodo_siguiente >= 0)
                {
                    printf("[NODO %d][%s %d] -> peticion prioritaria en nodo %d, enviando token\n", id, info->nombre, info->thread_num, nodo_siguiente);
                    token = 0;
                    enviar_token(nodo_siguiente);
                }
                if (procesos_quieren())
                {
                    if (nodo_siguiente >= 0)
                    {
                        hacer_peticiones();
                    }
                    printf("[NODO %d][%s %d] -> despertando siguiente\n", id, info->nombre, info->thread_num);
                    despertar_siguiente();
                }
                else
                {
                    printf("[NODO %d][%s %d] -> nodo en espera\n", id, info->nombre, info->thread_num);
                    nodo_activo = 0;
                }
            }
        }
    }
}

// Hilo receptor de un mensaje especial para terminar con el nodo
void *kill_nodo()
{
    struct msg_nodo msg_kill = (const struct msg_nodo){0};
    // Esperamos a recibir una solicitud para terminar con el nodo
    msgrcv(cola_msg, &msg_kill, sizeof(msg_kill), KILL, 0);

    printf("\nSe ha recibido una señal para terminar con el nodo\n");
    // Matamos al nodo
    kill(getpid(), SIGTERM);

    return 0;
}

// Función receptora de mensajes de otros nodos
void receptor()
{
    while (1)
    {
        struct msg_nodo msg_peticion = (const struct msg_nodo){0};
        // Esperamos a recibir una solicitud de token
        msgrcv(cola_msg, &msg_peticion, sizeof(msg_peticion), REQUEST, 0);
        if (msg_peticion.devolucion)
        {
            printf("[NODO %d][RECEPTOR] -> devolucion token consultas de nodo %d\n", id, msg_peticion.id_nodo_origen);
            quitar_lista(msg_peticion.id_nodo_origen);
            if (lista_vacia())
            {
                printf("[NODO %d][RECEPTOR] -> lista vacia, despertando ultima consulta %d\n", id, msg_peticion.id_nodo_origen);
                sem_post(&lista_vacia_sem);
            }
        }
        else
        {
            printf("[NODO %d][RECEPTOR] -> peticion de nodo %d  prioridad %d\n", id, msg_peticion.id_nodo_origen, msg_peticion.prioridad_origen);
            // Actualizamos el vector de peticiones
            vector_peticiones[msg_peticion.prioridad_origen][msg_peticion.id_nodo_origen] = MAX(vector_peticiones[msg_peticion.prioridad_origen][msg_peticion.id_nodo_origen], msg_peticion.num_peticion_nodo_origen);
            // Pasamos el token si procede
            if (token && !seccion_critica && prioridad_superior(msg_peticion.prioridad_origen) && (vector_peticiones[msg_peticion.prioridad_origen][msg_peticion.id_nodo_origen] > vector_atendidas[msg_peticion.prioridad_origen][msg_peticion.id_nodo_origen]))
            {
                printf("[NODO %d][RECEPTOR] -> enviando token a nodo %d\n", id, msg_peticion.id_nodo_origen);
                token = 0;
                enviar_token(msg_peticion.id_nodo_origen);
            }
            else if (paso_consultas && msg_peticion.prioridad_origen == 2 && (vector_peticiones[2][msg_peticion.id_nodo_origen] > vector_atendidas[2][msg_peticion.id_nodo_origen]))
            {
                printf("[NODO %d][RECEPTOR] -> enviando token consultas a nodo %d\n", id, msg_peticion.id_nodo_origen);
                enviar_token_consulta(msg_peticion.id_nodo_origen);
            }
        }
    }
}

int main(int argc, char *argv[])
{
    if (argc != 7)
    {
        printf("Uso: %s <ID> <procesos_pagos> <procesos_anulaciones> <procesos_reservas> <procesos_administracion> <procesos_consultas>\n", argv[0]);
        return EXIT_FAILURE;
    }

    // Obtenemos el ID del nodo de los parámetros
    id = atoi(argv[1]);
    if (id < 0)
    {
        printf("El ID del nodo debe ser cero o superior\n");
        return EXIT_FAILURE;
    }

    // Obtenemos el número de hilos para cada tipo de proceso de los parámetros
    int num_hilos[5];
    for (int i = 0; i < 5; i++)
    {
        num_hilos[i] = atoi(argv[2 + i]);
        if (num_hilos[i] < 0)
        {
            printf("El número de procesos no puede ser negativo\n");
            return EXIT_FAILURE;
        }
    }

    // Damos el token al nodo con ID = 0
    if (id == 0)
    {
        token = 1;
    }

    // Inicializamos los arrays a cero
    for (int i = 0; i < 3; i++)
    {
        for (int j = 0; j < N; j++)
        {
            vector_atendidas[i][j] = 0;
            vector_peticiones[i][j] = 0;
        }
    }

    // Inicializamos los semáforos
    sem_init(&mutex_sc_sem, 0, 1);
    sem_init(&cola_t0_sem, 0, 0);
    sem_init(&cola_t1_sem, 0, 0);
    sem_init(&cola_t2_sem, 0, 0);
    sem_init(&lista_vacia_sem, 0, 0);

    // Incializamos la cola de mensajes del nodo
    cola_msg = msgget(1000 + id, 0666 | IPC_CREAT);
    if (cola_msg != -1)
    {
        msgctl(cola_msg, IPC_RMID, NULL);
        cola_msg = msgget(1000 + id, 0666 | IPC_CREAT);
    }

    // Creamos los hilos de tipo PAGOS
    pthread_t hilo_t0[num_hilos[0] + num_hilos[1]];
    for (int i = 0; i < num_hilos[0]; i++)
    {
        struct thread_info *info =  malloc(sizeof(struct thread_info));
        info->thread_num = i;
        info->tipo = PAGOS;
        strcpy(info->nombre,"PAGOS");
        pthread_create(&hilo_t0[i], NULL, t0, info);
    }

    // Creamos los hilos de tipo ANULACIONES
    for (int i = num_hilos[0]; i < (num_hilos[0] + num_hilos[1]); i++)
    {
        struct thread_info *info =  malloc(sizeof(struct thread_info));
        info->thread_num = i - num_hilos[0];
        info->tipo = ANULACIONES;
        strcpy(info->nombre,"ANULACIONES");
        pthread_create(&hilo_t0[i], NULL, t0, info);
    }

    // Creamos los hilos de tipo RESERVAS
    pthread_t hilo_t1[num_hilos[2] + num_hilos[3]];
    for (int i = 0; i < num_hilos[2]; i++)
    {
        struct thread_info *info =  malloc(sizeof(struct thread_info));
        info->thread_num = i;
        info->tipo = RESERVAS;
        strcpy(info->nombre,"RESERVAS");
        pthread_create(&hilo_t1[i], NULL, t1, info);
    }

    // Creamos los hilos de tipo ADMINISTRACION
    for (int i = num_hilos[2]; i <(num_hilos[2] + num_hilos[3]); i++)
    {
        struct thread_info *info =  malloc(sizeof(struct thread_info));
        info->thread_num = i - num_hilos[2];
        info->tipo = ADMINISTRACION;
        strcpy(info->nombre,"ADMINISTRACION");
        pthread_create(&hilo_t1[3], NULL, t1, info);
    }

    // Creamos los hilos de tipo CONSULTAS
    pthread_t hilo_t2[num_hilos[4]];
    for (int i = 0; i < num_hilos[4]; i++)
    {
        struct thread_info *info =  malloc(sizeof(struct thread_info));
        info->thread_num = i;
        info->tipo = CONSULTAS;
        strcpy(info->nombre,"CONSULTAS");
        pthread_create(&hilo_t2[4], NULL, t2, info);
    }

    // Creamos el hilo que espera un mensaje para terminar el programa
    pthread_t hilo_kill;
    pthread_create(&hilo_kill, NULL, kill_nodo, NULL);

    // Ejecutamos el receptor de mensajes (bucle infinito)
    receptor();
}