#ifndef UTILS_H

#define N 3 // Número de nodos

#define TOKEN 1           // Mensajes de envío de testigo
#define REQUEST 2         // Mensajes de solicitud de testigo
#define KILL 3            // Mensajes de solicitud para terminar con el nodo
#define PAGOS 11          // Mensajes de solicitud de pagos
#define ANULACIONES 12    // Mensajes de solicitud de anulaciones
#define RESERVAS 13       // Mensajes de solicitud de reservas
#define ADMINISTRACION 14 // Mensajes de solicitud de administración
#define CONSULTAS 15      // Mensajes de solicitud de consultas

#define MAX(a, b) (((a) > (b)) ? (a) : (b)) // Cálculo de valor máximo entre dos variables

// Estructura de los mensajes
struct msg_nodo
{
    long mtype;                   // Tipo de mensaje, 1 -> token, 2 -> peticion nodo, 3 -> peticion cliente
    int id_nodo_origen;           // ID del nodo origen
    int num_peticion_nodo_origen; // Número de petición del nodo origen
    int prioridad_origen;         // Prioridad de la solicitud
    int consulta;                 // Si 1 -> token de consulta
    int devolucion;               // Si 1 -> es el retorno de un token de consulta
    int vector_atendidas[3][N];   // Vector atendidas
};

// Estructura de los elementos de la lista enlazada de IDs
struct NodoLista
{
    int id;
    struct NodoLista *sig;
};

// Declaraciones de las funciones
void enviar_token(int id_nodo);
void broadcast(int prioridad);
void actualizar_atendidas(int vector_atendidas_nuevo[3][N]);
int buscar_nodo_siguiente();
int peticion_activa(int prioridad);
int prioridad_superior(int prioridad);
void anadir_lista(int id);
void quitar_lista(int id);
int lista_vacia();

#endif