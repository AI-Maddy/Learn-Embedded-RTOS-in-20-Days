# :material-timer-outline: Day 09 — Latency, Jitter & Timing Analysis

!!! abstract "Day at a Glance"
    **Goal:** Instrument real-time tasks with the ARM DWT cycle counter, measure interrupt latency, quantify jitter, and apply Response Time Analysis to validate schedulability.
    **Prerequisites:** Day 08 — Memory & Stack Management
    **Estimated Time:** 90 minutes

<div class="grid cards" markdown>
- :material-lightbulb-on: **Core Concept** — Jitter is the variation in task response time; even a schedulable system can miss deadlines if jitter is not bounded
- :material-chip: **RTOS Component** — DWT cycle counter, `vTaskDelayUntil`, Response Time Analysis (RTA)
- :material-alert: **Watch Out** — `vTaskDelay` accumulates drift; use `vTaskDelayUntil` for jitter-free periodic tasks
- :material-check-circle: **By End of Day** — Measure WCET with DWT, compute interrupt latency, verify schedulability with RTA
</div>

## :material-lightbulb-on: Intuition

!!! info "Core Idea"
    A task that is *schedulable on paper* can still fail at runtime if timing variability (jitter) pushes its response time past its deadline. Latency is the delay from an event to the start of handling; jitter is how much that latency varies between invocations. Bounding both is the job of timing analysis.

!!! success "Real-World Context"
    In automotive AUTOSAR systems, every runnable has a measured WCET (Worst-Case Execution Time) that feeds into OS scheduling tables. An unmeasured task that takes 200 µs instead of the assumed 50 µs can cascade and cause a safety-critical deadline miss on the braking ECU.

## :material-timer-sand: Timing Measurement Chain

```mermaid
flowchart LR
    A["Hardware Event\n(GPIO / IRQ)"] -->|"Interrupt latency\n(ISR entry delay)"| B["ISR Runs\n(interrupt handler)"]
    B -->|"Semaphore / notification\ngive from ISR"| C["Task Unblocked\n(enters Ready state)"]
    C -->|"Scheduler delay\n(higher priority tasks)"| D["Task Runs\n(context switch in)"]
    D -->|"WCET\n(worst-case execution)"| E["Task Complete\n(deadline check)"]

    subgraph Measured with DWT
        D
        E
    end
    subgraph Response Time = B to E
        B
        C
        D
        E
    end
```

## :material-book-open-variant: Lesson

### The DWT Cycle Counter (ARM Cortex-M)

The Data Watchpoint and Trace (DWT) unit on Cortex-M3/M4/M7 provides a free-running 32-bit cycle counter at CPU clock speed — the most accurate on-chip timer available.

```c
/* dwt_timer.h - DWT cycle counter for ARM Cortex-M */
#include <stdint.h>

/* CoreDebug and DWT register addresses (CMSIS) */
#define DWT_CTRL   (*(volatile uint32_t*)0xE0001000)
#define DWT_CYCCNT (*(volatile uint32_t*)0xE0001004)
#define DEMCR      (*(volatile uint32_t*)0xE000EDFC)

static inline void dwt_init(void) {
    DEMCR    |= (1u << 24);   /* Enable DWT */
    DWT_CYCCNT = 0;
    DWT_CTRL  |= (1u << 0);   /* Enable CYCCNT */
}

static inline uint32_t dwt_get(void) {
    return DWT_CYCCNT;
}

/* Returns elapsed cycles between two dwt_get() calls */
static inline uint32_t dwt_elapsed(uint32_t start, uint32_t end) {
    return end - start;       /* Handles 32-bit wrap-around correctly */
}

/* Convert cycles to microseconds (for 168 MHz clock) */
#define CYCLES_TO_US(c)  ((c) / 168u)
```

### Measuring WCET with DWT

