# :material-arrow-right-circle: Producer-Consumer

<div class="grid cards" markdown>
- :material-lightbulb-on: **Queue** — FreeRTOS queues are the canonical bounded-buffer mechanism; they copy data by value, not by pointer
- :material-chip: **Backpressure** — use a finite send timeout so the producer blocks (or drops) when the queue is full rather than silently losing data
- :material-alert: **ISR as Producer** — ISRs must use `xQueueSendFromISR`; calling `xQueueSend` from an ISR corrupts kernel state
- :material-check-circle: **Use When** — decouple producers and consumers that run at different rates or with different priorities
</div>

---

## :material-lightbulb-on: Pattern Overview

The **producer-consumer** pattern decouples a data source (producer) from a data sink (consumer) using a bounded buffer (queue). The producer generates data items without knowing how fast the consumer can process them; the queue absorbs bursts and provides flow control.

```
┌──────────────┐    xQueueSend()    ┌─────────────┐    xQueueReceive()    ┌──────────────┐
│   Producer   │ ─────────────────▶ │    Queue    │ ──────────────────▶   │   Consumer   │
│  (ISR/Task)  │                    │  [bounded   │                       │    (Task)    │
│              │ ◀─── backpressure─ │   buffer]   │                       │              │
└──────────────┘   (send blocks or  └─────────────┘                       └──────────────┘
                    returns error)
```

---

## :material-code-tags: Full Code Example: Task Producer, Task Consumer

```c
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

/* ---- Shared data type ----------------------------------------- */
typedef struct {
    uint32_t timestamp_ms;
    int16_t  raw_adc[4];
    uint8_t  channel;
} SensorSample_t;

/* ---- Queue ---------------------------------------------------- */
#define QUEUE_LENGTH    16
static QueueHandle_t xSensorQueue;

/* ---- Producer task (higher priority, runs at 50 Hz) ----------- */
static void vSensorProducer(void *pv) {
    TickType_t       xLast   = xTaskGetTickCount();
    const TickType_t xPeriod = pdMS_TO_TICKS(20);   /* 50 Hz */
    SensorSample_t   sample;

    for (;;) {
        /* Acquire sensor data */
        sample.timestamp_ms = xTaskGetTickCount() * portTICK_PERIOD_MS;
        for (int i = 0; i < 4; i++) {
            sample.raw_adc[i] = adc_read_channel(i);
        }
        sample.channel = 0;

        /* Send to queue — block up to 5 ms before dropping */
        if (xQueueSend(xSensorQueue, &sample, pdMS_TO_TICKS(5)) != pdTRUE) {
            /* Queue full — consumer too slow; record overflow */
            stats.queue_overflows++;
        }

        vTaskDelayUntil(&xLast, xPeriod);
    }
}

/* ---- Consumer task (lower priority, processes samples) --------- */
static void vDataConsumer(void *pv) {
    SensorSample_t sample;

    for (;;) {
        /* Block indefinitely until a sample arrives */
        xQueueReceive(xSensorQueue, &sample, portMAX_DELAY);

        /* Process — may take variable time */
        filter_and_log(&sample);
        update_control_state(&sample);
    }
}

/* ---- Initialisation ------------------------------------------- */
void producer_consumer_init(void) {
    xSensorQueue = xQueueCreate(QUEUE_LENGTH, sizeof(SensorSample_t));
    configASSERT(xSensorQueue != NULL);

    xTaskCreate(vSensorProducer, "SensorProd", 512, NULL, 4, NULL);
    xTaskCreate(vDataConsumer,   "DataCons",   512, NULL, 3, NULL);
}
```

---

## :material-flash: ISR as Producer

When the producer is an interrupt (e.g., UART byte received, DMA transfer complete), the ISR must use the `FromISR` variant:

