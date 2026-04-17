# :material-lightning-bolt: FreeRTOS Cheatsheet

> **Dense quick-reference** for FreeRTOS V11.x. All major APIs, config options, heap schemes, and common patterns in one page.

---

<div class="grid cards" markdown>

-   :material-cpu-64-bit: **Kernel**

    ---
    Preemptive / cooperative. Priority 0 (lowest) to `configMAX_PRIORITIES-1` (highest). Tick-based time with optional tickless idle.

-   :material-memory: **Footprint**

    ---
    Min ~4 KB RAM, ~6 KB Flash. Heap1–5 selectable. Static allocation eliminates heap entirely.

-   :material-github: **Source**

    ---
    [github.com/FreeRTOS/FreeRTOS-Kernel](https://github.com/FreeRTOS/FreeRTOS-Kernel) — MIT License

-   :material-book-open: **Config**

    ---
    All tuning via `FreeRTOSConfig.h`. See config table below.

</div>

---

## Task API

| Function | Signature | Description |
|----------|-----------|-------------|
| `xTaskCreate` | `BaseType_t xTaskCreate(TaskFunction_t pvTaskCode, const char *pcName, configSTACK_DEPTH_TYPE usStackDepth, void *pvParameters, UBaseType_t uxPriority, TaskHandle_t *pxCreatedTask)` | Create task with dynamic stack allocation |
| `xTaskCreateStatic` | `TaskHandle_t xTaskCreateStatic(TaskFunction_t pvTaskCode, const char *pcName, uint32_t ulStackDepth, void *pvParameters, UBaseType_t uxPriority, StackType_t *puxStackBuffer, StaticTask_t *pxTaskBuffer)` | Create task with static stack (no heap) |
| `vTaskDelete` | `void vTaskDelete(TaskHandle_t xTask)` | Delete task; pass `NULL` to delete calling task |
| `vTaskDelay` | `void vTaskDelay(TickType_t xTicksToDelay)` | Block for relative number of ticks |
| `vTaskDelayUntil` | `void vTaskDelayUntil(TickType_t *pxPreviousWakeTime, TickType_t xTimeIncrement)` | Block until absolute tick — use for fixed-rate loops |
| `uxTaskGetStackHighWaterMark` | `UBaseType_t uxTaskGetStackHighWaterMark(TaskHandle_t xTask)` | Stack remaining at minimum — detect overflow risk |
| `xTaskGetHandle` | `TaskHandle_t xTaskGetHandle(const char *pcNameToQuery)` | Look up handle by task name (slow — avoid in fast paths) |
| `vTaskSuspend` | `void vTaskSuspend(TaskHandle_t xTaskToSuspend)` | Suspend indefinitely until resumed |
| `vTaskResume` | `void vTaskResume(TaskHandle_t xTaskToResume)` | Resume a suspended task |
| `uxTaskPriorityGet` | `UBaseType_t uxTaskPriorityGet(TaskHandle_t xTask)` | Get current priority |
| `vTaskPrioritySet` | `void vTaskPrioritySet(TaskHandle_t xTask, UBaseType_t uxNewPriority)` | Change priority at runtime |

```c title="Task Pattern — Fixed-Rate Loop"
void vSensorTask(void *pvParameters) {
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xPeriod = pdMS_TO_TICKS(100); // 100 ms

    for (;;) {
        // Do work here
        sensor_read();
        process_data();

        // Block until next period — compensates for execution time
        vTaskDelayUntil(&xLastWakeTime, xPeriod);
    }
    // Never reaches here; vTaskDelete(NULL) if task must exit
}

// Create with 512-word stack, priority 3
xTaskCreate(vSensorTask, "Sensor", 512, NULL, 3, NULL);
```

---

## Semaphore API

| Function | Signature | Description |
|----------|-----------|-------------|
| `xSemaphoreCreateBinary` | `SemaphoreHandle_t xSemaphoreCreateBinary(void)` | Binary semaphore (0 or 1). Not given initially — must `Give` first |
| `xSemaphoreCreateBinaryStatic` | `SemaphoreHandle_t xSemaphoreCreateBinaryStatic(StaticSemaphore_t *pxSemaphoreBuffer)` | Static version (no heap) |
| `xSemaphoreCreateCounting` | `SemaphoreHandle_t xSemaphoreCreateCounting(UBaseType_t uxMaxCount, UBaseType_t uxInitialCount)` | Counting semaphore up to `uxMaxCount` |
| `xSemaphoreCreateMutex` | `SemaphoreHandle_t xSemaphoreCreateMutex(void)` | Mutex with priority inheritance |
| `xSemaphoreCreateRecursiveMutex` | `SemaphoreHandle_t xSemaphoreCreateRecursiveMutex(void)` | Reentrant mutex — same task can lock multiple times |
| `xSemaphoreTake` | `BaseType_t xSemaphoreTake(SemaphoreHandle_t xSemaphore, TickType_t xTicksToWait)` | Acquire; `portMAX_DELAY` to block forever |
| `xSemaphoreGive` | `BaseType_t xSemaphoreGive(SemaphoreHandle_t xSemaphore)` | Release from task context |
| `xSemaphoreGiveFromISR` | `BaseType_t xSemaphoreGiveFromISR(SemaphoreHandle_t xSemaphore, BaseType_t *pxHigherPriorityTaskWoken)` | Release from ISR — call `portYIELD_FROM_ISR` after |
| `xSemaphoreTakeFromISR` | `BaseType_t xSemaphoreTakeFromISR(SemaphoreHandle_t xSemaphore, BaseType_t *pxHigherPriorityTaskWoken)` | Take from ISR (binary/counting only) |

```c title="Semaphore Pattern — ISR to Task Synchronization"
SemaphoreHandle_t xDataReadySem;

// ISR — fires when hardware data is ready
void EXTI0_IRQHandler(void) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xSemaphoreGiveFromISR(xDataReadySem, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);  // reschedule if needed
}

// Task — waits for data
void vProcessTask(void *pv) {
    xDataReadySem = xSemaphoreCreateBinary();
    for (;;) {
        if (xSemaphoreTake(xDataReadySem, pdMS_TO_TICKS(1000)) == pdTRUE) {
            process_data();
        } else {
            handle_timeout();
        }
    }
}
```

---

## Queue API

| Function | Signature | Description |
|----------|-----------|-------------|
| `xQueueCreate` | `QueueHandle_t xQueueCreate(UBaseType_t uxQueueLength, UBaseType_t uxItemSize)` | Create queue with N items of given size (bytes) |
| `xQueueCreateStatic` | `QueueHandle_t xQueueCreateStatic(UBaseType_t uxQueueLength, UBaseType_t uxItemSize, uint8_t *pucQueueStorage, StaticQueue_t *pxQueueBuffer)` | Static version |
| `xQueueSend` | `BaseType_t xQueueSend(QueueHandle_t xQueue, const void *pvItemToQueue, TickType_t xTicksToWait)` | Send to back of queue (same as `xQueueSendToBack`) |
| `xQueueSendToFront` | `BaseType_t xQueueSendToFront(QueueHandle_t xQueue, const void *pvItemToQueue, TickType_t xTicksToWait)` | Send to front (high priority message) |
| `xQueueReceive` | `BaseType_t xQueueReceive(QueueHandle_t xQueue, void *pvBuffer, TickType_t xTicksToWait)` | Receive and remove from front |
| `xQueuePeek` | `BaseType_t xQueuePeek(QueueHandle_t xQueue, void *pvBuffer, TickType_t xTicksToWait)` | Read without removing |
| `xQueueSendFromISR` | `BaseType_t xQueueSendFromISR(QueueHandle_t xQueue, const void *pvItemToQueue, BaseType_t *pxHigherPriorityTaskWoken)` | Send from ISR |
| `xQueueReceiveFromISR` | `BaseType_t xQueueReceiveFromISR(QueueHandle_t xQueue, void *pvBuffer, BaseType_t *pxHigherPriorityTaskWoken)` | Receive from ISR |
| `uxQueueMessagesWaiting` | `UBaseType_t uxQueueMessagesWaiting(QueueHandle_t xQueue)` | Number of items in queue |
| `uxQueueSpacesAvailable` | `UBaseType_t uxQueueSpacesAvailable(QueueHandle_t xQueue)` | Remaining capacity |
| `xQueueReset` | `BaseType_t xQueueReset(QueueHandle_t xQueue)` | Flush all items from queue |

```c title="Queue Pattern — Producer/Consumer"
typedef struct { uint32_t id; uint8_t data[16]; } Message_t;

QueueHandle_t xMsgQueue;

void vProducerTask(void *pv) {
    Message_t msg = {0};
    xMsgQueue = xQueueCreate(10, sizeof(Message_t));
    for (;;) {
        msg.id++;
        collect_data(msg.data);
        // Block up to 100ms if queue full
        xQueueSend(xMsgQueue, &msg, pdMS_TO_TICKS(100));
    }
}

void vConsumerTask(void *pv) {
    Message_t msg;
    for (;;) {
        if (xQueueReceive(xMsgQueue, &msg, portMAX_DELAY) == pdTRUE) {
            handle_message(&msg);
        }
    }
}
```

---

## Event Group API

| Function | Signature | Description |
|----------|-----------|-------------|
| `xEventGroupCreate` | `EventGroupHandle_t xEventGroupCreate(void)` | Create event group (24 bits usable) |
| `xEventGroupSetBits` | `EventBits_t xEventGroupSetBits(EventGroupHandle_t xEventGroup, EventBits_t uxBitsToSet)` | Set bits from task context |
| `xEventGroupClearBits` | `EventBits_t xEventGroupClearBits(EventGroupHandle_t xEventGroup, EventBits_t uxBitsToClear)` | Clear bits from task context |
| `xEventGroupWaitBits` | `EventBits_t xEventGroupWaitBits(EventGroupHandle_t xEventGroup, EventBits_t uxBitsToWaitFor, BaseType_t xClearOnExit, BaseType_t xWaitForAllBits, TickType_t xTicksToWait)` | Wait for any or all bits |
| `xEventGroupGetBits` | `EventBits_t xEventGroupGetBits(EventGroupHandle_t xEventGroup)` | Read current bits without blocking |
| `xEventGroupSetBitsFromISR` | `BaseType_t xEventGroupSetBitsFromISR(EventGroupHandle_t xEventGroup, EventBits_t uxBitsToSet, BaseType_t *pxHigherPriorityTaskWoken)` | Set bits from ISR |

```c title="Event Group Pattern — Multiple Event Sources"
#define EVT_UART_RX   (1 << 0)
#define EVT_BUTTON    (1 << 1)
#define EVT_TIMER     (1 << 2)

EventGroupHandle_t xEvents;

void vControlTask(void *pv) {
    xEvents = xEventGroupCreate();
    EventBits_t bits;
    for (;;) {
        // Wait for ANY of the three events, clear on exit
        bits = xEventGroupWaitBits(xEvents,
                                   EVT_UART_RX | EVT_BUTTON | EVT_TIMER,
                                   pdTRUE,   // clear bits on exit
                                   pdFALSE,  // wait for ANY (not all)
                                   portMAX_DELAY);
        if (bits & EVT_UART_RX)  handle_uart();
        if (bits & EVT_BUTTON)   handle_button();
        if (bits & EVT_TIMER)    handle_timer();
    }
}
```

---

## Timer API

| Function | Signature | Description |
|----------|-----------|-------------|
| `xTimerCreate` | `TimerHandle_t xTimerCreate(const char *pcTimerName, TickType_t xTimerPeriodInTicks, BaseType_t xAutoReload, void *pvTimerID, TimerCallbackFunction_t pxCallbackFunction)` | Create software timer |
| `xTimerStart` | `BaseType_t xTimerStart(TimerHandle_t xTimer, TickType_t xTicksToWait)` | Start / restart timer |
| `xTimerStop` | `BaseType_t xTimerStop(TimerHandle_t xTimer, TickType_t xTicksToWait)` | Stop timer |
| `xTimerReset` | `BaseType_t xTimerReset(TimerHandle_t xTimer, TickType_t xTicksToWait)` | Reset period (restart countdown) |
| `xTimerChangePeriod` | `BaseType_t xTimerChangePeriod(TimerHandle_t xTimer, TickType_t xNewPeriod, TickType_t xTicksToWait)` | Change period (and start if stopped) |
| `xTimerIsTimerActive` | `BaseType_t xTimerIsTimerActive(TimerHandle_t xTimer)` | Check if timer is running |
| `pvTimerGetTimerID` | `void *pvTimerGetTimerID(TimerHandle_t xTimer)` | Retrieve timer ID (shared callback pattern) |

```c title="Timer Pattern — Watchdog / Debounce"
TimerHandle_t xWatchdogTimer;

void vWatchdogCallback(TimerHandle_t xTimer) {
    // Called in timer task context — not ISR
    // System did not kick watchdog in time
    trigger_system_reset();
}

// One-shot watchdog — must reset before 5s expiry
xWatchdogTimer = xTimerCreate("WDT", pdMS_TO_TICKS(5000),
                               pdFALSE,  // one-shot
                               NULL, vWatchdogCallback);
xTimerStart(xWatchdogTimer, 0);

// In main task — periodic kick:
xTimerReset(xWatchdogTimer, pdMS_TO_TICKS(10));
```

---

## Task Notification API

Task notifications are faster than semaphores/queues — zero additional memory, direct-to-task signaling.

| Function | Signature | Description |
|----------|-----------|-------------|
| `xTaskNotify` | `BaseType_t xTaskNotify(TaskHandle_t xTaskToNotify, uint32_t ulValue, eNotifyAction eAction)` | Send notification with action (set bits, increment, overwrite, etc.) |
| `xTaskNotifyWait` | `BaseType_t xTaskNotifyWait(uint32_t ulBitsToClearOnEntry, uint32_t ulBitsToClearOnExit, uint32_t *pulNotificationValue, TickType_t xTicksToWait)` | Wait for notification |
| `xTaskNotifyGive` | `BaseType_t xTaskNotifyGive(TaskHandle_t xTaskToNotify)` | Increment notification value (like semaphore give) |
| `ulTaskNotifyTake` | `uint32_t ulTaskNotifyTake(BaseType_t xClearCountOnExit, TickType_t xTicksToWait)` | Decrement/clear notification (like semaphore take) |
| `xTaskNotifyFromISR` | `BaseType_t xTaskNotifyFromISR(TaskHandle_t xTaskToNotify, uint32_t ulValue, eNotifyAction eAction, BaseType_t *pxHigherPriorityTaskWoken)` | Notify from ISR |
| `xTaskNotifyGiveFromISR` | `BaseType_t xTaskNotifyGiveFromISR(TaskHandle_t xTaskToNotify, BaseType_t *pxHigherPriorityTaskWoken)` | Give from ISR (like semaphore) |

```c title="Task Notification Pattern — Lightweight ISR Signaling"
TaskHandle_t xProcessTaskHandle;

void UART_IRQHandler(void) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    // Faster than xSemaphoreGiveFromISR — no separate semaphore object
    vTaskNotifyGiveFromISR(xProcessTaskHandle, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

void vProcessTask(void *pv) {
    xProcessTaskHandle = xTaskGetCurrentTaskHandle();
    for (;;) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY); // clear on exit
        process_uart_data();
    }
}
```

---

## FreeRTOSConfig.h Key Settings

| Macro | Typical Value | Description |
|-------|---------------|-------------|
| `configUSE_PREEMPTION` | `1` | Enable preemptive scheduling |
| `configUSE_TIME_SLICING` | `1` | Round-robin among equal-priority tasks |
| `configUSE_TICKLESS_IDLE` | `1` | Low-power tickless idle |
| `configCPU_CLOCK_HZ` | (board-specific) | CPU frequency in Hz |
| `configTICK_RATE_HZ` | `1000` | Tick frequency (1 ms tick) |
| `configMAX_PRIORITIES` | `5` to `32` | Number of priority levels |
| `configMINIMAL_STACK_SIZE` | `128` | Minimum stack in words for idle task |
| `configTOTAL_HEAP_SIZE` | `(16*1024)` | Total heap for all dynamic allocations |
| `configMAX_TASK_NAME_LEN` | `16` | Task name string length limit |
| `configUSE_TRACE_FACILITY` | `1` | Enable trace/stats (costs ROM/RAM) |
| `configUSE_STATS_FORMATTING_FUNCTIONS` | `1` | Enable `vTaskList()` / `vTaskGetRunTimeStats()` |
| `configUSE_MUTEXES` | `1` | Enable mutex with priority inheritance |
| `configUSE_RECURSIVE_MUTEXES` | `1` | Enable recursive mutexes |
| `configUSE_COUNTING_SEMAPHORES` | `1` | Enable counting semaphores |
| `configUSE_QUEUE_SETS` | `1` | Enable queue sets |
| `configUSE_TASK_NOTIFICATIONS` | `1` | Enable task notifications |
| `configUSE_TIMERS` | `1` | Enable software timers |
| `configTIMER_TASK_PRIORITY` | `(configMAX_PRIORITIES-1)` | Timer daemon task priority |
| `configTIMER_QUEUE_LENGTH` | `10` | Timer command queue depth |
| `configUSE_MALLOC_FAILED_HOOK` | `1` | Call hook if heap exhausted |
| `configCHECK_FOR_STACK_OVERFLOW` | `2` | Stack overflow detection mode (0/1/2) |
| `configUSE_IDLE_HOOK` | `1` | Call `vApplicationIdleHook()` in idle |
| `configUSE_TICK_HOOK` | `0` | Call hook each tick (adds overhead) |
| `configSUPPORT_STATIC_ALLOCATION` | `1` | Enable static allocation APIs |
| `configSUPPORT_DYNAMIC_ALLOCATION` | `1` | Enable dynamic allocation APIs |

---

## Heap Scheme Selection Guide

| Scheme | File | Algorithm | Thread Safe | Free | Best For |
|--------|------|-----------|:-----------:|:----:|----------|
| **Heap 1** | `heap_1.c` | Simple bump allocator | Yes | No | Tasks created at startup only, never deleted |
| **Heap 2** | `heap_2.c` | Best-fit, no coalesce | Yes | Yes | Fixed-size allocations; same-size realloc pattern |
| **Heap 3** | `heap_3.c` | Wraps `malloc()`/`free()` | Suspends scheduler | Yes | When existing libc malloc is already present |
| **Heap 4** | `heap_4.c` | First-fit, coalesces adjacent | Yes | Yes | **Default choice** — general purpose, no fragmentation in steady state |
| **Heap 5** | `heap_5.c` | Heap 4 across multiple regions | Yes | Yes | Non-contiguous RAM (SRAM + CCMRAM, etc.) |

---

## Gotchas & Pitfalls

!!! danger "Priority Inversion — Always Use Mutexes, Not Binary Semaphores for Mutual Exclusion"
    Binary semaphores do NOT implement priority inheritance. A low-priority task holding a binary semaphore can starve a high-priority task. Use `xSemaphoreCreateMutex()` for mutual exclusion — it implements priority inheritance automatically.

!!! danger "Never Call Blocking API from Timer Callbacks or ISRs"
    Timer callbacks run in the timer daemon task context. Calling `xQueueReceive(portMAX_DELAY)` or `xSemaphoreTake(portMAX_DELAY)` will deadlock. Use `FromISR` variants in ISRs and non-blocking calls (timeout=0) in timer callbacks.

!!! warning "Stack Overflow Detection — Set configCHECK_FOR_STACK_OVERFLOW to 2"
    Mode 1 checks only at context switch (fast but misses transient overflows). Mode 2 also checks the stack watermark pattern — recommended for development.

!!! warning "vTaskDelay vs vTaskDelayUntil — Drift Accumulates with vTaskDelay"
    `vTaskDelay(100)` means "100 ticks after now" — execution time accumulates as drift. `vTaskDelayUntil` means "wake at the next absolute multiple of the period" — use it for precise timing.

!!! tip "Use pdMS_TO_TICKS() Always"
    Never hardcode tick counts. `pdMS_TO_TICKS(100)` correctly converts 100 ms to ticks regardless of `configTICK_RATE_HZ`. If you change the tick rate, tick-count literals silently break.

!!! tip "Minimum Stack Size — Add Margin"
    `configMINIMAL_STACK_SIZE` is the idle task stack in words. Application tasks need more — typically 256–1024 words. Call `uxTaskGetStackHighWaterMark(NULL)` periodically during testing to measure actual usage.
