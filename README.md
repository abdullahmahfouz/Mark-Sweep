# C Mark-and-Sweep Garbage Collector

A custom implementation of a Virtual Machine (VM) memory manager with a Mark-and-Sweep Garbage Collector. This project demonstrates low-level understanding of language runtimes, memory management lifecycles, and pointer manipulation in C.

## 🚀 Key Features

* **Mark-and-Sweep Algorithm**: Implements a two-phase garbage collection system:
    * **Mark Phase**: Uses recursive DFS to traverse object graphs starting from the VM stack (roots).
    * **Sweep Phase**: Iterates through the heap to reclaim memory from unreachable objects (white objects) while resetting flags on survivors.
* **Cycle Detection**: Capable of collecting circular references (e.g., Object A -> Object B -> Object A) which Reference Counting algorithms fail to handle.
* **Dynamic Heap Growth**: Automatically triggers GC when the heap limit is reached and dynamically doubles heap size to accommodate growing workloads.
* **VM Simulation**: Simulates a stack-based virtual machine with support for Integers and nested Object Pairs.

## 🛠️ Technical Implementation

The system uses a `struct` based object model with a tagged union for type safety:

```c
typedef struct sObject {
    ObjectType type;
    unsigned char marked; // GC Mark Bit
    struct sObject* next; // Heap Linked List

    union {
        int value;        // For Integers
        struct {          // For Pairs
            struct sObject* head;
            struct sObject* tail;
        };
    };
} Object;
