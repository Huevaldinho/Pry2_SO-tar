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
    char fileName[MAX_FILENAME_LENGTH];
    mode_t mode;
    off_t size;//0: No hay archivo, >0: Si hay archivo
    off_t start;
    off_t end;
    int deleted;//0: No, 1: Si
};

struct Header{
    struct File fileList[MAX_FILES];
} header;


struct BlankSpace{
    off_t start;
    off_t end;
    int index;
    struct BlankSpace * nextBlankSpace;
} * firstBlankSpace;



off_t currentPosition = 0; // Lleva un seguimiento de la posición actual en el archivo TAR


int counterFiles(const char* files[]) {
    int contador = 0;
    // Recorre el arreglo hasta encontrar un puntero nulo (NULL)
    while (files[contador] != NULL) {
        contador++;
    }
    return contador;
}


/*
    Funcion para abrir o crear un archivo.
    fileName es el nombre del archivo que se va a abrir o crear.
    opcion es 0 para abrir y 1 para crear.
    si es 1 se crea el archivo vacio.
    si es 0 se abre el archivo en modo lectura y escritura sin borrar su contenido.
*/
int openFile(const char * fileName,int opcion){
    int fd;
    if (opcion==0){//Lectura y escritura sin borrar contenido
        fd = open(fileName, O_RDWR  | O_CREAT, 0666);
        if (fd == -1) {
            perror("Error al crear el archivo empacado");
            exit(1);
        }
        //printf("Se abre el archivo en opcion: 0 -> O_RDWR  | O_CREAT\n");
        return fd;
    }else if (opcion==1){//Crear archivo vacio
        fd = open(fileName, O_WRONLY | O_CREAT | O_TRUNC, 0666);
        if (fd == -1) {
            perror("Error al crear el archivo empacado");
            exit(1);
        }
        //printf("Se abre el archivo en opcion: 1 -> O_WRONLY | O_CREAT | O_TRUNC\n");
        return fd;
    }else if (opcion==2){//Extender el archivo (para el body)
        fd = open(fileName, O_WRONLY | O_APPEND, 0666);
        if (fd == -1) {
            perror("Error al crear el archivo empacado");
            exit(1);
        }
        //printf("Se abre el archivo en opcion: 2 -> O_WRONLY | O_APPEND\n");
        return fd;
    }else{
        printf("Opcion de creacion de archivo incorrecta...\n");
        return -1;
    }
}

off_t getFileSize(const char * fileName){
    int tarFile = openFile(fileName,0);
    off_t sizeOfTar = lseek(tarFile, 0, SEEK_END);// Obtiene el tamaño del archivo tar.
    if (sizeOfTar == -1) {
        perror("getFileSize: Error al obtener el tamaño del archivo tar.");
        close(tarFile);
        exit(1);
    }
    if (sizeOfTar == 0 ){
        printf("getFileSize: El archivo tar esta vacio\n");
        close(tarFile);
        exit(2);
    }
    close(tarFile);
    return sizeOfTar;
}


