#!/bin/bash

# Comprobar que se pasó el parámetro N
if [ -z "$1" ] || [ -z "$2" ]; then
  echo "Uso: ./ejecutar_nodo.sh <N> <procesos_pagos> <procesos_anulaciones> <procesos_reservas> <procesos_administracion> <procesos_consultas> <TIMEOUT(s)>"
  exit 1
fi

N=$1                        # Tomar el primer argumento como el valor de N
PROCESOS_PAGOS=$2           # Tomar el segundo argumento como el valor de PROCESOS_PAGOS
PROCESOS_ANULACIONES=$3     # Tomar el tercer argumento como el valor de PROCESOS_ANULACIONES
PROCESOS_RESERVAS=$4        # Tomar el cuarto argumento como el valor de PROCESOS_RESERVAS
PROCESOS_ADMINISTRACION=$5  # Tomar el quinto argumento como el valor de PROCESOS_ADMINISTRACION
PROCESOS_CONSULTAS=$6       # Tomar el sexto argumento como el valor de PROCESOS_CONSULTAS
TIMEOUT=$7                  # Tomar el séptimo argumento como el valor de TIMEOUT

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
  # Ejecutar en segundo plano con timeout
  timeout $TIMEOUT ./src/nodo $i $PROCESOS_PAGOS $PROCESOS_ANULACIONES $PROCESOS_RESERVAS $PROCESOS_ADMINISTRACION $PROCESOS_CONSULTAS &
done

echo ""

# Esperar a que todas las instancias terminen (si es necesario)
wait