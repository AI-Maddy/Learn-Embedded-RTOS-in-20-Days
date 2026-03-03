Day 02 Lesson - Tasks and Threads
==================================

Introduction
------------

In RTOS development, **tasks** (also called **threads**) are the fundamental units of concurrent execution. Each task represents an independent flow of control with its own execution context, stack, and priority. Understanding how to design, create, and manage tasks is essential to building responsive, maintainable embedded systems.

This lesson covers task lifecycle, creation APIs across different RTOS platforms, priority management, stack allocation, and best practices for task design.

What is a Task?
---------------

A **task** is an independent execution context that:

- Has its own **stack** for local variables and function calls
- Has its own **priority** level that determines when it runs
- Maintains its own **program counter (PC)** and CPU register state
- Can be in one of several **states**: Running, Ready, Blocked, or Suspended

**Key Distinction: Tasks vs. Threads**

In RTOS terminology:

- **Task** typically refers to the unit of execution (common in FreeRTOS, Zephyr)
- **Thread** means the same thing (common in ThreadX, POSIX)
- Both terms are often used interchangeably

**Task vs. Process:**

+------------------+----------------------------+----------------------------+
| Feature          | Task/Thread                | Process (OS)              |
+==================+============================+===========================+
| Address Space    | Shared with all tasks      | Isolated/Protected        |
+------------------+----------------------------+----------------------------+
| Context Switch   | Fast (~1-2 μs)             | Slow (~10-100 μs)         |
+------------------+----------------------------+----------------------------+
| Memory Overhead  | Small (stack only)         | Large (page tables, etc.) |
+------------------+----------------------------+----------------------------+
| Communication    | Direct (shared memory)     | IPC mechanisms required   |
+------------------+----------------------------+----------------------------+

Task States
-----------

Every task exists in one of four states:

.. code-block:: text

                 ┌─────────────┐
                 │  SUSPENDED  │
                 └──────┬──────┘
                        │ Resume
                        ↓
    ┌─────────────────────────────────────┐
    │                                     │
    │  ┌──────────┐  Scheduler  ┌─────────▼──┐
    │  │ RUNNING  │◄────────────┤   READY    │
    │  └────┬─────┘              └─────────▲──┘
    │       │                              │
    │       │ Wait on                      │
    │       │ Resource                     │ Resource
    │       ↓                              │ Available
    │  ┌─────────┐                         │
    └──┤ BLOCKED ├─────────────────────────┘
       └─────────┘

**State Descriptions:**

1. **Running**: Task is currently executing on the CPU
2. **Ready**: Task is ready to run but waiting for scheduler
3. **Blocked**: Task is waiting for an event (semaphore, delay, queue)
4. **Suspended**: Task is explicitly suspended and won't be scheduled

Task Creation
-------------

FreeRTOS Task Creation
~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: c

   #include "FreeRTOS.h"
   #include "task.h"

   // Task function prototype
   void vMyTask(void *pvParameters)
   {
       // Task initialization
       uint32_t counter = 0;
       
       // Task infinite loop
       for(;;)
       {
           // Do periodic work
           printf("Task running: %lu\n", counter++);
           
           // Delay for 1000 ticks (usually 1000ms with 1ms tick)
           vTaskDelay(pdMS_TO_TICKS(1000));
       }
   }

   // Create and start task
   void create_my_task(void)
   {
       TaskHandle_t xTaskHandle = NULL;
       BaseType_t xReturned;
       
       xReturned = xTaskCreate(
           vMyTask,              // Task function
           "MyTask",             // Task name (for debugging)
           configMINIMAL_STACK_SIZE,  // Stack size in words
           NULL,                 // Task parameter
           tskIDLE_PRIORITY + 1, // Priority
           &xTaskHandle          // Task handle (optional)
       );
       
       if(xReturned != pdPASS)
       {
           // Task creation failed - likely out of heap memory
           printf("ERROR: Task creation failed\n");
       }
   }

