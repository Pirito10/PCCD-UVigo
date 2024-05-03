/*Crear 3 funciones de lista enlazada:
    1. Añadir id a la lista
    2. Eliminar elemento
    3. Comprobar que la lista esté vacía
    */

#include <stdio.h>
#include <stdlib.h>
#include "lista-testigo-copia.h"

int añadir_lista(int id);
int quitar_lista(int id);
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
    return 0;
}

int quitar_lista(int id){
    if(nodo_cabeza == NULL){
        return -1;
        //La lista está vacía
    }

    struct NodoLista *nodo_actual = nodo_cabeza;
    struct NodoLista *nodo_anterior = NULL;

    while(nodo_actual != NULL && nodo_actual->id != id){
        nodo_anterior = nodo_actual;
        nodo_actual = nodo_actual->sig;
    }

    if(nodo_actual == NULL){
        //El id no existe en la lista
        return -2;
    }
    if(nodo_anterior == NULL){
        nodo_cabeza = nodo_actual->sig;
    }
    else{
        nodo_anterior->sig = nodo_actual->sig;
    }
    free(nodo_actual);
    return 0;
}

int lista_vacia(){
    if(nodo_cabeza == NULL){
        return 1; //La lista está vacía
    }
    else{
        return 0; //La lista no está vacía
    }
}