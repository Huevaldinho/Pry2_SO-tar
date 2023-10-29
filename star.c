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
int numFiles=0;

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

/*
    Función que devuelve el tamanno de un archivo
*/
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
    Obtiene el tamano en bytes de la suma de los tamaños de los archivos contenidos en el tar.
*/
off_t getSizeOfContents(){
    off_t totalSum = 0;
    for (int i = 0; i < MAX_FILES; i++) {
        if (header.fileList[i].size!=0){
            totalSum += header.fileList[i].size;
        }
    }
    return totalSum;
}

/*
    Funcion para guardar un archivo en el header.
    Por ahora solo se va a usar en createHeader
    CUIDADO: Solo guarda en posiciones NULL del array de Files. Revisa de principio a fin.
*/
void addFileToHeaderFileList(struct File newFile) {
    printf("Adding \"%s\" to header's file list.\n", newFile.fileName);
    for (int i = 0; i < MAX_FILES; i++) {
        if (header.fileList[i].size==0){//Posicion vacia
            header.fileList[i]= newFile;
            numFiles++;
            break;
        }
    }
}

/*
    Funcion para encontrar el index del ultimo archivo en el header.
    Retorna el index del array de ese ultimo file.
    Retorna -1 si no hay archivos.
*/
int findIndexLastFileInHeader(){
    for (int i = MAX_FILES-1; i >= 0; i--) 
        if (header.fileList[i].size !=  0) 
            return i;
    return -1;
} 

/*
    Funcion para agregar un archivo de ultimo en el header.
    NO de ultimo en el array, si no que este nuevo archivo
    sea el ultimo archivo de la lista.
*/
void addFileToHeaderListInLastPosition(struct File newFile){
    int lastPosition = findIndexLastFileInHeader();
    if (lastPosition==-1){
        printf("No hay archivos.\n");
        header.fileList[0] = newFile;
        return;
    }
    if (lastPosition==MAX_FILES-1){
        printf("No hay espacio en el header.\n");
        exit(1);
    }
    header.fileList[lastPosition+1] = newFile;
}

/*
    Funcion para contar cuantos archivos hay en el tar.
    Retorna la cantidad de archivos que hay en el tar.
*/
int sumFiles(){
    for (int i =0; i<MAX_FILES ; i++){
        if (header.fileList[i].size!=0)
            numFiles++;
    }
    return numFiles;
}

/*
    Retorna un arreglo con todos los archivos existentes.
*/
struct File * modifiedExistentFiles(int sumFiles){
    struct File* modifiedFiles = (struct File*)malloc(sumFiles * sizeof(struct File));
    int bytePosition = 0;
    int selectedCount = 0;
    for (int i = 0; i < MAX_FILES; i++) {
        if (header.fileList[i].size != 0){
            struct File modifiedFile = header.fileList[i];
            //Nuevas posiciones
            if (bytePosition == 0) modifiedFile.start = bytePosition = sizeof(header)+1;
            else modifiedFile.start = currentPosition;
            modifiedFile.end = currentPosition = modifiedFile.start + modifiedFile.size;
            
            modifiedFiles[selectedCount] = modifiedFile;
            selectedCount++;
        }
    } 
    return modifiedFiles;
}

/*
    Cambia el tamaño de un archivo al tamaño indicado.
*/
void truncateFile(const char * fileName, off_t newSize){
    int file = openFile(fileName, 0);
    if (file == -1) {
        perror("truncateFile: Error al abrir el archivo");
        exit(1);
    }
    // Llama a ftruncate para cambiar el tamaño
    if (ftruncate(file, newSize) == -1) {
        perror("truncateFile: Error al cambiar el tamaño del archivo");
        close(file);
        exit(1);
    }
    close(file);
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
    memcpy(&header, headerBlock, sizeof(header));// Copia el contenido del bloque en la estructura 'header'
    return 1;
}

