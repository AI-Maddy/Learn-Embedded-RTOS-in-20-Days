# :material-refresh: Periodic Scheduler

<div class="grid cards" markdown>
- :material-lightbulb-on: **vTaskDelayUntil** — the correct primitive for drift-free periodic execution; always prefer it over vTaskDelay
- :material-chip: **DWT Period** — use the DWT cycle counter to measure the exact achieved period and detect overruns
- :material-alert: **Overrun** — if a task's body takes longer than its period the next activation is missed; detect and handle it explicitly
- :material-check-circle: **Use When** — use multi-rate scheduling when different sensors/actuators have different natural update frequencies
</div>

---

## :material-lightbulb-on: Superloop vs RTOS Periodic Tasks

### The Classic Superloop

```c
/* Bare-metal superloop — all tasks share one period */
int main(void) {
    init_hardware();
    while (1) {
        read_sensors();       /* 2 ms */
        run_control_law();    /* 5 ms */
        update_display();     /* 15 ms */
        delay_ms(10);         /* fixed delay — all tasks run at same rate */
    }
}
```

**Problems:**
- All tasks share the same period — wasteful or too slow for some tasks
- `delay_ms` is blind to how long the body took — period drifts
- One slow task delays everything else

### RTOS Periodic Task Pattern

Each task runs at its own natural rate. The RTOS scheduler time-multiplexes the CPU.

```c
void vSensorTask(void *pv) {               /* 5 ms period */
    TickType_t xLast = xTaskGetTickCount();
    for (;;) {
        read_sensors();
        vTaskDelayUntil(&xLast, pdMS_TO_TICKS(5));
    }
}

void vControlTask(void *pv) {              /* 10 ms period */
    TickType_t xLast = xTaskGetTickCount();
    for (;;) {
        run_control_law();
        vTaskDelayUntil(&xLast, pdMS_TO_TICKS(10));
    }
}

void vDisplayTask(void *pv) {              /* 100 ms period */
    TickType_t xLast = xTaskGetTickCount();
    for (;;) {
        update_display();
        vTaskDelayUntil(&xLast, pdMS_TO_TICKS(100));
    }
}
```

---

## :material-code-tags: Complete Periodic Task Template

```c
#include "FreeRTOS.h"
#include "task.h"

/* ---- Configuration -------------------------------------------- */
#define TASK_PERIOD_MS      10U
#define TASK_STACK_WORDS    256U
#define TASK_PRIORITY       3U

/* ---- DWT helpers ---------------------------------------------- */
static inline void dwt_init(void) {
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL  |= DWT_CTRL_CYCCNTENA_Msk;
}

/* ---- Task ----------------------------------------------------- */
static void vPeriodicTask(void *pvParams) {
    (void)pvParams;

    TickType_t       xLastWakeTime = xTaskGetTickCount();
    const TickType_t xPeriod       = pdMS_TO_TICKS(TASK_PERIOD_MS);

    /* Optional: record previous cycle timestamp for DWT period check */
    uint32_t ulPrevCycle = DWT->CYCCNT;

    for (;;) {
        /* --- 1. Timestamp actual activation --- */
        uint32_t ulNow     = DWT->CYCCNT;
        uint32_t ulActualPeriodCycles = ulNow - ulPrevCycle;
        ulPrevCycle = ulNow;

        /* --- 2. Overrun detection --- */
        uint32_t ulBudgetCycles = (SystemCoreClock / 1000U) * TASK_PERIOD_MS;
        if (ulActualPeriodCycles > ulBudgetCycles + (ulBudgetCycles / 10U)) {
            /* Actual period exceeded budget by >10% — overrun occurred */
            handle_overrun(ulActualPeriodCycles, ulBudgetCycles);
        }

        /* --- 3. Task body --- */
        do_periodic_work();

        /* --- 4. Yield until absolute next-period boundary --- */
        vTaskDelayUntil(&xLastWakeTime, xPeriod);
    }
}

/* ---- Task creation -------------------------------------------- */
void periodic_task_init(void) {
    dwt_init();
    xTaskCreate(vPeriodicTask, "Periodic", TASK_STACK_WORDS,
                NULL, TASK_PRIORITY, NULL);
}
```

---

## :material-chart-timeline-variant: Timing Diagram

```
Period = 10 ms
        |<------ 10 ms ------->|<------ 10 ms ------->|
CPU:    |=====work(6ms)========|=======work(6ms)=======|
        |                 ↑    |                  ↑
      activate         vTaskDelayUntil         activate
      (tick)           (blocks until t=10)     (tick)

With vTaskDelay (drifting):
        |<-- 6ms work -->|<-- 10ms delay -->|<-- 6ms -->|<-- 10ms delay -->|
        Period = 16 ms ← WRONG (should be 10 ms)
```

---

## :material-timer-sync: Overrun Detection Pattern

An overrun occurs when the task body takes longer than the period. `vTaskDelayUntil` handles this gracefully—it returns immediately without sleeping—but the missed deadline must be logged and acted upon.

```c
static void handle_overrun(uint32_t actual, uint32_t budget) {
    static uint32_t overrun_count = 0;
    overrun_count++;

    /* Log: actual cycles, budget cycles, count */
    log_warning("Overrun #%lu: actual=%lu budget=%lu cycles",
                overrun_count, actual, budget);

    /* Policy options:
     *  1. Continue — skip the missed deadline window
     *  2. Degrade  — reduce processing load (lower filter order, etc.)
     *  3. Fault    — assert / trigger watchdog for hard deadlines
     */
    if (overrun_count > 5U) {
        enter_safe_state();   /* hard real-time: cannot tolerate overruns */
    }
}
```