```c
#include "dwt_timer.h"
#include "FreeRTOS.h"
#include "task.h"

#define WCET_SAMPLES  1000

static uint32_t wcet_us    = 0;
static uint32_t wcet_min   = UINT32_MAX;
static uint32_t wcet_total = 0;
static uint32_t wcet_count = 0;

void vControlTask(void *pvParameters) {
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xPeriod = pdMS_TO_TICKS(10);  /* 10ms period */

    dwt_init();

    for (;;) {
        uint32_t t_start = dwt_get();

        /* --- Critical work starts here --- */
        sensor_read_and_filter();
        pid_compute();
        actuator_set();
        /* --- Critical work ends here --- */

        uint32_t t_end  = dwt_get();
        uint32_t cycles = dwt_elapsed(t_start, t_end);
        uint32_t us     = CYCLES_TO_US(cycles);

        if (us > wcet_us)          wcet_us  = us;   /* Track maximum */
        if (us < wcet_min)         wcet_min = us;   /* Track minimum */
        wcet_total += us;
        wcet_count++;

        /* Wait for next period — no drift accumulation */
        vTaskDelayUntil(&xLastWakeTime, xPeriod);
    }
}
```

### `vTaskDelay` vs `vTaskDelayUntil` — The Jitter Difference

| Function | Behavior | Jitter Source |
|---|---|---|
| `vTaskDelay(N)` | Sleep N ticks **from now** | Execution time adds to period — jitter accumulates |
| `vTaskDelayUntil(&last, N)` | Wake at absolute tick `last + N` | Execution time **consumed within period** — near-zero jitter |

```c
/* BAD: Accumulated drift — execution time shifts the next wake point */
void vPeriodicBad(void *p) {
    for (;;) {
        do_work();             /* Takes variable time */
        vTaskDelay(pdMS_TO_TICKS(10));   /* 10ms from NOW */
    }
}

/* GOOD: Fixed period — vTaskDelayUntil absorbs execution variation */
void vPeriodicGood(void *p) {
    TickType_t xLast = xTaskGetTickCount();
    for (;;) {
        do_work();             /* Variable time consumed inside period */
        vTaskDelayUntil(&xLast, pdMS_TO_TICKS(10));  /* Wake at fixed absolute time */
    }
}
```

### ITM Printf (SWO Debug Output Without UART)

```c
/* Redirect printf to ITM SWO — zero CPU overhead at sampling side */
#include "core_cm4.h"

int _write(int fd, char *ptr, int len) {
    (void)fd;
    for (int i = 0; i < len; i++) {
        /* Wait until ITM port 0 is ready */
        while (ITM->PORT[0].u32 == 0) {}
        ITM->PORT[0].u8 = (uint8_t)ptr[i];
    }
    return len;
}

/* Usage — readable in Ozone, OpenOCD, or Segger J-Link RTT viewer */
void vTimingTask(void *p) {
    TickType_t xLast = xTaskGetTickCount();
    for (;;) {
        uint32_t t0 = dwt_get();
        do_work();
        uint32_t elapsed = CYCLES_TO_US(dwt_elapsed(t0, dwt_get()));
        printf("[%lu] WCET=%lu us\n", xTaskGetTickCount(), elapsed);
        vTaskDelayUntil(&xLast, pdMS_TO_TICKS(10));
    }
}
```

### Interrupt Latency Measurement

Interrupt latency = time from hardware event to first instruction of ISR.

```c
/* Measure GPIO IRQ latency using two DWT timestamps */
static volatile uint32_t irq_trigger_cycle = 0;
static volatile uint32_t irq_entry_cycle   = 0;
static volatile uint32_t irq_latency_us    = 0;

/* Trigger: set GPIO output HIGH and record timestamp */
void trigger_event(void) {
    GPIO_SetBits(GPIOA, GPIO_PIN_0);
    irq_trigger_cycle = dwt_get();   /* Stamp before IRQ fires */
}

/* ISR: record entry timestamp immediately */
void EXTI0_IRQHandler(void) {
    irq_entry_cycle = dwt_get();     /* First thing in ISR */
    irq_latency_us  = CYCLES_TO_US(
        dwt_elapsed(irq_trigger_cycle, irq_entry_cycle));
    EXTI_ClearITPendingBit(EXTI_Line0);
    /* ... rest of handler ... */
}
```

Typical values on Cortex-M4 at 168 MHz: **12–15 cycles** (< 0.1 µs) with no OS, **20–40 cycles** with FreeRTOS critical sections active.

### Timing Budget Table

