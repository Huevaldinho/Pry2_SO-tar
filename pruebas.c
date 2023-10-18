#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


struct BlankSpace {
    off_t start;
    off_t end;
    int index;
    struct BlankSpace *nextBlankSpace;
} *firstBlankSpace;

void insertBlankSpace(struct BlankSpace *newBlankSpace) {
    if (firstBlankSpace == NULL || newBlankSpace->index < firstBlankSpace->index) {
        // Insert at the beginning
        newBlankSpace->nextBlankSpace = firstBlankSpace;
        firstBlankSpace = newBlankSpace;
    } else {
        // Insert in the middle or at the end
        struct BlankSpace *current = firstBlankSpace;
        while (current->nextBlankSpace != NULL && current->nextBlankSpace->index <= newBlankSpace->index) {
            current = current->nextBlankSpace;
        }
        newBlankSpace->nextBlankSpace = current->nextBlankSpace;
        current->nextBlankSpace = newBlankSpace;
    }
}

void printList(){
    // Print the sorted list
    struct BlankSpace *current = firstBlankSpace;
    while (current != NULL) {
        printf("Index: %d, Start: %ld, End: %ld\n", current->index, current->start, current->end);
        current = current->nextBlankSpace;
    }

}

int main() {
    // Example usage:
    struct BlankSpace space1 = {100, 200, 10, NULL};
    struct BlankSpace space2 = {300, 400, 20, NULL};
    struct BlankSpace space3 = {50, 75, 30, NULL};

    insertBlankSpace(&space3);//index 30
    insertBlankSpace(&space1);//index 10
    insertBlankSpace(&space2);//index 20
    printList();
    printf("Termina agregar 3 primeros\n");
    struct BlankSpace space4 = {250, 275, 4, NULL};
    struct BlankSpace space5 = {500, 600, 40, NULL};
    struct BlankSpace space6 = {700, 800, 25, NULL};
    insertBlankSpace(&space4);//index 4
    insertBlankSpace(&space5);//index 40
    insertBlankSpace(&space6);//index 25
    printList();

    
    return 0;
}
