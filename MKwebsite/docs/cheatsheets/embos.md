# :material-shield-check: embOS Cheatsheet

> **Dense quick-reference** for SEGGER embOS 5.x. Tasks, semaphores, mutexes, mailboxes, events, timers, and SystemView integration.

---

<div class="grid cards" markdown>

-   :material-cpu-64-bit: **Kernel**

    ---
    Preemptive priority-based. Priority 1 (lowest) to 255 (highest). Round-robin option for equal-priority tasks. Deterministic O(1) context switch.

-   :material-memory: **Footprint**

    ---
    Min ~1 KB RAM, ~4 KB Flash. All objects statically or dynamically allocatable. No fragmentation in block-based allocator.

-   :material-shield-star: **Certifications**

    ---
    IEC 61508 SIL 3, ISO 26262 ASIL D, DO-178C Level A, IEC 62304 Class C, EN 50128 SIL 4. Gold standard for safety-critical embedded.

-   :material-tools: **Toolchain**

    ---
    SEGGER Embedded Studio, Ozone debugger, SystemView real-time trace, J-Link debug probe. Best-in-class debug ecosystem.

</div>

---

## Task API

| Function | Signature | Description |
|----------|-----------|-------------|
| `OS_TASK_CREATE` | `void OS_TASK_CREATE(OS_TASK *pTask, const char *sName, OS_UINT Priority, void (*pRoutine)(void), void *pStack, OS_UINT StackSize)` | Create task (macro form) |
| `OS_TASK_CreateEx` | `void OS_TASK_CreateEx(OS_TASK *pTask, const char *sName, OS_UINT Priority, void (*pRoutine)(void *), void *pStack, OS_UINT StackSize, OS_UINT TimeSlice, void *pContext)` | Create with context pointer and time slice |
| `OS_TASK_Terminate` | `void OS_TASK_Terminate(OS_TASK *pTask)` | Terminate task (pass NULL for current task) |
| `OS_TASK_Suspend` | `void OS_TASK_Suspend(OS_TASK *pTask)` | Suspend task |
| `OS_TASK_Resume` | `void OS_TASK_Resume(OS_TASK *pTask)` | Resume suspended task |
| `OS_TASK_Delay` | `void OS_TASK_Delay(OS_TIME ms)` | Delay current task for milliseconds |
| `OS_TASK_DelayUntil` | `void OS_TASK_DelayUntil(OS_TIME *pTime, OS_TIME ms)` | Delay until absolute time (fixed-rate) |
| `OS_TASK_SetPriority` | `OS_UINT OS_TASK_SetPriority(OS_TASK *pTask, OS_UINT NewPrio)` | Change priority; returns old priority |
| `OS_TASK_GetPriority` | `OS_UINT OS_TASK_GetPriority(const OS_TASK *pTask)` | Get task priority |
| `OS_TASK_GetID` | `OS_TASK *OS_TASK_GetID(void)` | Get pointer to current task |
| `OS_TASK_Yield` | `void OS_TASK_Yield(void)` | Yield to equal/higher priority task |
| `OS_TASK_GetStackUsed` | `OS_UINT OS_TASK_GetStackUsed(const OS_TASK *pTask)` | Maximum stack bytes used (watermark) |

```c title="Task Pattern — Basic Creation"
#include "RTOS.h"

static OS_TASK   SensorTCB;
static OS_STACK  SensorStack[512];  // 512 bytes

static void SensorTask(void) {
    OS_TIME nextWake = OS_GetTime();
    while (1) {
        sensor_read();
        process_data();
        nextWake += 100;  // 100ms period
        OS_TASK_DelayUntil(&nextWake, 100);
    }
}

// In OS_InitKern() callback or main:
OS_TASK_CREATE(&SensorTCB, "Sensor", 50, SensorTask,
               SensorStack, sizeof(SensorStack));
```

---

## Semaphore API