A timing budget allocates the period among all activities. If the sum exceeds the period, deadlines are missed.

| Activity | WCET (µs) | % of 10 ms period |
|---|---|---|
| Sensor ADC read | 45 | 0.45% |
| Digital filter (IIR) | 18 | 0.18% |
| PID computation | 12 | 0.12% |
| CAN frame transmit | 28 | 0.28% |
| Stack + context switch | 8 | 0.08% |
| **Total WCET** | **111** | **1.11%** |
| Safety margin (10×) | — | 10% recommended |

**Rule of thumb:** total WCET should not exceed 10% of the period for safety margin. The remaining 89% absorbs worst-case jitter, interrupt nesting, and measurement uncertainty.

### Response Time Analysis (RTA)

RTA computes the worst-case response time `R_i` for each task `i` in a fixed-priority preemptive system.

**Formula** (iterative):

```
R_i^(0) = C_i
R_i^(n+1) = C_i + sum over j in hp(i) [ ceil(R_i^(n) / T_j) * C_j ]
```

Where `C_i` = WCET, `T_i` = period, `hp(i)` = set of tasks with higher priority than `i`.

Iterate until `R_i^(n+1) == R_i^(n)` (converged) or `R_i > D_i` (deadline missed).

```c
/* RTA in C — compute worst-case response time */
#include <math.h>
#include <stdio.h>

typedef struct { float C; float T; float D; } task_t;

float rta(task_t *tasks, int n, int i) {
    float R = tasks[i].C;
    for (int iter = 0; iter < 100; iter++) {
        float R_new = tasks[i].C;
        for (int j = 0; j < n; j++) {
            if (j == i) continue;
            /* Only sum higher-priority tasks (lower index = higher priority) */
            if (j < i)
                R_new += ceilf(R / tasks[j].T) * tasks[j].C;
        }
        if (R_new == R) return R;   /* Converged */
        R = R_new;
        if (R > tasks[i].D) return R;  /* Failed — report over-deadline */
    }
    return R;
}

int main(void) {
    task_t tasks[] = {
        /* C (ms), T (ms), D (ms) */
        {1.0f,  5.0f,  5.0f},   /* Task 1 — highest priority */
        {2.0f, 10.0f, 10.0f},   /* Task 2 */
        {3.0f, 20.0f, 20.0f},   /* Task 3 — lowest priority */
    };
    int n = 3;
    for (int i = 0; i < n; i++) {
        float R = rta(tasks, n, i);
        printf("Task %d: WCRT=%.2f ms, Deadline=%.2f ms — %s\n",
               i+1, R, tasks[i].D,
               R <= tasks[i].D ? "PASS" : "FAIL");
    }
    return 0;
}
```

### RMS Schedulability Bound

For `n` tasks with implicit deadlines, the **Rate Monotonic** utilization bound is:

```
U = sum(C_i / T_i) <= n * (2^(1/n) - 1)
```

| Tasks (n) | Utilization Bound |
|---|---|
| 1 | 100.0% |
| 2 | 82.8% |
| 3 | 78.0% |
| 5 | 74.3% |
| 10 | 71.8% |
| ∞ | 69.3% (ln 2) |

If `U <= bound`, the task set is **guaranteed schedulable** under RM. If `U > bound` but `<= 100%`, use RTA to check individual deadlines.

## :material-pencil: Exercises

**Exercise 1 — Measure Task Jitter with DWT:**
Instrument `vControlTask` (10 ms period) to record the actual inter-arrival time between consecutive executions using `dwt_get()`. Store 200 samples in a ring buffer. After the run, compute: mean period, max jitter, min jitter, and peak-to-peak jitter. Report results via ITM.

**Exercise 2 — Compute RMS Schedulability:**
Given three tasks: T1 (C=1 ms, T=4 ms), T2 (C=2 ms, T=8 ms), T3 (C=3 ms, T=16 ms). (a) Compute total utilization. (b) Check against the RM bound. (c) Run RTA for each task. (d) Add a fourth task T4 (C=1 ms, T=4 ms) and re-run — does the system remain schedulable?

