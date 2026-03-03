====================
Periodic Scheduler
====================

Introduction
============

The **periodic scheduler pattern** ensures tasks execute at regular intervals with precise timing. This is essential for control systems, sensor sampling, and any application requiring deterministic periodic behavior.

**Key Requirements:**
- Accurate period maintenance
- Minimal jitter
- Deadline monitoring
- Phase control (synchronized start times)

Pattern Overview
================

Basic Concepts
--------------

.. code-block:: text

    Task Period (T)
    │◄─────────────►│
    │               │
    ▼               ▼
    ████            ████            ████
    Exec           Exec            Exec
    
    │◄─►│ Execution Time (C)
    │   │◄────────►│ Delay until next period

**Timing Metrics:**
- **Period (T)**: Time between activations
- **Execution Time (C)**: Time to complete work
- **Deadline (D)**: Latest acceptable completion time
- **Jitter (J)**: Variation in actual period

Simple Periodic Task
====================

Using vTaskDelay
----------------

❌ **Incorrect** (accumulates drift):

.. code-block:: c

    void drifting_periodic_task(void *param) {
        const TickType_t period = pdMS_TO_TICKS(100);  // 100ms
        
        while (1) {
            do_work();  // Takes variable time
            
            // Delay doesn't account for execution time!
            vTaskDelay(period);  // WRONG - causes drift
        }
    }

✅ **Correct** (maintains period):

.. code-block:: c

    void accurate_periodic_task(void *param) {
        const TickType_t period = pdMS_TO_TICKS(100);  // 100ms
        TickType_t last_wake_time = xTaskGetTickCount();
        
        while (1) {
            // Do work
            do_work();
            
            // Delay until next period (absolute timing)
            vTaskDelayUntil(&last_wake_time, period);
        }
    }

Multiple Rates
==============

Sub-Dividing Base Rate
-----------------------

.. code-block:: c

    #define BASE_RATE_MS 10  // 10ms base rate (100 Hz)
    
    void multi_rate_task(void *param) {
        const TickType_t base_period = pdMS_TO_TICKS(BASE_RATE_MS);
        TickType_t last_wake_time = xTaskGetTickCount();
        uint32_t tick_count = 0;
        
        while (1) {
            // 100 Hz - every cycle
            fast_task();
            
            // 50 Hz - every 2nd cycle
            if ((tick_count % 2) == 0) {
                medium_task();
            }
            
            // 10 Hz - every 10th cycle
            if ((tick_count % 10) == 0) {
                slow_task();
            }
            
            // 1 Hz - every 100th cycle
            if ((tick_count % 100) == 0) {
                very_slow_task();
            }
            
            tick_count++;
            vTaskDelayUntil(&last_wake_time, base_period);
        }
    }

Separate Tasks with Different Periods
--------------------------------------

.. code-block:: c

    // 1000 Hz - 1ms period (highest priority)
    void fast_control_loop(void *param) {
        const TickType_t period = pdMS_TO_TICKS(1);
        TickType_t last_wake = xTaskGetTickCount();
        
        while (1) {
            read_sensors();
            calculate_control();
            update_actuators();
            
            vTaskDelayUntil(&last_wake, period);
        }
    }
    
    // 100 Hz - 10ms period
    void medium_control_loop(void *param) {
        const TickType_t period = pdMS_TO_TICKS(10);
        TickType_t last_wake = xTaskGetTickCount();
        
        while (1) {
            update_navigation();
            process_telemetry();
            
            vTaskDelayUntil(&last_wake, period);
        }
    }
    
    // 10 Hz - 100ms period
    void slow_control_loop(void *param) {
        const TickType_t period = pdMS_TO_TICKS(100);
        TickType_t last_wake = xTaskGetTickCount();
        
        while (1) {
            update_display();
            log_data();
            
            vTaskDelayUntil(&last_wake, period);
        }
    }
    
    void init_periodic_tasks(void) {
        // Priority assignment: faster = higher priority
        xTaskCreate(fast_control_loop, "Fast", 512, NULL, 5, NULL);
        xTaskCreate(medium_control_loop, "Medium", 512, NULL, 4, NULL);
        xTaskCreate(slow_control_loop, "Slow", 512, NULL, 3, NULL);
    }

Synchronized Start (Phase Control)
===================================

