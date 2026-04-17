# :material-timer-outline: Timing Analysis

<div class="grid cards" markdown>
- :material-lightbulb-on: **WCET** — Worst-Case Execution Time is the maximum time a code path can ever take; must be measured, not guessed
- :material-chip: **DWT Cycle Counter** — the ARM Cortex-M Data Watchpoint and Trace unit provides free, nanosecond-resolution timing with zero intrusion
- :material-alert: **Jitter** — variation in task activation time; even a periodic task activated via `vTaskDelayUntil` has jitter from ISR latency and scheduler overhead
- :material-check-circle: **Use When** — use `vTaskDelayUntil` (not `vTaskDelay`) for truly periodic tasks to prevent period drift
</div>

---

## :material-timer-sand: Worst-Case Execution Time (WCET)

**WCET** is the longest time a task or function can ever take to complete, across all possible inputs and system states. It is the foundation of schedulability analysis—if you use C/T (utilisation) with average execution time, you will miss deadlines under worst-case conditions.

### How to Measure WCET

1. **Instrument with DWT cycle counter** — most accurate, zero intrusion
2. **Logic analyser / oscilloscope** — toggle a GPIO at start/end of the function
3. **Static analysis tools** — AbsInt aiT, Bound-T (model CPU pipeline, cache)
4. **Statistical profiling** — run under stress / boundary-condition inputs and record maximum

!!! warning "Never use average execution time for schedulability analysis"
    Use WCET. Tasks that appear schedulable based on average load can miss deadlines during bursts of worst-case inputs.

---

## :material-speedometer: DWT Cycle Counter

The ARM Cortex-M **Data Watchpoint and Trace (DWT)** unit contains a free-running 32-bit cycle counter (`DWT->CYCCNT`) clocked at the CPU frequency.

```c
/* Enable DWT cycle counter (Cortex-M3/M4/M7) */
static inline void dwt_init(void) {
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;  /* enable trace */
    DWT->CYCCNT = 0;
    DWT->CTRL  |= DWT_CTRL_CYCCNTENA_Msk;            /* start counter */
}

/* Measure execution time of a function */
static inline uint32_t dwt_cycles(void) {
    return DWT->CYCCNT;
}

void measure_wcet(void) {
    uint32_t start, end, elapsed_cycles, elapsed_us;
    uint32_t worst = 0;

    for (int i = 0; i < 10000; i++) {
        start = dwt_cycles();
        my_function_under_test();
        end   = dwt_cycles();
        uint32_t dt = end - start;   /* wraps safely on overflow */
        if (dt > worst) worst = dt;
    }

    /* Convert to microseconds (CPU at 168 MHz) */
    elapsed_us = worst / 168;
    printf("WCET: %lu cycles = %lu us\n", worst, elapsed_us);
}
```

### GPIO Toggle Method (Logic Analyser)

```c
/* Simple intrusive measurement */
#define PERF_PIN_SET()   GPIOB->BSRR = (1 << 5)
#define PERF_PIN_CLR()   GPIOB->BSRR = (1 << 5) << 16

void my_function_under_test(void) {
    PERF_PIN_SET();
    /* ... actual work ... */
    PERF_PIN_CLR();
}
```

---

## :material-chart-bar: Jitter Definition and Measurement

**Jitter** is the variation in a task's actual activation time relative to its ideal periodic schedule.

```
Ideal:  |---T---|---T---|---T---|---T---|
Actual: |--T-ε--|---T+δ--|--T---|--T+2ε-|
            ↑        ↑
          early     late  = jitter
```

### Sources of Jitter

| Source | Typical magnitude | Mitigation |
|--------|-----------------|------------|
| Tick interrupt latency | 0–1 tick (1 ms at 1 kHz) | Use `vTaskDelayUntil` |
| Higher-priority task blocking | Bounded by WCET of higher tasks | RTA analysis |
| ISR execution time | Bounded by ISR WCET | Keep ISRs short |
| Cache miss (Cortex-M7) | 1–20 cycles | Prefetch / pin to TCM |
| Branch misprediction | 1–3 cycles | Unavoidable, measure |

### Measuring Jitter

```c
/* Record activation timestamps to compute jitter */
#define JITTER_SAMPLES  1000

static uint32_t activation_times[JITTER_SAMPLES];
static uint32_t sample_index = 0;

void vPeriodicTask(void *pv) {
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xPeriod = pdMS_TO_TICKS(10);

    for (;;) {
        /* Record actual activation time */
        if (sample_index < JITTER_SAMPLES) {
            activation_times[sample_index++] = DWT->CYCCNT;
        }

        /* Task work here */
        do_periodic_work();

        vTaskDelayUntil(&xLastWakeTime, xPeriod);
    }
}

/* Post-process: compute max jitter from activation_times[] */
```

