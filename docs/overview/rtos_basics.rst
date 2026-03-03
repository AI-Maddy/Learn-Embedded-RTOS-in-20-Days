====================
RTOS Basics
====================

Introduction
============

A **Real-Time Operating System (RTOS)** is an operating system designed to serve real-time applications that process data with strict timing constraints. Unlike general-purpose operating systems, an RTOS guarantees deterministic behavior and predictable response times.

Core Concepts
=============

Tasks and Threads
-----------------

Tasks (or threads) are the fundamental units of execution in an RTOS. Each task represents an independent flow of control with its own:

- **Stack**: Local variables and function call context
- **Priority**: Determines scheduling order
- **State**: Running, Ready, Blocked, or Suspended

**Task States:**

.. code-block:: text

    ┌─────────┐  Preempted   ┌───────┐
    │ RUNNING ├─────────────►│ READY │
    └────┬────┘              └───┬───┘
         │                       │
         │ Block on            │ Schedule
         │ Resource             │
         │                       │
         ▼                       ▼
    ┌─────────┐              ┌───────┐
    │ BLOCKED │              │ READY │
    └─────────┘              └───────┘

Task Example:

.. code-block:: c

    void sensor_task(void *param) {
        while (1) {
            // Read sensor data
            int data = read_sensor();
            
            // Process data
            process_sensor_data(data);
            
            // Delay until next period
            vTaskDelay(pdMS_TO_TICKS(100));
        }
    }

The Scheduler
-------------

The **scheduler** is the core component that decides which task runs at any given time. It ensures:

- **Highest priority ready task always runs** (priority-based scheduling)
- **Deterministic task switching** with predictable latency
- **Minimal overhead** for context switching

Scheduling Policies:

1. **Preemptive Priority-Based**: Higher priority tasks preempt lower priority ones
2. **Cooperative**: Tasks voluntarily yield control
3. **Time-Slicing**: Equal priority tasks share CPU time

Determinism
-----------

**Determinism** means the system's behavior is predictable and repeatable. In an RTOS:

- Task scheduling latency has a **known upper bound**
- Interrupt response time is **guaranteed**
- System calls complete within **predictable timeframes**

Factors affecting determinism:

- Interrupt masking duration
- Critical section length
- Scheduling algorithm complexity
- Memory access patterns

Real-Time Constraints
---------------------

Real-time systems classify timing requirements into:

Hard Real-Time
~~~~~~~~~~~~~~

Missing a deadline causes **system failure**. Examples:

- Airbag deployment (must trigger within 10ms)
- Anti-lock braking control
- Pacemaker timing

.. code-block:: c

    // Hard real-time constraint example
    void airbag_task(void *param) {
        const uint32_t MAX_RESPONSE_TIME_MS = 10;
        
        while (1) {
            if (detect_collision()) {
                // MUST complete within 10ms
                deploy_airbag();
            }
        }
    }

Soft Real-Time
~~~~~~~~~~~~~~

Missing deadlines **degrades performance** but doesn't cause failure:

- Audio/video streaming
- User interface updates
- Network packet processing

Firm Real-Time
~~~~~~~~~~~~~~

Missing occasional deadlines is acceptable, but repeated misses cause failure:

- Manufacturing control systems
- Telecommunications

Priority Inversion
------------------

**Priority inversion** occurs when a high-priority task waits for a resource held by a low-priority task, while a medium-priority task runs.

**Solution**: Priority inheritance - the low-priority task temporarily inherits the high-priority task's priority.

.. code-block:: c

    // Without priority inheritance
    mutex_lock(&shared_resource);    // Low priority holds mutex
    // High priority blocked, medium priority runs!
    
    // With priority inheritance
    mutex_lock_inherit(&shared_resource);  // Low priority inherits high priority
    // Medium priority cannot preempt

Context Switching
-----------------

**Context switching** is the process of saving one task's state and restoring another's. An RTOS minimizes context switch time through:

- Efficient save/restore of CPU registers
- Optimized stack management
- Hardware-assisted switching (on some architectures)

Typical context switch times: **1-10 microseconds**

Stack Management
----------------

Each task requires its own stack. Key considerations:

- **Stack size**: Must accommodate worst-case usage
- **Stack overflow detection**: Debug guards or MPU protection
- **Stack allocation**: Static vs dynamic

.. code-block:: c

    // Static stack allocation
    static uint8_t sensor_task_stack[2048];
    
    // Create task with specific stack
    xTaskCreateStatic(sensor_task,
                      "Sensor",
                      2048,
                      NULL,
                      PRIORITY_NORMAL,
                      sensor_task_stack,
                      &sensor_task_tcb);

Tick Timer
----------

The RTOS **tick timer** provides the time base for:

- Task delays
- Timeouts
- Time-slicing
- Time tracking

Common tick rates: **100Hz to 1000Hz** (1-10ms periods)

.. code-block:: c

    // Configure tick rate (FreeRTOS example)
    #define configTICK_RATE_HZ  1000  // 1ms tick

Kernel Objects
==============

Critical Section
----------------

A code region that must execute atomically:

.. code-block:: c

    // Disable interrupts briefly
    taskENTER_CRITICAL();
    shared_counter++;
    taskEXIT_CRITICAL();

Idle Task
---------

The **idle task** runs when no other task is ready. Uses:

- Put CPU in low-power mode
- Background garbage collection
- Monitor system health

.. code-block:: c

    void vApplicationIdleHook(void) {
        // Enter low-power mode
        __WFI();  // Wait For Interrupt
    }

Best Practices
==============

1. **Keep ISRs short**: Defer processing to tasks
2. **Avoid blocking in ISRs**: Never call blocking functions
3. **Size stacks carefully**: Use stack analysis tools
4. **Minimize critical sections**: Keep interrupt-disabled time minimal
5. **Use appropriate priorities**: Assign based on deadline urgency
6. **Design for worst-case**: Plan for maximum load scenarios
7. **Test timing**: Measure actual response times under load

Common Pitfalls
===============

- **Stack overflow**: Insufficient stack allocation
- **Priority inversion**: Not using priority inheritance
- **Starvation**: Low-priority tasks never run
- **Deadlock**: Circular resource dependencies
- **Race conditions**: Unsynchronized shared data access

See Also
========

- :doc:`../days/day01` - Introduction to RTOS
- :doc:`../days/day02` - Tasks and Threads
- :doc:`../days/day03` - Scheduling and Determinism
- :doc:`scheduling` - Detailed scheduling algorithms
- :doc:`synchronization` - Synchronization primitives
- :doc:`../cheatsheets/rtos_basics_cheatsheet` - Quick reference

Further Reading
===============

- "Real-Time Systems" by Jane W.S. Liu
- "MicroC/OS-II: The Real-Time Kernel" by Jean Labrosse
- "Real-Time Concepts for Embedded Systems" by Qing Li and Caroline Yao
