#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_FILENAME_LENGTH 100


struct File {
    char fileName[MAX_FILENAME_LENGTH];
    mode_t mode;
    off_t size;
    off_t start;
    off_t end;
};

int deleteFileContentFromBody(const char* tarFileName,struct File fileToBeDeleted) {
    FILE * archivo = fopen(tarFileName, "r+");
    if (archivo == NULL) {
        perror("Error al abrir el archivo");
        return 1;
    }
    fseek(archivo, 0, SEEK_END);
    printf("Tamanno de tar antes de limpiar: %ld\n",ftell(archivo));
    char buffer[1024];
    fseek(archivo, fileToBeDeleted.start, SEEK_SET); // Mueve el puntero al inicio del rango

    // Rellena el rango con caracteres nulos
    size_t tamanoRango = fileToBeDeleted.end - fileToBeDeleted.start + 1;
    memset(buffer, 0, sizeof(buffer)); // Puedes usar '\0' para caracteres nulos

    while (tamanoRango > 0) {
        size_t bytesAEscribir = tamanoRango < sizeof(buffer) ? tamanoRango : sizeof(buffer);
        fwrite(buffer, 1, bytesAEscribir, archivo);
        tamanoRango -= bytesAEscribir;
    }
    printf("\n");

    fseek(archivo, 0, SEEK_END);
    printf("Tamanno de tar despues de limpiar: %ld\n",ftell(archivo));
    fclose(archivo);
    return 0;
}

void imprimirArchivo(const char* nombreArchivo) {
    FILE* archivo = fopen(nombreArchivo, "r");

    if (archivo == NULL) {
        perror("Error al abrir el archivo");
        return;
    }

    int caracter;

    while ((caracter = fgetc(archivo)) != EOF) {
        putchar(caracter);  // Imprime el caracter en la salida estándar
    }

    fclose(archivo);
}

int main() {
    const char* nombreArchivo = "filePruebas.txt";
    off_t inicio = 5;  // Posición de inicio del rango
    off_t fin = 10;     // Posición de fin del rango
    struct File file = {"filePruebas.txt", 0, 0, inicio, fin};
    int resultado = deleteFileContentFromBody(nombreArchivo, file);

    if (resultado == 0) {
        printf("Contenido en el rango [%ld, %ld] limpiado con éxito.\n", inicio, fin);
    } else {
        printf("Error al limpiar contenido en el rango.\n");
    }

    imprimirArchivo(nombreArchivo);
    return 0;
}