| Function | Signature | Description |
|----------|-----------|-------------|
| `OS_SEMAPHORE_Create` | `void OS_SEMAPHORE_Create(OS_SEMAPHORE *pSema, OS_UINT InitialValue)` | Create counting semaphore |
| `OS_SEMAPHORE_Take` | `void OS_SEMAPHORE_Take(OS_SEMAPHORE *pSema)` | Acquire; blocks if count = 0 |
| `OS_SEMAPHORE_TakeBlocked` | `void OS_SEMAPHORE_TakeBlocked(OS_SEMAPHORE *pSema)` | Acquire with guaranteed block (for priority)  |
| `OS_SEMAPHORE_TakeTimed` | `OS_INT OS_SEMAPHORE_TakeTimed(OS_SEMAPHORE *pSema, OS_TIME Timeout)` | Acquire with timeout; returns 0 on success |
| `OS_SEMAPHORE_Give` | `void OS_SEMAPHORE_Give(OS_SEMAPHORE *pSema)` | Release (ISR-safe) |
| `OS_SEMAPHORE_GiveMax` | `void OS_SEMAPHORE_GiveMax(OS_SEMAPHORE *pSema, OS_UINT MaxVal)` | Give with ceiling value |
| `OS_SEMAPHORE_GetValue` | `OS_UINT OS_SEMAPHORE_GetValue(const OS_SEMAPHORE *pSema)` | Read current count |
| `OS_SEMAPHORE_Delete` | `void OS_SEMAPHORE_Delete(OS_SEMAPHORE *pSema)` | Delete semaphore |

```c title="Semaphore Pattern — ISR Sync"
static OS_SEMAPHORE DataReadySema;

// In init:
OS_SEMAPHORE_Create(&DataReadySema, 0);  // start at 0

// ISR (embOS ISR wrapper required):
OS_ISR_ENTER();
OS_SEMAPHORE_Give(&DataReadySema);
OS_ISR_LEAVE();

// Task:
OS_SEMAPHORE_Take(&DataReadySema);
process_data();
```

---

## Mutex API

| Function | Signature | Description |
|----------|-----------|-------------|
| `OS_MUTEX_Create` | `void OS_MUTEX_Create(OS_MUTEX *pMutex)` | Create mutex (priority inheritance built-in) |
| `OS_MUTEX_Lock` | `void OS_MUTEX_Lock(OS_MUTEX *pMutex)` | Lock mutex (recursive — same task can lock again) |
| `OS_MUTEX_LockTimed` | `OS_INT OS_MUTEX_LockTimed(OS_MUTEX *pMutex, OS_TIME Timeout)` | Lock with timeout |
| `OS_MUTEX_Unlock` | `void OS_MUTEX_Unlock(OS_MUTEX *pMutex)` | Unlock mutex |
| `OS_MUTEX_Delete` | `void OS_MUTEX_Delete(OS_MUTEX *pMutex)` | Delete mutex |
| `OS_MUTEX_GetOwner` | `OS_TASK *OS_MUTEX_GetOwner(const OS_MUTEX *pMutex)` | Get task that currently holds mutex |

```c title="Mutex Pattern"
static OS_MUTEX SPIMutex;
OS_MUTEX_Create(&SPIMutex);

void AccessSPI(void) {
    OS_MUTEX_Lock(&SPIMutex);
    SPI_Transfer();
    OS_MUTEX_Unlock(&SPIMutex);
}
```

---

## Mailbox API

| Function | Signature | Description |
|----------|-----------|-------------|
| `OS_MAILBOX_Create` | `void OS_MAILBOX_Create(OS_MAILBOX *pMB, OS_UINT SizeofMsg, OS_UINT NumMsgs, void *pBuffer)` | Create mailbox with buffer |
| `OS_MAILBOX_Put` | `OS_INT OS_MAILBOX_Put(OS_MAILBOX *pMB, const void *pMsg)` | Send message; returns 0 if queued, 1 if full |
| `OS_MAILBOX_PutBlocked` | `void OS_MAILBOX_PutBlocked(OS_MAILBOX *pMB, const void *pMsg)` | Send and block if full |
| `OS_MAILBOX_PutFront` | `OS_INT OS_MAILBOX_PutFront(OS_MAILBOX *pMB, const void *pMsg)` | Send to front of queue (priority) |
| `OS_MAILBOX_Get` | `void OS_MAILBOX_Get(OS_MAILBOX *pMB, void *pMsg)` | Receive; blocks if empty |
| `OS_MAILBOX_GetTimed` | `OS_INT OS_MAILBOX_GetTimed(OS_MAILBOX *pMB, void *pMsg, OS_TIME Timeout)` | Receive with timeout |
| `OS_MAILBOX_GetNoBlocking` | `OS_INT OS_MAILBOX_GetNoBlocking(OS_MAILBOX *pMB, void *pMsg)` | Non-blocking receive; returns 1 if empty |
| `OS_MAILBOX_GetUsed` | `OS_UINT OS_MAILBOX_GetUsed(const OS_MAILBOX *pMB)` | Count of pending messages |
| `OS_MAILBOX_Delete` | `void OS_MAILBOX_Delete(OS_MAILBOX *pMB)` | Delete mailbox |