/*
    Funcion para guardar un archivo en el header.
    Por ahora solo se va a usar en createHeader
    CUIDADO: Solo guarda en posiciones NULL del array de Files. Revisa de principio a fin.
*/
void addFileToHeaderFileList(struct File newFile) {
    printf("ADD FILE TO HEADER FILE LIST\n");

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
    NO cierra el archivo tar.
*/
int readHeaderFromTar(int tarFile){
    //printf("READ HEADER FROM TAR \n");

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
    return 1;
}

/*
    Funcion para encontrar la posicion de un archivo en el tar.
    Retorna NULL si no encuentra el archivo en el tar.
    Retorna el archivo encontrado la lista de archivos del header.
*/
struct File findFile(const char * tarFileName,const char * fileName){
    int tarFile = openFile(tarFileName,0);
    if (readHeaderFromTar(tarFile)==1){
        for (int i = 0; i < MAX_FILES; i++) {
            if (strcmp(header.fileList[i].fileName,fileName)==0){//Encontro el archivo
                close(tarFile);
                return header.fileList[i];
            }
        }
    }else{
        printf("findFile: Error 1 al leer el header del tar file\n");
        close(tarFile);
        exit(10);
    }
    close(tarFile);
    printf("findFile: Error 2 al leer el header del tar file\n");
    exit(11);
}

/*
    Funcion para encontrar la posicion de un archivo en el header del tar.
    Retorna -1 si no encuentra el archivo en el header del tar..
    Retorna el indice del archivo encontrado en la lista de archivos del header.
*/
int findIndexFile(const char * tarFileName,const char * fileName){
    int tarFile = openFile(tarFileName,0);
    if (readHeaderFromTar(tarFile)==1){
        for (int i = 0; i < MAX_FILES; i++) {
            if (strcmp(header.fileList[i].fileName,fileName)==0){//Encontro el archivo
                close(tarFile);
                return i;
            }
        }
    }else{
        printf("findFile: Error 1 al leer el header del tar file\n");
        close(tarFile);
        exit(10);
    }
    close(tarFile);
    printf("findFile: Error 2 al leer el header del tar file\n");
    exit(11);
}

/*
    Funcion para imprimir el header.
*/
void printHeader(){
    printf("HEADER: \n");
    for (int i = 0; i < MAX_FILES; i++) {
        if (header.fileList[i].size !=  0) {
            printf("\tArchivo: %s \t Size: %ld \t Start: %ld \t End: %ld\n", header.fileList[i].fileName, header.fileList[i].size, header.fileList[i].start, header.fileList[i].end);
        }
    }
}

/*
    Funcion para obtener el primer archivo en la lista del header.
    Retorna el primer archivo en la lista del header.
    Si no hay elementos en la lista retorna estructura vacia.
*/
struct File findFirstFileInHeader(){
    for (int i = 0; i < MAX_FILES; i++) 
        if (header.fileList[i].size !=  0) 
            return header.fileList[i];
    return header.fileList[0];//Estructura vacia. No hay elementos en la lista.
}
/*
    Funcion para obtener el ultimo archivo de la lista del header.
    Retorna el ultimo archivo de la lista del header.
    Si no hay elementos en la lista retorna estructura vacia.
*/
struct File findLastFileInHeader(){
    for (int i = MAX_FILES-1; i >= 0; i--) 
        if (header.fileList[i].size !=  0) 
            return header.fileList[i];
    return header.fileList[0];//Estructura vacia. No hay elementos en la lista.
}
/*
    Funcion para saber si la lista de archivos del header esta vacia.
    Retorna 0 si no esta vacia.
    Retorna 1 si si esta vacia.
*/
int isHeaderFileListEmpty(){
    for (int i = 0; i < MAX_FILES; i++) 
        if (header.fileList[i].size !=  0) 
            return 0;
    return 1;
}

/*
    Funcion para encontrar el archivo que sigue despues de file.
    Retorna File con el archivo que sigue despues de file del parametro.
*/
struct File findNextFile(const char * file){
    int encontreFile = 0;//Para saber cuando ya pase por file.
    for (int i = 0; i < MAX_FILES; i++){//Itero hasta encontrar el archivo file.
        if (strcmp(header.fileList[i].fileName,file)==0){//Encontro el archivo file.
            encontreFile = 1;//Ahora ya puedo buscar el siguiente archivo.
        }
        if ((encontreFile == 1) && (header.fileList[i].size != 0) && (strcmp(header.fileList[i].fileName,file) != 0))//Encontro el final de la lista de archivos del header
            return header.fileList[i];//Archivo que le sigue a file.
    }
    struct File fileVacio = {0};
    return fileVacio;//No encontro el archivo file.
}

/*
    Funcion para agregar un espacio vacio al BlankSpaceList.
    cabeza primer elemento de la lista de espacios en blanco.
    start es el inicio del espacio en blanco.
    end es el final del espacio en blanco.  
    retorna 1 si se agrega correctamente.
*/
int addBlankSpace(struct BlankSpace** cabeza, off_t start, off_t end,int index){
    // Crear un nuevo nodo de BlankSpace
    struct BlankSpace * nuevoBlankSpace = (struct BlankSpace*)malloc(sizeof(struct BlankSpace));
    if (nuevoBlankSpace == NULL) {
        fprintf(stderr, "Error: No se pudo asignar memoria para el nuevo nodo de BlankSpace.\n");
        exit(1);
    }
    // Inicializar el nuevo nodo de BlankSpace
    nuevoBlankSpace->start = start;
    nuevoBlankSpace->end = end;
    nuevoBlankSpace->index = index;
    nuevoBlankSpace->nextBlankSpace = NULL;

    //Lista vacia o el nuevo nodo va antes que el primero. Index menor.
    if (firstBlankSpace == NULL || nuevoBlankSpace->index < firstBlankSpace->index) {
        nuevoBlankSpace->nextBlankSpace = firstBlankSpace;//Agrega el nuevo nodo al inicio de la lista
        firstBlankSpace = nuevoBlankSpace;
    } else {//Lista con elementos
        struct BlankSpace * actual = firstBlankSpace;
        //Itera mientras haya siguiente y el index del nuevo sea mayor o igual que el actual
        while ((actual->nextBlankSpace != NULL) && (actual->nextBlankSpace->index <= nuevoBlankSpace->index)) {
            actual = actual->nextBlankSpace;
        }
        //Cuando se sale del ciclo es porque o llega al final de la lista
        //o porque el index del nuevo es menor que el actual
        nuevoBlankSpace->nextBlankSpace = actual->nextBlankSpace;
        actual->nextBlankSpace = nuevoBlankSpace;
    }
    printf("Se agrego espacio en blanco en Index: %i\tStart: %ld\tEnd: %ld\n",index,start,end);
    return 1;
}

void printBlankSpaces(){
    printf("\nBlankSpaces: \n");
    struct BlankSpace * actual = firstBlankSpace;
    if (actual==NULL){
        printf("\tNo hay espacios en blanco\n");
        return;
    }
    while (actual != NULL) {
        printf("\tBlankSpace: Start: %ld \t End: %ld\n", actual->start, actual->end);
        actual = actual->nextBlankSpace;
    }
}

/*  
    Funcion para escribir el header en el tar.
    tarFile es el numero del archivo tar. Debe estar abierto en modo escritura.
*/
void writeHeaderToTar(int tarFile) {
    printf("WRITE HEADER TO TAR \n");
    char headerBlock[sizeof(header)];//Donde se guarda el contenido del header para escribirlo en el tar
    memset(headerBlock, 0, sizeof(headerBlock));  // Inicializa el bloque con bytes nulos
    memcpy(headerBlock, &header, sizeof(header));// Copia el contenido de la estructura header en el headerBlock
    if (write(tarFile, headerBlock, sizeof(headerBlock)) != sizeof(headerBlock)){// Escribe el headerBlock en el archivo TAR
        perror("writeHeaderToTar: Error al escribir el Header en el archivo TAR");
        exit(1);
    }
}

/*
    Funcion encargada de agregar el contenido de fileName 
    en el tar. Debe buscar espacio disponible.
    tarFileName es el nombre del archivo tar.
*/
void append(const char * tarFileName,const char * fileName){//TODO
    //Buscar espacio en blanco
    //Escribir el contenido de fileName en el espacio en blanco
    //Actualizar el header
    //Actualizar el espacio en blanco
    //Escribir el header en el tar
}

/*
    Funcion para escribir el contenido de un archivo en el tar.
    tarFileName es el nombre del archivo tar.
    fileName es el nombre del archivo del cual se grabara el contenido en el body del tar.
*/
void writeFileContentToTar(const char * tarFileName,const char * fileName){//!Reemplazar por append
    int file = openFile(fileName,0);//Abre el archivo que se va a escribir en el tar
    struct File fileInfo = findFile(tarFileName,fileName);//Encuentra el archivo en el header
    char buffer[fileInfo.size];//Buffer para guardar el contenido del archivo a guardar
    read(file,buffer, sizeof(buffer));//Lee el contenido del archivo y lo guarda en el buffer
    int tarFile = openFile(tarFileName,0);
    lseek(tarFile, fileInfo.start, SEEK_SET);//Coloca el puntero del tar donde de escribir.

    if (write(tarFile, buffer, sizeof(buffer)) == -1) {
        perror("writeFileContentToTar: Error al escribir en el archivo");
        close(tarFile);
        exit(1);
    }
    close(file);
    close(tarFile);
}

/*
    Funcion para escribir el contenido de los archivos
    que se encuentra en el header del tar.
    tarFile es el numero del archivo. Debe estar abierto en modo escritura.
    fileName es el nombre del archivo del cual se grabara el contenido en el body del tar.
*/
void writeBodyToTar(const char * tarFileName,const char * fileNames[],int numFiles){
    printf("WRITE BODY TO TAR \n");
    for (int i=0;i<numFiles;i++){
        struct stat fileStat;
        if (lstat(fileNames[i], &fileStat) == -1) {//Extrae la info del archivo de esta iteracion y la guarda en fileStat
            perror("writeBodyToTar: Error al obtener información del archivo. Se sale de la funcion.");
            //exit(1);
        }else
            writeFileContentToTar(tarFileName,fileNames[i]);
    }
}

/*
    Funcion encargada de escribir el cuerpo del tar.
    tarFile es el numero del archivo tar.
    El archivo tar debe estar abierto en modo lectura y escritura.
*/
void createBody(const char * tarFileName, const char *fileNames[],int numFiles){
    printf("CREATE BODY\n");
    //Ver info del header para poder escribir el cuerpo
    int tarFile = openFile(tarFileName,0);
    if (readHeaderFromTar(tarFile)==1){
        close(tarFile);
        writeBodyToTar(tarFileName, fileNames,numFiles);
    }else{
        printf("createBody: Error al leer el header del tar file\n");
        close(tarFile);
        exit(10);
    }
    close(tarFile);
}

/*
    Funcion para crear el encabezado del tar.
    numFiles es la cantidad de archivos que se van a empacar.
    tarFileName es el nombre del archivo tar que se creara.
    fileNames es un arreglo con los nombres de los archivos que se van a empacar
    retorna el tamanno del tar.
*/
void createHeader(int numFiles,int tarFile, const char * fileNames[]){
    printf("CREATE HEADER \n");

    const char * fileName;
    struct stat fileStat;
    struct File newFile;
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
        if (currentPosition==0)//Primer archivo
            newFile.start = currentPosition = sizeof(header)+1;
        else 
            newFile.start = currentPosition;
        newFile.end = currentPosition = newFile.start + fileStat.st_size;
        addFileToHeaderFileList(newFile);
    }
    writeHeaderToTar(tarFile);
    close(tarFile);//Necesito cerrar el archivo para poder volver a abrirlo en el modo lectura.
}