```c
static QueueHandle_t xUartQueue;

/* ---- ISR producer --------------------------------------------- */
void USART1_IRQHandler(void) {
    uint8_t byte = (uint8_t)USART1->RDR;  /* clears interrupt flag */
    BaseType_t xWoken = pdFALSE;

    /* Queue the byte — discard if full (ISR cannot block) */
    xQueueSendFromISR(xUartQueue, &byte, &xWoken);

    /* Yield to unblocked consumer if it has higher priority */
    portYIELD_FROM_ISR(xWoken);
}

/* ---- Task consumer -------------------------------------------- */
static void vUartFramer(void *pv) {
    uint8_t byte;
    uint8_t frame_buf[MAX_FRAME_LEN];
    uint8_t frame_idx = 0;

    for (;;) {
        xQueueReceive(xUartQueue, &byte, portMAX_DELAY);

        /* Assemble frame */
        frame_buf[frame_idx++] = byte;
        if (is_frame_complete(frame_buf, frame_idx)) {
            dispatch_frame(frame_buf, frame_idx);
            frame_idx = 0;
        }
        if (frame_idx >= MAX_FRAME_LEN) {
            frame_idx = 0;  /* overflow guard */
        }
    }
}
```

---

## :material-arrow-split-horizontal: Multi-Producer Single Consumer

Multiple producers writing to a single queue is inherently safe — the RTOS queue is internally thread-safe. Each producer sends independently; the consumer receives in FIFO order.

```c
/* Three sensor tasks all sending to the same processing queue */
typedef struct {
    uint8_t  source_id;   /* which sensor */
    uint32_t value;
} MeasurementMsg_t;

static QueueHandle_t xMeasQueue;

/* Producer A — temperature at 1 Hz */
static void vTempTask(void *pv) {
    MeasurementMsg_t msg = { .source_id = SENSOR_TEMP };
    TickType_t xLast = xTaskGetTickCount();
    for (;;) {
        msg.value = read_temperature();
        xQueueSend(xMeasQueue, &msg, pdMS_TO_TICKS(50));
        vTaskDelayUntil(&xLast, pdMS_TO_TICKS(1000));
    }
}

/* Producer B — pressure at 2 Hz */
static void vPressTask(void *pv) {
    MeasurementMsg_t msg = { .source_id = SENSOR_PRESS };
    TickType_t xLast = xTaskGetTickCount();
    for (;;) {
        msg.value = read_pressure();
        xQueueSend(xMeasQueue, &msg, pdMS_TO_TICKS(50));
        vTaskDelayUntil(&xLast, pdMS_TO_TICKS(500));
    }
}

/* Single consumer — logs all measurements */
static void vLoggerTask(void *pv) {
    MeasurementMsg_t msg;
    for (;;) {
        xQueueReceive(xMeasQueue, &msg, portMAX_DELAY);
        log_measurement(msg.source_id, msg.value);
    }
}
```

---

## :material-scale-balance: Backpressure Strategies

| Strategy | `xQueueSend` call | Behaviour when queue full | Use case |
|----------|------------------|--------------------------|----------|
| **Drop** | timeout = 0 | Return `errQUEUE_FULL`, discard item | Streaming telemetry — old data is stale anyway |
| **Block** | timeout = portMAX_DELAY | Producer blocks until space available | Commands — every item must be processed |
| **Timed block** | timeout = N ticks | Block up to N ticks, then drop with error | Sensor data with soft deadline |
| **Overwrite** | `xQueueOverwrite()` | Always writes; overwrites oldest item | Setpoint/reference signals — only latest matters |

```c
/* xQueueOverwrite: always posts, overwrites if full (queue length must be 1) */
QueueHandle_t xSetpointQ = xQueueCreate(1, sizeof(float));

void set_motor_setpoint(float rpm) {
    xQueueOverwrite(xSetpointQ, &rpm);   /* latest wins */
}

void vControlTask(void *pv) {
    float setpoint;
    for (;;) {
        xQueuePeek(xSetpointQ, &setpoint, 0);   /* non-destructive read */
        apply_control(setpoint);
        vTaskDelayUntil(&xLast, xPeriod);
    }
}
```

---

## :material-message-arrow-right: `xQueueSendToFront` for Priority Messages