**Exercise 3 — Jitter Histogram Concept:**
Build a software jitter histogram with 10 buckets (each 10 µs wide, covering 0–100 µs). Each time a periodic task executes, measure the deviation from its ideal wake time and increment the appropriate bucket. After 1000 iterations, print the histogram to UART. Identify the P99 (99th percentile) latency bucket.

**Exercise 4 — Profile Interrupt Latency:**
Connect PA0 output to PA1 input (loopback). Assert PA0 in software, record `dwt_get()` immediately. Inside the EXTI1 ISR, record `dwt_get()` as the first instruction. Compute latency in nanoseconds. Repeat 500 times. Report min, max, and average interrupt latency. Then enable a high-priority task to run concurrently and observe how it affects latency.

## :material-check: Solutions

??? success "Show Solutions"
    **Exercise 1 — Jitter measurement ring buffer:**
    ```c
    #define JITTER_SAMPLES 200

    static uint32_t arrival_cycles[JITTER_SAMPLES];
    static uint32_t jitter_us[JITTER_SAMPLES];
    static int      jitter_idx = 0;

    void vControlTask(void *pvParameters) {
        TickType_t xLast = xTaskGetTickCount();
        uint32_t   prev  = dwt_get();
        dwt_init();

        for (;;) {
            uint32_t now  = dwt_get();
            uint32_t diff = CYCLES_TO_US(dwt_elapsed(prev, now));
            prev = now;

            if (jitter_idx < JITTER_SAMPLES) {
                jitter_us[jitter_idx++] = diff;
            } else {
                /* Print summary via ITM */
                uint32_t sum = 0, mx = 0, mn = UINT32_MAX;
                for (int i = 1; i < JITTER_SAMPLES; i++) {
                    sum += jitter_us[i];
                    if (jitter_us[i] > mx) mx = jitter_us[i];
                    if (jitter_us[i] < mn) mn = jitter_us[i];
                }
                printf("Mean=%lu us, Max=%lu us, Min=%lu us, PkPk=%lu us\n",
                       sum / (JITTER_SAMPLES-1), mx, mn, mx - mn);
                jitter_idx = 1;  /* Reset, keep last sample */
            }

            do_work();
            vTaskDelayUntil(&xLast, pdMS_TO_TICKS(10));
        }
    }
    ```

    **Exercise 2 — RMS utilization:**
    - U = 1/4 + 2/8 + 3/16 = 0.25 + 0.25 + 0.1875 = **0.6875 (68.75%)**
    - RM bound for n=3: 3*(2^(1/3)-1) ≈ **0.780**
    - 68.75% < 78.0% → **Guaranteed schedulable**
    - RTA: R1=1ms ✓, R2=1+2=3ms ✓, R3=3+2*(ceil(5/4)*1)+2=3+4+2=... iterates to ≤16ms ✓
    - Adding T4 (C=1, T=4): U = 0.6875+0.25 = **0.9375** — exceeds bound but ≤100%, must use RTA.

    **Exercise 4 — Interrupt latency with critical section interference:**
    ```c
    #define LAT_SAMPLES 500
    static uint32_t lat_cycles[LAT_SAMPLES];
    static volatile int lat_idx = 0;
    static volatile uint32_t trigger_stamp;

    void measure_latency(void) {
        GPIO_SetBits(GPIOA, GPIO_PIN_0);
        trigger_stamp = dwt_get();
    }

    void EXTI1_IRQHandler(void) {
        uint32_t entry = dwt_get();
        if (lat_idx < LAT_SAMPLES)
            lat_cycles[lat_idx++] = dwt_elapsed(trigger_stamp, entry);
        EXTI_ClearITPendingBit(EXTI_Line1);
    }
    ```
    Typical results: min ~12 cycles, max ~60 cycles when a critical section is active (FreeRTOS disables IRQs briefly during scheduler operations).

## :material-alert: Common Pitfalls

!!! warning "vTaskDelay Drift"
    Using `vTaskDelay(N)` in a periodic task causes jitter equal to the task's own execution time. Over 1000 iterations this becomes a visible drift. Always use `vTaskDelayUntil` for periodic tasks where timing accuracy matters.

