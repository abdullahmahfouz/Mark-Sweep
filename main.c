#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum{
    OBJ_TRUE, // Represents the boolean value true
    OBJ_FALSE // Represents the boolean value false
} ObjectType;

typedef struct sObject {
    ObjectType type; // store the type of the object
    unsigned char marked; // for garbage collection
    struct sObject* next; // pointer to the next object in memory
    union{
        int value; // for OBJ_TRUE and OBJ_FALSE, we can use an integer value
        struct{
            struct sObject* head; // pointer to the head of the list
            struct sObject* tail; // pointer to the tail of the list
        };
    };

} Object;

#define STACK_MAX 256 // Maximum size of the stack
#define INITIAL_GC_THRESHOLD 8

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