---

## :material-chart-timeline: Response Time Analysis (RTA)

RTA computes the actual **worst-case response time** R_i for each task, accounting for preemption by higher-priority tasks:

$$R_i^{(0)} = C_i$$

$$R_i^{(k+1)} = C_i + \sum_{j \in hp(i)} \left\lceil \frac{R_i^{(k)}}{T_j} \right\rceil C_j$$

Iterate until R_i^(k+1) = R_i^(k) (converged) or R_i > D_i (deadline missed).

**Example:**

| Task | C (ms) | T (ms) | Priority |
|------|--------|--------|----------|
| τ₁ | 1 | 5 | High |
| τ₂ | 2 | 10 | Medium |
| τ₃ | 3 | 20 | Low |

Response time of τ₃:
```
R₃⁰ = 3
R₃¹ = 3 + ⌈3/5⌉×1 + ⌈3/10⌉×2 = 3 + 1 + 2 = 6
R₃² = 3 + ⌈6/5⌉×1 + ⌈6/10⌉×2 = 3 + 2 + 2 = 7
R₃³ = 3 + ⌈7/5⌉×1 + ⌈7/10⌉×2 = 3 + 2 + 2 = 7  ← converged
R₃ = 7 ms ≤ D₃ = 20 ms  ✓ schedulable
```

---

## :material-clock-check: `vTaskDelayUntil` for Periodic Tasks

`vTaskDelay(n)` delays for n ticks from the **current time** — each period the drift accumulates. `vTaskDelayUntil(&xLastWake, n)` delays until an **absolute wake time**, correcting for the task's own execution time.

```c
/* WRONG — period drifts by execution time each iteration */
void vDriftingTask(void *pv) {
    for (;;) {
        do_work();                        /* takes variable time */
        vTaskDelay(pdMS_TO_TICKS(100));   /* 100 ms from NOW — drift! */
    }
}

/* CORRECT — period is exactly 100 ms wall-clock */
void vPeriodicTask(void *pv) {
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xPeriod = pdMS_TO_TICKS(100);

    for (;;) {
        do_work();
        vTaskDelayUntil(&xLastWakeTime, xPeriod);  /* absolute wake */
    }
}
```

---

## :material-table: Timing Budget Allocation Example

For a 10 ms control loop deadline:

| Component | Budget | Notes |
|-----------|--------|-------|
| ISR entry / NVIC latency | 0.1 ms | Fixed hardware overhead |
| Semaphore give (ISR→task) | 0.05 ms | FreeRTOS API overhead |
| Context switch to task | 0.05 ms | PendSV + restore context |
| Sensor read (SPI, 3 bytes) | 0.5 ms | @ 6 MHz SPI clock |
| Filtering algorithm (WCET) | 2.0 ms | Measured with DWT |
| Control law calculation (WCET) | 1.5 ms | Measured with DWT |
| Actuator write (I2C, 2 bytes) | 1.0 ms | @ 400 kHz I2C |
| Scheduling margin (20%) | 1.0 ms | Buffer for jitter/overrun |
| **Total** | **6.2 ms** | Leaves 3.8 ms margin |

---

## :material-tools: Timing Analysis Tools

| Tool | Type | What It Shows |
|------|------|---------------|
| **Tracealyzer** (Percepio) | Software trace | Task states timeline, CPU load, queue events, screenshots |
| **SystemView** (Segger) | Software trace | Real-time task/ISR timeline, sub-µs resolution via J-Link |
| **GDB + DWT** | Manual profiling | Cycle counts for individual functions |
| **Logic analyser + GPIO** | Hardware | GPIO toggle timestamps, interrupt latency |
| **Lauterbach Trace32** | Hardware trace | ETM/PTM instruction-level trace, cache misses |
| **AbsInt aiT** | Static analysis | Formal WCET bound without measurement |

---

## :material-chart-histogram: Jitter Histogram Concept

Plotting activation jitter over many samples reveals the distribution:

```
Count
  |
40|       ██
35|      ████
30|     ██████
25|    ████████
20|   ██████████
15|  ████████████
10| ██████████████
 5|████████████████
  +--+--+--+--+--+--→ Jitter (µs)
    0  5 10 15 20 25
        ↑
    Typical RTOS
    context-switch
    jitter ~10 µs
```