!!! warning "DWT Not Enabled After Reset"
    On some toolchains and QEMU targets, `DWT_CTRL` bit 0 and `DEMCR` bit 24 are zero after reset. If `DWT_CYCCNT` is always 0, your `dwt_init()` call was skipped or the debugger reset the counter. Verify both enable bits are set.

!!! danger "RTA Assumes Independent Tasks"
    Response Time Analysis assumes tasks are independent and fully preemptable. If tasks share a mutex (priority inversion risk) or disable interrupts, the analysis is invalid. Always account for **blocking time** `B_i` from lower-priority tasks holding shared resources.

## :material-help-circle: Flashcards

???+ question "What is the difference between latency and jitter?"
    **Latency** is the absolute delay from an event to the system's response (e.g., 50 µs from interrupt to ISR entry). **Jitter** is the *variation* of that latency across multiple occurrences (e.g., latency varies from 48 µs to 63 µs → 15 µs jitter). A system can have high latency but low jitter (predictable) or low latency but high jitter (unpredictable).

???+ question "Why does vTaskDelayUntil reduce jitter compared to vTaskDelay?"
    `vTaskDelay(N)` delays N ticks *from when it is called* — the call happens after `do_work()`, so variable execution time shifts the next wake point, accumulating drift. `vTaskDelayUntil(&last, N)` wakes at an *absolute* tick count `last + N`, absorbing execution time variation within the period. The wake point does not drift.

???+ question "What does WCET stand for and why is it the worst case?"
    **Worst-Case Execution Time** — the maximum time a code path takes under the worst combination of input data, cache misses, branch mispredictions, and memory access patterns. Scheduling analysis uses WCET (not average) because missing a deadline once due to an underestimated execution time causes a real-time failure.

???+ question "What is the Rate Monotonic utilization bound for 5 tasks?"
    For n=5: `U_bound = 5 * (2^(1/5) - 1) ≈ 5 * 0.1487 ≈ 0.743` = **74.3%**. If the sum of C_i/T_i for all 5 tasks is ≤ 74.3%, the task set is guaranteed schedulable under Rate Monotonic scheduling regardless of phase.

## :material-clipboard-check: Self Test

=== "Question 1"
    The DWT cycle counter rolls over (wraps to 0) every 2^32 cycles. At 168 MHz, how often does this happen? Is `dwt_elapsed(start, end)` still correct after a wrap?

=== "Answer 1"
    At 168 MHz, 2^32 / 168,000,000 ≈ **25.6 seconds**. For tasks with periods << 25 seconds this is not a problem.

    `dwt_elapsed(start, end)` computes `end - start` as unsigned 32-bit subtraction. Due to two's complement modular arithmetic, the result is correct even across a single wrap-around, *as long as elapsed time < 2^32 cycles* (< 25.6 s). For longer measurements, use a 64-bit extension or the SysTick/HAL tick.

=== "Question 2"
    A task set has U = 85%. The RM utilization bound for 4 tasks is 75.7%. Does this mean the system will miss deadlines?

=== "Answer 2"
    **Not necessarily.** The RM utilization bound is a *sufficient* condition for schedulability — if U ≤ bound, schedulability is *guaranteed*. But it is not a *necessary* condition. A task set with U > bound may still be schedulable. The definitive test is **Response Time Analysis (RTA)**: compute R_i for each task and verify R_i ≤ D_i. Many systems with U = 85–99% are fully schedulable under RM when periods are harmonically related.

## :material-check-circle: Summary

!!! success "Key Takeaways"
    - The **DWT cycle counter** on Cortex-M provides nanosecond-resolution timing with zero overhead — always use it for WCET measurement
    - **`vTaskDelayUntil`** eliminates drift by targeting absolute wake times; `vTaskDelay` accumulates jitter equal to execution time
    - **Interrupt latency** on bare-metal Cortex-M4 is ~12–15 cycles; FreeRTOS critical sections can extend this to ~40 cycles
    - **Timing budgets** sum all WCETs against the period; total should not exceed 10% for adequate safety margin
    - **RTA** is the gold-standard schedulability test; the RM utilization bound is a sufficient (not necessary) short-cut
    - **Tomorrow (Day 10):** Build systems and board bringup — CMake, FreeRTOS port layers, linker scripts, and QEMU simulation
