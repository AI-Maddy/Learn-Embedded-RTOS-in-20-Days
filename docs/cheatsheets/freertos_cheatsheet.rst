========================
FreeRTOS Cheat Sheet
========================

Quick Start
===========

Configuration
-------------

**FreeRTOSConfig.h** - Key settings:

.. code-block:: c

    #define configUSE_PREEMPTION                1
    #define configUSE_IDLE_HOOK                 0
    #define configUSE_TICK_HOOK                 0
    #define configCPU_CLOCK_HZ                  168000000
    #define configTICK_RATE_HZ                  1000
    #define configMAX_PRIORITIES                5
    #define configMINIMAL_STACK_SIZE            128
    #define configTOTAL_HEAP_SIZE               20480
    #define configMAX_TASK_NAME_LEN             16
    #define configUSE_16_BIT_TICKS              0
    #define configIDLE_SHOULD_YIELD             1
    #define configUSE_MUTEXES                   1
    #define configUSE_COUNTING_SEMAPHORES       1
    #define configUSE_QUEUE_SETS                1
    #define configQUEUE_REGISTRY_SIZE           10

Task API
========

Create/Delete
-------------

.. code-block:: c

    // Dynamic creation
    BaseType_t xTaskCreate(
        TaskFunction_t pvTaskCode,    // Task function
        const char * const pcName,     // Name (debug)
        configSTACK_DEPTH_TYPE usStackDepth,  // Words
        void *pvParameters,             // Parameter
        UBaseType_t uxPriority,         // Priority (0=lowest)
        TaskHandle_t *pxCreatedTask     // Handle (or NULL)
    );
    
    // Static creation (no dynamic allocation)
    TaskHandle_t xTaskCreateStatic(
        TaskFunction_t pvTaskCode,
        const char * const pcName,
        uint32_t ulStackDepth,
        void *pvParameters,
        UBaseType_t uxPriority,
        StackType_t * const puxStackBuffer,
        StaticTask_t * const pxTaskBuffer
    );
    
    // Delete task
    void vTaskDelete(TaskHandle_t xTask);  // NULL = delete self

Delay/Blocking
--------------

.. code-block:: c

    // Delay (relative, can drift)
    void vTaskDelay(const TickType_t xTicksToDelay);
    
    // Delay until (absolute, no drift)
    void vTaskDelayUntil(
        TickType_t *pxPreviousWakeTime,
        const TickType_t xTimeIncrement
    );
    
    // Example periodic task
    void vPeriodicTask(void *pvParameters) {
        TickType_t xLastWakeTime = xTaskGetTickCount();
        const TickType_t xFrequency = pdMS_TO_TICKS(100);
        
        for (;;) {
            // Work here
            vTaskDelayUntil(&xLastWakeTime, xFrequency);
        }
    }

Priority/Suspend
----------------

.. code-block:: c

    // Change priority
    void vTaskPrioritySet(TaskHandle_t xTask, UBaseType_t uxNewPriority);
    UBaseType_t uxTaskPriorityGet(TaskHandle_t xTask);
    
    // Suspend/Resume
    void vTaskSuspend(TaskHandle_t xTaskToSuspend);
    void vTaskResume(TaskHandle_t xTaskToResume);
    BaseType_t xTaskResumeFromISR(TaskHandle_t xTaskToResume);

Info/Debug
----------

.. code-block:: c

    // Get current task handle
    TaskHandle_t xTaskGetCurrentTaskHandle(void);
    
    // Get task by name
    TaskHandle_t xTaskGetHandle(const char *pcNameToQuery);
    
    // Stack high water mark (minimum free stack ever)
    UBaseType_t uxTaskGetStackHighWaterMark(TaskHandle_t xTask);
    
    // Get task state
    eTaskState eTaskGetState(TaskHandle_t xTask);
    // Returns: eRunning, eReady, eBlocked, eSuspended, eDeleted

Semaphore API
=============

Binary Semaphore
----------------

.. code-block:: c

    // Create
    SemaphoreHandle_t xSemaphoreCreateBinary(void);
    SemaphoreHandle_t xSemaphoreCreateBinaryStatic(StaticSemaphore_t *pxBuffer);
    
    // Take (wait)
    BaseType_t xSemaphoreTake(
        SemaphoreHandle_t xSemaphore,
        TickType_t xTicksToWait
    );
    
    // Give (signal)
    BaseType_t xSemaphoreGive(SemaphoreHandle_t xSemaphore);
    
    // From ISR
    BaseType_t xSemaphoreGiveFromISR(
        SemaphoreHandle_t xSemaphore,
        BaseType_t *pxHigherPriorityTaskWoken
    );
    BaseType_t xSemaphoreTakeFromISR(
        SemaphoreHandle_t xSemaphore,
        BaseType_t *pxHigherPriorityTaskWoken
    );

Counting Semaphore
------------------

