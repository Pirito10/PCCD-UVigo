// Sin esta línea se rompe mi entorno, se puede suprimir en la entrega final pero la necesito para debuggear -Manu
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <sys/msg.h>

// Estructura de los mensajes
struct msg_nodo
{
    long mtype; // Tipo de mensaje, 11 -> pago, 12 -> anulacion, 13 -> reserva, 14 -> administracion, 15 -> consulta
};

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        printf("Uso: %s <N>\n", argv[0]);
        return EXIT_FAILURE;
    }

    // Tipo de consulta e ID del nodo receptor
    int tipo_solicitud, ID;
    int N = atoi(argv[1]);

    if (N <= 0)
    {
        printf("El número de nodos debe ser mayor que cero\n");
        return EXIT_FAILURE;
    }

    // Bucle infinito
    while (1)
    {
        // Tomamos el ID
        printf("Introduce el ID del nodo al que quieres enviar la solicitud (0-%d), o -1 para salir: ", N);
        scanf("%d", &ID);

        if (ID == -1)
        {
            return EXIT_SUCCESS;
        }

        if (ID > N || ID < -1)
        {
            printf("Has seleccionado un nodo que no existe\n\n");
            continue;
        }

        // Tomamos el tipo de solicitud
        printf("Selecciona el tipo de solicitud:\n- Pago: 1\n- Anulación: 2\n- Reserva: 3\n- Administración: 4\n- Consulta: 5\n");
        scanf("%d", &tipo_solicitud);

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

        printf("Solicitud enviada\n\n");
    }
}