**Static Allocation Alternative:**

.. code-block:: c

   // For systems where dynamic allocation is prohibited
   static StackType_t xTaskStack[configMINIMAL_STACK_SIZE];
   static StaticTask_t xTaskBuffer;
   
   TaskHandle_t xTaskHandle = xTaskCreateStatic(
       vMyTask,                 // Task function
       "MyTask",                // Name
       configMINIMAL_STACK_SIZE, // Stack size
       NULL,                    // Parameters
       tskIDLE_PRIORITY + 1,    // Priority
       xTaskStack,              // Stack buffer
       &xTaskBuffer             // Task control block
   );

Zephyr Thread Creation
~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: c

   #include <zephyr/kernel.h>

   #define STACK_SIZE 1024
   #define THREAD_PRIORITY 5

   // Define thread stack
   K_THREAD_STACK_DEFINE(my_stack_area, STACK_SIZE);
   struct k_thread my_thread_data;
   k_tid_t my_tid;

   // Thread entry function
   void my_thread_entry(void *p1, void *p2, void *p3)
   {
       uint32_t counter = 0;
       
       while (1) {
           printk("Thread running: %u\n", counter++);
           k_sleep(K_MSEC(1000));
       }
   }

   // Create thread
   void create_my_thread(void)
   {
       my_tid = k_thread_create(&my_thread_data, my_stack_area,
           K_THREAD_STACK_SIZEOF(my_stack_area),
           my_thread_entry,
           NULL, NULL, NULL,              // Thread parameters
           THREAD_PRIORITY, 0, K_NO_WAIT); // Priority, options, delay
           
       k_thread_name_set(my_tid, "my_thread");
   }

ThreadX Thread Creation
~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: c

   #include "tx_api.h"

   #define STACK_SIZE 1024

   TX_THREAD my_thread;
   UCHAR my_thread_stack[STACK_SIZE];

   // Thread entry function
   void my_thread_entry(ULONG thread_input)
   {
       UINT counter = 0;
       
       while(1)
       {
           printf("Thread running: %u\n", counter++);
           tx_thread_sleep(100);  // Sleep for 100 ticks
       }
   }

   // Create thread
   void create_my_thread(void)
   {
       UINT status;
       
       status = tx_thread_create(
           &my_thread,              // Thread control block
           "My Thread",             // Thread name
           my_thread_entry,         // Entry function
           0x1234,                  // Entry input (passed to function)
           my_thread_stack,         // Stack pointer
           STACK_SIZE,              // Stack size
           5,                       // Priority (0-31, lower is higher)
           5,                       // Preemption threshold
           TX_NO_TIME_SLICE,        // Time slice disabled
           TX_AUTO_START            // Start immediately
       );
       
       if(status != TX_SUCCESS)
       {
           printf("ERROR: Thread creation failed\n");
       }
   }

Task Priority Management
------------------------

Priority Fundamentals
~~~~~~~~~~~~~~~~~~~~~

**Priority determines scheduling order:**

- Higher priority tasks preempt lower priority tasks
- Equal priority tasks share CPU via round-robin or cooperative scheduling
- Priority assignment is a critical design decision

**Priority Numbering (varies by RTOS):**

+----------------+------------------+---------------------------+
| RTOS           | Priority Range   | Direction                |
+================+==================+===========================+
| FreeRTOS       | 0 to N           | Higher number = Higher   |
+----------------+------------------+---------------------------+
| Zephyr         | -16 to 15        | Lower number = Higher    |
+----------------+------------------+---------------------------+
| ThreadX        | 0 to 31          | Lower number = Higher    |
+----------------+------------------+---------------------------+

**Priority Inversion:**

Priority inversion occurs when a high-priority task waits for a resource held by a low-priority task, while a medium-priority task runs:

.. code-block:: text

   Time ──────────────────────────────────►
   
   High Priority Task:    [WAIT on mutex M]────────────────┐
   Medium Priority Task:  ────[RUNNING]───────[RUNNING]─── │
   Low Priority Task:     [M]───────────────────[M]──────  │
                           ↑                        ↑       │
                         Acquires M            Releases M   │
                                                            ↓
                                                    [RUNNING]

