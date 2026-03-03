====================
Memory Management
====================

Introduction
============

**Memory management** in RTOS systems involves careful allocation, tracking, and protection of RAM resources. Unlike desktop systems with virtual memory and gigabytes of RAM, embedded systems often have:

- Limited RAM (often kilobytes, not megabytes)
- No virtual memory or memory protection
- Deterministic memory requirements
- Long-term reliability needs (days, months, years of operation)

This guide covers heap allocation schemes, stack management, memory pools, and best practices.

Memory Layout
=============

Typical Embedded Memory Map
---------------------------

.. code-block:: text

    HIGH ADDRESS
    ┌──────────────────┐
    │   Stack (Task N) │ ← Grows down
    ├──────────────────┤
    │        ...       │
    ├──────────────────┤
    │   Stack (Task 1) │ ← Grows down
    ├──────────────────┤
    │   Heap (dynamic) │ ← Grows up
    ├──────────────────┤
    │   BSS (zeroed)   │ ← Uninitialized globals
    ├──────────────────┤
    │   Data (init)    │ ← Initialized globals
    ├──────────────────┤
    │   Text (code)    │ ← Program instructions
    └──────────────────┘
    LOW ADDRESS

Sections
--------

- **Text**: Program code (Flash/ROM)
- **Data**: Initialized global/static variables (RAM)
- **BSS**: Zero-initialized global/static variables (RAM)
- **Heap**: Dynamic allocations (RAM)
- **Stack**: Function calls, local variables (RAM)

.. code-block:: c

    // Different memory sections
    const int rom_var = 100;        // .text (Flash)
    int data_var = 42;              // .data (RAM, initialized)
    int bss_var;                    // .bss (RAM, zero-init)
    static int static_var = 10;     // .data
    
    void function(void) {
        int local_var;              // Stack
        static int func_static = 5; // .data
        int *heap_var = malloc(4);  // Heap
    }

Stack Management
================

Task Stacks
-----------

Each task requires its own stack for:
- Local variables
- Function call return addresses
- CPU register context
- Interrupt context (if interrupted)

.. code-block:: c

    // FreeRTOS: Stack size in words (4 bytes on ARM Cortex-M)
    #define STACK_SIZE_SMALL   128   // 512 bytes
    #define STACK_SIZE_MEDIUM  256   // 1 KB
    #define STACK_SIZE_LARGE   512   // 2 KB
    
    xTaskCreate(
        small_task,       // Task function
        "Small",          // Name
        STACK_SIZE_SMALL, // Stack size
        NULL,             // Parameters
        1,                // Priority
        NULL              // Task handle
    );

Stack Size Determination
------------------------

**Methods:**

1. **Static Analysis**: Calculate maximum call depth and local variables
2. **Runtime Measurement**: Use stack watermarking
3. **Conservative Estimation**: Oversize and measure

.. code-block:: c

    // Stack watermarking (FreeRTOS example)
    void monitor_task(void *param) {
        while (1) {
            // Get minimum free stack space (watermark)
            UBaseType_t stack_free = uxTaskGetStackHighWaterMark(NULL);
            
            printf("Stack free: %u words\n", (unsigned)stack_free);
            
            if (stack_free < 50) {
                // Warning: stack nearly full!
            }
            
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }

Stack Overflow Detection
------------------------

**Method 1: Stack Canary** (FreeRTOS)

.. code-block:: c

    // FreeRTOSConfig.h
    #define configCHECK_FOR_STACK_OVERFLOW  2  // Method 2 (canary checking)
    
    // Hook function
    void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName) {
        // Task stack overflow detected!
        printf("Stack overflow in task: %s\n", pcTaskName);
        // Handle error (log, reset, enter safe mode)
        while (1);  // Halt for debugging
    }

**Method 2: MPU Protection** (ARM Cortex-M with MPU)

