/*Crear 3 funciones de lista enlazada:
    0. Crear lista vacía??¿
    1. Añadir id a la lista
    2. Eliminar elemento
    3. Comprobar que la lista esté vacía
    */

#include <stdio.h>
#include <stdlib.h>
#include "lista-testigo-copia.h"


int crear_lista();
int añadir_lista();
int quitar_lista();
int lista_vacia();

struct NodoLista *nodo_cabeza = NULL;   //Puntero que apunta al primer nodo de la lista


int crear_lista(){

     
}

int añadir_lista(int id){
    if (nodo_cabeza == NULL){
        struct NodoLista *nodoNuevo = (struct NodoLista *) malloc(sizeof(struct NodoLista));
        nodoNuevo->sig = NULL;
        nodoNuevo->id = id;
        nodo_cabeza = nodoNuevo;
    }
    else{
        struct NodoLista *nodoNuevo = (struct NodoLista *) malloc(sizeof(struct NodoLista));
        nodoNuevo->sig = nodo_cabeza;
        nodoNuevo->id = id;
        nodo_cabeza = nodoNuevo;
        }
}

        