/*
    Funcion para calcular el espacio en blanco entre el ultimo file y
    el final del archivo tar.
    sizeOfTar es el tamanno del archivo tar.
    lastFile es el ultimo archivo del header.
    tarFileName es el nombre del archivo tar.
*/
void calculateSpaceLastFileEndTar(off_t sizeOfTar,struct File lastFile,const char * tarFileName){//*Falta probarla
    printf("Calculate Space Last File - End Tar\n");
    if (sizeOfTar-lastFile.end < 1){//Debe ser mayor o igual que 1 para poder metar un nuevo file.
        printf("calculateBlankSpaces: No hay espacio para un nuevo archivo\n");
        return;
    }
    int indexLastFile = findIndexFile(tarFileName,lastFile.fileName);//Busca el index del ultimo file.
    if (indexLastFile == -1){
        printf("calculateBlankSpaces: Error al encontrar el index del ultimo archivo\n");
        exit(1);
    }else if (indexLastFile > MAX_FILES-1){//100-1 = 99. Ya no hay espacio para mas archivos.
        printf("calculateBlankSpaces: No caben mas archivos.\n");
        exit(1);
    }
    addBlankSpace(&firstBlankSpace, lastFile.end + 1 , sizeOfTar , indexLastFile + 1 );
}

/*
    Funcion para calcular el espacio en blanco entre el header y el primer file.
    firstFile es el primer archivo del header.
*/
void calculateSpaceHeaderFistFile(struct File firstFile){//*Falta probarla
    //No puede terminar en el firstFile.start porque estaria comprimiendo los espacios en blanco
    printf("Calculate Space Header - First File\n");
    addBlankSpace(&firstBlankSpace, sizeof(header) + 1 , firstFile.start , 0 );
}
/*
    Funcion para calcular el espacio en blanco entre los archivos del tar.
*/
void calculateSpaceBetweenFilesAux(struct File firstFile,struct File lastFile,off_t sizeOfTar,const char * tarFileName){//*Falta probarla
    printf("calculateSpaceBetweenFilesAux\n");
    struct File nextFile;
    for (int i = 0; i < MAX_FILES ; i++ ){
        //if (header.fileList[i].size !=  0)
            printf("i= %i FileName= %s Size= %ld Start= %ld End= %ld Deleted= %i\n",i,header.fileList[i].fileName,header.fileList[i].size,header.fileList[i].start,header.fileList[i].end,header.fileList[i].deleted);   

        if (header.fileList[i].deleted==1){//Esa posicion fue borrada con anterioridad.
            addBlankSpace(&firstBlankSpace,header.fileList[i].start,header.fileList[i].end,i);
            continue;
        }
        if ((header.fileList[i].size !=  0) && ( i + 1 < MAX_FILES)){//Hay archivo en pos y no es la ultima iteracion
            //Si el primer archivo no empieza justo despues del header
            if ((strcmp(firstFile.fileName,header.fileList[i].fileName) ==0) && (firstFile.start != sizeof(header)+1 )){
                calculateSpaceHeaderFistFile(firstFile);
                continue;
            }
            //Espacio entre ultimo archivo y fin de tar.
            else if ((strcmp(lastFile.fileName,header.fileList[i].fileName)==0) &&(lastFile.end != sizeOfTar)){
                calculateSpaceLastFileEndTar(sizeOfTar,lastFile,tarFileName);
                continue;
            }
            nextFile = findNextFile(header.fileList[i].fileName);//Buscar donde esta el siguiente archivo. Y sacar la diferencia.
            printf("Next file - Name: %s \t Start: %ld \t End: %ld\n",nextFile.fileName,nextFile.start,nextFile.end);
            if ((nextFile.size == 0) && (strcmp(nextFile.fileName,lastFile.fileName) != 0) ){
                printf("No hay mas archivos.\n");
                continue;
            }
            if ((strcmp(header.fileList[i].fileName,nextFile.fileName) != 0) && (nextFile.start - header.fileList[i].end > 1)){
                addBlankSpace(&firstBlankSpace,header.fileList[i].end,nextFile.start,i);
            }
        }else{
            
        }
    }
}

