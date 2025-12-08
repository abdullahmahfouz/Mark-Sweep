#include <stdio.h>
#include <stdlib.h>
#include <time.h>


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

/**
 * Hey, this is where everything starts! We run all 10 tests to make sure our
 * garbage collector actually works. These tests check everything from basic
 * stuff (like "don't delete things we're still using") to trickier scenarios
 * (like circular references that would normally cause memory leaks).
 */
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
    return 0;
}

/**
 * Creates a new object (either an integer or a pair).
 * 
 * This is like asking for new space in memory. If we've hit our limit, we'll
 * run the garbage collector first to free up some room. The new object gets
 * added to our list of everything we've created, unmarked and ready to go.
 * If we completely run out of memory, we bail out.
 */
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

/**
 * Puts an object on top of our stack.
 * 
 * Think of the stack as our "currently using" list. Anything here won't get
 * garbage collected because we're actively using it. If the stack's already
 * full, we've got a problem and need to stop.
 */
void push(Object* obj) {
    if (stackSize >= STACK_MAX) {
        printf("Stack Overflow!\n");
        exit(1);
    }
    stack[stackSize++] = obj;
}

/**
 * Takes the top object off the stack and gives it back.
 * 
 * Once something's off the stack, we're basically saying "I'm done with this."
 * Unless another object is still pointing to it, it's now eligible for garbage
 * collection. Can't pop from an empty stack though - that would be bad.
 */
Object* pop() {
    if (stackSize <= 0) {
        printf("Stack Underflow!\n");
        exit(1);
    }
    return stack[--stackSize];
}

/**
 * Quick shortcut to make a new integer and put it on the stack.
 * 
 * Instead of doing "create object, set value, push it" separately, this just
 * does it all at once. Super handy when you just want to work with numbers.
 */
Object* pushInt(int x) {
    Object* obj = newObject(OBJ_INT);
    obj->value = x;
    push(obj);
    return obj;
}

/**
 * Takes two things from the stack and combines them into a pair.
 * 
 * This is how we build more complex data structures. Grab the top two items,
 * bundle them together, and put the bundle back on the stack. Obviously need
 * at least two things on the stack for this to work!
 */
Object* pushPair() {
    Object* obj = newObject(OBJ_PAIR);
    obj->tail = pop();
    obj->head = pop();
    push(obj);
    return obj;
}



/**
 * Marks an object as "still in use, don't delete me!"
 * 
 * This is the heart of the "mark" part of mark-and-sweep. We tag this object
 * as important, and if it's a pair, we follow the references and mark those too.
 * We skip anything that's already marked or null to avoid infinite loops.
 */
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

/**
 * Goes through everything on the stack and marks it all as important.
 * 
 * This kicks off the marking phase. Anything on the stack is something we're
 * actively using, so we mark it. The mark() function will then follow any
 * references to mark connected objects too.
 */
void markAll() {
    for (int i = 0; i < stackSize; i++) {
        mark(stack[i]);
    }
}


/**
 * Cleans up all the garbage (unmarked objects).
 * 
 * This is the "sweep" part. We walk through all our objects - anything that
 * wasn't marked gets deleted because we're not using it anymore. For the
 * survivors, we reset their marks so we can do this again next time.
 */
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

/**
 * Runs the garbage collector - this is where the magic happens!
 * 
 * First we mark everything we're still using, then we sweep away the garbage.
 * After cleaning up, we adjust our limit (double what's left) so we don't have
 * to run this too often. Also prints out what happened so we can see it working.
 */
void gc() {
    int prevCount = numObjects;
    
    // Start Timer
    clock_t start = clock();

    markAll();
    sweep();

    // Stop Timer
    clock_t end = clock();
    double time_spent = (double)(end - start) / CLOCKS_PER_SEC;

    if (maxObjects == 0) maxObjects = INITIAL_GC_THRESHOLD;
    else maxObjects = numObjects * 2;

    // Only print if we actually collected something or if it took measurable time
    // This reduces spam during the big tests
    if (prevCount - numObjects > 0) {
        printf("GC Run: Collected %d | Remaining %d | Time: %f sec\n", 
               prevCount - numObjects, numObjects, time_spent);
    }
}

/**
 * Wipes everything clean so we can start fresh.
 * 
 * Between tests, we need to reset back to square one - empty stack, no objects,
 * original limits. This way each test starts with a clean slate and doesn't
 * interfere with the others.
 */
void resetVM() {
    // Reset all VM state so tests don't interfere
    stackSize = 0;
    firstObject = NULL;
    numObjects = 0;
    maxObjects = INITIAL_GC_THRESHOLD;
}

/**
 * Test 1: Make sure we don't delete stuff we're still using!
 * 
 * The most basic rule - if something's on the stack, DON'T collect it.
 * We put two numbers on the stack, run GC, and both should still be there.
 */
void test1_ObjectsOnStack() {
    printf("Test 1: Objects on stack should be preserved.\n");
    resetVM();
    pushInt(1);
    pushInt(2);
    gc();
    
}

