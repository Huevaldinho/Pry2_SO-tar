#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <tar.h>


#define MAX_FILENAME_LENGTH 100
#define MAX_FILES 100

struct Header header;


struct File {
    char filename[MAX_FILENAME_LENGTH];
    mode_t mode;
    off_t size;
    off_t start;
    off_t end;
};
struct Header{
    struct File * fileList[MAX_FILES];
};
int createTarFile( const char *tarFileName){
    int tarFile = open(tarFileName, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (tarFile == -1) {
        perror("Error al crear el archivo empacado");
        exit(1);
    }
    return tarFile;
}
void createHeader(int numFiles, const char *tarFileName, const char *fileNames[]){
    //Meter una entrada del struct File x archivo en la lista de archivos de Header
    for (int i=0;i<numFiles;i++) {
        //Guardar File en la lista de Files del header
        struct File newFile;//Este estruct se guarda en header.files

    }
}
void createBody(){

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
int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Uso: %s -c|-t <archivoTar> [archivos]\n", argv[0]);
        exit(1);
    }

    const char * opcion = argv[1];
    const char * archivoTar = argv[2];
    

    return 0;
}