.. code-block:: c

    // Configure MPU region for stack guard
    void configure_stack_protection(void *stack_base, size_t stack_size) {
        // Create no-access guard page below stack
        MPU->RBAR = (uint32_t)stack_base | MPU_RBAR_VALID | 0;
        MPU->RASR = MPU_RASR_ENABLE 
                  | MPU_RASR_SIZE_256  // Guard page size
                  | MPU_NO_ACCESS;     // No read/write
        
        // Any access to guard triggers MemManage fault
    }

Dynamic Memory Allocation
=========================

Standard malloc/free
--------------------

**Problems for RTOS:**
- Non-deterministic timing
- Fragmentation over time
- Not thread-safe by default
- Can fail unpredictably

.. code-block:: c

    // Standard malloc - generally NOT recommended for real-time
    void *ptr = malloc(256);
    if (ptr == NULL) {
        // Out of memory - what now?
    }
    // Use ptr...
    free(ptr);  // May cause fragmentation

RTOS Heap Schemes
=================

FreeRTOS provides multiple heap implementations with different trade-offs.

Heap_1: Static Allocation
--------------------------

**Characteristics:**
- Allocation only (no free)
- Zero fragmentation
- Simplest and fastest
- For static task creation

.. code-block:: c

    // heap_1: Allocation from fixed array
    static uint8_t heap[10240];  // 10KB heap
    
    void *pvPortMalloc(size_t size) {
        static size_t next_free = 0;
        void *ret = NULL;
        
        if (next_free + size < sizeof(heap)) {
            ret = &heap[next_free];
            next_free += size;
        }
        
        return ret;
    }
    
    void vPortFree(void *ptr) {
        // Does nothing
    }

**Use case**: Systems where all tasks/objects created at startup.

Heap_2: Best-Fit Allocation
----------------------------

**Characteristics:**
- Allocation and free supported
- Best-fit algorithm
- Can fragment
- Not deterministic

.. code-block:: c

    // Use like malloc/free
    void *ptr = pvPortMalloc(100);
    // ...
    vPortFree(ptr);

**Use case**: Applications with moderate dynamic allocation needs.

Heap_3: Wrapped malloc/free
----------------------------

**Characteristics:**
- Uses standard library malloc/free
- Thread-safety added
- Timing depends on libc implementation

.. code-block:: c

    // Suspends scheduler during allocation
    void *pvPortMalloc(size_t size) {
        vTaskSuspendAll();
        void *ptr = malloc(size);
        xTaskResumeAll();
        return ptr;
    }

**Use case**: When you must use standard malloc (for compatibility).

Heap_4: First-Fit with Coalescing
----------------------------------

**Characteristics:**
- Allocation and free
- Adjacent free blocks coalesce
- Reduces fragmentation
- Reasonably deterministic

.. code-block:: c

    // FreeRTOS heap_4 - recommended for most applications
    #define configTOTAL_HEAP_SIZE  (20 * 1024)  // 20KB
    
    void example(void) {
        void *p1 = pvPortMalloc(100);
        void *p2 = pvPortMalloc(200);
        vPortFree(p1);
        vPortFree(p2);
        // p1 and p2 regions coalesce into one free block
    }

**Use case**: General-purpose applications with dynamic allocation.

Heap_5: Multi-Region Support
-----------------------------

**Characteristics:**
- Like heap_4 but supports multiple memory regions
- Useful for systems with separate RAM banks

.. code-block:: c

    // Define memory regions
    const HeapRegion_t xHeapRegions[] = {
        { (uint8_t *)0x20000000, 0x8000 },  // 32KB internal RAM
        { (uint8_t *)0x60000000, 0x40000 }, // 256KB external RAM
        { NULL, 0 }                         // Terminator
    };
    
    void setup_heap(void) {
        vPortDefineHeapRegions(xHeapRegions);
    }

**Use case**: Systems with multiple RAM regions (internal + external).

Heap Monitoring
---------------

.. code-block:: c

    void check_heap_status(void) {
        size_t free_heap = xPortGetFreeHeapSize();
        size_t min_ever = xPortGetMinimumEverFreeHeapSize();
        
        printf("Heap free: %u bytes\n", (unsigned)free_heap);
        printf("Minimum ever free: %u bytes\n", (unsigned)min_ever);
        
        if (min_ever < 1024) {
            // Warning: heap nearly exhausted
        }
    }