.. code-block:: c

    SemaphoreHandle_t start_semaphore;
    
    void synchronized_task_1(void *param) {
        const TickType_t period = pdMS_TO_TICKS(10);
        
        // Wait for synchronized start
        xSemaphoreTake(start_semaphore, portMAX_DELAY);
        
        TickType_t last_wake = xTaskGetTickCount();
        
        while (1) {
            task1_work();
            vTaskDelayUntil(&last_wake, period);
        }
    }
    
    void synchronized_task_2(void *param) {
        const TickType_t period = pdMS_TO_TICKS(20);
        
        // Wait for synchronized start
        xSemaphoreTake(start_semaphore, portMAX_DELAY);
        
        TickType_t last_wake = xTaskGetTickCount();
        
        while (1) {
            task2_work();
            vTaskDelayUntil(&last_wake, period);
        }
    }
    
    void init_synchronized_tasks(void) {
        start_semaphore = xSemaphoreCreateCounting(10, 0);
        
        xTaskCreate(synchronized_task_1, "Sync1", 512, NULL, 3, NULL);
        xTaskCreate(synchronized_task_2, "Sync2", 512, NULL, 3, NULL);
        
        // Let tasks block on semaphore
        vTaskDelay(pdMS_TO_TICKS(100));
        
        // Release all tasks simultaneously
        xSemaphoreGive(start_semaphore);
        xSemaphoreGive(start_semaphore);
    }

Deadline Monitoring
===================

.. code-block:: c

    typedef struct {
        const char *name;
        uint32_t period_ms;
        uint32_t deadline_ms;
        uint32_t wcet_ms;
        uint32_t miss_count;
        uint32_t exec_time_max;
    } periodic_task_stats_t;
    
    periodic_task_stats_t task_stats = {
        .name = "ControlLoop",
        .period_ms = 10,
        .deadline_ms = 10,
        .wcet_ms = 5,
        .miss_count = 0,
        .exec_time_max = 0
    };
    
    void monitored_periodic_task(void *param) {
        const TickType_t period = pdMS_TO_TICKS(task_stats.period_ms);
        const TickType_t deadline = pdMS_TO_TICKS(task_stats.deadline_ms);
        TickType_t last_wake = xTaskGetTickCount();
        
        while (1) {
            TickType_t start_time = xTaskGetTickCount();
            
            // Do work
            control_loop_work();
            
            TickType_t end_time = xTaskGetTickCount();
            TickType_t exec_time = end_time - start_time;
            
            // Update statistics
            if (exec_time > task_stats.exec_time_max) {
                task_stats.exec_time_max = exec_time;
            }
            
            // Check deadline
            TickType_t completion_time = end_time - last_wake;
            if (completion_time > deadline) {
                task_stats.miss_count++;
                printf("Deadline miss! Time: %u ms (deadline: %u ms)\n",
                       (unsigned)(completion_time * portTICK_PERIOD_MS),
                       (unsigned)task_stats.deadline_ms);
            }
            
            vTaskDelayUntil(&last_wake, period);
        }
    }

Jitter Measurement
==================

.. code-block:: c

    typedef struct {
        uint32_t min_period_us;
        uint32_t max_period_us;
        uint32_t avg_period_us;
        uint32_t sample_count;
        uint64_t sum_periods_us;
    } jitter_stats_t;
    
    jitter_stats_t jitter_stats = {
        .min_period_us = UINT32_MAX,
        .max_period_us = 0,
        .avg_period_us = 0,
        .sample_count = 0,
        .sum_periods_us = 0
    };
    
    void jitter_monitored_task(void *param) {
        const TickType_t period = pdMS_TO_TICKS(10);
        TickType_t last_wake = xTaskGetTickCount();
        uint32_t last_time_us = get_microseconds();
        
        while (1) {
            // Do work
            periodic_work();
            
            // Measure actual period
            uint32_t current_time_us = get_microseconds();
            uint32_t actual_period_us = current_time_us - last_time_us;
            last_time_us = current_time_us;
            
            // Update statistics
            if (actual_period_us < jitter_stats.min_period_us) {
                jitter_stats.min_period_us = actual_period_us;
            }
            if (actual_period_us > jitter_stats.max_period_us) {
                jitter_stats.max_period_us = actual_period_us;
            }
            
            jitter_stats.sum_periods_us += actual_period_us;
            jitter_stats.sample_count++;
            jitter_stats.avg_period_us = jitter_stats.sum_periods_us / jitter_stats.sample_count;
            
            // Calculate jitter
            uint32_t jitter_us = jitter_stats.max_period_us - jitter_stats.min_period_us;
            
            if (jitter_stats.sample_count % 1000 == 0) {
                printf("Jitter: %u us (min:%u, max:%u, avg:%u)\n",
                       (unsigned)jitter_us,
                       (unsigned)jitter_stats.min_period_us,
                       (unsigned)jitter_stats.max_period_us,
                       (unsigned)jitter_stats.avg_period_us);
            }
            
            vTaskDelayUntil(&last_wake, period);
        }
    }