/*
    Función para leer el contenido de un archivo en el tar.
    Retorna el contenido en un buffer.
    Si hay error, retorna NULL
*/
char * readContentFromTar(int tarFile, struct File fileToRead){
    if(lseek(tarFile, fileToRead.start, SEEK_SET) == -1){ // Pone puntero del archivo tar al inicio del contenido a extraer.
        perror("readContentFromTar: Error al posicionar el puntero al principio del archivo a extraer.");
        close(tarFile);
        return NULL;
    }

    char* buffer = (char*)malloc(fileToRead.size); // To store content

    if (read(tarFile, buffer, fileToRead.size) == -1){ // Reads content in 'buffer'
        perror("Error al leer el contenido del archivo tar");
        free(buffer);
        close(tarFile);
        return NULL;
    }

    close(tarFile);
    return buffer;
}

/*
    Funcion para encontrar un archivo en el tar.
    Retorna un archivo.
    Si lo encuentra, retorna el archivo encontrado. Si no, retorna un archivo con size=0.
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
    printf("findFile: No se encuentra el archivo.\n");
    struct File notFound;
    notFound.size=0;
    return notFound;
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
    printf("\nHEADER: \n");
    for (int i = 0; i < MAX_FILES; i++) {
        if (header.fileList[i].size !=  0) {
            printf("File name: %s \t Index:%i \t Size: %lld \t Start: %lld \t End: %lld\n", header.fileList[i].fileName,i, header.fileList[i].size, header.fileList[i].start, header.fileList[i].end);
        }
    }
    printf("\n");
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

int isBlankSpaceRepeated(off_t start, off_t end){
    struct BlankSpace * actual = firstBlankSpace;
    while (actual!=NULL){
        if (actual->start == start && actual->end == end)
            return 1;
        actual=actual->nextBlankSpace;
    }
    return 0;
}

/*
    Funcion para agregar un espacio vacio al BlankSpaceList.
    cabeza primer elemento de la lista de espacios en blanco.
    start es el inicio del espacio en blanco.
    end es el final del espacio en blanco.  
    retorna 1 si se agrega correctamente.
*/
int addBlankSpace(struct BlankSpace ** cabeza, off_t start, off_t end,int index){
    if (isBlankSpaceRepeated(start,end)==1){
        printf("Espacio en blanco repetido start:%lld end:%lld\n",start,end);
        return 0;
    }   
    if (start > end) {
        fprintf(stderr, "Error: El inicio del espacio en blanco es mayor que el final.\n");
        exit(1);
    }
    if (start ==0 && end==0)
        return 0;
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
    //Espacio en blanco agregado
    return 1;
}

/*
    Funcion para eliminar un espacio en blanco de la lista de espacio en blanco.
    targetIndex es el indice del espacio en blanco que se va a eliminar.
*/
void deleteBlankSpace(int targetIndex) {
    struct BlankSpace * current = firstBlankSpace;
    struct BlankSpace * prev = NULL;
    // Buscar el elemento que coincida con el índice objetivo
    while (current != NULL && current->index != targetIndex) {
        prev = current; //Deja el puntero en el elemento anterior
        current = current->nextBlankSpace;//Avanza al siguiente elemento
    }
    //Leyo la lista y no encontro el elemento a eliminar.
    if (current == NULL) {
        printf("No se encontró el elemento con el índice %d\n", targetIndex);
        return;
    }
    // Ajustar los punteros para eliminar el elemento
    if (prev == NULL){ //Si prev queda en NULl es porque nunca entro al ciclo. Si no entro es porque
                       //Current es el primer elemento de la lista.
        firstBlankSpace = current->nextBlankSpace;// El elemento a eliminar es el primer elemento de la lista
    }else{//No es el primer elemento de la lista.
        prev->nextBlankSpace = current->nextBlankSpace;
    }
    // Liberar la memoria del elemento eliminado
    free(current);
    printf("Se elimino espacio en blanco satisfactoriamente.\n");
}

/*
    Reincia y limpia por completo la lista de espacios en blanco.
*/
void resetBlankSpaceList() {
    struct BlankSpace * current = firstBlankSpace;
    while (current != NULL) {
        struct BlankSpace *next = current->nextBlankSpace;
        free(current); // Libera la memoria del nodo actual
        current = next;
    }
    firstBlankSpace = NULL; // Establece el puntero a NULL para indicar que la lista está vacía
}

