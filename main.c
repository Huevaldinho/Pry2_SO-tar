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
    off_t start;
    off_t end;
};

void createTar(int numFiles, const char *tarFileName, const char *fileNames[]){
    int tarFile = open(tarFileName, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (tarFile == -1) {
        perror("Error al crear el archivo empacado");
        exit(1);
    }
    //Necesito

    close(tarFile);
}
void listTar(const char *tarFileName) {
    int tarFile = open(tarFileName, O_RDONLY);
    if (tarFile == -1) {
        perror("Error al abrir el archivo empacado");
        exit(1);
    }
    struct File readFile;
    ssize_t bytesRead;
    while ((bytesRead = read(tarFile, &readFile, sizeof(struct File))) > 0) {
        printf("Archivo: %s\n", readFile.filename);
        printf("Tamaño: %ld bytes\n", (long)readFile.size);
        printf("Permisos: %o\n", readFile.mode & (S_IRWXU | S_IRWXG | S_IRWXO));
        printf("Starts: %ld\n",(long)readFile.start);
        printf("Ends: %ld\n",(long)readFile.end);
        printf("\n");
        lseek(tarFile, readFile.size, SEEK_CUR);
    }
    close(tarFile);
}
int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Uso: %s -c|-t <archivoTar> [archivos]\n", argv[0]);
        exit(1);
    }

    const char * opcion = argv[1];
    const char * archivoTar = argv[2];

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