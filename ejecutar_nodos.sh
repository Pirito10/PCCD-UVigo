#!/bin/bash

# Comprobar que se pasó el parámetro N
if [ -z "$1" ] || [ -z "$2" ]; then
  echo "Uso: ./ejecutar_nodo.sh <N> <TIMEOUT(s)>"
  exit 1
fi

N=$1       # Tomar el primer argumento como el valor de N
TIMEOUT=$2 # Tomar el segundo argumento como el valor de TIMEOUT

# Verificar si el valor de N y TIMEOUT es un entero mayor que cero
if ! [[ "$N" =~ ^[0-9]+$ ]] || [ "$N" -le 0 ]; then
  echo "El valor de N debe ser un entero mayor que cero"
  exit 1
fi

if ! [[ "$TIMEOUT" =~ ^[0-9]+$ ]] || [ "$TIMEOUT" -le 0 ]; then
  echo "El valor de TIMEOUT debe ser un entero mayor que cero"
  exit 1
fi

# Ejecutar el bucle usando el valor de N proporcionado
for ((i = 0; i < N; i++)); do
  echo "Iniciado nodo con ID=$i con un timeout de $TIMEOUT segundos"
  timeout $TIMEOUT ./nodo $i & # Ejecutar en segundo plano con timeout
done

# Esperar a que todas las instancias terminen (si es necesario)
wait