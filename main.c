#include <stdio.h>
#include <stdlib.h>

/* --- 1. DATA STRUCTURES --- */

typedef enum {
    OBJ_INT,
    OBJ_PAIR
} ObjectType;

typedef struct sObject {
    ObjectType type;
    unsigned char marked;
    struct sObject* next; // The internal link for the "Sweep" phase

    union {
        int value; // For Integers
        struct {   // For Pairs
            struct sObject* head;
            struct sObject* tail;
        };
    };
} Object;

#define STACK_MAX 256
#define INITIAL_GC_THRESHOLD 8

/* Global VM State */
Object* stack[STACK_MAX];
int stackSize = 0;

Object* firstObject = NULL; // Head of the linked list of all objects
int numObjects = 0;
int maxObjects = INITIAL_GC_THRESHOLD;


/* --- 2. HELPER FUNCTIONS --- */

/* Forward declarations */
void gc(void);
void test1_ObjectsOnStack(void);
void test2_UnreachedObjects(void);
void test3_Reachability(void);
void test4_Cycles(void);
void test5_HeapGrowth(void);
void test6_PerformanceChurn(void);
void test7_DeepRecursion(void);
void test8_PartialDelete(void);
void test9_FullClear(void);
void test10_Reallocation(void);

// Main - Runs all 10 garbage collection tests
int main() {
    test1_ObjectsOnStack();
    test2_UnreachedObjects();
    test3_Reachability();
    test4_Cycles();
    test5_HeapGrowth();
    test6_PerformanceChurn();
    test7_DeepRecursion();
    test8_PartialDelete();
    test9_FullClear();
    test10_Reallocation();
    
    printf("All tests complete.\n");
    return 0;
}
// newObject - Make a new object (int or pair)
// Triggers GC if we hit the limit. Adds object to the heap list.
Object* newObject(ObjectType type) {
    // Run GC if we've reached max objects
    if (numObjects == maxObjects) {
        gc();
    }

    // Allocate memory
    Object* object = malloc(sizeof(Object));
    if (object == NULL) {
        printf("Out of memory!\n");
        exit(1);
    }

    object->type = type;
    object->marked = 0; // Starts unmarked

    // Add to linked list of all objects
    object->next = firstObject;
    firstObject = object;
    numObjects++;

    return object;
}

// push - Put an object on the stack
void push(Object* obj) {
    if (stackSize >= STACK_MAX) {
        printf("Stack Overflow!\n");
        exit(1);
    }
    stack[stackSize++] = obj;
}

// pop - Remove and return top object from stack
Object* pop() {
    if (stackSize <= 0) {
        printf("Stack Underflow!\n");
        exit(1);
    }
    return stack[--stackSize];
}

// pushInt - Create an integer object and push it onto the stack
Object* pushInt(int x) {
    Object* obj = newObject(OBJ_INT);
    obj->value = x;
    push(obj);
    return obj;
}

// pushPair - Pop 2 objects, make a pair from them, push pair back
Object* pushPair() {
    Object* obj = newObject(OBJ_PAIR);
    obj->tail = pop();
    obj->head = pop();
    push(obj);
    return obj;
}

/* --- 3. MARK PHASE (Algorithm 2.2) --- */

// mark - Mark this object as reachable
// If it's a pair, recursively mark what it points to
void mark(Object* object) {
    // Skip if null or already marked (avoids infinite loops)
    if (object == NULL || object->marked) return;

    // Mark it
    object->marked = 1;

    // If pair, mark both parts
    if (object->type == OBJ_PAIR) {
        mark(object->head);
        mark(object->tail);
    }
}

// markAll - Mark everything on the stack (the "roots")
void markAll() {
    for (int i = 0; i < stackSize; i++) {
        mark(stack[i]);
    }
}

/* --- 4. SWEEP PHASE (Algorithm 2.3) --- */

// sweep - Walk the heap and free unmarked objects
// Reset marked flag on objects that survive
void sweep() {
    Object** object = &firstObject;
    while (*object) {
        if (!(*object)->marked) {
            // Not marked = garbage, free it
            Object* unreached = *object;
            *object = unreached->next;
            free(unreached);
            numObjects--;
        } else {
            // Marked = alive, reset flag for next GC
            (*object)->marked = 0;
            object = &(*object)->next;
        }
    }
}

// gc - Run garbage collection (mark and sweep)
// Then grow the heap limit based on what's left
void gc() {
    int prevCount = numObjects;
    markAll(); // Mark reachable objects
    sweep();   // Free unreachable objects

    // Grow heap limit (double current size)
    if (maxObjects == 0) maxObjects = INITIAL_GC_THRESHOLD;
    else maxObjects = numObjects * 2;

    printf("-- GC Run: Collected %d, Remaining %d\n", prevCount - numObjects, numObjects);
}

