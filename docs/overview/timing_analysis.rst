================
Timing Analysis
================

Introduction
============

**Timing analysis** is critical for real-time systems to ensure they meet their temporal requirements. This involves measuring and analyzing:

- **WCET** (Worst-Case Execution Time)
- **Response time** (task activation to completion)
- **Latency** (delay from stimulus to response)
- **Jitter** (variation in timing)

Proper timing analysis ensures the system is schedulable and meets all deadlines.

Key Timing Metrics
==================

Worst-Case Execution Time (WCET)
---------------------------------

**WCET** is the maximum time a task or code segment can take to execute.

.. code-block:: text

    Time
    ─────►
    │         ┌────────┐
    │         │  Task  │ ← Nominal case (typical)
    │         └────────┘
    │
    │         ┌──────────────┐
    │         │     Task     │ ← Average case
    │         └──────────────┘
    │
    │         ┌─────────────────────┐
    │         │        Task         │ ← Worst case (WCET)
    │         └─────────────────────┘

**Factors affecting WCET:**
- Code path complexity (loops, branches)
- Cache behavior
- Memory access patterns
- Interrupt preemption
- DMA activity

Response Time
-------------

**Response time** is the total time from task activation to completion, including:
- Waiting time (for higher-priority tasks)
- Preemption time
- Execution time

.. code-block:: text

    Task Released        Task Completes
         │                     │
         ▼                     ▼
    ─────●═════════════════════●─────►
         └──────────────────────┘
            Response Time (R)

.. math::

    R_i = C_i + I_i

Where:
- :math:`R_i` = Response time of task i
- :math:`C_i` = Execution time of task i
- :math:`I_i` = Interference from higher-priority tasks

Latency
-------

**Latency** is the delay between an event and its response:

- **Interrupt latency**: Event to ISR start
- **Task latency**: Event to task execution
- **End-to-end latency**: Input to output delay

.. code-block:: c

    // Measuring latency
    void measure_interrupt_latency(void) {
        // External event sets pin HIGH
        // ISR measures time to entry
        
        volatile uint32_t latency_cycles;
        
        void ISR_Handler(void) {
            // Read timestamp immediately
            latency_cycles = DWT->CYCCNT - event_timestamp;
            
            // Process interrupt
        }
    }

Jitter
------

**Jitter** is the variation in timing (response time, latency, period).

.. code-block:: text

    Expected periodic task execution:
    │   │   │   │   │   │
    ▼   ▼   ▼   ▼   ▼   ▼
    ─────────────────────────► Time
    
    Actual with jitter:
      │ │    │  │   │    │
      ▼ ▼    ▼  ▼   ▼    ▼
    ─────────────────────────► Time
      └─┘ ← Jitter

.. code-block:: c

    // Measuring jitter
    void periodic_task(void *param) {
        const TickType_t period = pdMS_TO_TICKS(100);
        TickType_t last_wake = xTaskGetTickCount();
        uint32_t min_period = UINT32_MAX;
        uint32_t max_period = 0;
        
        while (1) {
            TickType_t now = xTaskGetTickCount();
            uint32_t actual_period = now - last_wake;
            
            if (actual_period < min_period) min_period = actual_period;
            if (actual_period > max_period) max_period = actual_period;
            
            uint32_t jitter = max_period - min_period;
            
            // Do work
            perform_task();
            
            vTaskDelayUntil(&last_wake, period);
        }
    }

Measuring Execution Time
=========================

Hardware Timers
---------------

**Cycle Counter (ARM Cortex-M):**

.. code-block:: c

    // Enable DWT cycle counter (Cortex-M3/M4/M7)
    void enable_cycle_counter(void) {
        CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
        DWT->CYCCNT = 0;
        DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    }
    
    // Measure execution time
    uint32_t measure_function_time(void) {
        uint32_t start = DWT->CYCCNT;
        
        function_under_test();
        
        uint32_t end = DWT->CYCCNT;
        uint32_t cycles = end - start;
        
        // Convert to microseconds (assuming 168 MHz)
        uint32_t microseconds = cycles / 168;
        
        return microseconds;
    }

**System Timer:**

.. code-block:: c

    // Using SysTick or hardware timer
    uint32_t start_time = xTaskGetTickCount();
    
    perform_operation();
    
    uint32_t end_time = xTaskGetTickCount();
    uint32_t elapsed_ticks = end_time - start_time;
    uint32_t elapsed_ms = elapsed_ticks * portTICK_PERIOD_MS;

