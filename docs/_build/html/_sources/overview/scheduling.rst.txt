=====================
Scheduling Algorithms
=====================

Introduction
============

The **scheduler** is the heart of an RTOS, responsible for determining which task executes at any given moment. Scheduling algorithms balance multiple competing goals: meeting deadlines, maximizing CPU utilization, ensuring fairness, and maintaining determinism.

Fundamental Concepts
====================

Preemptive vs Cooperative
--------------------------

**Preemptive Scheduling**
  The scheduler can interrupt a running task to give control to a higher-priority task. Guarantees responsiveness but requires careful synchronization.

.. code-block:: c

    // High-priority task preempts low-priority task immediately
    void high_priority_task(void *param) {
        while (1) {
            // This runs immediately when signaled
            xSemaphoreTake(event_sem, portMAX_DELAY);
            handle_critical_event();
        }
    }

**Cooperative Scheduling**
  Tasks voluntarily yield control. Simpler to implement but can cause deadline misses if a task doesn't yield promptly.

.. code-block:: c

    // Task must explicitly yield
    void cooperative_task(void *param) {
        while (1) {
            do_some_work();
            taskYIELD();  // Explicitly give up CPU
        }
    }

Static vs Dynamic Priority
---------------------------

- **Static**: Priority assigned at creation, never changes
- **Dynamic**: Priority can change at runtime (e.g., priority inheritance)

Priority-Based Scheduling
==========================

Most RTOSes use **fixed-priority preemptive scheduling**: the highest-priority ready task always runs.

Algorithm
---------

.. code-block:: text

    while (system running) {
        ready_task = find_highest_priority_ready_task();
        
        if (ready_task != current_task) {
            save_context(current_task);
            restore_context(ready_task);
            current_task = ready_task;
        }
        
        execute(current_task);
    }

Priority Assignment
-------------------

**Rate Monotonic Priority Assignment (RMPA)**: Assign higher priorities to tasks with shorter periods.

.. code-block:: c

    // Correct priority assignment
    #define PRIORITY_1MS_TASK    5  // Highest
    #define PRIORITY_10MS_TASK   4
    #define PRIORITY_100MS_TASK  3
    #define PRIORITY_1S_TASK     2  // Lowest

**Deadline Monotonic Priority Assignment (DMPA)**: Assign higher priorities to tasks with shorter relative deadlines.

Advantages
----------

- Simple and efficient
- Predictable behavior
- Low scheduling overhead
- Widely supported

Disadvantages
-------------

- Priority inversion possible
- Low-priority tasks may starve
- Requires careful priority assignment

Round-Robin Scheduling
=======================

Tasks at the **same priority level** share CPU time equally through **time slicing**.

Configuration
-------------

.. code-block:: c

    // FreeRTOS configuration
    #define configUSE_PREEMPTION        1
    #define configUSE_TIME_SLICING      1
    #define configTICK_RATE_HZ          1000
    
    // Each task gets one tick (1ms) before switching

Example
-------

.. code-block:: c

    // Three tasks at same priority
    xTaskCreate(task1, "Task1", 1024, NULL, 3, NULL);
    xTaskCreate(task2, "Task2", 1024, NULL, 3, NULL);
    xTaskCreate(task3, "Task3", 1024, NULL, 3, NULL);
    
    // Execution: Task1 -> Task2 -> Task3 -> Task1 -> ...

Use Cases
---------

- Background processing tasks
- Tasks with similar importance
- Load balancing

Rate Monotonic Scheduling (RMS)
================================

**RMS** is a proven algorithm for periodic tasks with deadlines equal to periods.

Theory
------

**Liu & Layland Theorem**: A set of n periodic tasks is schedulable by RMS if:

.. math::

    U = \sum_{i=1}^{n} \frac{C_i}{T_i} \leq n(2^{1/n} - 1)

Where:
- C_i = Worst-case execution time
- T_i = Period
- U = CPU utilization

For n → ∞, the bound approaches **69.3%**

Example
-------

.. code-block:: c

    // Task 1: Period=10ms, WCET=3ms, Priority=HIGH
    void task1(void *param) {
        TickType_t last_wake = xTaskGetTickCount();
        while (1) {
            execute_task1();  // Takes 3ms worst-case
            vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(10));
        }
    }
    
    // Task 2: Period=20ms, WCET=5ms, Priority=MEDIUM
    void task2(void *param) {
        TickType_t last_wake = xTaskGetTickCount();
        while (1) {
            execute_task2();  // Takes 5ms worst-case
            vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(20));
        }
    }
    
    // Utilization = 3/10 + 5/20 = 0.55 = 55% < 69.3% ✓ Schedulable

Advantages
----------

- Optimal for fixed-priority scheduling
- Provably correct analysis
- Simple to implement

Limitations
-----------

- Conservative (69.3% utilization bound)
- Requires periodic tasks
- Deadlines must equal periods

Earliest Deadline First (EDF)
==============================

**EDF** is a dynamic priority algorithm: tasks closer to their deadline get higher priority.

Algorithm
---------

.. code-block:: text

    At each scheduling point:
        1. Calculate absolute deadline for each ready task
        2. Select task with earliest deadline
        3. If deadline changes, reschedule

Example
-------