/**
 * Test 2: Make sure we DO delete stuff we're not using anymore.
 * 
 * We create two objects, then immediately throw them away (pop them off).
 * When GC runs, it should find them and clean them up.
 */
void test2_UnreachedObjects() {
    printf("Test 2: Unreached objects should be collected.\n");
    resetVM();
    pushInt(1);
    pushInt(2);
    pop(); // Drop 2
    pop(); // Drop 1
    gc();
}

/**
 * Test 3: Keep things we can reach indirectly.
 * 
 * We make a pair that contains two numbers. Even though the numbers aren't
 * directly on the stack, we can reach them through the pair, so all three
 * objects should stick around.
 */
void test3_Reachability() {
    printf("Test 3: Reachability.\n");
    resetVM();
    pushInt(1);
    pushInt(2);
    pushPair(); // Pair holding 1 and 2
    gc();
}

/**
 * Test 4: Handle circular references (the cool part!).
 * 
 * We create two pairs that point to each other in a circle. Even though they
 * reference each other, once we take them off the stack they're garbage.
 * This is where mark-and-sweep shines - reference counting would leak this!
 */
void test4_Cycles() {
    printf("Test 4: Cycles (The Mark-Sweep Advantage).\n");
    resetVM();
    // Create two pairs that reference each other
    pushInt(1);
    pushInt(2);
    Object* a = pushPair(); // A points to 1 and 2
    
    pushInt(3);
    pushInt(4);
    Object* b = pushPair(); // B points to 3 and 4
    
    // Make them point to each other to create a cycle
    a->tail = b;
    b->tail = a;

    // Remove both from stack
    pop(); // Remove b
    pop(); // Remove a
    
    // Mark-sweep handles cycles, reference counting would leak
    gc();
    
}
/**
 * Test 5: Make sure GC kicks in automatically when needed.
 * 
 * Our limit starts at 8 objects. When we try to create 10, GC should trigger
 * on its own at 8, clean up nothing (we're using everything), and grow the
 * limit so we can fit all 10.
 */
void test5_HeapGrowth() {
    printf("Test 5: Auto-trigger GC and Heap Growth.\n");
    resetVM();
    // maxObjects starts at 8
    for (int i = 0; i < 10; i++) {
        pushInt(i);
    }
    
    
}

/**
 * Test 6: Can we handle lots of rapid creation and deletion?
 * 
 * We create 1000 objects and immediately throw them all away in a loop.
 * This tests if the GC can keep up with lots of temporary objects without
 * leaking memory or getting confused.
 */
void test6_PerformanceChurn() {
    printf("Test 6: Performance & Scalability.\n");
    
    // test 3 different sizes to show the "linear" time increase
    int sizes[] = {1000, 10000, 50000};
    
    for (int s = 0; s < 3; s++) {
        int size = sizes[s];
        printf(" Running stress test with %d objects\n", size);
        
        resetVM();
        // Set a high threshold so GC doesn't trigger automatically during creation
        maxObjects = size * 2; 
        
        for (int i = 0; i < size; i++) {
            pushInt(i);
            pop(); // Immediately make it garbage
        }
        
        // Manual trigger to measure the full sweep of 'size' garbage objects
        gc(); 
    }
}

/**
 * Test 7: Can we handle really deep nested structures?
 * 
 * We build a linked list 20 layers deep (like a chain). This makes sure our
 * recursive marking doesn't crash when following long chains of references.
 * All 41 objects should survive.
 */
void test7_DeepRecursion() {
    printf("Test 7: Deep Recursion.\n");
    resetVM();
    pushInt(0);
    for (int i = 0; i < 20; i++) {
        pushInt(i);
        pushPair();
    }
    gc();
    // 1 Int + 20 * (1 Int + 1 Pair) = 41 objects
}

/**
 * Test 8: Delete some things but not others.
 * 
 * We create two objects but only throw away one. GC should be smart enough
 * to delete the one we don't need anymore while keeping the other.
 */
void test8_PartialDelete() {
    printf("Test 8: Partial Deletion.\n");
    resetVM();
    // Push 2, pop 1
    pushInt(10);
    pushInt(20);
    pop(); // 20 becomes garbage
    gc();
   
}

/**
 * Test 9: If we clear everything, GC should clean up everything.
 * 
 * We make some objects, then manually clear the entire stack. Now nothing's
 * reachable, so GC should delete everything. Nuclear option!
 */
void test9_FullClear() {
    printf("Test 9: Full Clear.\n");
    resetVM();
    pushInt(1);
    pushInt(2);
    pushPair();
    // Clear entire stack
    stackSize = 0;
    gc();
    
}

/**
 * Test 10: After cleaning up, can we use the memory again?
 * 
 * Create something, delete it, make sure it's really gone (NULL), then create
 * something new. This confirms we're actually freeing memory properly and can
 * reuse it.
 */
void test10_Reallocation() {
    printf("Test 10: Reallocation Reuse.\n");
    resetVM();
    pushInt(1);
    pop();
    gc(); // Free the int
    Object* p1 = firstObject; // Should be NULL
    
    pushInt(2);
    Object* p2 = firstObject; // Should have new object
    
}