**Solutions:**

1. **Priority Inheritance**: Low-priority task temporarily inherits high-priority
2. **Priority Ceiling**: Lock automatically raises task to ceiling priority

Dynamic Priority Changes
~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: c

   // FreeRTOS: Change task priority
   void adjust_priority_example(void)
   {
       TaskHandle_t xHandle;
       UBaseType_t uxOriginalPriority, uxNewPriority;
       
       // Get current task's priority
       xHandle = xTaskGetCurrentTaskHandle();
       uxOriginalPriority = uxTaskPriorityGet(xHandle);
       
       // Temporarily boost priority for critical section
       uxNewPriority = uxOriginalPriority + 5;
       vTaskPrioritySet(xHandle, uxNewPriority);
       
       // Do critical work at higher priority
       perform_critical_operation();
       
       // Restore original priority
       vTaskPrioritySet(xHandle, uxOriginalPriority);
   }

Priority Assignment Guidelines
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Rate Monotonic Scheduling (RMS):**

For periodic tasks, assign priority inversely to period:

- Shortest period → Highest priority
- Longer period → Lower priority

Example:

.. code-block:: c

   // Task periods and priorities
   void create_periodic_tasks(void)
   {
       // 10ms period - Highest priority
       xTaskCreate(fast_sensor_task, "FastSensor", 
                   512, NULL, 5, NULL);
       
       // 100ms period - Medium priority
       xTaskCreate(medium_control_task, "Control", 
                   512, NULL, 3, NULL);
       
       // 1000ms period - Lower priority
       xTaskCreate(slow_display_task, "Display", 
                   512, NULL, 1, NULL);
   }

Stack Management
----------------

Stack Sizing
~~~~~~~~~~~~

Each task needs sufficient stack for:

1. **Local variables**
2. **Function call frames** (return addresses, saved registers)
3. **Interrupt context** (if interrupts use task stack)
4. **Worst-case call depth**

**Common Stack Sizes:**

+---------------------------+------------------------+
| Task Complexity           | Typical Stack Size     |
+===========================+========================+
| Simple LED blinker        | 128 - 256 bytes       |
+---------------------------+------------------------+
| Sensor reading/filtering  | 512 - 1024 bytes      |
+---------------------------+------------------------+
| Protocol handler          | 1024 - 2048 bytes     |
+---------------------------+------------------------+
| Heavy computation         | 2048 - 4096 bytes     |
+---------------------------+------------------------+

Stack Overflow Detection
~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: c

   // FreeRTOS: Enable stack overflow detection
   // In FreeRTOSConfig.h:
   #define configCHECK_FOR_STACK_OVERFLOW 2

   // Overflow hook (called when overflow detected)
   void vApplicationStackOverflowHook(TaskHandle_t xTask, 
                                       char *pcTaskName)
   {
       printf("FATAL: Stack overflow in task %s\n", pcTaskName);
       // Log error, reset system, etc.
       for(;;);  // Halt
   }

**Runtime Stack Usage Check:**

.. code-block:: c

   // Check remaining stack space
   void monitor_stack_usage(void)
   {
       TaskHandle_t xHandle = xTaskGetCurrentTaskHandle();
       UBaseType_t uxHighWaterMark;
       
       uxHighWaterMark = uxTaskGetStackHighWaterMark(xHandle);
       printf("Task has %u words of unused stack\n", 
              uxHighWaterMark);
       
       if(uxHighWaterMark < 50) {
           printf("WARNING: Stack almost full!\n");
       }
   }

Task Design Patterns
--------------------

Periodic Task Pattern
~~~~~~~~~~~~~~~~~~~~~

