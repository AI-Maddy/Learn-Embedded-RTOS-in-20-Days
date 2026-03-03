================
Synchronization
================

Introduction
============

**Synchronization primitives** coordinate access to shared resources and enable communication between tasks. They are essential for:

- Preventing race conditions
- Coordinating task execution
- Sharing data safely
- Signaling events

Common synchronization mechanisms include semaphores, mutexes, queues, and event groups.

Semaphores
==========

A **semaphore** is a signaling mechanism that controls access to resources or coordinates task execution.

Binary Semaphores
-----------------

A binary semaphore has two states: available (1) or unavailable (0). Used for event notification.

.. code-block:: c

    // Create binary semaphore
    SemaphoreHandle_t event_sem;
    event_sem = xSemaphoreCreateBinary();
    
    // Task waiting for event
    void consumer_task(void *param) {
        while (1) {
            // Block until signaled
            if (xSemaphoreTake(event_sem, portMAX_DELAY) == pdTRUE) {
                process_event();
            }
        }
    }
    
    // ISR signals event
    void UART_IRQHandler(void) {
        BaseType_t higher_priority_woken = pdFALSE;
        xSemaphoreGiveFromISR(event_sem, &higher_priority_woken);
        portYIELD_FROM_ISR(higher_priority_woken);
    }

Counting Semaphores
-------------------

A counting semaphore maintains a count (0 to N), useful for managing multiple identical resources.

.. code-block:: c

    // Create counting semaphore for 5 buffers
    SemaphoreHandle_t buffer_sem;
    #define MAX_BUFFERS 5
    buffer_sem = xSemaphoreCreateCounting(MAX_BUFFERS, MAX_BUFFERS);
    
    void task(void *param) {
        while (1) {
            // Acquire buffer
            xSemaphoreTake(buffer_sem, portMAX_DELAY);
            
            uint8_t *buffer = allocate_buffer();
            process_data(buffer);
            free_buffer(buffer);
            
            // Release buffer
            xSemaphoreGive(buffer_sem);
        }
    }

Semaphore Patterns
------------------

**Signaling Pattern:**

.. code-block:: c

    // Producer signals consumer
    void producer(void *param) {
        while (1) {
            produce_data();
            xSemaphoreGive(data_ready_sem);  // Signal
        }
    }
    
    void consumer(void *param) {
        while (1) {
            xSemaphoreTake(data_ready_sem, portMAX_DELAY);  // Wait
            consume_data();
        }
    }

**Rendezvous Pattern:**

.. code-block:: c

    SemaphoreHandle_t sem1, sem2;
    
    void task1(void *param) {
        do_work1();
        xSemaphoreGive(sem1);        // Signal task2
        xSemaphoreTake(sem2, portMAX_DELAY);  // Wait for task2
    }
    
    void task2(void *param) {
        do_work2();
        xSemaphoreGive(sem2);        // Signal task1
        xSemaphoreTake(sem1, portMAX_DELAY);  // Wait for task1
    }

Mutexes
=======

A **mutex** (mutual exclusion) protects shared resources, ensuring only one task accesses the resource at a time.

Basic Mutex
-----------