/*
    Funcion para calcular el espacio en blanco cuando el tar esta vacio.
    sizeOfTar es el tamanno del archivo tar.
*/
void calculateSpaceInEmptyTar(off_t sizeOfTar){//*Falta probarla
    //Espacio entre fin del header y fin del tar.
    addBlankSpace(&firstBlankSpace, sizeof(header) + 1 , sizeOfTar , 0 );
}

/*
    Funcion para calcular los espacios en blanco del tar.
    Imprime los espacios en blanco cuando termina.
*/
void calculateBlankSpaces(const char * tarFileName){//TODO: CORREGIR ESTA FUNCION
    //!Error en espacio entre header y primer file. Porque comprime.
    printf("CALCULATING BLANKSPACES\n");
    off_t sizeOfTar = getFileSize(tarFileName);// Obtiene el tamaño del archivo tar.
    int tarFile = openFile(tarFileName, 0);// Abre el archivo para leer el header.

    if (readHeaderFromTar(tarFile) != 1){
        printf("calculateBlankSpaces: Error al leer el header del tar file\n");
        close(tarFile);
        exit(10);
    }
    close(tarFile);

    if (isHeaderFileListEmpty() == 1)//Lista de archivos del header vacia.
        calculateSpaceInEmptyTar(sizeOfTar);
    else{//Lista de archivos del header tiene archivos.
        struct File firstFile = findFirstFileInHeader();
        struct File lastFile = findLastFileInHeader();
        calculateSpaceBetweenFilesAux(firstFile,lastFile,sizeOfTar,tarFileName);
    }
    printBlankSpaces();
}

