#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#define MAX_FILENAME_LENGTH 100
#define MAX_FILES 100

struct Header header;


struct File {
    char fileName[MAX_FILENAME_LENGTH];
    mode_t mode;
    off_t size;
    off_t start;
    off_t end;
};
struct Header{
    struct File * fileList[MAX_FILES];
} header;

off_t currentPosition = 0; // Lleva un seguimiento de la posición actual en el archivo TAR


/*
    Funcion para abrir o crear un archivo.
    fileName es el nombre del archivo que se va a abrir o crear.
    Si el archivo no existe, se crea y se retorna el file descriptor.
    Si el archivo existe, se abre y se retorna el file descriptor.
*/
int openFile(const char * fileName){
    int fd = open(fileName, O_RDWR  | O_CREAT, 0666);
    if (fd == -1) {
        perror("Error al crear el archivo empacado");
        exit(1);
    }
    return fd;
}



/*
    Funcion para guardar un archivo en el header.
    Por ahora solo se va a usar en createHeader
    CUIDADO: Solo guarda en posiciones NULL del array de Files.
*/
void addFileToHeaderFileList(struct File newFile) {
    for (int i = 0; i < MAX_FILES; i++) {
        if (header.fileList[i] == NULL) {
            header.fileList[i] = malloc(sizeof(struct File));
            if (header.fileList[i] == NULL) {
                perror("Error al asignar memoria para el nuevo archivo en el header");
                exit(1);
            }
            // Copiar los datos del nuevo archivo en la posición libre del header
            strcpy(header.fileList[i]->fileName, newFile.fileName);
            header.fileList[i]->mode = newFile.mode;
            header.fileList[i]->size = newFile.size;
            header.fileList[i]->start = newFile.start;
            header.fileList[i]->end = newFile.end;
            printf("Archivo: %s \t Size: %ld \t Start: %ld \t End: %ld\n", header.fileList[i]->fileName, header.fileList[i]->size, header.fileList[i]->start, header.fileList[i]->end);
            break;
        }
    }
}

/*  
    Funcion para escribir el header en el tar.
    tarFile es el numero del archivo tar.
*/
void writeHeaderToTar(int tarFile) {
    char headerBlock[sizeof(header)];//Donde se guarda el contenido del header para escribirlo en el tar
    memset(headerBlock, 0, sizeof(headerBlock));  // Inicializa el bloque con bytes nulos
    memcpy(headerBlock, &header, sizeof(header));// Copia el contenido de la estructura header en el headerBlock
    if (write(tarFile, headerBlock, sizeof(headerBlock)) != sizeof(headerBlock)){// Escribe el headerBlock en el archivo TAR
        perror("Error al escribir el Header en el archivo TAR");
        exit(1);
    }else
        printf("Se escribio el Header correctamente\n");
}
/*  
    Funcion para leer el Header del tar file.
    tarFileName es el nombre del tar file.
*/
void readHeaderFromTar(int tarFile){
    if (lseek(tarFile, 0, SEEK_SET) == -1) {
        perror("Error al posicionar el puntero al principio del archivo");
        close(tarFile);
        exit(1);
    }
    
    char headerBlock[sizeof(header)]; // Debe ser lo suficientemente grande para contener el encabezado
    ssize_t bytesRead = read(tarFile, headerBlock, sizeof(headerBlock));
    printf("Bytes read: %ld\n", bytesRead);

    if (bytesRead < 0) {
        perror("Error al leer el Header del archivo TAR");
        exit(1);
    } else if (bytesRead < sizeof(header)) {
        fprintf(stderr, "No se pudo leer todo el Header del archivo TAR\n");
        exit(1);
    }

    // Copia el contenido del bloque en la estructura 'header'
    memcpy(&header, headerBlock, sizeof(header));
    printf("\nLeyendo Header...\n");

    for (int i = 0; i < MAX_FILES; i++) {
        if (header.fileList[i] != NULL) {
            printf("Archivo: %s \t Size: %ld \t Start: %ld \t End: %ld\n", header.fileList[i]->fileName, header.fileList[i]->size, header.fileList[i]->start, header.fileList[i]->end);
        }
    }
}