.. code-block:: c

    SemaphoreHandle_t resource_mutex;
    resource_mutex = xSemaphoreCreateMutex();
    
    int shared_counter = 0;
    
    void task(void *param) {
        while (1) {
            // Lock mutex
            if (xSemaphoreTake(resource_mutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
                // Critical section
                shared_counter++;
                
                // Unlock mutex
                xSemaphoreGive(resource_mutex);
            }
        }
    }

Priority Inheritance
--------------------

Mutexes support **priority inheritance** to prevent priority inversion.

.. code-block:: c

    // Create mutex with priority inheritance
    SemaphoreHandle_t mutex;
    mutex = xSemaphoreCreateMutex();  // Automatic priority inheritance
    
    // Without priority inheritance (binary semaphore):
    // Low-priority task holds resource
    // High-priority task blocks
    // Medium-priority task preempts low-priority task
    // = Priority inversion!
    
    // With priority inheritance:
    // Low-priority task inherits high-priority task's priority
    // Medium-priority task cannot preempt
    // = Problem solved!

Recursive Mutexes
-----------------

A **recursive mutex** can be locked multiple times by the same task.

.. code-block:: c

    SemaphoreHandle_t recursive_mutex;
    recursive_mutex = xSemaphoreCreateRecursiveMutex();
    
    void function_a(void) {
        xSemaphoreTakeRecursive(recursive_mutex, portMAX_DELAY);
        function_b();  // Also needs mutex
        xSemaphoreGiveRecursive(recursive_mutex);
    }
    
    void function_b(void) {
        xSemaphoreTakeRecursive(recursive_mutex, portMAX_DELAY);
        // Same task can acquire again
        do_work();
        xSemaphoreGiveRecursive(recursive_mutex);
    }

Mutex Guidelines
----------------

**DO:**
- Use mutexes for protecting shared data
- Keep critical sections short
- Always unlock in the same task that locked
- Use priority inheritance

**DON'T:**
- Call from ISRs (use semaphores instead)
- Hold multiple mutexes (avoid deadlock)
- Perform blocking operations while holding mutex
- Use for task synchronization (use semaphores)

Deadlock Prevention
-------------------

.. code-block:: c

    // BAD: Potential deadlock
    void task1(void) {
        xSemaphoreTake(mutex_A, portMAX_DELAY);
        xSemaphoreTake(mutex_B, portMAX_DELAY);  // Inverted order!
        // ...
    }
    
    void task2(void) {
        xSemaphoreTake(mutex_B, portMAX_DELAY);
        xSemaphoreTake(mutex_A, portMAX_DELAY);  // Inverted order!
        // ...
    }
    
    // GOOD: Always acquire in same order
    void task1(void) {
        xSemaphoreTake(mutex_A, portMAX_DELAY);  // A before B
        xSemaphoreTake(mutex_B, portMAX_DELAY);
        // ...
    }
    
    void task2(void) {
        xSemaphoreTake(mutex_A, portMAX_DELAY);  // A before B
        xSemaphoreTake(mutex_B, portMAX_DELAY);
        // ...
    }

Queues
======

**Queues** enable safe, ordered data transfer between tasks.

Creating and Using Queues
--------------------------

.. code-block:: c

    // Define message structure
    typedef struct {
        uint8_t sensor_id;
        int16_t temperature;
        uint32_t timestamp;
    } sensor_msg_t;
    
    // Create queue
    QueueHandle_t sensor_queue;
    sensor_queue = xQueueCreate(10, sizeof(sensor_msg_t));
    
    // Producer task
    void sensor_reader_task(void *param) {
        sensor_msg_t msg;
        
        while (1) {
            msg.sensor_id = 1;
            msg.temperature = read_temperature();
            msg.timestamp = xTaskGetTickCount();
            
            // Send to queue (blocks if full)
            xQueueSend(sensor_queue, &msg, pdMS_TO_TICKS(100));
            
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }
    
    // Consumer task
    void data_logger_task(void *param) {
        sensor_msg_t msg;
        
        while (1) {
            // Receive from queue (blocks if empty)
            if (xQueueReceive(sensor_queue, &msg, portMAX_DELAY) == pdTRUE) {
                log_data(msg.sensor_id, msg.temperature, msg.timestamp);
            }
        }
    }

Queue Variations
----------------

**Send to Front:**

.. code-block:: c

    // Urgent message - jump the queue
    xQueueSendToFront(queue, &urgent_msg, 0);

**Peek Without Removing:**

.. code-block:: c

    // Look at message without consuming it
    if (xQueuePeek(queue, &msg, 0) == pdTRUE) {
        // Message still in queue
    }

**From ISR:**

.. code-block:: c

    void UART_IRQHandler(void) {
        char received_char = UART->DR;
        BaseType_t higher_priority_woken = pdFALSE;
        
        xQueueSendFromISR(uart_queue, &received_char, &higher_priority_woken);
        portYIELD_FROM_ISR(higher_priority_woken);
    }

Queue Use Cases
---------------

1. **Producer-Consumer**: Data pipeline between tasks
2. **Command Dispatcher**: Serialize command execution
3. **Event Logging**: Collect events from multiple sources
4. **Rate Limiting**: Buffer bursts of data

Event Groups
============

**Event groups** manage multiple binary flags (events) as a group, useful for synchronizing on multiple conditions.

Basic Usage
-----------

.. code-block:: c

    #include "event_groups.h"
    
    // Define event bits
    #define EVENT_BIT_0  (1 << 0)  // 0x01
    #define EVENT_BIT_1  (1 << 1)  // 0x02
    #define EVENT_BIT_2  (1 << 2)  // 0x04
    
    EventGroupHandle_t event_group;
    event_group = xEventGroupCreate();
    
    // Task sets events
    void task1(void *param) {
        while (1) {
            do_work();
            xEventGroupSetBits(event_group, EVENT_BIT_0);
        }
    }
    
    // Task waits for multiple events
    void task2(void *param) {
        while (1) {
            // Wait for BOTH bit 0 AND bit 1
            EventBits_t bits = xEventGroupWaitBits(
                event_group,
                EVENT_BIT_0 | EVENT_BIT_1,  // Bits to wait for
                pdTRUE,   // Clear on exit
                pdTRUE,   // Wait for ALL bits (AND)
                portMAX_DELAY
            );
            
            process_combined_event();
        }
    }

Synchronization Patterns
------------------------

**Barrier Synchronization:**

.. code-block:: c

    #define TASK1_BIT  (1 << 0)
    #define TASK2_BIT  (1 << 1)
    #define TASK3_BIT  (1 << 2)
    #define ALL_TASKS  (TASK1_BIT | TASK2_BIT | TASK3_BIT)
    
    void task1(void *param) {
        phase1_work();
        
        // Signal completion and wait for others
        xEventGroupSync(event_group, TASK1_BIT, ALL_TASKS, portMAX_DELAY);
        
        phase2_work();  // All tasks synchronized here
    }

**State Machine:**

.. code-block:: c

    #define STATE_INIT       (1 << 0)
    #define STATE_READY      (1 << 1)
    #define STATE_RUNNING    (1 << 2)
    #define STATE_ERROR      (1 << 3)
    
    void control_task(void *param) {
        xEventGroupSetBits(state_group, STATE_INIT);
        
        if (initialize() == SUCCESS) {
            xEventGroupClearBits(state_group, STATE_INIT);
            xEventGroupSetBits(state_group, STATE_READY);
        }
    }
    
    void monitor_task(void *param) {
        while (1) {
            EventBits_t state = xEventGroupWaitBits(
                state_group,
                STATE_ERROR,
                pdFALSE,  // Don't clear
                pdFALSE,  // Any bit
                pdMS_TO_TICKS(100)
            );
            
            if (state & STATE_ERROR) {
                handle_error();
            }
        }
    }

Critical Sections
=================

Disable Interrupts
------------------

.. code-block:: c

    // FreeRTOS critical sections
    taskENTER_CRITICAL();
    shared_variable++;
    taskEXIT_CRITICAL();
    
    // From ISR
    void IRQHandler(void) {
        UBaseType_t saved_mask = taskENTER_CRITICAL_FROM_ISR();
        shared_isr_data++;
        taskEXIT_CRITICAL_FROM_ISR(saved_mask);
    }

⚠️ **Warning**: Keep critical sections extremely short (< 10 µs) to maintain real-time performance.

Scheduler Locking
-----------------

.. code-block:: c

    // Suspend scheduler (interrupts still active)
    vTaskSuspendAll();
    // Only this task can run; no context switching
    shared_data.value1 = new_value1;
    shared_data.value2 = new_value2;
    xTaskResumeAll();

Notification Mechanism
======================

**Task notifications** are a lightweight alternative to semaphores/queues for simple signaling.

.. code-block:: c

    TaskHandle_t task_handle;
    
    void waiting_task(void *param) {
        while (1) {
            // Wait for notification
            ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
            handle_notification();
        }
    }
    
    void signaling_task(void *param) {
        while (1) {
            do_work();
            // Notify task
            xTaskNotifyGive(task_handle);
        }
    }
    
    // From ISR
    void IRQHandler(void) {
        BaseType_t higher_priority_woken = pdFALSE;
        vTaskNotifyGiveFromISR(task_handle, &higher_priority_woken);
        portYIELD_FROM_ISR(higher_priority_woken);
    }

**Advantages:**
- Faster than semaphores (direct task notification)
- No separate object creation
- Lower RAM usage

**Limitations:**
- Only one notification per task
- Only one task can wait
- No queuing of multiple notifications

Best Practices
==============

1. **Choose the right primitive:**
   - Mutex: Protecting shared data
   - Binary semaphore: Event signaling
   - Counting semaphore: Managing pools
   - Queue: Data transfer
   - Event group: Multiple conditions

2. **Avoid common mistakes:**
   - Don't use mutexes in ISRs
   - Don't hold mutexes while blocking
   - Always check return values
   - Use timeouts to prevent deadlock

3. **Performance considerations:**
   - Task notifications faster than semaphores
   - Direct-to-task notifications when possible
   - Size queues appropriately

4. **Testing:**
   - Stress test with multiple tasks
   - Test timeout scenarios
   - Validate priority inheritance
   - Check for resource leaks

Comparison Table
================

+-------------------+------------+-------------+---------+---------------+
| Feature           | Semaphore  | Mutex       | Queue   | Event Group   |
+===================+============+=============+=========+===============+
| Signaling         | Yes        | No          | Yes     | Yes           |
+-------------------+------------+-------------+---------+---------------+
| Data Transfer     | No         | No          | Yes     | No            |
+-------------------+------------+-------------+---------+---------------+
| Priority Inherit  | No         | Yes         | No      | No            |
+-------------------+------------+-------------+---------+---------------+
| From ISR          | Yes        | No          | Yes     | Yes           |
+-------------------+------------+-------------+---------+---------------+
| Multiple Waiters  | Yes        | Yes         | Yes     | Yes           |
+-------------------+------------+-------------+---------+---------------+
| Recursive         | No         | Optional    | No      | No            |
+-------------------+------------+-------------+---------+---------------+

See Also
========

- :doc:`../days/day05` - Semaphores and Mutexes
- :doc:`../days/day06` - Queues and Event Groups
- :doc:`rtos_basics` - Task states and scheduling
- :doc:`../patterns/producer_consumer` - Producer-consumer pattern
- :doc:`interrupts` - ISR synchronization

Further Reading
===============

- "The Little Book of Semaphores" by Allen B. Downey
- "Concurrent Programming" by Ben-Ari
- FreeRTOS documentation on synchronization primitives