GPIO Toggle Method
------------------

Simple but effective for oscilloscope measurement:

.. code-block:: c

    void measured_function(void) {
        GPIO_SET(DEBUG_PIN);    // Set pin HIGH at start
        
        // Function body
        do_complex_computation();
        
        GPIO_CLEAR(DEBUG_PIN);  // Set pin LOW at end
    }
    
    // Measure pulse width with oscilloscope or logic analyzer
    // Directly see execution time and jitter

Multiple Points
---------------

.. code-block:: c

    // Instrument multiple points
    void complex_task(void) {
        GPIO_SET(PIN0);        // Overall function
        
        GPIO_SET(PIN1);
        phase1_processing();
        GPIO_CLEAR(PIN1);
        
        GPIO_SET(PIN2);
        phase2_processing();
        GPIO_CLEAR(PIN2);
        
        GPIO_CLEAR(PIN0);      // Overall function end
    }

Statistical Measurement
-----------------------

.. code-block:: c

    #define NUM_SAMPLES 1000
    
    typedef struct {
        uint32_t min;
        uint32_t max;
        uint32_t sum;
        uint32_t count;
    } timing_stats_t;
    
    timing_stats_t stats = {
        .min = UINT32_MAX,
        .max = 0,
        .sum = 0,
        .count = 0
    };
    
    void update_timing_stats(uint32_t sample) {
        if (sample < stats.min) stats.min = sample;
        if (sample > stats.max) stats.max = sample;
        stats.sum += sample;
        stats.count++;
    }
    
    void measure_task_timing(void) {
        uint32_t start = DWT->CYCCNT;
        
        task_function();
        
        uint32_t cycles = DWT->CYCCNT - start;
        update_timing_stats(cycles);
        
        if (stats.count >= NUM_SAMPLES) {
            uint32_t avg = stats.sum / stats.count;
            printf("Min: %u, Max: %u, Avg: %u cycles\n",
                   (unsigned)stats.min, (unsigned)stats.max, (unsigned)avg);
            printf("Jitter: %u cycles\n", (unsigned)(stats.max - stats.min));
        }
    }

Schedulability Analysis
=======================

Rate Monotonic Analysis (RMA)
------------------------------

For periodic tasks with deadline = period:

.. code-block:: c

    typedef struct {
        const char *name;
        uint32_t period_ms;      // T
        uint32_t wcet_ms;        // C
        uint32_t priority;
    } task_info_t;
    
    task_info_t tasks[] = {
        {"Fast",   10,  3, 5},   // C=3ms, T=10ms, U=0.30
        {"Medium", 20,  5, 4},   // C=5ms, T=20ms, U=0.25
        {"Slow",   50,  8, 3},   // C=8ms, T=50ms, U=0.16
    };
    
    bool check_rma_schedulability(task_info_t *tasks, int n) {
        // Calculate total utilization
        float U = 0.0f;
        for (int i = 0; i < n; i++) {
            U += (float)tasks[i].wcet_ms / tasks[i].period_ms;
        }
        
        // Liu & Layland bound
        float bound = n * (powf(2.0f, 1.0f/n) - 1.0f);
        
        printf("Utilization: %.2f%%\n", U * 100);
        printf("RMA bound: %.2f%%\n", bound * 100);
        
        if (U <= bound) {
            printf("Schedulable by RMA\n");
            return true;
        } else if (U <= 1.0f) {
            printf("May be schedulable (requires exact analysis)\n");
            return false;  // Needs further analysis
        } else {
            printf("NOT schedulable (overload)\n");
            return false;
        }
    }

Response Time Analysis
----------------------

More precise than RMA utilization bound:

.. code-block:: c

    // Calculate worst-case response time for task i
    uint32_t calculate_response_time(task_info_t *tasks, int n, int i) {
        uint32_t R = tasks[i].wcet_ms;  // Start with WCET
        uint32_t R_prev;
        
        do {
            R_prev = R;
            R = tasks[i].wcet_ms;  // Execution time
            
            // Add interference from higher-priority tasks
            for (int j = 0; j < n; j++) {
                if (tasks[j].priority > tasks[i].priority) {
                    // Number of preemptions from task j
                    uint32_t preemptions = (R_prev + tasks[j].period_ms - 1) 
                                         / tasks[j].period_ms;
                    R += preemptions * tasks[j].wcet_ms;
                }
            }
        } while (R != R_prev && R <= tasks[i].period_ms);
        
        return R;
    }
    
    void analyze_all_tasks(task_info_t *tasks, int n) {
        printf("\nResponse Time Analysis:\n");
        bool all_schedulable = true;
        
        for (int i = 0; i < n; i++) {
            uint32_t R = calculate_response_time(tasks, n, i);
            
            printf("%s: WCET=%ums, Period=%ums, Response=%ums",
                   tasks[i].name, tasks[i].wcet_ms, 
                   tasks[i].period_ms, R);
            
            if (R <= tasks[i].period_ms) {
                printf(" [OK]\n");
            } else {
                printf(" [DEADLINE MISS]\n");
                all_schedulable = false;
            }
        }
        
        printf("\nSystem %sschedulable\n", 
               all_schedulable ? "" : "NOT ");
    }

Runtime Monitoring
==================

Deadline Miss Detection
-----------------------

.. code-block:: c

    typedef struct {
        TickType_t release_time;
        TickType_t deadline;
        uint32_t miss_count;
    } task_deadline_t;
    
    task_deadline_t task_dl;
    
    void deadline_task(void *param) {
        const TickType_t period = pdMS_TO_TICKS(100);
        const TickType_t deadline_offset = period;  // Deadline = period
        
        TickType_t last_wake = xTaskGetTickCount();
        
        while (1) {
            task_dl.release_time = xTaskGetTickCount();
            task_dl.deadline = task_dl.release_time + deadline_offset;
            
            // Do work
            perform_task_work();
            
            // Check if deadline met
            TickType_t completion_time = xTaskGetTickCount();
            if (completion_time > task_dl.deadline) {
                task_dl.miss_count++;
                // Log deadline miss
                printf("Deadline miss #%u\n", (unsigned)task_dl.miss_count);
            }
            
            vTaskDelayUntil(&last_wake, period);
        }
    }

CPU Load Monitoring
-------------------

.. code-block:: c

    // FreeRTOS runtime stats
    void print_task_stats(void) {
        TaskStatus_t *task_array;
        uint32_t total_runtime;
        UBaseType_t task_count;
        
        // Get task count
        task_count = uxTaskGetNumberOfTasks();
        task_array = pvPortMalloc(task_count * sizeof(TaskStatus_t));
        
        if (task_array != NULL) {
            // Get task stats
            task_count = uxTaskGetSystemState(task_array, 
                                             task_count, 
                                             &total_runtime);
            
            // Calculate percentage
            printf("\nTask CPU Usage:\n");
            for (UBaseType_t i = 0; i < task_count; i++) {
                uint32_t percentage = (task_array[i].ulRunTimeCounter * 100)
                                    / total_runtime;
                                    
                printf("%s: %u%%\n", 
                       task_array[i].pcTaskName,
                       (unsigned)percentage);
            }
            
            vPortFree(task_array);
        }
    }

Idle Time Measurement
---------------------

.. code-block:: c

    volatile uint32_t idle_counter = 0;
    
    void vApplicationIdleHook(void) {
        idle_counter++;
    }
    
    void calculate_cpu_usage(void) {
        static uint32_t last_idle = 0;
        static TickType_t last_time = 0;
        
        uint32_t current_idle = idle_counter;
        TickType_t current_time = xTaskGetTickCount();
        
        uint32_t idle_delta = current_idle - last_idle;
        uint32_t time_delta = current_time - last_time;
        
        // Assume idle loop runs N iterations per ms (calibrated)
        uint32_t idle_time_ms = idle_delta / IDLE_LOOPS_PER_MS;
        uint32_t cpu_usage = 100 - (idle_time_ms * 100 / time_delta);
        
        printf("CPU Usage: %u%%\n", (unsigned)cpu_usage);
        
        last_idle = current_idle;
        last_time = current_time;
    }

Trace Analysis Tools
====================

RTOS Trace
----------