/* --- 5. TEST SUITE (10 Cases) --- */

// resetVM - Clear everything between tests
void resetVM() {
    // Reset all VM state so tests don't interfere
    stackSize = 0;
    firstObject = NULL;
    numObjects = 0;
    maxObjects = INITIAL_GC_THRESHOLD;
}

// test1 - Objects on stack should survive GC
void test1_ObjectsOnStack() {
    printf("Test 1: Objects on stack should be preserved.\n");
    resetVM();
    pushInt(1);
    pushInt(2);
    gc();
    if (numObjects == 2) printf("PASS\n"); else printf("FAIL: %d != 2\n", numObjects);
}

// test2 - Objects not on stack should be collected
void test2_UnreachedObjects() {
    printf("Test 2: Unreached objects should be collected.\n");
    resetVM();
    pushInt(1);
    pushInt(2);
    pop(); // Drop 2
    pop(); // Drop 1
    gc();
    if (numObjects == 0) printf("PASS\n"); else printf("FAIL: %d != 0\n", numObjects);
}

// test3 - Nested objects (pair with ints) should all survive
void test3_Reachability() {
    printf("Test 3: Reachability (Nested objects).\n");
    resetVM();
    pushInt(1);
    pushInt(2);
    pushPair(); // Pair holding 1 and 2
    gc();
    // Should have 3 objects: The Pair, Int(1), Int(2)
    if (numObjects == 3) printf("PASS\n"); else printf("FAIL: %d != 3\n", numObjects);
}

// test4 - Circular references should be collected when unreachable
void test4_Cycles() {
    printf("Test 4: Cycles (The Mark-Sweep Advantage).\n");
    resetVM();
    // Create A -> B and B -> A
    pushInt(1);
    pushInt(2);
    Object* a = pushPair();
    Object* b = pushPair();
    
    // Make them point to each other
    a->tail = b;
    b->tail = a;

    // Remove from stack
    pop();
    pop();
    
    // Mark-sweep handles cycles, reference counting would leak
    gc();
    if (numObjects == 0) printf("PASS\n"); else printf("FAIL: Cycle leaked %d objects\n", numObjects);
}

// test5 - GC should auto-trigger and heap should grow
void test5_HeapGrowth() {
    printf("Test 5: Auto-trigger GC and Heap Growth.\n");
    resetVM();
    // maxObjects starts at 8
    for (int i = 0; i < 10; i++) {
        pushInt(i);
    }
    // Pushed 10, GC runs at 8, heap grows
    if (numObjects == 10 && maxObjects > 8) printf("PASS\n"); else printf("FAIL\n");
}

// test6 - Lots of temporary objects should all get collected
void test6_PerformanceChurn() {
    printf("Test 6: Performance (Allocate/Free churn).\n");
    resetVM();
    // Make and drop 1000 objects
    for (int i = 0; i < 1000; i++) {
        pushInt(i);
        pop();
    }
    gc();
    if (numObjects == 0) printf("PASS\n"); else printf("FAIL\n");
}

// test7 - Deep nested structure (linked list) should all survive
void test7_DeepRecursion() {
    printf("Test 7: Deep Recursion (Linked List).\n");
    resetVM();
    pushInt(0);
    for (int i = 0; i < 20; i++) {
        pushInt(i);
        pushPair();
    }
    gc();
    // 1 Int + 20 * (1 Int + 1 Pair) = 41 objects
    if (numObjects == 41) printf("PASS\n"); else printf("FAIL: %d\n", numObjects);
}

// test8 - Only popped objects should be collected
void test8_PartialDelete() {
    printf("Test 8: Partial Deletion.\n");
    resetVM();
    // Push 2, pop 1
    pushInt(10);
    pushInt(20);
    pop(); // 20 becomes garbage
    gc();
    if (numObjects == 1) printf("PASS\n"); else printf("FAIL\n");
}

// test9 - Clearing the stack should let GC collect everything
void test9_FullClear() {
    printf("Test 9: Full Clear.\n");
    resetVM();
    pushInt(1);
    pushPair();
    // Clear entire stack
    stackSize = 0;
    gc();
    if (numObjects == 0) printf("PASS\n"); else printf("FAIL\n");
}

// test10 - Memory should be reusable after GC
void test10_Reallocation() {
    printf("Test 10: Reallocation Reuse.\n");
    resetVM();
    pushInt(1);
    pop();
    gc(); // Free the int
    Object* p1 = firstObject; // Should be NULL
    
    pushInt(2);
    Object* p2 = firstObject; // Should have new object
    
    // Check we got memory back
    if (p1 == NULL && p2 != NULL) printf("PASS\n"); else printf("FAIL\n");
}







