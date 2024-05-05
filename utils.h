#ifndef UTILS_H
#define UTILS_H

#define N 3 // Número de nodos

// Estructura de los mensajes
struct msg_nodo
{
    long mtype;                   // Tipo de mensaje, 1 -> token, 2 -> peticion nodo, 3 -> peticion cliente
    int id_nodo_origen;           // ID del nodo origen
    int num_peticion_nodo_origen; // Número de petición del nodo origen
    int prioridad_origen;         // Prioridad de la solicitud
    int vector_atendidas[3][N];   // Vector atendidas
};

// Declaraciones de las funciones
void enviar_token(int id_nodo);
void broadcast(int prioridad);
void actualizar_atendidas(int vector_atendidas_nuevo[3][N]);
int buscar_nodo_siguiente();
int peticion_activa(int prioridad);
int prioridad_superior(int prioridad);

#endif