Memory Pools
============

**Memory pools** provide fixed-size block allocation with:
- O(1) constant-time allocation/free
- Zero fragmentation
- Deterministic behavior
- Perfect for real-time systems

Implementation
--------------

.. code-block:: c

    #define POOL_SIZE 10
    #define BLOCK_SIZE 64
    
    typedef struct {
        uint8_t blocks[POOL_SIZE][BLOCK_SIZE];
        bool allocated[POOL_SIZE];
        SemaphoreHandle_t mutex;
    } memory_pool_t;
    
    memory_pool_t pool;
    
    void pool_init(memory_pool_t *p) {
        memset(p->allocated, 0, sizeof(p->allocated));
        p->mutex = xSemaphoreCreateMutex();
    }
    
    void *pool_alloc(memory_pool_t *p) {
        void *ptr = NULL;
        
        xSemaphoreTake(p->mutex, portMAX_DELAY);
        
        for (int i = 0; i < POOL_SIZE; i++) {
            if (!p->allocated[i]) {
                p->allocated[i] = true;
                ptr = p->blocks[i];
                break;
            }
        }
        
        xSemaphoreGive(p->mutex);
        return ptr;
    }
    
    void pool_free(memory_pool_t *p, void *ptr) {
        xSemaphoreTake(p->mutex, portMAX_DELAY);
        
        for (int i = 0; i < POOL_SIZE; i++) {
            if (p->blocks[i] == ptr) {
                p->allocated[i] = false;
                break;
            }
        }
        
        xSemaphoreGive(p->mutex);
    }

Using Memory Pools
------------------

.. code-block:: c

    // Network packet pool example
    typedef struct {
        uint8_t data[1500];  // MTU size
        uint16_t length;
        uint32_t timestamp;
    } packet_t;
    
    #define PACKET_POOL_SIZE 20
    packet_t packet_pool[PACKET_POOL_SIZE];
    QueueHandle_t free_packets;
    
    void init_packet_pool(void) {
        free_packets = xQueueCreate(PACKET_POOL_SIZE, sizeof(packet_t *));
        
        // Add all packets to free list
        for (int i = 0; i < PACKET_POOL_SIZE; i++) {
            packet_t *pkt = &packet_pool[i];
            xQueueSend(free_packets, &pkt, 0);
        }
    }
    
    packet_t *packet_alloc(void) {
        packet_t *pkt = NULL;
        xQueueReceive(free_packets, &pkt, pdMS_TO_TICKS(100));
        return pkt;
    }
    
    void packet_free(packet_t *pkt) {
        xQueueSend(free_packets, &pkt, 0);
    }

Memory Protection
=================

MPU (Memory Protection Unit)
-----------------------------

ARM Cortex-M3/M4/M7 with MPU can provide:
- Stack overflow protection
- Memory region access control
- Protection between tasks

.. code-block:: c

    // Configure MPU region
    void configure_task_memory_protection(void) {
        // Region 0: Task stack (RW access)
        MPU->RBAR = TASK_STACK_BASE | MPU_RBAR_VALID | 0;
        MPU->RASR = MPU_RASR_ENABLE
                  | MPU_RASR_SIZE_4KB
                  | MPU_RW_ACCESS;
        
        // Region 1: Task data (RW access)
        MPU->RBAR = TASK_DATA_BASE | MPU_RBAR_VALID | 1;
        MPU->RASR = MPU_RASR_ENABLE
                  | MPU_RASR_SIZE_16KB
                  | MPU_RW_ACCESS;
        
        // Region 2: Shared peripheral (read-only)
        MPU->RBAR = PERIPHERAL_BASE | MPU_RBAR_VALID | 2;
        MPU->RASR = MPU_RASR_ENABLE
                  | MPU_RASR_SIZE_1KB
                  | MPU_RO_ACCESS;
        
        // Enable MPU
        MPU->CTRL = MPU_CTRL_ENABLE | MPU_CTRL_PRIVDEFENA;
    }