void printBlankSpaces(){
    printf("\nBLANK SPACES: \n");
    struct BlankSpace * actual = firstBlankSpace;
    if (actual==NULL){
        printf("There are no blank spaces\n");
        return;
    }
    while (actual != NULL) {
        printf("BlankSpace ->\tStart: %lld\tEnd: %lld\n", actual->start, actual->end);
        actual = actual->nextBlankSpace;
    }
    printf("\n");

}

/*  
    Funcion para escribir el header en el tar.
    tarFile es el numero del archivo tar. Debe estar abierto en modo escritura.
*/
void writeHeaderToTar(int tarFile){
    printf("\nWriting header to tar...\n");
    char headerBlock[sizeof(header)];//Donde se guarda el contenido del header para escribirlo en el tar
    memset(headerBlock, 0, sizeof(headerBlock));  // Inicializa el bloque con bytes nulos
    memcpy(headerBlock, &header, sizeof(header));// Copia el contenido de la estructura header en el headerBlock
    if (write(tarFile, headerBlock, sizeof(headerBlock)) != sizeof(headerBlock)){// Escribe el headerBlock en el archivo TAR
        perror("writeHeaderToTar: Error al escribir el Header en el archivo TAR");
        exit(1);
    }
}

/*
    Funcion para escribir el contenido de un archivo en el tar.
    tarFileName es el nombre del archivo tar.
    fileName es el nombre del archivo del cual se grabara el contenido en el body del tar.
*/
void writeFileContentToTar(const char * tarFileName,const char * fileName){//!Reemplazar por append
    int file = openFile(fileName,0);//Abre el archivo que se va a escribir en el tar
    struct File fileInfo = findFile(tarFileName,fileName);//Encuentra el archivo en el header
    if (fileInfo.size==0){
        printf("writeFileContentToTar: El archivo no se encuentra en el tar file\n");
        exit(11);
    }
    char buffer[fileInfo.size];//Buffer para guardar el contenido del archivo a guardar
    read(file,buffer, sizeof(buffer));//!!!!!!!!!!Lee el contenido del archivo y lo guarda en el buffer
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
    printf("\nWriting body to tar...\n");
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
    tarFile es el archivo tar que se creara.
    fileNames es un arreglo con los nombres de los archivos que se van a empacar
    retorna el tamanno del tar.
*/
void createHeader(int numFiles,int tarFile, const char * fileNames[]){
    printf("\nCREATE HEADER\n");
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
        newFile.deleted = 0;
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
    Funcion para calcular el espacio en blanco entre los archivos del tar.
*/
void calculateSpaceBetweenFilesAux(struct File lastFile,off_t sizeOfTar,const char * tarFileName){
    struct File nextFile;
    int indexLastFile = findIndexFile(tarFileName,lastFile.fileName);
    for (int i = 0; i < MAX_FILES ; i++ ){
        if (header.fileList[i].deleted == 1){//Esa posicion fue borrada con anterioridad.
            if (i != MAX_FILES-1 && i != 0 ){//No es el ultimo archivo ni el primero
                if (header.fileList[i+1].start != 0){//El archivo siguiente no es el ultimo
                    addBlankSpace(&firstBlankSpace, header.fileList[i-1].end , header.fileList[i+1].start , i );
                } else{//No hay mas archivos
                    addBlankSpace(&firstBlankSpace, header.fileList[i-1].end , sizeOfTar-1 , i );
                }
            }else if (i==0){//Es el primer archivo
                addBlankSpace(&firstBlankSpace, sizeof(header) + 1 , header.fileList[i+1].start , 0 );
            }
        }else{
            //printf("Archivo en index %i no ha sido borrado.\n",i);
            if (i < MAX_FILES-1 && (header.fileList[i+1].size>0) && (header.fileList[i].end != header.fileList[i+1].start)){//No es el ultimo
                addBlankSpace(&firstBlankSpace, header.fileList[i].end , header.fileList[i+1].start , i );
            }
        }
    }
}

/*
    Funcion para calcular los espacios en blanco del tar.
    Imprime los espacios en blanco cuando termina.
*/
void calculateBlankSpaces(const char * tarFileName){
    printf("Calculating blank spaces...\n");
    off_t sizeOfTar = getFileSize(tarFileName);// Obtiene el tamaño del archivo tar.
    int tarFile = openFile(tarFileName, 0);// Abre el archivo para leer el header.

    if (readHeaderFromTar(tarFile) != 1){
        printf("calculateBlankSpaces: Error al leer el header del tar file\n");
        close(tarFile);
        exit(10);
    }
    close(tarFile);
    calculateSpaceBetweenFilesAux(findLastFileInHeader(),sizeOfTar,tarFileName);
    printBlankSpaces();
}

/*
    Funcion que crea un archivo tar con los archivos especificados
    en fileNames. El nombre del archivo tar es tarFileName.
    numFiles es la cantidad de archivos que se van a empacar.
*/
void createStar(int numFiles, const char *tarFileName, const char *fileNames[]){
    printf("\nCREATE TAR FILE \n");
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
    printf("Deleting file from header...\n");
    for (int i=0;i<MAX_FILES;i++){
        if (strcmp(header.fileList[i].fileName,file.fileName)==0){
            header.fileList[i].size=0;
            header.fileList[i].deleted=1;//Se usa para que no se junten los espacios en blanco.
            numFiles--;
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
    printf("Deleting file from body...\n");
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
    fseek(archivo, 0, SEEK_END);
    fclose(archivo);
}

/*
    Funcion encargada de eliminar un archivo del tar.
    fileName es el nombre del archivo a eliminar.
    Retorna 0 si se elimino correctamente.
*/
int deleteFile(const char * tarFileName,const char * fileNameTobeDeleted){
    printf("\nDELETE FILE\n");
    int tarFile = openFile(tarFileName,0);
    struct File fileTobeDeleated = findFile(tarFileName,fileNameTobeDeleted);
    if (fileTobeDeleated.size==0){
        printf("deleteFile: El archivo no se encuentra en el tar file\n");
        exit(11);
    }
    printf("File to be deleted: %s\tStart:%lld\tEnd: %lld\n",fileNameTobeDeleted,fileTobeDeleated.start,fileTobeDeleated.end);

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
    printf("\n \t LIST TAR FILES \n");
    printHeader();
    close(tarFile);
}

/*
    Funcion para encontrar un espacio en blanco para colocar el nuevo archivo.
    Retorna el espacio en blanco encontrado
    Retorna NULL si no hay espacios lo suficientemente grandes para ese archivo.
*/
struct BlankSpace * findBlankSpaceForNewFile(off_t sizeOfNewFile){
    struct BlankSpace * current = firstBlankSpace;
    off_t spaceSize;
    while (current != NULL) {
        spaceSize = current->end - current->start;
        if (spaceSize >= sizeOfNewFile)
            return current;
        current = current->nextBlankSpace;
    }
    return NULL;  // No se encontró espacio lo suficientemente grande
}

/* 
    Funcion para escribir contenido al final del archivo tar.
*/
void writeAtTheEndOfTar(const char * tarFileName ,const char * fileName){
    int tarFile = openFile(tarFileName,0);//Archivo tar
    int file = openFile(fileName,0);//Archivo a guardar
    struct stat fileStat;
    if (lstat(fileName, &fileStat) == -1) {//Sacar info del archivo a guardar.
        perror("append: Error al obtener información del archivo.\n");
        exit(1);
    }
    struct File fileInfo;
    struct File lastFile = findLastFileInHeader();
    strncpy(fileInfo.fileName,fileName,MAX_FILENAME_LENGTH);
    fileInfo.mode = fileStat.st_mode; 
    fileInfo.size = fileStat.st_size;
    fileInfo.start = lastFile.end;  
    fileInfo.end = lastFile.end+ fileStat.st_size;
    fileInfo.deleted = 0; 
    addFileToHeaderListInLastPosition(fileInfo);
    writeHeaderToTar(tarFile);
    char buffer[fileInfo.size];//Buffer para guardar el contenido del archivo a guardar
    read(file,buffer, sizeof(buffer));//Lee el contenido del archivo y lo guarda en el buffer
    lseek(tarFile, fileInfo.start, SEEK_SET);//Coloca puntero del tar al final.
    if (write(tarFile, buffer, sizeof(buffer)) == -1) {
        perror("writeFileContentToTar: Error al escribir en el archivo");
        close(tarFile);
        exit(1);
    }
    close(file);
    close(tarFile);
    printf("Se agrego archivo satisfactoriamente.\n");
}

/*
    Reincia el header para que pierda todos sus valores.
*/
void resetHeader() {
    for (int i = 0; i < MAX_FILES; i++) {
        memset(header.fileList[i].fileName, 0, MAX_FILENAME_LENGTH); // Establece el nombre del archivo a una cadena vacía
        header.fileList[i].mode = 0; // Establece el modo a 0 (u otro valor por defecto)
        header.fileList[i].size = 0; // Establece el tamaño a 0
        header.fileList[i].start = 0; // Establece la posición de inicio a 0
        header.fileList[i].end = 0; // Establece la posición de fin a 0
        header.fileList[i].deleted = 0; // Establece el indicador de eliminado a 0 (no eliminado)
    }
}


/*
    Funcion encargada de agregar el contenido de fileName 
    en el tar. Debe buscar espacio disponible.
    tarFileName es el nombre del archivo tar.
    Si aún así el nuevo contenido no cabe, se hace crecer el archivo empacado. 
    No debeutilizar ningún archivo auxiliar para hacer crecer el archivo.
*/
void append(const char * tarFileName,const char * fileName){
    printf("\nAPPEND\n");
    if (sumFiles() >= MAX_FILES){
        printf("Cantidad maxima de archivos alcanzada.\n");
        exit(1);
    }
    calculateBlankSpaces(tarFileName);
    struct stat fileStat;
    if (lstat(fileName, &fileStat) == -1) {//Sacar info del archivo a guardar.
        perror("append: Error al obtener información del archivo.\n");
        exit(1);
    }
    //Buscar espacio disponible para este file.
    struct BlankSpace * espacioDisponible = findBlankSpaceForNewFile(fileStat.st_size);
    if (espacioDisponible == NULL){//Si no encuentra, se agrega al final del archivo.
        writeAtTheEndOfTar(tarFileName,fileName);
    }else{
        //Actualiza header.
        strncpy(header.fileList[espacioDisponible->index].fileName,fileName,MAX_FILENAME_LENGTH);
        header.fileList[espacioDisponible->index].deleted = 0;
        header.fileList[espacioDisponible->index].start = espacioDisponible->start;
        header.fileList[espacioDisponible->index].end = espacioDisponible->start + fileStat.st_size;
        header.fileList[espacioDisponible->index].size = fileStat.st_size;
        int tarFile = openFile(tarFileName,0);
        writeHeaderToTar(tarFile);//Escribir el header en el tar
        printf("Size of tar antes de guardar el contenido del archivo nuevo %lld \n",lseek(tarFile, 0, SEEK_END));
        close(tarFile);
        writeFileContentToTar(tarFileName,fileName);//Escribir contenido del archivo en el tar.
        //Borrar espacio en blanco de la lista
        deleteBlankSpace(espacioDisponible->index);
    }
    printf("Header despues del append.\n");
    printHeader();
    calculateBlankSpaces(tarFileName);    
}

/*
    Función encargada de extraer los archivos especificados del tar.
    Lee el contenido de cada archivo desde el tar y lo copia en un archivo
    nuevo con el nombre del original.
*/
void extract(int numFiles, const char *tarFileName, const char *fileNames[]){
    for (int i=0; i<numFiles; i++){
        struct File fileToBeExtracted = findFile(tarFileName, fileNames[i]); // reads header
        if (fileToBeExtracted.size==0){
            printf("extract: El archivo no se encuentra en el tar file\n");
            exit(11);
        }
        char* content = readContentFromTar(openFile(tarFileName, 0), fileToBeExtracted);

        int extractedFile = openFile(fileToBeExtracted.fileName, 1); // New File
        if (write(extractedFile, content, fileToBeExtracted.size) == -1){
            perror("Error al escribir en el archivo de salida.");
            free(content);
            close(extractedFile);
            exit(1);
        }
        printf("File \"%s\" extracted in execution directory.\n", fileToBeExtracted.fileName);
        free(content);
        close(extractedFile);
    }
    printHeader();

}

/*
    Extrae el contenido de todos los archivos contenidos en el .tar.
    Crea nuevos archivos con el contenido y nombre original del .tar.
*/
void extractAll(const char *tarFileName){
    int tarFile = openFile(tarFileName,0);
    if (readHeaderFromTar(tarFile)!=1){
        printf("extractAll: Error 1 al leer el header del tar file\n");
        close(tarFile);
        exit(10);
    }
    close (tarFile);
    for (int i = 0; i < MAX_FILES; i++) {
        if (header.fileList[i].size!=0){//Encontro el archivo
            struct File fileToBeExtracted = header.fileList[i];
            char* content = readContentFromTar(openFile(tarFileName, 0), fileToBeExtracted);
            int extractedFile = openFile(fileToBeExtracted.fileName, 1); // New File
            if (write(extractedFile, content, fileToBeExtracted.size) == -1){
                perror("Error al escribir en el archivo de salida.");
                free(content);
                close(extractedFile);
                exit(1);
            }
            printf("\nArchivo \"%s\" extraído en la carpeta de ejecución.\n", fileToBeExtracted.fileName);
            free(content);
            close(extractedFile);
            struct stat fileStat;
            if (lstat(fileToBeExtracted.fileName, &fileStat) == -1) {//Sacar info del archivo a guardar.
                perror("append: Error al obtener información del archivo.\n");
                exit(1);
            }
            printf("Tamanno del archivo nuevo: %lld\n", fileStat.st_size);
        }
    }
}

/*
    Actualiza el contenido de un archivo contenido en el .tar
    Primero elimina el contenido original del archivo mencionado.
    Luego, agrega el contenido nuevo del mismo archivo; su posición se modifica
    según la función append.
*/
void update(const char *tarFileName, const char *fileToBeUpdatedName){
    if (deleteFile(tarFileName, fileToBeUpdatedName) == 0){ // Valida y elimina el archivo a actualizar
        append(tarFileName, fileToBeUpdatedName);
    }
}

/*
    Devuelve una cadena de caracteres única con todos los contenidos válidos
    en el body del tar.
    Recibe un entero y lo modifica para indicar la cantidad de archivos reales iterados.
*/
char * getWholeBodyContentInfo(const char * tarFileName, int * filesSum){
    char * wholeContent = (char*)malloc(getSizeOfContents());
    char * content;
    for (int i=0; i < MAX_FILES; i++){
        if (header.fileList[i].size!=0){
            (*filesSum) += 1;
            content = readContentFromTar(openFile(tarFileName, 0), header.fileList[i]);
            strcat(wholeContent, content);
        }
    }
    free(content);
    return wholeContent;
}

/*
    Defragmenta el archivo: elimina los espacios en blancos y comprime el contenido.
*/
void pack(const char * tarFileName){
    int tarFile = openFile(tarFileName,0);
    if (readHeaderFromTar(tarFile)!=1){ // Read header from tar
        printf("extractAll: Error 1 al leer el header del tar file\n");
        close(tarFile);
        exit(10);
    }
    close (tarFile);
    calculateBlankSpaces(tarFileName); // Calcular espacios en blanco

    printf("\nPACK\n");
    
    int sumFiles = 0;
    char * bodyContentBuffer = getWholeBodyContentInfo(tarFileName, &sumFiles); // cadena de caracteres con todos los contenidos de forma secuencial.
    struct File * modifiedFiles = modifiedExistentFiles(sumFiles); // archivos existentes con sus bytes de posicion modificados.
    resetHeader(); // reinica el header para poner la nueva información por completo.
    for (int j=0; j<sumFiles; j++){
        addFileToHeaderFileList(modifiedFiles[j]);
    }
    tarFile = openFile(tarFileName,0);
    writeHeaderToTar(tarFile); // Reescribir header en el archivo.
    close(tarFile);
    truncateFile(tarFileName, sizeof(header)); // Deja solo el header en el archivo .tar
    tarFile = openFile(tarFileName, 0);
    if (lseek(tarFile, 0, SEEK_END) == -1) { // Mover el puntero del archivo al final del archivo .tar
        perror("pack: Error al mover el puntero al final del archivo .tar");
        close(tarFile);
        exit(1);
    }
    // Escribir el contenido almacenado en bodyContentBuffer al final del archivo .tar
    if (write(tarFile, bodyContentBuffer, strlen(bodyContentBuffer)) == -1) {
        perror("pack: Error al escribir el contenido en el archivo .tar");
        close(tarFile);
        exit(1);
    }
    close(tarFile);
    resetBlankSpaceList(); // Reiniciar lista de espacios en blanco.
    printHeader();
    printBlankSpaces();
}
int main(int argc, char *argv[]) {//!Modificar forma de usar las opciones
    if (argc < 3) {
        fprintf(stderr, "Use: %s -c|-t|-d|-r|-x|-u|-p <tarFile.tar> [files]\n", argv[0]);
        exit(1);
    }
    const char * opcion = argv[1];
    const char * archivoTar = argv[2];

    // Recorrer cada carácter de la opción
    for (int i = 1; i < strlen(opcion); i++) {
        char opt = opcion[i];
        printf("%c\n",opt);
        if (opt == 'c'){//* Create
            int numFiles = argc - 3;
            const char * fileNames[MAX_FILES];
            for (int i = 0; i < numFiles; i++) 
                fileNames[i] = argv[i + 3];
            createStar(numFiles, archivoTar, fileNames);
        } 
        else if (opt == 't'){//* List
            listStar(archivoTar);
        } 
        else if (opt == 'd'){//* Delete
            const char * fileName;
            fileName = argv[0 + 3];//Sacar el nombre del archivo a borrar
            deleteFile(archivoTar,fileName);//Solo el primer archivo de la lista de archivos
        }
        else if (opt == 'r'){//* Append
            const char * fileName;
            fileName = argv[0 + 3];//Sacar el nombre del archivo a agregar
            append(archivoTar,fileName);
        }
        else if (opt == 'x') {//* Extract
            const char * fileNames[MAX_FILES];
            if (argc == 3){ // Extract all
                extractAll(archivoTar);
            } else { // Extract some
                int numFiles = argc - 3;
                for (int i = 0; i < numFiles; i++) 
                    fileNames[i] = argv[i + 3];
                extract(numFiles, archivoTar, fileNames);
            }
        }
        else if (opt == 'u'){//* Update
            const char * fileName;
            fileName = argv[0 + 3];//Sacar el nombre del archivo a agregar
            update(archivoTar,fileName);
        }
        else if (opt == 'p'){
            pack(archivoTar);
        }else{
            fprintf(stderr, "Uso: %s -c|-t|-d|-r|-x <archivoTar> [archivos]\n", argv[0]);
            exit(1);
        }
    }
    
    
    // Usa la función lseek para mover el puntero al final del archivo
    int tarFile = openFile(archivoTar,0);
    off_t size = lseek(tarFile, 0, SEEK_END);

    if (size == -1) {
        perror("main: Error al obtener el tamaño del archivo tar.");
        close(tarFile);
        exit(1);
    }

    printf("\n----------------------------------------------------\n");
    printf("\nSize of tar file is of: %ld bytes.", (long)size);
    printf("\nPROGRAM ENDS SUCCESSFULLY\n");
    
    return 0;
}