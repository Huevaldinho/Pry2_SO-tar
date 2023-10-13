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
};

off_t currentPosition = 0; // Lleva un seguimiento de la posición actual en el archivo TAR



/*
    Funcion para crear archivo tar vacio.
    tarFileName es el nombre del archivo tar que se creara.
    retorna numero del archivo tar.
*/
int createTarFile( const char *tarFileName){
    int tarFile = open(tarFileName, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (tarFile == -1) {
        perror("Error al crear el archivo empacado");
        exit(1);
    }
    return tarFile;
}
/*
    Funcion para crear el encabezado del tar.
    numFiles es la cantidad de archivos que se van a empacar.
    tarFileName es el nombre del archivo tar que se creara.
    fileNames es un arreglo con los nombres de los archivos que se van a empacar
*/
void createHeader(int numFiles, const char *tarFileName, const char *fileNames[]){//!TODO
    //Abre el archivo tar
    int tarFile = open(tarFileName, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (tarFile == -1) {
        perror("Error al crear el archivo empacado");
        exit(1);
    }
    //Meter una entrada del struct File x archivo en la lista de archivos de Header
    const char * fileName;
    for (int i=0;i<numFiles;i++) {
        fileName = fileNames[i];
        struct stat fileStat;
        if (lstat(fileName, &fileStat) == -1) {//Extrae la info del archivo de esta iteracion y la guarda en fileStat
            perror("Error al obtener información del archivo");
            close(tarFile);
            exit(1);
        }
        struct File newFile;
        strncpy(newFile.fileName, fileName, MAX_FILENAME_LENGTH);//nombre del archivo se copia a la estructura
        newFile.size = fileStat.st_size;
        newFile.mode = fileStat.st_mode;
        newFile.start = currentPosition;
        newFile.end = currentPosition + fileStat.st_size;
        currentPosition = newFile.end + 1;
        printf("Archivo %s Size: %ld Start: %ld End: %ld \n",newFile.fileName,newFile.size,newFile.start,newFile.end);


    }
    close(tarFile);
}
void createBody(){//!TODO

}
/*
    Funcion que crea un archivo tar con los archivos especificados
    en fileNames. El nombre del archivo tar es tarFileName.
    numFiles es la cantidad de archivos que se van a empacar.
*/
void createStar(int numFiles, const char *tarFileName, const char *fileNames[]) {
    int tarFile = createTarFile(tarFileName);//Crear tar vacio

    if (tarFile == -1) {
        perror("Error al crear el archivo empacado");
        exit(1);
    }
    //Se creo el .tar correctamente
    createHeader(numFiles,tarFileName,fileNames);//Crear header con los datos de los archivos
    createBody();//Guardar el contenido de los Files segun las posiciones de 
    //la lista de files del header
}


void listStar(const char * tarFileName) {
    
}
int main(int argc, char *argv[]) {//!Modificar forma de usar las opciones
//Debe poder usarse combinacion de opciones
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