Static Allocation
=================

Preferred approach: Allocate at compile-time when possible.

.. code-block:: c

    // Static task creation (FreeRTOS)
    static StackType_t task_stack[256];
    static StaticTask_t task_tcb;
    
    TaskHandle_t handle = xTaskCreateStatic(
        task_function,
        "StaticTask",
        256,              // Stack size
        NULL,
        2,
        task_stack,       // Pre-allocated stack
        &task_tcb         // Pre-allocated TCB
    );
    
    // Static queue
    static uint8_t queue_storage[10 * sizeof(int)];
    static StaticQueue_t queue_struct;
    
    QueueHandle_t queue = xQueueCreateStatic(
        10,                // Queue length
        sizeof(int),       // Item size
        queue_storage,     // Storage array
        &queue_struct      // Queue structure
    );

Advantages:
- No runtime allocation failures
- Deterministic memory usage
- Can calculate total RAM requirements at compile-time

Best Practices
==============

1. **Prefer static allocation** over dynamic when possible
2. **Use memory pools** for fixed-size objects
3. **Size stacks appropriately** (measure, don't guess)
4. **Enable stack overflow detection** during development
5. **Monitor heap usage** regularly
6. **Avoid malloc/free in ISRs** (never!)
7. **Plan for worst-case** memory usage
8. **Test with heap exhaustion** scenarios
9. **Use const for read-only data** (saves RAM)
10. **Consider external RAM** for large buffers

Common Pitfalls
===============

1. **Stack overflow**: Insufficient stack size
2. **Heap fragmentation**: Poor allocation patterns
3. **Memory leaks**: Forgetting to free
4. **Double free**: Freeing same pointer twice
5. **Use after free**: Accessing freed memory
6. **Wild pointers**: Uninitialized pointers
7. **Buffer overflow**: Writing past array bounds

Memory Analysis Tools
=====================

Static Analysis
---------------

.. code-block:: bash

    # GCC: Get size information
    arm-none-eabi-size firmware.elf
    
    # Output:
    #   text    data     bss     dec     hex filename
    #  12345    1234    5678   19257    4b39 firmware.elf

Runtime Analysis
----------------

.. code-block:: c

    void print_memory_stats(void) {
        printf("\n=== Memory Statistics ===\n");
        printf("Heap free: %u bytes\n", 
               (unsigned)xPortGetFreeHeapSize());
        printf("Heap minimum ever: %u bytes\n",
               (unsigned)xPortGetMinimumEverFreeHeapSize());
        
        printf("\nTask Stack Watermarks:\n");
        TaskHandle_t task;
        char name[16];
        
        task = xTaskGetHandle("Task1");
        printf("  %s: %u words free\n", "Task1",
               (unsigned)uxTaskGetStackHighWaterMark(task));
        
        // Repeat for each task
    }

Memory Debugging
================

.. code-block:: c

    // Heap allocation tracking
    void *tracked_malloc(size_t size, const char *file, int line) {
        void *ptr = pvPortMalloc(size);
        
        if (ptr) {
            printf("Alloc: %p, size: %u at %s:%d\n", 
                   ptr, (unsigned)size, file, line);
        } else {
            printf("Alloc FAILED: size %u at %s:%d\n",
                   (unsigned)size, file, line);
        }
        
        return ptr;
    }
    
    #define MALLOC(size) tracked_malloc(size, __FILE__, __LINE__)

See Also
========

- :doc:`../days/day08` - Memory and Stack Management
- :doc:`rtos_basics` - Stack and task concepts
- :doc:`../patterns/periodic_scheduler` - Static allocation patterns
- :doc:`timing_analysis` - Memory access timing

Further Reading
===============

- "Embedded Systems Architecture" by Tammy Noergaard
- FreeRTOS memory management documentation
- ARM Cortex-M MPU programming guide
- "C Dynamic Memory Allocation" by Colin Walls
