typedef struct NodoLista{
    int id;
    struct NodoLista *sig;
};

int añadir_lista(int id);

int quitar_lista(int id);

int lista_vacia();