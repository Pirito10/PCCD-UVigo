// Sin esta línea se rompe mi entorno, se puede suprimir en la entrega final pero la necesito para debuggear -Manu
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <sys/msg.h>
#include <time.h>
#include <unistd.h>

// Estructura de los mensajes
struct msg_nodo
{
    long mtype; // Tipo de mensaje, 11 -> pago, 12 -> anulacion, 13 -> reserva, 14 -> administracion, 15 -> consulta
};

int main(int argc, char *argv[])
{
    if (argc < 4)
    {
        printf("Uso: %s <N> <cantidad de solicitudes> <tiempo máximo entre solicitud(ms)>\n", argv[0]);
        return EXIT_FAILURE;
    }

    int N = atoi(argv[1]);
    int num_solicitudes = atoi(argv[2]);
    int max_espera = atoi(argv[3]);

    if (N <= 0)
    {
        printf("El número de nodos debe ser mayor que cero\n");
        return EXIT_FAILURE;
    }

    if (num_solicitudes <= 0)
    {
        printf("La cantidad de solicitudes debe ser mayor que cero\n");
        return EXIT_FAILURE;
    }

    if (max_espera < 0)
    {
        printf("El tiempo máximo de espera entre solicitudes debe ser mayor que cero\n");
        return EXIT_FAILURE;
    }

    srand(time(NULL)); // Generador de números aleatorios

    for (int i = 0; i < num_solicitudes; ++i)
    {
        // Seleccionamos el nodo a enviar de forma aleatoria, entre 0 y N
        int ID = rand() % N;
        // Seleccionamos el tipo de solicitud de forma aleatoria, entre 1 y 5
        int tipo_solicitud = (rand() % 5) + 1;

        // Obtenemos la cola de mensajes del nodo
        int msgid = msgget(ID, 0666 | IPC_CREAT);

        // Creamos el mensaje
        struct msg_nodo msg_nodo;
        msg_nodo.mtype = tipo_solicitud + 10; // Ver la defición del struct

        // Enviamos el mensaje
        if (msgsnd(msgid, &msg_nodo, sizeof(msg_nodo), 0) == -1)
        {
            perror("Error al enviar mensaje");
            continue;
        }

        printf("Solicitud de tipo %d enviada al nodo %d\n", tipo_solicitud, ID);

        // Generamos un tiempo de espera aleatorio, entre 0 y max_espera
        int espera = (rand() % (max_espera + 1));
        usleep(espera * 1000); // Pasamos de milisegundos a microsegundos
    }

    printf("\nSe han enviado %d solicitudes\n", num_solicitudes);
    return EXIT_SUCCESS;
}