Normal `xQueueSend` appends to the back (FIFO). `xQueueSendToFront` prepends to the front, allowing urgent messages to jump the queue:

```c
/* Emergency stop command goes to front, bypassing pending normal commands */
void emergency_stop(void) {
    Command_t cmd = { .type = CMD_EMERGENCY_STOP };
    xQueueSendToFront(xCommandQueue, &cmd, pdMS_TO_TICKS(10));
}
```

---

## :material-help-circle: Flashcards

???+ question "FreeRTOS queues copy data by value. Why is this important?"
    Copying by value means the sender's local variable can go out of scope (e.g., a stack-allocated struct in an ISR) immediately after `xQueueSend`. The queue owns its own copy. If queues used pointers, the sender would need to keep the data alive until the receiver processed it, introducing lifetime management complexity and potential use-after-free bugs.

???+ question "What is the difference between xQueueSend and xQueueSendToBack vs xQueueSendToFront?"
    `xQueueSend` and `xQueueSendToBack` are identical — they append the item to the **tail** of the queue (FIFO order). `xQueueSendToFront` prepends to the **head**, so the item will be the next one received. Use `xQueueSendToFront` for high-priority messages that must bypass pending normal messages.

???+ question "A queue of length 16 is used by an ISR producer. The consumer task is delayed by a high-priority task for 20 ms, and the ISR fires every 1 ms. What happens?"
    The ISR produces 20 items in 20 ms but the queue holds only 16. After item 16, `xQueueSendFromISR` returns `errQUEUE_FULL` for the remaining 4 items — those items are **silently dropped** unless the ISR checks the return value and increments an overflow counter. Solution: increase queue length, increase consumer priority, or reduce ISR rate.

???+ question "When should you use xQueueOverwrite instead of xQueueSend?"
    Use `xQueueOverwrite` (queue length = 1) when only the **latest value** matters and older values should be discarded. Classic use cases: motor setpoints, PID reference signals, display refresh data. Never use it for commands that must all be executed or for logged data where every sample matters.

---

## :material-clipboard-check: Self Test

=== "Question 1"
    You are designing a data logger. A temperature sensor produces a reading every 100 ms. The SD-card write task processes batches of 10 readings at once (once per 1 second). What minimum queue length is needed to avoid dropping data, assuming the write task always completes within 1 second?

=== "Answer 1"
    In 1 second the sensor produces **10 readings**. The consumer drains all 10 at once. The maximum queue depth reached is therefore **10**. Set queue length to **at least 10**, but add margin (e.g., 16) to handle jitter in both producer timing and consumer scheduling.

=== "Question 2"
    An ISR calls `xQueueSendFromISR` but never calls `portYIELD_FROM_ISR`. The consumer task has higher priority than the task that was running when the ISR fired. What is the consequence?

=== "Answer 2"
    The queue item is successfully posted. However, without `portYIELD_FROM_ISR`, the ISR returns to the **interrupted task**, not to the consumer. The consumer will not run until the interrupted task blocks or yields on its own. This introduces latency equal to the remaining execution time of the interrupted task — potentially violating timing requirements. Always call `portYIELD_FROM_ISR(xHigherPriorityTaskWoken)` after `FromISR` calls to ensure immediate context switch when needed.

---

## :material-check-circle: Summary

!!! success "Key Takeaways"
    - FreeRTOS queues provide a **thread-safe, bounded buffer** that copies items by value — safe for ISR producers and concurrent task use.
    - Always use `xQueueSendFromISR` + `portYIELD_FROM_ISR` in ISR producers; never call `xQueueSend` from an ISR.
    - Choose a **backpressure strategy** appropriate to data semantics: block for commands, drop for streaming, overwrite for latest-value signals.
    - `xQueueSendToFront` provides a simple priority lane for urgent messages within a single queue.
    - Check the return value of every `xQueueSend` call — silently dropping data masks production bugs.
    - Size the queue to absorb worst-case producer bursts during maximum consumer latency, then add ≥50% margin.