.. code-block:: c

   void periodic_task(void *pvParameters)
   {
       TickType_t xLastWakeTime;
       const TickType_t xPeriod = pdMS_TO_TICKS(100); // 100ms
       
       // Initialize with current time
       xLastWakeTime = xTaskGetTickCount();
       
       for(;;)
       {
           // Wait for next cycle (drift-free)
           vTaskDelayUntil(&xLastWakeTime, xPeriod);
           
           // Execute periodic work
           read_sensors();
           update_control_loop();
       }
   }

**Why vTaskDelayUntil() instead of vTaskDelay()?**

- ``vTaskDelay()``: Delays RELATIVE to when called (accumulates drift)
- ``vTaskDelayUntil()``: Delays ABSOLUTE to original wake time (no drift)

Event-Driven Task Pattern
~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: c

   void event_handler_task(void *pvParameters)
   {
       EventBits_t uxBits;
       const EventBits_t uxAllEvents = EVENT_A | EVENT_B | EVENT_C;
       
       for(;;)
       {
           // Wait indefinitely for any event
           uxBits = xEventGroupWaitBits(
               xEventGroup,
               uxAllEvents,
               pdTRUE,   // Clear on exit
               pdFALSE,  // Wait for any bit
               portMAX_DELAY
           );
           
           if(uxBits & EVENT_A) {
               handle_event_a();
           }
           if(uxBits & EVENT_B) {
               handle_event_b();
           }
           if(uxBits & EVENT_C) {
               handle_event_c();
           }
       }
   }

Producer-Consumer Pattern
~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: c

   QueueHandle_t xDataQueue;
   
   // Producer task
   void producer_task(void *pvParameters)
   {
       sensor_data_t data;
       
       for(;;)
       {
           // Acquire data from sensor
           data = read_sensor();
           
           // Send to queue (wait up to 100ms if full)
           if(xQueueSend(xDataQueue, &data, 
              pdMS_TO_TICKS(100)) != pdPASS)
           {
               printf("WARNING: Queue full, data dropped\n");
           }
           
           vTaskDelay(pdMS_TO_TICKS(50));
       }
   }
   
   // Consumer task
   void consumer_task(void *pvParameters)
   {
       sensor_data_t data;
       
       for(;;)
       {
           // Wait for data (blocks indefinitely if empty)
           if(xQueueReceive(xDataQueue, &data, 
              portMAX_DELAY) == pdPASS)
           {
               // Process received data
               process_sensor_data(&data);
           }
       }
   }

Task Synchronization
~~~~~~~~~~~~~~~~~~~~

.. code-block:: c

   SemaphoreHandle_t xSyncSemaphore;
   
   // Initialization task
   void init_task(void *pvParameters)
   {
       // Perform system initialization
       initialize_hardware();
       configure_peripherals();
       
       printf("Initialization complete\n");
       
       // Signal other tasks that init is done
       xSemaphoreGive(xSyncSemaphore);
       
       // Delete self (one-shot task)
       vTaskDelete(NULL);
   }
   
   // Worker task that waits for init
   void worker_task(void *pvParameters)
   {
       // Wait for initialization to complete
       xSemaphoreTake(xSyncSemaphore, portMAX_DELAY);
       
       // Now safe to start work
       for(;;)
       {
           do_work();
           vTaskDelay(pdMS_TO_TICKS(100));
       }
   }

Task Lifecycle Management
--------------------------

Task Deletion
~~~~~~~~~~~~~

.. code-block:: c

   void temporary_task(void *pvParameters)
   {
       // Do one-time work
       perform_calibration();
       
       printf("Calibration complete, task exiting\n");
       
       // Delete self
       vTaskDelete(NULL);  // NULL = current task
   }

   // Delete another task
   void manager_task(void *pvParameters)
   {
       TaskHandle_t xWorkerHandle;
       
       // Create temporary worker
       xTaskCreate(temporary_task, "TempWorker", 
                   512, NULL, 2, &xWorkerHandle);
       
       // Wait for worker to finish
       vTaskDelay(pdMS_TO_TICKS(5000));
       
       // Forcefully delete if still running
       if(eTaskGetState(xWorkerHandle) != eDeleted) {
           vTaskDelete(xWorkerHandle);
       }
   }