/*
    Funcion que crea un archivo tar con los archivos especificados
    en fileNames. El nombre del archivo tar es tarFileName.
    numFiles es la cantidad de archivos que se van a empacar.
*/
void createStar(int numFiles, const char *tarFileName, const char *fileNames[]){
    printf("\n \t CREATE TAR FILE \n");
    printf("Size of header: %ld\n",sizeof(header));
    if (numFiles > MAX_FILES){
        printf("createStar: Se excedio el numero maximo de archivos que se pueden empacar\n");
        exit(1);
    }
    createHeader(numFiles,openFile(tarFileName,1),fileNames);//Crear header con los datos de los archivos
    printHeader();
    createBody(tarFileName,fileNames,numFiles);//Guardar el contenido de los Files segun las posiciones de 
}

/*
    Funcion para eliminar un archivo del header.
    file es el archivo que se va a eliminar.
    retorna 1 si lo elimina correctamente.

*/
int deleteFileFromHeader(struct File file){
    for (int i=0;i<MAX_FILES;i++){
        if (strcmp(header.fileList[i].fileName,file.fileName)==0){
            header.fileList[i].fileName[0]='\0';
            header.fileList[i].size=0;
            header.fileList[i].start=0;
            header.fileList[i].end=0;
            header.fileList[i].deleted=1;//Se usa para que no se junten los espacios en blanco.
            return 1;
        }
    }
    return 0;
}