---

## :material-layers-triple: Multi-Rate Scheduling

Different subsystems naturally operate at different rates. Assign each a separate task with its own period and priority:

| Task | Period | Priority | Rationale |
|------|--------|----------|-----------|
| ADC sampling | 1 ms | 5 (highest) | Nyquist for 500 Hz signal |
| Control law | 5 ms | 4 | Motor bandwidth ~100 Hz |
| State estimation | 10 ms | 3 | Filter update rate |
| Communication (CAN) | 20 ms | 2 | Bus cycle time |
| Display update | 200 ms | 1 | Human perception limit |
| Telemetry log | 1000 ms | 0+1 | Background storage |

**RMS principle:** shorter period → higher priority. The table above follows this rule.

```c
void vApp_Init(void) {
    xTaskCreate(vADCTask,     "ADC",   256, NULL, 5, NULL);
    xTaskCreate(vControlTask, "Ctrl",  512, NULL, 4, NULL);
    xTaskCreate(vEstimTask,   "Estim", 512, NULL, 3, NULL);
    xTaskCreate(vCANTask,     "CAN",   256, NULL, 2, NULL);
    xTaskCreate(vDisplayTask, "Disp",  512, NULL, 1, NULL);
    xTaskCreate(vLogTask,     "Log",   256, NULL, 1, NULL);
    vTaskStartScheduler();
}
```

---

## :material-help-circle: Flashcards

???+ question "Why does vTaskDelay cause period drift and how does vTaskDelayUntil fix it?"
    `vTaskDelay(n)` delays for n ticks from the **current moment**. If the task body takes time T_body, the effective period is T_body + n ticks — and T_body varies each iteration. Over time, periods accumulate drift.

    `vTaskDelayUntil(&xLast, n)` records the absolute wake time in xLast and always targets `xLast + n` as the next wake tick, regardless of how long the body took. Period = exactly n ticks (wall-clock), drift = zero.

???+ question "What happens when a task's body takes longer than its period with vTaskDelayUntil?"
    `vTaskDelayUntil` detects that the next target wake time is already in the past. Rather than sleeping, it **returns immediately** (the `xWasDelayed` return value is `pdFALSE`). The task misses that period and its next activation target advances by one or more periods to the first future tick. This is graceful catch-up—it will not keep running with zero delay indefinitely as long as load temporarily drops.

???+ question "What is multi-rate scheduling and what priority rule should govern it?"
    Multi-rate scheduling runs different tasks at different natural periods. Each task has a dedicated period matched to its subsystem's bandwidth requirement (e.g., ADC at 1 ms, display at 200 ms). The **Rate Monotonic Scheduling** rule governs priority assignment: shorter period → higher priority. This is provably optimal for fixed-priority preemptive scheduling.

???+ question "How do you measure whether a periodic task actually achieved its target period?"
    Read `DWT->CYCCNT` at the top of each task body (after `vTaskDelayUntil` returns). Compute the difference from the previous iteration's timestamp. Convert cycles to microseconds using CPU clock frequency. If the measured period exceeds budget by more than a small margin (e.g., >10%), an overrun or excessive jitter occurred.

---

## :material-clipboard-check: Self Test

=== "Question 1"
    A task runs at 100 Hz (10 ms period) with a body that takes exactly 3 ms using `vTaskDelay(pdMS_TO_TICKS(10))`. What is the actual achieved frequency? How many milliseconds of drift accumulates per second?

=== "Answer 1"
    With `vTaskDelay`, effective period = 3 ms (body) + 10 ms (delay) = **13 ms**. Achieved frequency = 1/0.013 ≈ **76.9 Hz** (not 100 Hz). Drift per second = 1000/13 × 3 ms ≈ **230 ms/second**. After 4.3 seconds, the task is a full second behind schedule.

=== "Question 2"
    You have four tasks with periods 2 ms, 5 ms, 10 ms, and 50 ms. Assign RMS priorities (1=lowest, 4=highest) and compute the total CPU utilisation. Use WCET values: 0.4 ms, 1.2 ms, 2.0 ms, 8.0 ms respectively. Is the set schedulable under the Liu & Layland bound?

=== "Answer 2"
    RMS priorities: 2 ms → P4, 5 ms → P3, 10 ms → P2, 50 ms → P1.
    
    Utilisation: 0.4/2 + 1.2/5 + 2.0/10 + 8.0/50 = 0.20 + 0.24 + 0.20 + 0.16 = **0.80**
    
    Liu & Layland bound for n=4: U_bound = 4×(2^(1/4)−1) = 4×0.1892 ≈ **0.757**
    
    0.80 > 0.757 — bound **exceeded**. Apply Response Time Analysis to determine if the set is still schedulable in practice.

---

## :material-check-circle: Summary

!!! success "Key Takeaways"
    - Always use `vTaskDelayUntil` for periodic tasks — it eliminates period drift by targeting absolute wake times.
    - Detect overruns explicitly with DWT timestamps; define a clear policy (log / degrade / fault) for when they occur.
    - Multi-rate scheduling assigns each subsystem its natural period; follow RMS priority assignment (shorter period = higher priority).
    - Measure actual achieved period and jitter during integration testing — don't assume the scheduler delivers exact timing.
    - Separate periodic tasks by concern: one task per rate domain keeps code modular and makes schedulability analysis tractable.
    - Each task's stack must account for its own local variables plus the interrupt frame that may be pushed during preemption.