.. code-block:: c

    // Create
    SemaphoreHandle_t xSemaphoreCreateCounting(
        UBaseType_t uxMaxCount,
        UBaseType_t uxInitialCount
    );
    
    // Same give/take as binary semaphore

Mutex API
=========

.. code-block:: c

    // Create
    SemaphoreHandle_t xSemaphoreCreateMutex(void);
    SemaphoreHandle_t xSemaphoreCreateMutexStatic(StaticSemaphore_t *pxBuffer);
    
    // Recursive mutex (can be taken multiple times by same task)
    SemaphoreHandle_t xSemaphoreCreateRecursiveMutex(void);
    
    // Take/Give mutex (same API as semaphore)
    xSemaphoreTake(xMutex, portMAX_DELAY);
    xSemaphoreGive(xMutex);
    
    // Recursive mutex (different API)
    xSemaphoreTakeRecursive(xMutex, portMAX_DELAY);
    xSemaphoreGiveRecursive(xMutex);

⚠️ **Never call from ISR!**

Queue API
=========

Create/Delete
-------------

.. code-block:: c

    // Create
    QueueHandle_t xQueueCreate(
        UBaseType_t uxQueueLength,
        UBaseType_t uxItemSize
    );
    
    // Static
    QueueHandle_t xQueueCreateStatic(
        UBaseType_t uxQueueLength,
        UBaseType_t uxItemSize,
        uint8_t *pucQueueStorageBuffer,
        StaticQueue_t *pxQueueBuffer
    );
    
    // Delete
    void vQueueDelete(QueueHandle_t xQueue);

Send/Receive
------------

.. code-block:: c

    // Send to back (FIFO)
    BaseType_t xQueueSend(
        QueueHandle_t xQueue,
        const void *pvItemToQueue,
        TickType_t xTicksToWait
    );
    
    // Send to front (urgent)
    BaseType_t xQueueSendToFront(
        QueueHandle_t xQueue,
        const void *pvItemToQueue,
        TickType_t xTicksToWait
    );
    
    // Receive
    BaseType_t xQueueReceive(
        QueueHandle_t xQueue,
        void *pvBuffer,
        TickType_t xTicksToWait
    );
    
    // Peek (don't remove)
    BaseType_t xQueuePeek(
        QueueHandle_t xQueue,
        void *pvBuffer,
        TickType_t xTicksToWait
    );
    
    // From ISR
    BaseType_t xQueueSendFromISR(
        QueueHandle_t xQueue,
        const void *pvItemToQueue,
        BaseType_t *pxHigherPriorityTaskWoken
    );
    
    BaseType_t xQueueReceiveFromISR(
        QueueHandle_t xQueue,
        void *pvBuffer,
        BaseType_t *pxHigherPriorityTaskWoken
    );

Info
----

.. code-block:: c

    // Number of messages waiting
    UBaseType_t uxQueueMessagesWaiting(QueueHandle_t xQueue);
    
    // Number of free spaces
    UBaseType_t uxQueueSpacesAvailable(QueueHandle_t xQueue);
    
    // Reset queue (empty it)
    BaseType_t xQueueReset(QueueHandle_t xQueue);

Event Groups API
================

.. code-block:: c

    // Create
    EventGroupHandle_t xEventGroupCreate(void);
    EventGroupHandle_t xEventGroupCreateStatic(StaticEventGroup_t *pxBuffer);
    
    // Set bits
    EventBits_t xEventGroupSetBits(
        EventGroupHandle_t xEventGroup,
        const EventBits_t uxBitsToSet
    );
    
    EventBits_t xEventGroupSetBitsFromISR(
        EventGroupHandle_t xEventGroup,
        const EventBits_t uxBitsToSet,
        BaseType_t *pxHigherPriorityTaskWoken
    );
    
    // Wait for bits
    EventBits_t xEventGroupWaitBits(
        EventGroupHandle_t xEventGroup,
        const EventBits_t uxBitsToWaitFor,
        const BaseType_t xClearOnExit,
        const BaseType_t xWaitForAllBits,
        TickType_t xTicksToWait
    );
    
    // Clear bits
    EventBits_t xEventGroupClearBits(
        EventGroupHandle_t xEventGroup,
        const EventBits_t uxBitsToClear
    );
    
    // Sync (barrier)
    EventBits_t xEventGroupSync(
        EventGroupHandle_t xEventGroup,
        const EventBits_t uxBitsToSet,
        const EventBits_t uxBitsToWaitFor,
        TickType_t xTicksToWait
    );

Task Notifications
==================

Faster alternative to semaphores/queues for simple signaling:

.. code-block:: c

    // Give notification (like semaphore give)
    BaseType_t xTaskNotifyGive(TaskHandle_t xTaskToNotify);
    
    // From ISR
    void vTaskNotifyGiveFromISR(
        TaskHandle_t xTaskToNotify,
        BaseType_t *pxHigherPriorityTaskWoken
    );
    
    // Take notification (like semaphore take)
    uint32_t ulTaskNotifyTake(
        BaseType_t xClearCountOnExit,
        TickType_t xTicksToWait
    );
    
    // Send value
    BaseType_t xTaskNotify(
        TaskHandle_t xTaskToNotify,
        uint32_t ulValue,
        eNotifyAction eAction  // eSetBits, eIncrement, eSetValueWithOverwrite, etc.
    );
    
    // Wait for notification
    BaseType_t xTaskNotifyWait(
        uint32_t ulBitsToClearOnEntry,
        uint32_t ulBitsToClearOnExit,
        uint32_t *pulNotificationValue,
        TickType_t xTicksToWait
    );

Memory Management
=================

.. code-block:: c

    // Allocate
    void *pvPortMalloc(size_t xWantedSize);
    
    // Free
    void vPortFree(void *pv);
    
    // Heap info
    size_t xPortGetFreeHeapSize(void);
    size_t xPortGetMinimumEverFreeHeapSize(void);

Critical Sections
=================

.. code-block:: c

    // Disable interrupts
    taskENTER_CRITICAL();
    // Critical code (very short!)
    taskEXIT_CRITICAL();
    
    // From ISR
    UBaseType_t uxSavedInterruptStatus = taskENTER_CRITICAL_FROM_ISR();
    // Critical code
    taskEXIT_CRITICAL_FROM_ISR(uxSavedInterruptStatus);
    
    // Suspend scheduler
    vTaskSuspendAll();
    // No context switches (interrupts still active)
    xTaskResumeAll();

Timers (Software Timers)
========================

.. code-block:: c

    // Create
    TimerHandle_t xTimerCreate(
        const char * const pcTimerName,
        TickType_t xTimerPeriodInTicks,
        UBaseType_t uxAutoReload,  // pdTRUE = auto-reload
        void * pvTimerID,
        TimerCallbackFunction_t pxCallbackFunction
    );
    
    // Start/Stop
    BaseType_t xTimerStart(TimerHandle_t xTimer, TickType_t xTicksToWait);
    BaseType_t xTimerStop(TimerHandle_t xTimer, TickType_t xTicksToWait);
    
    // Reset (restart)
    BaseType_t xTimerReset(TimerHandle_t xTimer, TickType_t xTicksToWait);
    
    // Change period
    BaseType_t xTimerChangePeriod(
        TimerHandle_t xTimer,
        TickType_t xNewPeriod,
        TickType_t xTicksToWait
    );

Utility Macros
==============

.. code-block:: c

    // Time conversion
    pdMS_TO_TICKS(ms)        // Milliseconds to ticks
    portTICK_PERIOD_MS       // Tick period in ms
    
    // Timeouts
    portMAX_DELAY            // Block forever
    
    // Return values
    pdTRUE / pdPASS          // Success
    pdFALSE / pdFAIL         // Failure
    
    // ISR yield
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken)

Heap Schemes
============

.. code-block:: text

    heap_1.c: Simple, no free, smallest code
    heap_2.c: Allow free, best fit, can fragment
    heap_3.c: Wraps malloc/free
    heap_4.c: Coalescence, recommended for most
    heap_5.c: Multiple regions, like heap_4

Select in FreeRTOSConfig.h or Makefile

Example Application
===================

.. code-block:: c

    #include "FreeRTOS.h"
    #include "task.h"
    #include "queue.h"
    
    QueueHandle_t xQueue;
    
    void vProducerTask(void *pvParameters) {
        uint32_t ulValueToSend = 0;
        
        for (;;) {
            xQueueSend(xQueue, &ulValueToSend, portMAX_DELAY);
            ulValueToSend++;
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }
    
    void vConsumerTask(void *pvParameters) {
        uint32_t ulReceivedValue;
        
        for (;;) {
            xQueueReceive(xQueue, &ulReceivedValue, portMAX_DELAY);
            printf("Received: %u\n", (unsigned)ulReceivedValue);
        }
    }
    
    int main(void) {
        xQueue = xQueueCreate(10, sizeof(uint32_t));
        
        xTaskCreate(vProducerTask, "Producer", 1000, NULL, 1, NULL);
        xTaskCreate(vConsumerTask, "Consumer", 1000, NULL, 2, NULL);
        
        vTaskStartScheduler();
        
        for (;;);  // Should never reach here
    }

Common Pitfalls
===============

.. code-block:: text

    ✗ Using vTaskDelay() instead of vTaskDelayUntil()
    ✗ Not checking return values
    ✗ Using mutex from ISR
    ✗ Forgetting portYIELD_FROM_ISR()
    ✗ Stack too small (use uxTaskGetStackHighWaterMark())
    ✗ Heap too small (use xPortGetFreeHeapSize())
    ✗ Priority 0 = idle priority (use 1+)
    ✗ Using delay(0) expecting yield (use taskYIELD())

See Also
========

- :doc:`rtos_basics_cheatsheet` - Generic RTOS concepts
- :doc:`../days/day11` - FreeRTOS deep dive
- Official FreeRTOS documentation: freertos.org
