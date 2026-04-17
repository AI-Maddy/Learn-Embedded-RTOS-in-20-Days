# :material-chip: ChibiOS/RT Cheatsheet

> **Dense quick-reference** for ChibiOS/RT 21.x. Threads, semaphores, mutexes, mailboxes, events, HAL GPIO, and system lock APIs.

---

<div class="grid cards" markdown>

-   :material-cpu-64-bit: **Kernel**

    ---
    Preemptive priority-based. Priority 1 (lowest) to 127 (highest). Thread-based and IRQ-based designs supported. Extremely compact and fast.

-   :material-memory: **Footprint**

    ---
    Min ~2 KB RAM, ~8 KB Flash. All objects are statically allocated — no heap required for kernel primitives.

-   :material-github: **Source**

    ---
    [github.com/ChibiOS/ChibiOS](https://github.com/ChibiOS/ChibiOS) — GPL v3 / Commercial

-   :material-sticker-check: **Best For**

    ---
    STM32 ecosystem, clean HAL, real-time automotive and industrial applications. Excellent STM32CubeIDE integration.

</div>

---

## Thread API

| Function / Macro | Signature | Description |
|------------------|-----------|-------------|
| `THD_WORKING_AREA` | `THD_WORKING_AREA(name, size)` | Declare static stack working area for a thread |
| `chThdCreateStatic` | `thread_t *chThdCreateStatic(void *wsp, size_t size, tprio_t prio, tfunc_t pf, void *arg)` | Create thread with pre-declared working area |
| `chThdCreateFromHeap` | `thread_t *chThdCreateFromHeap(memory_heap_t *heapp, size_t size, const char *name, tprio_t prio, tfunc_t pf, void *arg)` | Create thread from heap (dynamic) |
| `chThdExit` | `void chThdExit(msg_t msg)` | Terminate calling thread with exit code |
| `chThdWait` | `msg_t chThdWait(thread_t *tp)` | Wait for thread termination and collect exit code |
| `chThdSleep` | `void chThdSleep(sysinterval_t time)` | Sleep for system time interval |
| `chThdSleepMilliseconds` | `void chThdSleepMilliseconds(uint32_t msec)` | Sleep for milliseconds |
| `chThdSleepMicroseconds` | `void chThdSleepMicroseconds(uint32_t usec)` | Sleep for microseconds |
| `chThdSleepUntil` | `void chThdSleepUntil(systime_t time)` | Sleep until absolute system time |
| `chThdSleepUntilWindowed` | `systime_t chThdSleepUntilWindowed(systime_t prev, systime_t next)` | Windowed sleep for fixed-rate loops |
| `chThdSuspendS` | `msg_t chThdSuspendS(thread_reference_t *trp)` | Suspend with reference (system-locked) |
| `chThdResumeI` | `void chThdResumeI(thread_reference_t *trp, msg_t msg)` | Resume suspended thread (ISR context) |
| `chThdGetPriorityX` | `tprio_t chThdGetPriorityX(void)` | Get current thread priority |
| `chThdSetPriority` | `tprio_t chThdSetPriority(tprio_t newprio)` | Change priority, returns old priority |

```c title="Thread Pattern — Static Working Area"
#include "ch.h"
#include "hal.h"

// Declare 1024-byte working area (stack) for sensor thread
THD_WORKING_AREA(waSensorThread, 1024);

THD_FUNCTION(SensorThread, arg) {
    (void)arg;
    systime_t deadline = chVTGetSystemTimeX();

    while (true) {
        read_sensor();
        // Fixed-rate: wake at next 100ms boundary (no drift)
        deadline = chTimeAddX(deadline, TIME_MS2I(100));
        chThdSleepUntil(deadline);
    }
}

// Create in main():
chThdCreateStatic(waSensorThread, sizeof(waSensorThread),
                  NORMALPRIO + 1, SensorThread, NULL);
```

---

## Semaphore API

| Function | Signature | Description |
|----------|-----------|-------------|
| `chSemObjectInit` | `void chSemObjectInit(semaphore_t *sp, cnt_t n)` | Initialize semaphore with count n |
| `chSemWait` | `msg_t chSemWait(semaphore_t *sp)` | Acquire (decrement); blocks if count = 0 |
| `chSemWaitTimeout` | `msg_t chSemWaitTimeout(semaphore_t *sp, sysinterval_t time)` | Acquire with timeout |
| `chSemSignal` | `void chSemSignal(semaphore_t *sp)` | Release (increment) from task context |
| `chSemSignalI` | `void chSemSignalI(semaphore_t *sp)` | Release from ISR (I-lock required) |
| `chSemReset` | `void chSemReset(semaphore_t *sp, cnt_t n)` | Reset count and wake all waiters |
| `chSemResetI` | `void chSemResetI(semaphore_t *sp, cnt_t n)` | Reset from ISR (I-lock required) |
| `chSemGetCounterI` | `cnt_t chSemGetCounterI(const semaphore_t *sp)` | Read current count |

```c title="Semaphore Pattern — ISR to Task"
static semaphore_t uart_rx_sem;

// In init:
chSemObjectInit(&uart_rx_sem, 0);

// UART RX ISR:
void USART2_IRQHandler(void) {
    CH_IRQ_PROLOGUE();
    chSysLockFromISR();
    chSemSignalI(&uart_rx_sem);
    chSysUnlockFromISR();
    CH_IRQ_EPILOGUE();
}

// Task:
THD_FUNCTION(RxTask, arg) {
    while (true) {
        chSemWait(&uart_rx_sem);
        process_uart_data();
    }
}
```

---

## Mutex API

| Function | Signature | Description |
|----------|-----------|-------------|
| `chMtxObjectInit` | `void chMtxObjectInit(mutex_t *mp)` | Initialize mutex |
| `chMtxLock` | `void chMtxLock(mutex_t *mp)` | Lock mutex (blocks if held; priority inheritance) |
| `chMtxTryLock` | `bool chMtxTryLock(mutex_t *mp)` | Try to lock; return false if already held |
| `chMtxUnlock` | `mutex_t *chMtxUnlock(mutex_t *mp)` | Unlock most recently locked mutex |
| `chMtxUnlockAll` | `void chMtxUnlockAll(void)` | Unlock all mutexes held by current thread |

!!! note "LIFO Mutex Unlock"
    ChibiOS mutexes must be unlocked in LIFO (last-in, first-out) order. This is a design constraint that eliminates deadlock scenarios. `chMtxUnlockAll()` provides an emergency release of all held mutexes.

```c title="Mutex Pattern"
static mutex_t i2c_mutex;
chMtxObjectInit(&i2c_mutex);

void access_i2c(void) {
    chMtxLock(&i2c_mutex);
    i2c_transfer();
    chMtxUnlock(&i2c_mutex);  // must match lock order
}
```

---

## Mailbox API

Mailboxes are bounded FIFO queues for passing pointers (messages as `msg_t = void*`).

| Function | Signature | Description |
|----------|-----------|-------------|
| `chMBObjectInit` | `void chMBObjectInit(mailbox_t *mbp, msg_t *buf, size_t n)` | Initialize mailbox with buffer of n slots |
| `chMBPost` | `msg_t chMBPost(mailbox_t *mbp, msg_t msg, sysinterval_t timeout)` | Send to back; blocks if full |
| `chMBPostAhead` | `msg_t chMBPostAhead(mailbox_t *mbp, msg_t msg, sysinterval_t timeout)` | Send to front (priority) |
| `chMBPostI` | `msg_t chMBPostI(mailbox_t *mbp, msg_t msg)` | Send from ISR (I-lock required) |
| `chMBFetch` | `msg_t chMBFetch(mailbox_t *mbp, msg_t *msgp, sysinterval_t timeout)` | Receive from front; blocks if empty |
| `chMBFetchI` | `msg_t chMBFetchI(mailbox_t *mbp, msg_t *msgp)` | Receive from ISR |
| `chMBGetUsedCountI` | `size_t chMBGetUsedCountI(const mailbox_t *mbp)` | Number of pending messages |
| `chMBReset` | `void chMBReset(mailbox_t *mbp)` | Flush mailbox and wake waiters |

```c title="Mailbox Pattern — Message Passing"
#define MAILBOX_SIZE 8
static mailbox_t cmd_mailbox;
static msg_t     cmd_buffer[MAILBOX_SIZE];

// In init:
chMBObjectInit(&cmd_mailbox, cmd_buffer, MAILBOX_SIZE);

// Producer:
Command_t *cmd = alloc_command();
chMBPost(&cmd_mailbox, (msg_t)cmd, TIME_MS2I(100));

// Consumer:
msg_t received;
chMBFetch(&cmd_mailbox, &received, TIME_INFINITE);
handle_command((Command_t *)received);
free_command((Command_t *)received);
```

---

## Event API

Events provide a broadcast mechanism — multiple threads can wait for events from multiple sources.

| Function | Signature | Description |
|----------|-----------|-------------|
| `chEvtObjectInit` | `void chEvtObjectInit(event_source_t *esp)` | Initialize event source |
| `chEvtRegister` | `void chEvtRegister(event_source_t *esp, event_listener_t *elp, eventid_t eid)` | Register listener with event ID |
| `chEvtUnregister` | `void chEvtUnregister(event_source_t *esp, event_listener_t *elp)` | Unregister listener |
| `chEvtBroadcast` | `void chEvtBroadcast(event_source_t *esp)` | Broadcast event to all listeners |
| `chEvtBroadcastI` | `void chEvtBroadcastI(event_source_t *esp)` | Broadcast from ISR |
| `chEvtWaitOne` | `eventmask_t chEvtWaitOne(eventmask_t ewmask)` | Wait for any one specified event |
| `chEvtWaitAny` | `eventmask_t chEvtWaitAny(eventmask_t ewmask)` | Wait for any specified events |
| `chEvtWaitAll` | `eventmask_t chEvtWaitAll(eventmask_t ewmask)` | Wait for ALL specified events |
| `chEvtWaitAnyTimeout` | `eventmask_t chEvtWaitAnyTimeout(eventmask_t ewmask, sysinterval_t time)` | Wait with timeout |
| `chEvtGetAndClearEvents` | `eventmask_t chEvtGetAndClearEvents(eventmask_t ewmask)` | Read and clear pending events |

```c title="Event Pattern — Multiple Sources"
#define EVT_UART  EVENT_MASK(0)
#define EVT_BTN   EVENT_MASK(1)
#define EVT_ADC   EVENT_MASK(2)

event_source_t uart_event, btn_event, adc_event;

THD_FUNCTION(DispatchTask, arg) {
    event_listener_t uart_listener, btn_listener, adc_listener;

    chEvtObjectInit(&uart_event);
    chEvtRegister(&uart_event, &uart_listener, 0);
    chEvtRegister(&btn_event,  &btn_listener,  1);
    chEvtRegister(&adc_event,  &adc_listener,  2);

    while (true) {
        eventmask_t evt = chEvtWaitAny(EVT_UART | EVT_BTN | EVT_ADC);
        if (evt & EVT_UART) handle_uart();
        if (evt & EVT_BTN)  handle_button();
        if (evt & EVT_ADC)  handle_adc();
    }
}
```

---

## HAL GPIO

| Function | Description |
|----------|-------------|
| `palSetPad(port, pad)` | Set pad HIGH |
| `palClearPad(port, pad)` | Set pad LOW |
| `palTogglePad(port, pad)` | Toggle pad |
| `palReadPad(port, pad)` | Read pad value (0 or 1) |
| `palSetPadMode(port, pad, mode)` | Set pad mode (INPUT, OUTPUT, ALTERNATE, etc.) |
| `palEnablePadEvent(port, pad, mode)` | Enable pad edge event (RISING, FALLING, BOTH) |
| `palSetLineMode(line, mode)` | Set mode using GPIO_LINE macro (board.h) |
| `palSetLine(line)` | Set line HIGH using LINE macro |
| `palClearLine(line)` | Set line LOW using LINE macro |
| `palToggleLine(line)` | Toggle using LINE macro |
| `palReadLine(line)` | Read LINE value |

```c title="GPIO Pattern — Using Board Lines"
// board.h typically defines: LINE_LED1, LINE_BUTTON, etc.
palSetLineMode(LINE_LED1, PAL_MODE_OUTPUT_PUSHPULL);
palClearLine(LINE_LED1);   // LED off

// In loop:
palToggleLine(LINE_LED1);
chThdSleepMilliseconds(500);
```

---

## System Lock / Unlock

ChibiOS has two lock levels: **S-lock** (scheduler lock, disables preemption) and **I-lock** (interrupt lock, disables interrupts).

| Function | Context | Description |
|----------|---------|-------------|
| `chSysLock()` | Task | Enter S-lock (disable scheduling) |
| `chSysUnlock()` | Task | Exit S-lock |
| `chSysLockFromISR()` | ISR | Enter I-lock inside ISR (nested disable) |
| `chSysUnlockFromISR()` | ISR | Exit I-lock inside ISR |
| `chSysDisable()` | Any | Disable all interrupts (strongest) |
| `chSysEnable()` | Any | Enable all interrupts |
| `CH_IRQ_PROLOGUE()` | ISR start | ChibiOS ISR entry bookkeeping macro |
| `CH_IRQ_EPILOGUE()` | ISR end | ChibiOS ISR exit + reschedule if needed |

```c title="System Lock Pattern — ISR with Kernel Interaction"
// Standard ChibiOS ISR template
CH_IRQ_HANDLER(USART2_IRQHandler) {
    CH_IRQ_PROLOGUE();         // ChibiOS bookkeeping

    chSysLockFromISR();
    // Safe to call I-class kernel functions here (_I suffix)
    chSemSignalI(&uart_sem);
    chSysUnlockFromISR();

    CH_IRQ_EPILOGUE();         // may reschedule
}
```

!!! warning "Function Naming Conventions"
    ChibiOS uses suffixes to indicate calling context:
    
    - No suffix — callable from normal thread context
    - `S` suffix (e.g., `chSemSignalS`) — requires S-lock (`chSysLock()`)
    - `I` suffix (e.g., `chSemSignalI`) — requires I-lock (`chSysLockFromISR()`)
    - `X` suffix — callable from any context (most restrictive internally)

---

## Key Configuration (chconf.h / halconf.h)

| Macro | Typical Value | Description |
|-------|---------------|-------------|
| `CH_CFG_ST_FREQUENCY` | `10000` | System tick frequency (Hz) — higher = finer resolution |
| `CH_CFG_ST_TIMEDELTA` | `2` | Minimum tick delta for tickless idle |
| `CH_CFG_TIME_QUANTUM` | `20` | Time quantum for round-robin (0 = disable) |
| `CH_CFG_USE_MUTEXES` | `TRUE` | Enable mutexes |
| `CH_CFG_USE_CONDVARS` | `TRUE` | Enable condition variables |
| `CH_CFG_USE_SEMAPHORES` | `TRUE` | Enable semaphores |
| `CH_CFG_USE_MAILBOXES` | `TRUE` | Enable mailboxes |
| `CH_CFG_USE_EVENTS` | `TRUE` | Enable event flags |
| `CH_CFG_USE_DYNAMIC` | `TRUE` | Enable dynamic thread creation |
| `CH_CFG_USE_HEAP` | `TRUE` | Enable heap (for dynamic threads) |
| `CH_DBG_ENABLE_CHECKS` | `TRUE` | Enable parameter checks (debug) |
| `CH_DBG_ENABLE_ASSERTS` | `TRUE` | Enable assertions (debug) |
| `CH_DBG_ENABLE_STACK_CHECK` | `TRUE` | Enable stack overflow check |
| `CH_DBG_FILL_THREADS` | `TRUE` | Fill thread stacks (watermark) |
| `CH_DBG_STATISTICS` | `TRUE` | Enable thread runtime statistics |

---

## Gotchas & Pitfalls

!!! danger "S-lock and I-lock Must Be Matched"
    Every `chSysLock()` must be paired with `chSysUnlock()`. Every `chSysLockFromISR()` must be paired with `chSysUnlockFromISR()`. Mismatching these corrupts the lock nesting counter and will cause subtle bugs or hangs.

!!! danger "Never Call Normal (no-suffix) Functions from ISR"
    Calling `chSemSignal()` from an ISR instead of `chSemSignalI()` will likely corrupt the kernel state or trigger an assertion. Use only `_I` suffix functions inside ISR bodies (between `CH_IRQ_PROLOGUE` / `CH_IRQ_EPILOGUE`).

!!! warning "LIFO Mutex Order — Design Your Lock Hierarchy"
    Mutexes must be released in LIFO order. If task A locks Mutex1 then Mutex2, it must unlock Mutex2 before Mutex1. Design a clear lock hierarchy to avoid lock ordering bugs. `chMtxUnlockAll()` is a safety escape hatch.

!!! warning "chThdSleepMilliseconds Minimum Resolution"
    The minimum sleep resolution is one system tick. With `CH_CFG_ST_FREQUENCY = 1000`, minimum is 1 ms. With 10000 Hz, minimum is 0.1 ms. Sleeping for 0 ms just yields.

!!! tip "TIME_MS2I / TIME_US2I Macros"
    Always use `TIME_MS2I(n)` and `TIME_US2I(n)` to convert time values. These correctly handle the conversion based on `CH_CFG_ST_FREQUENCY`. `TIME_INFINITE` is the infinite wait constant.

!!! tip "Stack Size Rule of Thumb"
    Start with 512 bytes for simple threads, 1024–2048 for threads using HAL drivers or floating point. Enable `CH_DBG_FILL_THREADS = TRUE` and call `chUnusedThreadStack(thd_p)` to measure actual usage during testing.
