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
    struct File fileList[MAX_FILES];
};

off_t currentPosition = 0; // Lleva un seguimiento de la posición actual en el archivo TAR


/*
    Funcion para abrir o crear un archivo.
    fileName es el nombre del archivo que se va a abrir o crear.
    opcion es 0 para abrir y 1 para crear.
    si es 1 se crea el archivo vacio.
    si es 0 se abre el archivo en modo lectura y escritura sin borrar su contenido.
*/
int openFile(const char * fileName,int opcion){
    int fd;
    if (opcion==1){
        fd = open(fileName, O_WRONLY | O_CREAT | O_TRUNC, 0666);
        if (fd == -1) {
            perror("Error al crear el archivo empacado");
            exit(1);
        }
        return fd;
    }else if (opcion==0){
        fd = open(fileName, O_RDWR  | O_CREAT, 0666);
        if (fd == -1) {
            perror("Error al crear el archivo empacado");
            exit(1);
        }
        return fd;
    }else{
        printf("Opcion de creacion de archivo incorrecta...\n");
        return -1;
    }
}



/*
    Funcion para guardar un archivo en el header.
    Por ahora solo se va a usar en createHeader
    CUIDADO: Solo guarda en posiciones NULL del array de Files. Revisa de principio a fin.
*/
void addFileToHeaderFileList(struct File newFile) {
    for (int i = 0; i < MAX_FILES; i++) {
        if (header.fileList[i].fileName[0] == '\0') {//Posicion vacia
            header.fileList[i]= newFile;
            break;
        }
    }
}

/*  
    Funcion para leer el Header del tar file.
    tarFile es el identificador del archivo tar de donde se debe leer el Header.
    retorna 1 si se leyo correctamente.
*/
int readHeaderFromTar(int tarFile){
    if (lseek(tarFile, 0, SEEK_SET) == -1) {
        perror("readHeaderFromTar: Error al posicionar el puntero al principio del archivo");
        close(tarFile);
        exit(1);
    }
    
    char headerBlock[sizeof(header)]; // Debe ser lo suficientemente grande para contener el encabezado
    ssize_t bytesRead = read(tarFile, headerBlock, sizeof(headerBlock));

    if (bytesRead < 0) {
        perror("readHeaderFromTar: Error al leer el Header del archivo TAR");
        exit(1);
    } else if (bytesRead < sizeof(header)) {
        fprintf(stderr, "readHeaderFromTar: No se pudo leer todo el Header del archivo TAR\n");
        exit(1);
    }
    // Copia el contenido del bloque en la estructura 'header'
    memcpy(&header, headerBlock, sizeof(header));
    for (int i = 0; i < MAX_FILES; i++) {
        if (header.fileList[i].size !=  0) {
            printf("Archivo: %s \t Size: %ld \t Start: %ld \t End: %ld\n", header.fileList[i].fileName, header.fileList[i].size, header.fileList[i].start, header.fileList[i].end);
        }
    }
    return 1;
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
        perror("writeHeaderToTar: Error al escribir el Header en el archivo TAR");
        exit(1);
    }else
        printf("writeHeaderToTar: Se escribio el Header correctamente\n");
}
/*
    Funcion para escribir el contenido de los archivos
    que se encuentra en el header del tar.
    tarFile es el numero del archivo.
*/
void writeBodyToTar(int tarFile){//!Falta probar que se haya escrito bien el contenido en el body.
    for (int i = 0; i < MAX_FILES; i++) {
        if (header.fileList[i].size !=  0) {//Hay un file en esa posicion del header.
            // Mueve el puntero de archivo a 100 bytes desde el principio del archivo.
            if (lseek(tarFile, header.fileList[i].start, SEEK_SET) == 0){//Posicionar puntero en lugar de escritura
                // Escribe el contenido del archivo en el archivo TAR
                char buffer[header.fileList[i].size];//Buffer para guardar el contenido del archivo a guardar
                read(tarFile,buffer, sizeof(buffer));//Lee el contenido del archivo y lo guarda en el buffer
                if (write(tarFile, buffer, sizeof(buffer)) == -1) {
                    perror("writeBodyToTar: Error al escribir en el archivo");
                    close(tarFile);
                    exit(1);
                }
            }
        }
    }
}

/*
    Funcion para crear el encabezado del tar.
    numFiles es la cantidad de archivos que se van a empacar.
    tarFileName es el nombre del archivo tar que se creara.
    fileNames es un arreglo con los nombres de los archivos que se van a empacar
*/
void createHeader(int numFiles,int tarFile, const char *fileNames[]){
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
    close(tarFile);//Necesito cerrar el archivo para poder volver a abrirlo en el modo lectura.
}
/*
    Funcion encargada de escribir el cuerpo del tar.
*/
void createBody(int tarFile){//!TODO
    //Ver info del header para poder graber cuerpo
    if (readHeaderFromTar(tarFile)==1)
        writeBodyToTar(tarFile);
    else{
        printf("createBody: Error al leer el header del tar file\n");
        exit(10);
    }
    close(tarFile);
}
/*
    Funcion que crea un archivo tar con los archivos especificados
    en fileNames. El nombre del archivo tar es tarFileName.
    numFiles es la cantidad de archivos que se van a empacar.
*/
void createStar(int numFiles, const char *tarFileName, const char *fileNames[]) {
    printf("\n \t CREATE TAR FILE \n");
    createHeader(numFiles,openFile(tarFileName,1),fileNames);//Crear header con los datos de los archivos
    createBody(openFile(tarFileName,0));//Guardar el contenido de los Files segun las posiciones de 

    struct stat st;//Para ver el tamaño del tar creado
    if (fstat(openFile(tarFileName,0), &st) == 0) {
        printf("createStar: El tamaño del archivo tar es %lld bytes.\n", (long long)st.st_size);
    } else {
        perror("createStar: Error al obtener el tamaño del archivo");
    }
}

/*
    Funcion para listar el archivo tar.
*/
void listStar(const char * tarFileName) {
    int tarFile = openFile(tarFileName,0);
    if (tarFile == -1) {
        fprintf(stderr, "Error al abrir el archivo TAR.\n");
        exit(1);
    }
    if (header.fileList == NULL) {
        fprintf(stderr, "La lista de archivos en el encabezado está vacía o no inicializada.\n");
        close(tarFile);
        exit(1);
    }
    printf("\n \t LIST TAR FILES \n");

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

    printf("Header size: %ld bytes\n", sizeof(header));


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

    
    printf("\n PROGRAMA TERMINA EXITOSAMENTE...\n");
    return 0;
}