A well-behaved RTOS control task shows a narrow, approximately normal distribution. Outliers in the tail represent worst-case scheduling events (long higher-priority tasks, cache misses). The WCET for scheduling analysis is the **rightmost tail** value.

---

## :material-help-circle: Flashcards

???+ question "What is the difference between vTaskDelay and vTaskDelayUntil?"
    `vTaskDelay(n)` delays for n ticks starting from the moment it is called. If the task's work takes variable time, each period the next activation drifts later. `vTaskDelayUntil(&xLastWake, n)` computes the absolute next-wake time by adding n to the last recorded wake time—the period is measured wall-clock, compensating for the task's own execution time. Use `vTaskDelayUntil` for all periodic tasks.

???+ question "How do you enable and read the DWT cycle counter on Cortex-M?"
    Enable trace: `CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk`. Enable the counter: `DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk`. Read it: `DWT->CYCCNT`. Take the difference between start and end reads for elapsed cycles. Divide by CPU frequency in MHz to get microseconds.

???+ question "What is Response Time Analysis and when do you need it?"
    RTA iteratively computes each task's worst-case response time by accounting for preemption by all higher-priority tasks. You need it when the Liu & Layland utilisation bound is exceeded (U > n×(2^(1/n)−1)) but you suspect the task set may still be schedulable in practice — or when deadlines differ from periods. RTA gives exact schedulability results where the utilisation bound is only sufficient.

???+ question "Name three sources of jitter in a periodic RTOS task and one mitigation for each."
    1. **Tick granularity** — `vTaskDelayUntil` wakes on the next tick ≥ target; mitigate with higher tick rate or hardware timer direct activation.
    2. **Higher-priority preemption** — a task of higher priority delays the scheduler returning to the periodic task; mitigate by bounding WCET of higher tasks.
    3. **ISR execution** — long ISRs delay PendSV and thus context switch; mitigate by keeping ISRs minimal and deferring work to tasks.

---

## :material-clipboard-check: Self Test

=== "Question 1"
    A task with period T=20 ms and WCET=3 ms is preempted by two higher-priority tasks: τ₁ (C=1 ms, T=5 ms) and τ₂ (C=2 ms, T=10 ms). Use RTA to compute its worst-case response time. Does it meet its deadline (D=20 ms)?

=== "Answer 1"
    R⁰ = 3
    
    R¹ = 3 + ⌈3/5⌉×1 + ⌈3/10⌉×2 = 3 + 1 + 2 = **6**
    
    R² = 3 + ⌈6/5⌉×1 + ⌈6/10⌉×2 = 3 + 2 + 2 = **7**
    
    R³ = 3 + ⌈7/5⌉×1 + ⌈7/10⌉×2 = 3 + 2 + 2 = **7** ← converged
    
    R = 7 ms ≤ D = 20 ms — **schedulable**.

=== "Question 2"
    A task calls `vTaskDelay(pdMS_TO_TICKS(10))` at the end of its body, which takes between 2–4 ms. What is the actual task period range? How would switching to `vTaskDelayUntil` fix this?

=== "Answer 2"
    With `vTaskDelay`, the period = execution time + 10 ms delay. Over time: minimum period = 2 + 10 = **12 ms**, maximum = 4 + 10 = **14 ms** — period varies by 2 ms per cycle. After many iterations the drift can accumulate significantly.
    
    With `vTaskDelayUntil`, the task wakes at the *absolute* scheduled time (last wake + 10 ms), regardless of how long the body took. Period = exactly **10 ms** (within one tick resolution).

---

## :material-check-circle: Summary

!!! success "Key Takeaways"
    - Use **WCET** (not average) for all scheduling calculations — measure it with DWT cycle counter or GPIO toggle.
    - The **DWT cycle counter** is the simplest, most accurate Cortex-M profiling tool: free-running, nanosecond resolution, zero code overhead when read inline.
    - **Jitter** comes from tick granularity, higher-priority preemption, and ISR duration — quantify it with timestamp logging and histogram analysis.
    - Use `vTaskDelayUntil` for periodic tasks to achieve zero drift; `vTaskDelay` accumulates timing error over time.
    - **Response Time Analysis** gives exact worst-case response times for fixed-priority task sets — use it when the Liu & Layland bound is exceeded.
    - Build a **timing budget table** for every hard-deadline loop: allocate time to each component including a ≥20% margin for jitter.
    - Use **Tracealyzer** or **SystemView** during integration testing to visualise task scheduling and catch unexpected preemption patterns.
