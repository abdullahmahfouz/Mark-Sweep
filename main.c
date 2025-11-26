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
void sweep ();
void gc();
Object* newObject(ObjectType type);
void pushInt(int intValue);
Object* pushPair();

int main() {
    
    return 0;
}
/**
 * MarkAll - Mark phase entry point for garbage collection
 * 
 * Iterates through all objects currently on the VM stack 
 * and marks them as reachable. This is the first phase of the mark-and-sweep
 * garbage collection algorithm.
 * 
 * The stack contains all objects that are directly accessible by the program,
 * and marking them recursively marks all objects reachable from them.
 */
void MarkAll(){
    // Loop through each object on the stack
    for(int i = 0; i < vm.stackSize; i++){
        mark(vm.stack[i]); // Mark this object and anything it references
    }
}

/**
 * mark - Recursively marks an object and its references as reachable
 * @object: Pointer to the object to mark
 * 
 * This function marks an object as "reachable" during garbage collection.
 * If the object is a pair (contains pointers to other objects), it recursively
 * marks those objects as well. This ensures all objects in use are preserved.
 * 
 * The marking process:
 * 1. Skip if object is NULL (nothing to mark)
 * 2. Skip if already marked (prevents infinite loops in circular references)
 * 3. Set marked flag to 1 (mark as reachable)
 * 4. If it's a pair, recursively mark both head and tail objects
 */
void mark(Object* object) {
    // Stop if the object is NULL (no object to mark)
    if(object == NULL) return; 

    // If already marked, skip it (prevents infinite recursion)
    if(object->marked) return;

    // Mark this object as reachable
    object->marked = 1;

    // If this is a pair, recursively mark the objects it points to
    if(object->type == OBJ_PAIR){
        mark(object->head); // Mark the first element
        mark(object->tail); // Mark the second element
    }
    // OBJ_INT types don't have references, so nothing more to do
}

/**
 * sweep - Sweep phase of garbage collection
 * 
 * Walks through the linked list of all allocated objects and:
 * 1. Frees objects that were NOT marked (unreachable/garbage)
 * 2. Resets the marked flag to 0 for objects that were marked (prepare for next GC)
 * 
 * This is the second phase of mark-and-sweep garbage collection.
 * After marking all reachable objects, this function removes everything else.
 */
void sweep (){
    Object** object = &vm.firstObject; // Start at the beginning of the linked list
    
    while(*object){
        if(!(*object)->marked){ // If object was NOT marked (it's garbage)
            Object* unreached = *object; // Save pointer to unreachable object

            *object = unreached->next; // Remove it from the linked list
            free(unreached); // Free the memory
            vm.numObjects--; // Decrease object count
        } else { // Object was marked (it's still in use)
            (*object)->marked = 0; // Reset the mark for next GC cycle
            object = &(*object)->next; // Move to next object in list
        }
    }
}

/**
 * gc - Main garbage collection function
 * 
 * Performs a full mark-and-sweep garbage collection cycle:
 * 1. Mark all reachable objects (starting from stack roots)
 * 2. Sweep away unmarked objects (garbage)
 * 3. Adjust the threshold for the next GC trigger
 * 4. Print statistics about the collection
 * 
 * The threshold is doubled based on remaining objects to reduce GC frequency
 * as the program grows, improving performance.
 */
void gc(){
    int numObjectsBefore = vm.numObjects; // Save count before GC

    MarkAll(); // Phase 1: Mark all reachable objects
    sweep();   // Phase 2: Free unmarked objects

   // Adjust the threshold for next GC trigger
   if(vm.numObjects == 0){
        vm.maxObjects = INITIAL_GC_THRESHOLD; // Reset to initial if all collected
   } else {
        vm.maxObjects = vm.numObjects * 2; // Double the current count
   }
   
   // Print GC statistics
   printf("Collected %d objects, %d remaining.\n", numObjectsBefore - vm.numObjects, vm.numObjects);
}

/**
 * newObject - Allocates a new object on the heap
 * @type: The type of object to create (OBJ_INT or OBJ_PAIR)
 * 
 * Creates a new object and adds it to the VM's object list.
 * If the number of objects reaches the GC threshold, triggers
 * garbage collection before allocating.
 * 
 * The new object is:
 * - Allocated on the heap with malloc
 * - Added to the front of the linked list
 * - Initially unmarked (marked = 0)
 * 
 * Return: Pointer to the newly created object
 */
Object* newObject(ObjectType type){
    // Check if we've hit the GC threshold
    if(vm.numObjects == vm.maxObjects){
        gc(); // Trigger garbage collection
    }

    // Allocate memory for the new object
    Object* object = (Object*)malloc(sizeof(Object));
    object->type = type;   // Set the object type
    object->marked = 0;    // Initially unmarked

    // Add to the front of the linked list
    object->next = vm.firstObject;
    vm.firstObject = object;

    vm.numObjects++; // Increment object count
    return object;   // Return pointer to new object
}

/**
 * pushInt - Creates an integer object and pushes it onto the stack
 * @intValue: The integer value to store in the new object
 * 
 * Creates a new OBJ_INT object with the given value and adds it to the VM stack.
 * This makes the object a "root" that will be considered reachable during GC.
 * 
 * Checks for stack overflow before pushing. If the stack is full (STACK_MAX items),
 * prints an error and exits the program.
 */
void pushInt(int intValue) {
    Object* object = newObject(OBJ_INT); // Allocate new integer object
    object->value = intValue; // Set its value
    
    // Check if stack is full
    if (vm.stackSize >= STACK_MAX) {
        printf("Stack Overflow!\n");
        exit(1);
    }
    
    // Push object onto the stack
    vm.stack[vm.stackSize++] = object;
}

/**
 * pushPair - Creates a pair object from the top two stack items
 * 
 * Pops the last two objects from the stack and creates a new OBJ_PAIR
 * that contains them as head and tail. The new pair is then pushed back
 * onto the stack.
 * 
 * This allows building complex data structures (like lists or trees) by
 * combining existing objects into pairs.
 * 
 * Stack before: [... A, B]
 * Stack after:  [... Pair(A, B)]
 * 
 * Note: In a production implementation, this should check for stack underflow
 * (ensuring at least 2 items exist before popping).
 * 
 * Return: Pointer to the newly created pair object
 */
Object* pushPair() {
    Object* object = newObject(OBJ_PAIR); // Allocate new pair object
    
    /* Pop the last two things on stack to be children */
    /* Note: In a real language, you'd check for stack underflow here */
    object->tail = vm.stack[--vm.stackSize]; // Pop second item (tail)
    object->head = vm.stack[--vm.stackSize]; // Pop first item (head)

    /* Push the new pair back onto the stack */
    vm.stack[vm.stackSize++] = object;
    return object;
}





