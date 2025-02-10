# Comando de compilación
CC = gcc

# Opciones de compilación, -Wall para mostrar los warnings
CFLAGS = -Wall

# Nombre de los ejecutables
EXEC_NODO = nodo
EXEC_CLIENTE = cliente
EXEC_CLIENTE_RAND = cliente_rand
EXEC_KILL = kill

# Archivos fuente
SRC_UTILS = utils.c
SRC_NODO = nodo.c $(SRC_UTILS)
SRC_CLIENTE = cliente.c
SRC_CLIENTE_RAND = cliente_rand.c
SRC_KILL = kill.c

# Regla por defecto (lo que se ejecuta si solo se llama 'make')
all: $(EXEC_NODO) $(EXEC_CLIENTE) $(EXEC_CLIENTE_RAND) $(EXEC_KILL)

# Reglas para compilar cada ejecutable
$(EXEC_NODO): $(SRC_NODO)
	$(CC) $(CFLAGS) -o $(EXEC_NODO) $(SRC_NODO)

$(EXEC_CLIENTE): $(SRC_CLIENTE)
	$(CC) $(CFLAGS) -o $(EXEC_CLIENTE) $(SRC_CLIENTE)

$(EXEC_CLIENTE_RAND): $(SRC_CLIENTE_RAND)
	$(CC) $(CFLAGS) -o $(EXEC_CLIENTE_RAND) $(SRC_CLIENTE_RAND)

$(EXEC_KILL): $(SRC_KILL)
	$(CC) $(CFLAGS) -o $(EXEC_KILL) $(SRC_KILL)

# Regla para limpiar archivos ejecutables
clean:
	rm -f $(EXEC_NODO) $(EXEC_CLIENTE) $(EXEC_CLIENTE_RAND) $(EXEC_KILL)

# Regla para reconstruir todo desde cero
rebuild: clean all