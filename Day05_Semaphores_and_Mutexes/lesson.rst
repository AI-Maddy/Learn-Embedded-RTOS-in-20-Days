Day 05 Lesson - Semaphores and Mutexes
========================================

Introduction
------------

Semaphores and mutexes are synchronization primitives that enable tasks to coordinate access to shared resources and signal events. Understanding when to use each and proper implementation patterns is critical for reliable multithreaded systems.

Binary Semaphores
-----------------

**Binary semaphore** has two states: available (1) or taken (0).

Primary uses:
- ISR-to-task synchronization
- Task notification of events
- Simple signaling between tasks

.. code-block:: c

   SemaphoreHandle_t xBinarySemaphore;
   
   void ISR_Handler(void)
   {
       BaseType_t xHigherPriorityTaskWoken = pdFALSE;
       xSemaphoreGiveFromISR(xBinarySemaphore, &xHigherPriorityTaskWoken);
       portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
   }
   
   void vTaskWaitingForISR(void *pvParameters)
   {
       for(;;)
       {
           xSemaphoreTake(xBinarySemaphore, portMAX_DELAY);
           // Process ISR event
       }
   }

Counting Semaphores
-------------------

**Counting semaphore** tracks resource count (0 to N).

Use cases:
- Resource pools (e.g., N buffers available)
- Event counting
- Rate limiting

.. code-block:: c

   #define NUM_BUFFERS 5
   SemaphoreHandle_t xBufferSemaphore;
   
   void init_buffer_pool(void)
   {
       xBufferSemaphore = xSemaphoreCreateCounting(NUM_BUFFERS, NUM_BUFFERS);
   }
   
   void* allocate_buffer(void)
   {
       if(xSemaphoreTake(xBufferSemaphore, pdMS_TO_TICKS(100)) == pdPASS)
       {
           return get_buffer_from_pool();
       }
       return NULL;
   }
   
   void free_buffer(void* buffer)
   {
       return_buffer_to_pool(buffer);
       xSemaphoreGive(xBufferSemaphore);
   }

Mutexes
-------

**Mutex** (mutual exclusion) protects shared resources with priority inheritance.

Key differences from binary semaphore:
- Has ownership (only owner can release)
- Priority inheritance prevents priority inversion
- Cannot be used from ISR

.. code-block:: c

   SemaphoreHandle_t xMutex;
   
   void init_shared_resource(void)
   {
       xMutex = xSemaphoreCreateMutex();
   }
   
   void access_shared_resource(void)
   {
       if(xSemaphoreTake(xMutex, pdMS_TO_TICKS(1000)) == pdPASS)
       {
           // Critical section - protected access
           modify_shared_data();
           
           xSemaphoreGive(xMutex);
       }
   }

Recursive Mutexes
-----------------

Allow same task to take mutex multiple times.

.. code-block:: c

   SemaphoreHandle_t xRecursiveMutex;
   
   xRecursiveMutex = xSemaphoreCreateRecursiveMutex();
   
   void function_a(void)
   {
       xSemaphoreTakeRecursive(xRecursiveMutex, portMAX_DELAY);
       function_b();  // Can call again
       xSemaphoreGiveRecursive(xRecursiveMutex);
   }

Best Practices
--------------

1. Use mutexes for shared resource protection
2. Use binary semaphores for signaling
3. Use counting semaphores for resource pools
4. Always use timeouts (never infinite wait in production)
5. Keep critical sections short (<100µs)
6. Never call blocking operations while holding mutex

Common Pitfalls
---------------

**Deadlock:**

.. code-block:: c

   // Task A: Lock mutex1, then mutex2
   // Task B: Lock mutex2, then mutex1
   // DEADLOCK if interleaved!

**Solution:** Always acquire locks in same order.

**Priority Inversion:**

Use mutexes (not semaphores) for shared resource protection.