Task Suspension/Resumption
~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: c

   TaskHandle_t xLowPriorityTask;
   
   void power_save_example(void)
   {
       // Entering low-power mode - suspend background tasks
       vTaskSuspend(xLowPriorityTask);
       
       // Do critical work only
       perform_urgent_operation();
       
       // Resume background processing
       vTaskResume(xLowPriorityTask);
   }

Best Practices
--------------

Task Design Guidelines
~~~~~~~~~~~~~~~~~~~~~~~

1. **Keep tasks simple and focused**: One responsibility per task
2. **Avoid busy-waiting**: Always use blocking APIs (queues, semaphores, delays)
3. **Size stacks appropriately**: Monitor high-water marks, add 20-30% margin
4. **Document priority rationale**: Why is this priority chosen?
5. **Use descriptive names**: Names appear in debugger and error messages

Common Pitfalls
~~~~~~~~~~~~~~~

**Pitfall 1: Stack Overflow**

.. code-block:: c

   // BAD: Large local arrays
   void task_with_large_buffer(void *pvParameters)
   {
       uint8_t buffer[4096];  // 4KB on stack!
       // ...
   }
   
   // GOOD: Use heap or static allocation
   void task_with_heap_buffer(void *pvParameters)
   {
       uint8_t *buffer = pvPortMalloc(4096);
       if(buffer == NULL) {
           // Handle error
           return;
       }
       // ... use buffer ...
       vPortFree(buffer);
   }

**Pitfall 2: Busy-Waiting**

.. code-block:: c

   // BAD: Wastes CPU
   void polling_task_bad(void *pvParameters)
   {
       for(;;)
       {
           if(is_data_ready()) {
               process_data();
           }
           // No delay - task runs continuously!
       }
   }
   
   // GOOD: Use blocking wait
   void event_task_good(void *pvParameters)
   {
       for(;;)
       {
           // Wait for semaphore signaled by ISR
           xSemaphoreTake(xDataReadySemaphore, portMAX_DELAY);
           process_data();
       }
   }

**Pitfall 3: Priority Inversion**

.. code-block:: c

   // Use priority inheritance mutexes
   SemaphoreHandle_t xMutex;
   
   void create_protected_mutex(void)
   {
       // Create mutex with priority inheritance
       xMutex = xSemaphoreCreateMutex();
       // xSemaphoreCreateMutex() automatically enables 
       // priority inheritance in FreeRTOS
   }

Debugging Tasks
---------------

Task State Inspection
~~~~~~~~~~~~~~~~~~~~~

.. code-block:: c

   void print_task_info(TaskHandle_t xTask)
   {
       eTaskState eState;
       UBaseType_t uxPriority;
       char *pcTaskName;
       
       pcTaskName = pcTaskGetName(xTask);
       eState = eTaskGetState(xTask);
       uxPriority = uxTaskPriorityGet(xTask);
       
       printf("Task: %s\n", pcTaskName);
       printf("  Priority: %u\n", uxPriority);
       printf("  State: ");
       
       switch(eState) {
           case eRunning:   printf("Running\n"); break;
           case eReady:     printf("Ready\n"); break;
           case eBlocked:   printf("Blocked\n"); break;
           case eSuspended: printf("Suspended\n"); break;
           case eDeleted:   printf("Deleted\n"); break;
       }
   }

Task List Generation
~~~~~~~~~~~~~~~~~~~~

.. code-block:: c

   void print_all_tasks(void)
   {
       char pcBuffer[1024];
       
       printf("\nTask Name       State   Prio    Stack   Num\n");
       printf("---------------------------------------------\n");
       
       vTaskList(pcBuffer);
       printf("%s\n", pcBuffer);
   }

**Example Output:**