Hardware Timer-Based Scheduling
================================

For sub-millisecond precision:

.. code-block:: c

    #define TIMER_FREQUENCY_HZ 1000000  // 1 MHz timer
    #define CONTROL_RATE_HZ 10000        // 10 kHz control loop
    
    volatile bool control_flag = false;
    
    // Hardware timer ISR (triggered at 10 kHz)
    void TIM2_IRQHandler(void) {
        if (TIM2->SR & TIM_SR_UIF) {
            TIM2->SR = ~TIM_SR_UIF;  // Clear flag
            
            // Set flag for task
            control_flag = true;
            
            // Optionally: directly notify task
            BaseType_t higher_priority_woken = pdFALSE;
            vTaskNotifyGiveFromISR(control_task_handle, &higher_priority_woken);
            portYIELD_FROM_ISR(higher_priority_woken);
        }
    }
    
    // Control task triggered by hardware timer
    void hardware_triggered_control_task(void *param) {
        while (1) {
            // Wait for timer notification
            ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
            
            // Execute time-critical control
            read_encoder();
            calculate_pid();
            update_pwm();
        }
    }
    
    void init_hardware_timer(void) {
        // Configure timer for 10 kHz
        TIM2->PSC = (SystemCoreClock / TIMER_FREQUENCY_HZ) - 1;
        TIM2->ARR = (TIMER_FREQUENCY_HZ / CONTROL_RATE_HZ) - 1;
        TIM2->DIER |= TIM_DIER_UIE;  // Enable update interrupt
        TIM2->CR1 |= TIM_CR1_CEN;     // Start timer
        
        NVIC_EnableIRQ(TIM2_IRQn);
    }

Cyclic Executive Pattern
=========================

For simple systems without full RTOS:

.. code-block:: c

    #define MAJOR_CYCLE_MS 100
    #define MINOR_CYCLE_MS 10
    
    typedef struct {
        void (*function)(void);
        uint32_t period_ms;
        uint32_t last_run;
    } scheduled_function_t;
    
    scheduled_function_t schedule[] = {
        {.function = task_10ms,  .period_ms = 10,  .last_run = 0},
        {.function = task_20ms,  .period_ms = 20,  .last_run = 0},
        {.function = task_50ms,  .period_ms = 50,  .last_run = 0},
        {.function = task_100ms, .period_ms = 100, .last_run = 0},
    };
    
    void cyclic_executive_task(void *param) {
        const TickType_t tick_period = pdMS_TO_TICKS(MINOR_CYCLE_MS);
        TickType_t last_wake = xTaskGetTickCount();
        uint32_t current_time_ms = 0;
        
        while (1) {
            // Execute all functions whose period has elapsed
            for (int i = 0; i < sizeof(schedule) / sizeof(scheduled_function_t); i++) {
                if ((current_time_ms - schedule[i].last_run) >= schedule[i].period_ms) {
                    schedule[i].function();
                    schedule[i].last_run = current_time_ms;
                }
            }
            
            current_time_ms += MINOR_CYCLE_MS;
            vTaskDelayUntil(&last_wake, tick_period);
        }
    }

Overrun Detection
=================

.. code-block:: c

    typedef struct {
        uint32_t overrun_count;
        uint32_t max_overrun_us;
        bool in_overrun;
    } overrun_stats_t;
    
    overrun_stats_t overrun_stats = {0};
    
    void overrun_monitored_task(void *param) {
        const TickType_t period = pdMS_TO_TICKS(10);
        const uint32_t period_us = 10000;
        TickType_t last_wake = xTaskGetTickCount();
        
        while (1) {
            uint32_t start_us = get_microseconds();
            
            // Do work
            time_critical_work();
            
            uint32_t end_us = get_microseconds();
            uint32_t exec_time_us = end_us - start_us;
            
            // Check for overrun
            if (exec_time_us > period_us) {
                overrun_stats.overrun_count++;
                uint32_t overrun_us = exec_time_us - period_us;
                
                if (overrun_us > overrun_stats.max_overrun_us) {
                    overrun_stats.max_overrun_us = overrun_us;
                }
                
                printf("OVERRUN! Execution: %u us, Period: %u us (overrun: %u us)\n",
                       (unsigned)exec_time_us,
                       (unsigned)period_us,
                       (unsigned)overrun_us);
                
                overrun_stats.in_overrun = true;
            } else {
                overrun_stats.in_overrun = false;
            }
            
            vTaskDelayUntil(&last_wake, period);
        }
    }

