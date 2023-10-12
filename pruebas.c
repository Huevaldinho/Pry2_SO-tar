#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#define MAX_FILENAME_LENGTH 100
#define MAX_FILES 100

struct File {
    char filename[MAX_FILENAME_LENGTH];
    mode_t mode;
    off_t size;
    off_t start_pos; // Puntero para la posición de inicio
    off_t end_pos;   // Puntero para la posición de fin
};

void createTar(int numFiles, const char *tarFileName, const char *fileNames[]) {
    int tarFile = open(tarFileName, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (tarFile == -1) {
        perror("Error al crear el archivo empacado");
        exit(1);
    }

    off_t currentPosition = 0; // Lleva un seguimiento de la posición actual en el archivo TAR

    for (int i = 0; i < numFiles; i++) {
        const char *fileName = fileNames[i];
        struct stat fileStat;

        if (lstat(fileName, &fileStat) == -1) {
            perror("Error al obtener información del archivo");
            close(tarFile);
            exit(1);
        }

        struct File header;
        strncpy(header.filename, fileName, MAX_FILENAME_LENGTH);
        header.mode = fileStat.st_mode;
        header.size = fileStat.st_size;
        header.start_pos = currentPosition; // Registra la posición de inicio

        if (write(tarFile, &header, sizeof(struct File)) == -1) {
            perror("Error al escribir la cabecera del archivo");
            close(tarFile);
            exit(1);
        }

        int file = open(fileName, O_RDONLY);
        if (file == -1) {
            perror("Error al abrir el archivo");
            close(tarFile);
            exit(1);
        }

        char buffer[MAX_FILENAME_LENGTH];
        ssize_t bytesRead;
        while ((bytesRead = read(file, buffer, sizeof(buffer))) > 0) {
            if (write(tarFile, buffer, bytesRead) == -1) {
                perror("Error al escribir el archivo");
                close(file);
                close(tarFile);
                exit(1);
            }
        }

        header.end_pos = currentPosition + header.size; // Registra la posición de fin
        if (write(tarFile, &header, sizeof(struct File)) == -1) {
            perror("Error al escribir la cabecera del archivo");
            close(tarFile);
            exit(1);
        }

        currentPosition = lseek(tarFile, 0, SEEK_CUR); // Actualiza la posición actual
        

        close(file);
    }

    close(tarFile);
}

void listTar(const char *tarFileName) {
    int tarFile = open(tarFileName, O_RDONLY);
    if (tarFile == -1) {
        perror("Error al abrir el archivo empacado");
        exit(1);
    }

    struct File header;
    ssize_t bytesRead;

    while ((bytesRead = read(tarFile, &header, sizeof(struct File))) > 0) {
        printf("Archivo: %s\n", header.filename);
        printf("Tamaño: %ld bytes\n", (long)header.size);
        printf("Permisos: %o\n", header.mode & (S_IRWXU | S_IRWXG | S_IRWXO));
        printf("Posición de inicio: %ld\n", (long)header.start_pos);
        printf("Posición de fin: %ld\n", (long)header.end_pos);
        printf("\n");
        lseek(tarFile, header.size, SEEK_CUR);
    }

    close(tarFile);
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Uso: %s -c|-t <archivoTar> [archivos]\n", argv[0]);
        exit(1);
    }

    const char *opcion = argv[1];
    const char *archivoTar = argv[2];

    if (strcmp(opcion, "-c") == 0) {
        if (argc < 4) {
            fprintf(stderr, "Uso: %s -c <archivoTar> [archivos]\n", argv[0]);
            exit(1);
        }
        int numFiles = argc - 3;
        const char *fileNames[MAX_FILES];
        for (int i = 0; i < numFiles; i++) {
            fileNames[i] = argv[i + 3];
        }
        createTar(numFiles, archivoTar, fileNames);
    } else if (strcmp(opcion, "-t") == 0) {
        listTar(archivoTar);
    } else {
        fprintf(stderr, "Uso: %s -c|-t <archivoTar> [archivos]\n", argv[0]);
        exit(1);
    }

    return 0;
}