```c title="Mailbox Pattern"
typedef struct { uint16_t id; uint8_t payload[8]; } Msg_t;

static OS_MAILBOX CmdMailbox;
static Msg_t      CmdBuffer[16];

OS_MAILBOX_Create(&CmdMailbox, sizeof(Msg_t), 16, CmdBuffer);

// Producer:
Msg_t m = { .id = 42 };
OS_MAILBOX_Put(&CmdMailbox, &m);

// Consumer:
Msg_t received;
OS_MAILBOX_Get(&CmdMailbox, &received);
HandleCommand(&received);
```

---

## Event API

| Function | Signature | Description |
|----------|-----------|-------------|
| `OS_EVENT_Create` | `void OS_EVENT_Create(OS_EVENT *pEvent)` | Create event object (auto-cleared) |
| `OS_EVENT_Set` | `void OS_EVENT_Set(OS_EVENT *pEvent)` | Signal event (wakes all waiters) |
| `OS_EVENT_Wait` | `void OS_EVENT_Wait(OS_EVENT *pEvent)` | Wait for event (clears after) |
| `OS_EVENT_WaitTimed` | `OS_INT OS_EVENT_WaitTimed(OS_EVENT *pEvent, OS_TIME Timeout)` | Wait with timeout |
| `OS_EVENT_Pulse` | `void OS_EVENT_Pulse(OS_EVENT *pEvent)` | Pulse event (wake waiters, then clear) |
| `OS_EVENT_Delete` | `void OS_EVENT_Delete(OS_EVENT *pEvent)` | Delete event |
| `OS_EVENT_GetState` | `OS_INT OS_EVENT_GetState(const OS_EVENT *pEvent)` | Check if event is set |

```c title="Event Pattern"
static OS_EVENT DataProcessedEvent;
OS_EVENT_Create(&DataProcessedEvent);

// Producer task sets event when data is ready:
OS_EVENT_Set(&DataProcessedEvent);

// Consumer task waits:
OS_EVENT_Wait(&DataProcessedEvent);
consume_results();
```

---

## Timer API

| Function | Signature | Description |
|----------|-----------|-------------|
| `OS_TIMER_Create` | `void OS_TIMER_Create(OS_TIMER *pTimer, void (*pfCallback)(void), OS_TIME Period)` | Create software timer |
| `OS_TIMER_CreateEx` | `void OS_TIMER_CreateEx(OS_TIMER_EX *pTimer, void (*pfCallback)(OS_TIMER_EX *), OS_TIME Period)` | Create with pointer to timer passed to callback |
| `OS_TIMER_Start` | `void OS_TIMER_Start(OS_TIMER *pTimer)` | Start/restart timer |
| `OS_TIMER_StartEx` | `void OS_TIMER_StartEx(OS_TIMER_EX *pTimer)` | Start extended timer |
| `OS_TIMER_Stop` | `void OS_TIMER_Stop(OS_TIMER *pTimer)` | Stop timer |
| `OS_TIMER_Restart` | `void OS_TIMER_Restart(OS_TIMER *pTimer)` | Restart (reset countdown) |
| `OS_TIMER_Delete` | `void OS_TIMER_Delete(OS_TIMER *pTimer)` | Delete timer |
| `OS_TIMER_GetStatus` | `OS_INT OS_TIMER_GetStatus(const OS_TIMER *pTimer)` | Check if timer is active |

```c title="Timer Pattern — Periodic Watchdog"
static OS_TIMER WatchdogTimer;

static void WatchdogCallback(void) {
    // Timer callback runs in timer task context
    // Not reset in time — trigger system watchdog
    WDT_Trigger();
    LOG_WARN("System watchdog expired!");
}

// One-shot watchdog — must restart within 5000ms
OS_TIMER_Create(&WatchdogTimer, WatchdogCallback, 5000);
OS_TIMER_Start(&WatchdogTimer);

// Main task kicks watchdog by restarting timer:
OS_TIMER_Restart(&WatchdogTimer);
```

---

## SystemView Integration

SEGGER SystemView provides real-time trace of task switches, API calls, and custom events. Works over J-Link RTT at very low overhead.

| Macro / Function | Description |
|------------------|-------------|
| `SEGGER_SYSVIEW_Start()` | Start recording events (call from application) |
| `SEGGER_SYSVIEW_Stop()` | Stop recording |
| `SEGGER_SYSVIEW_Print(s)` | Log string message to trace |
| `SEGGER_SYSVIEW_PrintfHost(fmt, ...)` | Printf-style message (formatted on host) |
| `SEGGER_SYSVIEW_RecordEnterISR()` | Mark ISR entry in trace |
| `SEGGER_SYSVIEW_RecordExitISR()` | Mark ISR exit in trace |
| `SEGGER_SYSVIEW_RecordEnterISRToScheduler()` | Mark ISR exit with reschedule |
| `SEGGER_SYSVIEW_OnIdle()` | Mark idle entry (for power analysis) |
| `SEGGER_SYSVIEW_Conf()` | Perform SystemView configuration (call once) |
| `SEGGER_SYSVIEW_SetRAMBase(addr)` | Set RAM base for address compression |