/*
    Funcion para crear el encabezado del tar.
    numFiles es la cantidad de archivos que se van a empacar.
    tarFileName es el nombre del archivo tar que se creara.
    fileNames es un arreglo con los nombres de los archivos que se van a empacar
*/
void createHeader(int numFiles, const char *tarFileName, const char *fileNames[]){//!TODO
    int tarFile = openFile(tarFileName);//Crear tar vacio
    const char * fileName;
    struct stat fileStat;
    struct File newFile;
    long sumFileSizes = 0;
    for (int i=0; i < numFiles; i++){
        fileName = fileNames[i];
        if (lstat(fileName, &fileStat) == -1) {//Extrae la info del archivo de esta iteracion y la guarda en fileStat
            perror("Error al obtener información del archivo");
            close(tarFile);
            exit(1);
        }
        strncpy(newFile.fileName, fileName, MAX_FILENAME_LENGTH);//nombre del archivo se copia a la estructura
        newFile.size = fileStat.st_size;
        newFile.mode = fileStat.st_mode;
        sumFileSizes += fileStat.st_size;
        if (currentPosition==0)//Primer archivo
            newFile.start = currentPosition = sizeof(header)+1;
        else 
            newFile.start = currentPosition + 1;
        newFile.end = currentPosition + fileStat.st_size;
        currentPosition = newFile.end;
        addFileToHeaderFileList(newFile);
    }
    writeHeaderToTar(tarFile);
    readHeaderFromTar(tarFile);
    close(tarFile);
}
void createBody(){//!TODO
    //Ver info del header para poder graber cuerpo

}
/*
    Funcion que crea un archivo tar con los archivos especificados
    en fileNames. El nombre del archivo tar es tarFileName.
    numFiles es la cantidad de archivos que se van a empacar.
*/
void createStar(int numFiles, const char *tarFileName, const char *fileNames[]) {
    int tarFile = openFile(tarFileName);//Crear tar vacio
    //Se creo el .tar correctamente
    createHeader(numFiles,tarFileName,fileNames);//Crear header con los datos de los archivos
    createBody();//Guardar el contenido de los Files segun las posiciones de 
    
    //Para ver el tamaño del tar creado
    struct stat st;
    if (fstat(tarFile, &st) == 0) {
        printf("El tamaño del archivo es %lld bytes.\n", (long long)st.st_size);
    } else {
        perror("Error al obtener el tamaño del archivo");
    }
}


void listStar(const char * tarFileName) {
    int tarFile = openFile(tarFileName);
    readHeaderFromTar(tarFile);
    close(tarFile);
}
int main(int argc, char *argv[]) {//!Modificar forma de usar las opciones
//Debe poder usarse combinacion de opciones
    if (argc < 3) {
        fprintf(stderr, "Uso: %s -c|-t <archivoTar> [archivos]\n", argv[0]);
        exit(1);
    }

    const char * opcion = argv[1];
    const char * archivoTar = argv[2];

    listStar(archivoTar);

    if (strcmp(opcion, "-c") == 0) {
        if (argc < 4) {
            fprintf(stderr, "Uso: %s -c <archivoTar> [archivos]\n", argv[0]);
            exit(1);
        }
        int numFiles = argc - 3;
        const char * fileNames[MAX_FILES];
        for (int i = 0; i < numFiles; i++) {
            fileNames[i] = argv[i + 3];
        }
        createStar(numFiles, archivoTar, fileNames);

    } else if (strcmp(opcion, "-t") == 0) {
        listStar(archivoTar);

    } else {
        fprintf(stderr, "Uso: %s -c|-t <archivoTar> [archivos]\n", argv[0]);
        exit(1);
    }

    

    return 0;
}