.. code-block:: c

    // FreeRTOS trace hooks (tracealyzer, SystemView, etc.)
    #define traceTASK_SWITCHED_IN()  \
        record_task_switch_in(pxCurrentTCB)
    
    #define traceTASK_SWITCHED_OUT() \
        record_task_switch_out(pxCurrentTCB)
    
    void record_task_switch_in(void *task) {
        uint32_t timestamp = DWT->CYCCNT;
        // Log task switch for offline analysis
        trace_buffer[trace_idx].task = task;
        trace_buffer[trace_idx].timestamp = timestamp;
        trace_buffer[trace_idx].event = EVENT_TASK_IN;
        trace_idx++;
    }

Segment Tracing
---------------

.. code-block:: c

    // Instrumentation for detailed analysis
    void instrumented_function(void) {
        TRACE_BEGIN(FUNC_ID);
        
        complex_operation();
        
        TRACE_END(FUNC_ID);
    }
    
    #define TRACE_BEGIN(id) \
        do { \
            trace_event(TRACE_ENTER, id, DWT->CYCCNT); \
        } while(0)
    
    #define TRACE_END(id) \
        do { \
            trace_event(TRACE_EXIT, id, DWT->CYCCNT); \
        } while(0)

Timing Optimization
===================

Code Optimization
-----------------

.. code-block:: c

    // Before: Inefficient
    void slow_version(void) {
        for (int i = 0; i < 1000; i++) {
            result += sqrt(data[i]);  // Floating point in loop
        }
    }
    
    // After: Optimized
    void fast_version(void) {
        // Use lookup table
        for (int i = 0; i < 1000; i++) {
            result += sqrt_table[data[i]];
        }
    }

Cache Optimization
------------------

.. code-block:: c

    // Align critical data structures to cache line
    __attribute__((aligned(32)))
    typedef struct {
        uint32_t data[16];
    } cache_aligned_struct_t;
    
    // Place hot code in fast memory (ITCM on Cortex-M7)
    __attribute__((section(".itcm")))
    void time_critical_function(void) {
        // Executes from fast instruction memory
    }

Reducing Preemption
-------------------

.. code-block:: c

    // Critical section for very short duration
    void atomic_update(void) {
        taskENTER_CRITICAL();
        
        // Quick operation (< 10 µs)
        shared_data++;
        
        taskEXIT_CRITICAL();
    }

Benchmarking
============

.. code-block:: c

    // Benchmark suite
    typedef struct {
        const char *name;
        void (*function)(void);
        uint32_t iterations;
    } benchmark_t;
    
    void run_benchmark(benchmark_t *bench) {
        uint32_t start = DWT->CYCCNT;
        
        for (uint32_t i = 0; i < bench->iterations; i++) {
            bench->function();
        }
        
        uint32_t end = DWT->CYCCNT;
        uint32_t total_cycles = end - start;
        uint32_t cycles_per_call = total_cycles / bench->iterations;
        
        printf("%s: %u cycles/call (%u iterations)\n",
               bench->name,
               (unsigned)cycles_per_call,
               (unsigned)bench->iterations);
    }

Best Practices
==============

1. **Measure, don't guess**: Use actual measurements, not assumptions
2. **Test worst-case scenarios**: Maximum load, all interrupts, cache misses
3. **Add timing margin**: Design for 70-80% utilization, not 100%
4. **Monitor in production**: Detect timing violations early
5. **Document timing requirements**: Make deadlines explicit
6. **Use static analysis tools**: Complement measurements
7. **Validate on target hardware**: Timing varies between platforms
8. **Test over temperature range**: Performance can degrade with temperature

Common Pitfalls
===============

1. **Testing only typical case**: Missing worst-case scenarios
2. **Ignoring interrupts**: They affect execution time
3. **Cache effects**: Timing varies with cache state
4. **Compiler optimization**: Release vs debug builds differ significantly
5. **Insufficient samples**: Need many measurements for statistical confidence

See Also
========

- :doc:`../days/day09` - Latency, Jitter, and Timing Analysis
- :doc:`scheduling` - Schedulability theory
- :doc:`interrupts` - Interrupt latency
- :doc:`rtos_basics` - Task concepts
- :doc:`../comparison/rtos_selection_guide` - Performance comparison

Further Reading
===============

- "Real-Time Systems" by Jane W.S. Liu
- "Hard Real-Time Computing Systems" by Giorgio Buttazzo
- WCET analysis tools documentation (aiT, RapiTime)
- ARM Cycle Counter documentation
