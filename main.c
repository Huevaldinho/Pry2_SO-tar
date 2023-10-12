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

struct File {
    char filename[MAX_FILENAME_LENGTH];
    mode_t mode;
    off_t size;
    off_t start;
    off_t end;
};


void createStar() {

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