.. code-block:: text

   Task Name       State   Prio    Stack   Num
   ---------------------------------------------
   IDLE            R       0       120     0
   Tmr Svc         B       2       200     1
   SensorTask      B       3       450     2
   ControlTask     B       4       380     3
   UartTask        R       5       220     4

Real-World Example: Multi-Task Sensor System
---------------------------------------------

.. code-block:: c

   #include "FreeRTOS.h"
   #include "task.h"
   #include "queue.h"
   #include "semphr.h"

   // Data structures
   typedef struct {
       uint32_t timestamp;
       float temperature;
       float pressure;
   } sensor_data_t;

   // Shared resources
   QueueHandle_t xSensorQueue;
   SemaphoreHandle_t xI2CMutex;

   // High-priority sensor acquisition task (10Hz)
   void vSensorTask(void *pvParameters)
   {
       TickType_t xLastWakeTime = xTaskGetTickCount();
       const TickType_t xPeriod = pdMS_TO_TICKS(100);
       sensor_data_t data;
       
       for(;;)
       {
           vTaskDelayUntil(&xLastWakeTime, xPeriod);
           
           // Acquire I2C mutex
           if(xSemaphoreTake(xI2CMutex, pdMS_TO_TICKS(10)) == pdPASS)
           {
               data.timestamp = xTaskGetTickCount();
               data.temperature = read_temperature_sensor();
               data.pressure = read_pressure_sensor();
               xSemaphoreGive(xI2CMutex);
               
               // Send to processing queue
               xQueueSend(xSensorQueue, &data, 0);
           }
           else
           {
               printf("ERROR: I2C mutex timeout\n");
           }
       }
   }

   // Medium-priority data processing task
   void vProcessingTask(void *pvParameters)
   {
       sensor_data_t data;
       
       for(;;)
       {
           // Wait for sensor data
           if(xQueueReceive(xSensorQueue, &data, 
              portMAX_DELAY) == pdPASS)
           {
               // Apply filtering and algorithms
               float filtered_temp = apply_kalman_filter(
                   data.temperature);
               
               // Make control decisions
               if(filtered_temp > 75.0f) {
                   activate_cooling();
               }
               
               // Log data
               log_data(&data);
           }
       }
   }

   // Low-priority display update task
   void vDisplayTask(void *pvParameters)
   {
       TickType_t xLastWakeTime = xTaskGetTickCount();
       const TickType_t xPeriod = pdMS_TO_TICKS(1000);
       
       for(;;)
       {
           vTaskDelayUntil(&xLastWakeTime, xPeriod);
           
           // Update display (slow operation)
           if(xSemaphoreTake(xI2CMutex, pdMS_TO_TICKS(50)) == pdPASS)
           {
               update_lcd_display();
               xSemaphoreGive(xI2CMutex);
           }
       }
   }

   // System initialization
   void system_init(void)
   {
       // Create queue (10 elements)
       xSensorQueue = xQueueCreate(10, sizeof(sensor_data_t));
       
       // Create I2C mutex
       xI2CMutex = xSemaphoreCreateMutex();
       
       // Create tasks with appropriate priorities
       xTaskCreate(vSensorTask, "Sensor", 512, NULL, 5, NULL);
       xTaskCreate(vProcessingTask, "Process", 1024, NULL, 3, NULL);
       xTaskCreate(vDisplayTask, "Display", 512, NULL, 1, NULL);
       
       // Start scheduler
       vTaskStartScheduler();
   }

Summary
-------

Tasks are the foundation of RTOS-based systems:

- **Tasks provide independent execution contexts** with private stacks and priorities
- **Priority determines scheduling**: higher priority preempts lower
- **Stack sizing is critical**: use monitoring tools, add safety margin
- **Design patterns**: periodic, event-driven, producer-consumer
- **Avoid common pitfalls**: stack overflow, busy-waiting, priority inversion
- **Use blocking APIs**: leverage the scheduler, don't poll

Mastering task creation and management enables building complex, responsive embedded systems with clean, maintainable code.
