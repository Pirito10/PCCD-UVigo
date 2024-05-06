// Sin esta línea se rompe mi entorno, se puede suprimir en la entrega final pero la necesito para debuggear -Manu
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <sys/msg.h>

// Estructura de los mensajes
struct msg_nodo
{
    long mtype;
};

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        printf("Uso: %s <N>\n", argv[0]);
        return EXIT_FAILURE;
    }

    int N = atoi(argv[1]); // Número de nodos

    if (N <= 0)
    {
        printf("El número de nodos debe ser mayor que cero\n");
        return EXIT_FAILURE;
    }

    // Recorremos todos los nodos, enviándole el mensaje a cada uno
    for (int i = 0; i < N; i++)
    {
        // Obtenemos la cola de mensajes del nodo
        int msgid = msgget(1000 + i, 0666 | IPC_CREAT);

        // Creamos el mensaje
        struct msg_nodo msg_nodo;
        msg_nodo.mtype = 3; // Tipo 3 -> mensaje KILL

        // Enviamos el mensaje
        if (msgsnd(msgid, &msg_nodo, sizeof(msg_nodo), 0) == -1)
        {
            perror("Error al enviar mensaje");
            continue;
        }

        printf("Nodo %d terminado\n\n", i);
    }
}