.. code-block:: c

    typedef struct {
        void (*function)(void);
        uint32_t period_ms;
        uint32_t deadline_ms;
        uint32_t next_deadline;
    } edf_task_t;
    
    void edf_scheduler(edf_task_t *tasks, int n) {
        while (1) {
            // Find task with earliest deadline
            edf_task_t *next = NULL;
            uint32_t earliest = UINT32_MAX;
            
            for (int i = 0; i < n; i++) {
                if (tasks[i].next_deadline < earliest) {
                    earliest = tasks[i].next_deadline;
                    next = &tasks[i];
                }
            }
            
            // Execute task
            next->function();
            next->next_deadline += next->period_ms;
        }
    }

Advantages
----------

- **Optimal**: Can achieve 100% CPU utilization
- Flexible deadline handling
- No priority inversion with proper resource protocols

Disadvantages
-------------

- More complex implementation
- Higher scheduling overhead
- Harder to analyze
- Not widely supported in commercial RTOSes

Schedulability Test
-------------------

A set of periodic tasks is schedulable by EDF if:

.. math::

    U = \sum_{i=1}^{n} \frac{C_i}{T_i} \leq 1

Deadline Monotonic Scheduling (DMS)
===================================

Extension of RMS where deadlines can be less than periods. Priority assigned by deadline, not period.

.. code-block:: c

    // Task with period ≠ deadline
    typedef struct {
        uint32_t period_ms;      // 50ms
        uint32_t deadline_ms;    // 30ms (< period)
        uint32_t wcet_ms;        // 10ms
    } dms_task_t;
    
    // Priority = 1 / deadline (higher priority for shorter deadline)

Least Laxity First (LLF)
=========================

**Laxity** = Time until deadline - Remaining execution time

Tasks with least laxity run first.

.. code-block:: c

    int32_t calculate_laxity(task_t *task, uint32_t current_time) {
        return (task->deadline - current_time) - task->remaining_time;
    }

Advantages:
- Can achieve 100% utilization
- Adapts to dynamic situations

Disadvantages:
- Very high overhead
- Excessive context switching
- Rarely used in practice

Scheduling in Multi-Core Systems
=================================

Partitioned Scheduling
----------------------

Assign each task to a specific core permanently.

.. code-block:: c

    // FreeRTOS SMP
    xTaskCreateAffinitySet(task1, "Task1", 1024, NULL, 5, 0x1, NULL);  // Core 0
    xTaskCreateAffinitySet(task2, "Task2", 1024, NULL, 5, 0x2, NULL);  // Core 1

Global Scheduling
-----------------

All tasks can run on any core; scheduler picks best task-core pairing.

Real-Time Scheduling Patterns
==============================

Periodic Tasks
--------------

.. code-block:: c

    void periodic_task(void *param) {
        const TickType_t period = pdMS_TO_TICKS(100);
        TickType_t last_wake = xTaskGetTickCount();
        
        while (1) {
            // Do work
            perform_periodic_action();
            
            // Wait for next period (absolute timing)
            vTaskDelayUntil(&last_wake, period);
        }
    }

Aperiodic Tasks
---------------

Triggered by events, not time.

.. code-block:: c

    void aperiodic_task(void *param) {
        while (1) {
            // Wait for event
            xSemaphoreTake(event_sem, portMAX_DELAY);
            
            // Handle event
            process_aperiodic_event();
        }
    }

Sporadic Tasks
--------------

Event-driven with a minimum inter-arrival time.

.. code-block:: c

    void sporadic_task(void *param) {
        const TickType_t min_separation = pdMS_TO_TICKS(50);
        TickType_t last_activation = 0;
        
        while (1) {
            xSemaphoreTake(event_sem, portMAX_DELAY);
            
            // Enforce minimum separation
            TickType_t now = xTaskGetTickCount();
            TickType_t elapsed = now - last_activation;
            if (elapsed < min_separation) {
                vTaskDelay(min_separation - elapsed);
            }
            
            process_sporadic_event();
            last_activation = xTaskGetTickCount();
        }
    }

Best Practices
==============

1. **Use RMS for periodic tasks** with simple requirements
2. **Assign priorities carefully** based on deadlines/periods
3. **Measure actual execution times** to validate schedulability
4. **Leave headroom** (target 70-80% utilization, not 100%)
5. **Use deadline analysis tools** for complex systems
6. **Monitor for deadline misses** during testing
7. **Document priority rationale** for maintainability

Performance Metrics
===================

- **Response time**: Time from task release to completion
- **Jitter**: Variation in response time
- **CPU utilization**: Percentage of time CPU is busy
- **Context switch overhead**: Time spent switching tasks

See Also
========

- :doc:`../days/day03` - Scheduling and Determinism
- :doc:`../days/day09` - Latency, Jitter, and Timing Analysis
- :doc:`rtos_basics` - Fundamental concepts
- :doc:`timing_analysis` - Timing analysis techniques
- :doc:`../comparison/rtos_selection_guide` - Choosing scheduling approaches

Further Reading
===============

- Liu, C. L. and Layland, J. W. "Scheduling Algorithms for Multiprogramming in a Hard-Real-Time Environment"
- Buttazzo, G. "Hard Real-Time Computing Systems"
- Burns, A. and Wellings, A. "Real-Time Systems and Programming Languages"