```c title="SystemView Integration"
#include "SEGGER_SYSVIEW.h"

void app_init(void) {
    // Initialize embOS
    OS_Init();
    OS_InitHW();

    // Start SystemView recording
    SEGGER_SYSVIEW_Conf();   // must be called before OS_Start
    SEGGER_SYSVIEW_Start();

    // Create tasks...
    OS_Start();              // starts scheduler — never returns
}

// Custom user events in application code:
SEGGER_SYSVIEW_Print("ADC conversion complete");
SEGGER_SYSVIEW_PrintfHost("ADC value: %d", adc_value);

// In ISR:
void EXTI0_IRQHandler(void) {
    OS_ISR_ENTER();
    SEGGER_SYSVIEW_RecordEnterISR();
    handle_exti();
    SEGGER_SYSVIEW_RecordExitISR();
    OS_ISR_LEAVE();
}
```

---

## ISR Wrapper Macros

```c title="embOS ISR Template"
void USART2_IRQHandler(void) {
    OS_ISR_ENTER();              // save state, disable nested ISR
    // --- ISR body ---
    OS_SEMAPHORE_Give(&UartSema);
    // ----------------
    OS_ISR_LEAVE();              // restore, trigger reschedule if needed
}
```

| Macro | Description |
|-------|-------------|
| `OS_ISR_ENTER()` | embOS ISR entry — must be first line |
| `OS_ISR_LEAVE()` | embOS ISR exit — must be last line |

!!! danger "Always Use OS_ISR_ENTER / OS_ISR_LEAVE"
    embOS tracks nesting level and manages context switching at ISR exit. Failing to use these macros will cause the scheduler to miss reschedule opportunities, leading to incorrect task switching behavior.

---

## Key Configuration (RTOSInit.c / OS_Config.h)

| Setting | Description |
|---------|-------------|
| `OS_TICK_FREQ` | Tick frequency in Hz (default 1000 = 1ms ticks) |
| `OS_CHECKSTACK` | Enable stack overflow detection (debug) |
| `OS_CHECKSTACKPTR` | Verify stack pointer on context switch |
| `OS_SUPPORT_MPU` | Enable MPU-based task isolation |
| `OS_LIBMODE_*` | Select library mode: `DPT` (debug+profiling+trace), `DP` (debug+profiling), `D` (debug), `R` (release) |
| `OS_SUPPORT_PROFILE` | Enable runtime profiling counters |

---

## Gotchas & Pitfalls

!!! danger "OS_ISR_ENTER / OS_ISR_LEAVE — Non-Negotiable"
    Every ISR that calls embOS API functions must be wrapped with `OS_ISR_ENTER()` / `OS_ISR_LEAVE()`. Omitting these will cause missed reschedules and unpredictable task timing.

!!! danger "OS_TASK_Create vs OS_TASK_CREATE — Stack Units"
    `OS_TASK_CREATE` takes stack size in bytes. Some older examples use stack size in words. Always confirm you are passing bytes. Too-small stack causes stack overflow; too-large wastes RAM.

!!! warning "OS_MAILBOX_Put Returns Immediately if Full"
    `OS_MAILBOX_Put()` is non-blocking — it returns 1 if the mailbox is full and the message is dropped. Use `OS_MAILBOX_PutBlocked()` if you need guaranteed delivery (at the cost of blocking the sending task).

!!! warning "OS_EVENT vs OS_SEMAPHORE — Key Difference"
    `OS_EVENT_Set()` wakes ALL waiting tasks simultaneously (broadcast). `OS_SEMAPHORE_Give()` wakes only ONE task (the highest-priority waiter). Choose based on whether you need broadcast or unicast behavior.

!!! tip "Safety Library Modes"
    For production (non-debug), use `OS_LIBMODE_R` (release). This removes all debug checks and logging for minimum footprint. For development, use `OS_LIBMODE_DPT` to get full stack checking and SystemView trace. Never ship with `DPT` mode in production.

!!! tip "OS_TASK_DelayUntil for Jitter-Free Periodic Tasks"
    Use `OS_TASK_DelayUntil()` instead of `OS_TASK_Delay()` for periodic tasks. `DelayUntil` compensates for execution time by sleeping until the next absolute deadline, eliminating cumulative drift.