/*
    Funcion para eliminar el contenido de un archivo del body del tar.
    tarFileName es el nombre del archivo tar.
    fileToBeDeleted es el archivo que se va a eliminar.
    NO se modifica el tamanno del archivo tar.
*/
void deleteFileContentFromBody(const char* tarFileName,struct File fileToBeDeleted) {
    FILE * archivo = fopen(tarFileName, "r+");
    if (archivo == NULL) {
        perror("deleteFileContentFromBody: Error al abrir el archivo");
        exit(1);
    }
    fseek(archivo, 0, SEEK_END);
    char buffer[1024];//Tamanno por defecto de escritura de '\0'
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
    fclose(archivo);
}

/*
    Funcion encargada de eliminar un archivo del tar.
    fileName es el nombre del archivo a eliminar.
    Retorna 0 si se elimino correctamente.
*/
int deleteFile(const char * tarFileName,const char * fileNameTobeDeleted){
    printf("\n \t DELETE FILE \n");
    int tarFile = openFile(tarFileName,0);
    struct File fileTobeDeleated = findFile(tarFileName,fileNameTobeDeleted);
    if (fileTobeDeleated.size==0){
        printf("deleteFile: El archivo no se encuentra en el tar file\n");
        exit(11);
    }
    printf("Nombre de archivo a borrar %s \t Start: %ld \t End: %ld\n",fileNameTobeDeleted,fileTobeDeleated.start,fileTobeDeleated.end);

    deleteFileContentFromBody(tarFileName,fileTobeDeleated);//Elimina archivo del body del tar.
    deleteFileFromHeader(fileTobeDeleated);//Elimina el archivo del header.
    writeHeaderToTar(tarFile);//Vuelve a escribir el header en el tar.
    close(tarFile);
    calculateBlankSpaces(tarFileName);//Calcular espacios en blanco
    return 0;
}


/*
    Funcion para listar el archivo tar.
*/
void listStar(const char * tarFileName) {
    int tarFile = openFile(tarFileName,0);
    if (tarFile == -1) {
        fprintf(stderr, "listStar: Error al abrir el archivo TAR.\n");
        exit(1);
    }
    if (readHeaderFromTar(tarFile)==1)
        printf("listStar: Se leyó el Header correctamente\n");
    else{
        printf("listStar: Error al leer el header del tar file\n");
        exit(1);
    }

    if (header.fileList == NULL) {
        fprintf(stderr, "La lista de archivos en el encabezado está vacía o no inicializada.\n");
        close(tarFile);
        exit(1);
    }
    printf("\n \t LIST TAR FILES \n");
    printHeader();
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

    } else if (strcmp(opcion, "-d") == 0){
        const char * fileName;
        fileName = argv[0 + 3];//Sacar el nombre del archivo a borrar
        deleteFile(archivoTar,fileName);//Solo el primer archivo de la lista de archivos
    }else {
        fprintf(stderr, "Uso: %s -c|-t <archivoTar> [archivos]\n", argv[0]);
        exit(1);
    }

    
    // Usa la función lseek para mover el puntero al final del archivo
    int tarFile = openFile(archivoTar,0);
    off_t tamano = lseek(tarFile, 0, SEEK_END);

    if (tamano == -1) {
        perror("main: Error al obtener el tamaño del archivo tar.");
        close(tarFile);
        exit(1);
    }

    printf("\nEl tamaño del archivo tar es de: %ld bytes\n", (long)tamano);
    
    printf("\n PROGRAMA TERMINA EXITOSAMENTE...\n");
    return 0;
}