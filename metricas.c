#include <time.h>
#include <stdio.h>

double diferencia_tiempo(clock_t inicio, clock_t fin){
    return (double)(fin - inicio) / CLOCKS_PER_SEC;
}


int escribir_tiempo(double tiempo, char tipo[]){
    
    FILE *fp;
    fp = fopen("tiempos_sync.csv", "a+");
    if (fp == NULL)
    {
        printf("Error al abrir el archivo");
    }
    fprintf(fp, "%s: %f\n", tipo, tiempo);
    fclose(fp);
}