Adaptive Rate Control
=====================

Adjust rate based on system load:

.. code-block:: c

    void adaptive_rate_task(void *param) {
        TickType_t period = pdMS_TO_TICKS(10);  // Start at 100 Hz
        TickType_t last_wake = xTaskGetTickCount();
        uint32_t consecutive_overruns = 0;
        
        while (1) {
            uint32_t start = get_microseconds();
            
            // Do work
            adaptive_work();
            
            uint32_t exec_time_us = get_microseconds() - start;
            uint32_t period_us = period * portTICK_PERIOD_MS * 1000;
            
            // Check for overrun
            if (exec_time_us > (period_us * 8 / 10)) {  // 80% threshold
                consecutive_overruns++;
                
                if (consecutive_overruns >= 3) {
                    // Slow down
                    period = period * 2;
                    printf("Slowing down to %u ms period\n",
                           (unsigned)(period * portTICK_PERIOD_MS));
                    consecutive_overruns = 0;
                }
            } else if (exec_time_us < (period_us / 2)) {  // 50% threshold
                consecutive_overruns = 0;
                
                // Can speed up
                if (period > pdMS_TO_TICKS(1)) {  // Min 1ms
                    period = period / 2;
                    printf("Speeding up to %u ms period\n",
                           (unsigned)(period * portTICK_PERIOD_MS));
                }
            } else {
                consecutive_overruns = 0;
            }
            
            vTaskDelayUntil(&last_wake, period);
        }
    }

Time-Triggered Communication
=============================

.. code-block:: c

    typedef struct {
        uint16_t msg_id;
        uint8_t data[8];
        uint32_t period_ms;
        uint32_t last_send_ms;
    } periodic_message_t;
    
    periodic_message_t messages[] = {
        {.msg_id = 0x100, .period_ms = 10,  .last_send_ms = 0},
        {.msg_id = 0x200, .period_ms = 20,  .last_send_ms = 0},
        {.msg_id = 0x300, .period_ms = 100, .last_send_ms = 0},
    };
    
    void periodic_comms_task(void *param) {
        const TickType_t tick_period = pdMS_TO_TICKS(1);  // 1ms base
        TickType_t last_wake = xTaskGetTickCount();
        uint32_t time_ms = 0;
        
        while (1) {
            // Check all messages
            for (int i = 0; i < sizeof(messages) / sizeof(periodic_message_t); i++) {
                periodic_message_t *msg = &messages[i];
                
                if ((time_ms - msg->last_send_ms) >= msg->period_ms) {
                    // Time to send
                    update_message_data(msg);
                    can_send(msg->msg_id, msg->data, 8);
                    msg->last_send_ms = time_ms;
                }
            }
            
            time_ms++;
            vTaskDelayUntil(&last_wake, tick_period);
        }
    }

Best Practices
==============

1. **Use vTaskDelayUntil()**: Maintains accurate periods
2. **Assign priorities by rate**: Faster tasks = higher priority
3. **Monitor deadlines**: Detect timing violations
4. **Measure jitter**: Ensure acceptable variation
5. **Leave margin**: Don't use 100% of CPU time
6. **Test worst-case**: Maximum load, all interrupts
7. **Use hardware timers**: For sub-ms precision
8. **Document timing**: Period, deadline, WCET
9. **Handle overruns gracefully**: Don't crash
10. **Validate schedulability**: Use RMA or response time analysis

Common Pitfalls
===============

1. **Using vTaskDelay() instead of vTaskDelayUntil()**: Causes drift
2. **Ignoring execution time variation**: Leads to deadline misses
3. **Too many high-rate tasks**: CPU overload
4. **Incorrect priority assignment**: Priority inversion
5. **Not monitoring timing**: Hidden timing violations

See Also
========

- :doc:`../days/day03` - Scheduling and Determinism
- :doc:`../days/day09` - Latency, Jitter, and Timing Analysis
- :doc:`../overview/scheduling` - Scheduling algorithms
- :doc:`../overview/timing_analysis` - Timing measurement

Further Reading
===============

- "Real-Time Systems" by Jane W.S. Liu
- "Rate Monotonic Analysis" by Liu & Layland
- Cyclic executive patterns documentation
