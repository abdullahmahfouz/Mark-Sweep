#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum{
    OBJ_INT, // Replaces True and False objects
    OBJ_PAIR // Represents a pair of objects (like a cons cell)
} ObjectType;

typedef struct sObject {
    ObjectType type; // store the type of the object
    unsigned char marked; // for garbage collection
    struct sObject* next; // pointer to the next object in memory
    union{
        int value; // for OBJ_INT, we can use an integer value
        struct{
            struct sObject* head; // pointer to the head of the list
            struct sObject* tail; // pointer to the tail of the list
        };
    }; 

} Object;

#define STACK_MAX 256 // Maximum size of the stack
#define INITIAL_GC_THRESHOLD 8 // Initial threshold to trigger GC

typedef struct {
  /* The Stack: Objects currently reachable by the "program" (Roots) */
  Object* stack[STACK_MAX];
  int stackSize;

  /* The Heap: A linked list of ALL allocated objects */
  Object* firstObject;

  /* GC Stats */
  int numObjects;   // Total objects currently allocated
  int maxObjects;   // Threshold to trigger the next GC
} VM; 

/* Global VM instance used by the runtime */
VM vm;

void mark(Object* object);
void MarkAll();

void MarkAll(){
    for(int i = 0; i < vm.stackSize; i++){
        mark(vm.stack[i]);
    }
}

void mark(Object* object) {
    // stop if the object is NULL
    if(object == NULL) return; 

    if(object->marked) return; // already marked

    object->marked = 1;

    if(object->type == OBJ_PAIR){
        mark(object->head);
        mark(object->tail);
    }
}

int main(){
    // Main function implementation
    return 0;
}