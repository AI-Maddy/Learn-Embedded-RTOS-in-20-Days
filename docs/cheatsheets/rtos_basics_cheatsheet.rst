===========================
RTOS Basics Cheat Sheet
===========================

Quick Reference
===============

Core RTOS Concepts
------------------

**Task States:**

.. code-block:: text

    RUNNING → Currently executing
    READY → Can run, waiting for CPU
    BLOCKED → Waiting for resource/event
    SUSPENDED → Manually suspended

**Scheduling:**

.. code-block:: text

    Preemptive: Higher priority preempts lower
    Round-Robin: Equal priority tasks share time
    Cooperative: Tasks must yield voluntarily

**Priority Assignment:**

.. code-block:: text

    Rate Monotonic: Shorter period = Higher priority
    Deadline Monotonic: Shorter deadline = Higher priority

Common API Patterns
===================

Task Management
---------------

.. code-block:: c

    // Create task
    create_task(function, name, stack_size, param, priority, &handle);
    
    // Delete task
    delete_task(handle);
    
    // Delay
    delay(ticks);              // Relative
    delay_until(&time, period); // Absolute (no drift)
    
    // Suspend/Resume
    suspend_task(handle);
    resume_task(handle);
    
    // Priority
    set_priority(handle, new_priority);
    get_priority(handle);

Synchronization
---------------

**Binary Semaphore (Signaling):**

.. code-block:: c

    // Create
    sem = create_binary_semaphore();
    
    // Signal (ISR or task)
    give_semaphore(sem);
    
    // Wait
    take_semaphore(sem, timeout);

**Counting Semaphore (Resource pool):**

.. code-block:: c

    // Create with initial count
    sem = create_counting_semaphore(max_count, initial_count);
    
    // Same give/take as binary

**Mutex (Mutual exclusion):**

.. code-block:: c

    // Create
    mutex = create_mutex();
    
    // Lock
    lock_mutex(mutex, timeout);
    
    // Unlock
    unlock_mutex(mutex);
    
    // ⚠ Never use from ISR!

**Queue (Data transfer):**

.. code-block:: c

    // Create
    queue = create_queue(length, item_size);
    
    // Send
    send_to_queue(queue, &item, timeout);
    send_to_front(queue, &item, timeout);  // Urgent
    
    // Receive
    receive_from_queue(queue, &buffer, timeout);

**Event Group (Multiple flags):**

.. code-block:: c

    // Create
    events = create_event_group();
    
    // Set bits
    set_bits(events, BIT0 | BIT1);
    
    // Wait for bits
    wait_bits(events, BIT0 | BIT1, clear_on_exit, wait_all, timeout);
    
    // Clear bits
    clear_bits(events, BIT0);

Memory Management
-----------------

.. code-block:: c

    // Dynamic allocation
    ptr = malloc(size);
    free(ptr);
    
    // Get heap status
    free_size = get_free_heap_size();
    min_free = get_minimum_ever_free_heap_size();
    
    // Static allocation (preferred)
    static uint8_t stack[STACK_SIZE];
    static TCB_t tcb;
    create_static_task(..., stack, &tcb);

ISR Integration
---------------

.. code-block:: c

    // From ISR variants
    void IRQ_Handler(void) {
        BaseType_t higher_woken = pdFALSE;
        
        // Signal semaphore
        give_semaphore_from_isr(sem, &higher_woken);
        
        // Send to queue
        send_from_isr(queue, &item, &higher_woken);
        
        // Notify task
        notify_from_isr(task_handle, value, &higher_woken);
        
        // Yield if needed
        yield_from_isr(higher_woken);
    }

Critical Sections
-----------------

.. code-block:: c

    // Disable interrupts (very short only!)
    enter_critical();
    // Critical code
    exit_critical();
    
    // Suspend scheduler (interrupts still active)
    suspend_all();
    // Multi-operation atomic section
    resume_all();
    
    // From ISR
    saved = enter_critical_from_isr();
    // Critical code
    exit_critical_from_isr(saved);

Timing
======

Time Units
----------

.. code-block:: c

    // Convert milliseconds to ticks
    ticks = MS_TO_TICKS(100);  // 100 ms
    
    // Get current time
    now = get_tick_count();
    
    // Calculate elapsed
    elapsed = get_tick_count() - start_time;

Periodic Tasks
--------------

.. code-block:: c

    void periodic_task(void *param) {
        TickType_t last_wake = get_tick_count();
        const TickType_t period = MS_TO_TICKS(100);
        
        while (1) {
            // Do periodic work
            do_work();
            
            // Wait for next period (no drift)
            delay_until(&last_wake, period);
        }
    }

Timeouts
--------

.. code-block:: c

    MAX_DELAY    // Block forever
    0            // Non-blocking (poll)
    MS_TO_TICKS(1000)  // 1 second timeout

Common Patterns
===============

Producer-Consumer
-----------------

.. code-block:: c

    QueueHandle_t queue;
    
    // Producer
    void producer(void *param) {
        while (1) {
            data_t item = generate_data();
            send_to_queue(queue, &item, MAX_DELAY);
        }
    }
    
    // Consumer
    void consumer(void *param) {
        data_t item;
        while (1) {
            receive_from_queue(queue, &item, MAX_DELAY);
            process_data(&item);
        }
    }

Deferred ISR Processing
------------------------

.. code-block:: c

    SemaphoreHandle_t isr_sem;
    
    void ISR_Handler(void) {
        BaseType_t higher_woken = pdFALSE;
        give_semaphore_from_isr(isr_sem, &higher_woken);
        yield_from_isr(higher_woken);
    }
    
    void deferred_task(void *param) {
        while (1) {
            take_semaphore(isr_sem, MAX_DELAY);
            // Do heavy processing here
            process_interrupt();
        }
    }

State Machine
-------------

.. code-block:: c

    typedef enum { IDLE, RUNNING, ERROR } state_t;
    typedef enum { START, STOP, ERR } event_t;
    
    void state_machine_task(void *param) {
        state_t state = IDLE;
        event_t event;
        QueueHandle_t event_queue;
        
        while (1) {
            receive_from_queue(event_queue, &event, MAX_DELAY);
            
            switch (state) {
                case IDLE:
                    if (event == START) state = RUNNING;
                    break;
                case RUNNING:
                    if (event == STOP) state = IDLE;
                    else if (event == ERR) state = ERROR;
                    break;
                case ERROR:
                    if (event == STOP) state = IDLE;
                    break;
            }
        }
    }

Debugging Checklist
===================

Common Issues
-------------

.. code-block:: text

    ☐ Stack overflow?
      → Enable stack checking
      → Check high-water mark
      → Increase stack size
      
    ☐ Heap exhausted?
      → Check free heap size
      → Look for memory leaks
      → Use static allocation
      
    ☐ Priority inversion?
      → Use mutexes (with priority inheritance)
      → Not semarphores for mutual exclusion
      
    ☐ Deadlock?
      → Always acquire locks in same order
      → Use timeouts
      → Avoid nested locking
      
    ☐ Task starvation?
      → Check task priorities
      → Monitor CPU usage per task
      → Consider round-robin for equal priority
      
    ☐ Timing violations?
      → Measure worst-case execution time
      → Check for interrupt storms
      → Verify schedulability

Performance Tips
================

Optimization
------------

.. code-block:: text

    ✓ Use delay_until() not delay() for periodic tasks
    ✓ Size stacks carefully (not too large)
    ✓ Minimize critical sections (< 10 µs)
    ✓ Use ISR-safe APIs from interrupts
    ✓ Defer work from ISRs to tasks
    ✓ Use task notifications instead of semaphores when possible
    ✓ Batch operations when possible
    ✓ Prefer static over dynamic allocation
    ✓ Measure actual timing, don't guess
    ✓ Leave 20-30% CPU headroom

Best Practices
==============

.. code-block:: text

    ✓ One task = one responsibility
    ✓ Use queues for data transfer
    ✓ Use semaphores for signaling
    ✓ Use mutexes for resource protection
    ✓ Keep ISRs short (< 50 µs)
    ✓ Validate return values
    ✓ Use timeouts to prevent deadlock
    ✓ Log errors for debugging
    ✓ Monitor stack/heap usage
    ✓ Document priority rationale

Quick Formula Reference
=======================

Schedulability
--------------

.. code-block:: text

    Rate Monotonic Utilization Bound:
    U ≤ n(2^(1/n) - 1)
    
    For n → ∞: U ≤ 0.693 (69.3%)
    
    Example: 3 tasks
    U ≤ 3(2^(1/3) - 1) = 0.78 (78%)
    
    EDF (Earliest Deadline First):
    U ≤ 1.0 (100%)

Timing
------

.. code-block:: text

    Response Time (R) = Execution (C) + Interference (I)
    
    Utilization (U) = Σ(C_i / T_i)
    
    Jitter = Max_Response - Min_Response
    
    Deadline Miss = Completion_Time > Deadline

Priority Assignment
-------------------

.. code-block:: text

    Rate Monotonic: Priority ∝ 1/Period
      Shorter period → Higher priority
      
    Deadline Monotonic: Priority ∝ 1/Deadline  
      Shorter deadline → Higher priority

See Also
========

- :doc:`../overview/rtos_basics` - Detailed RTOS concepts
- :doc:`../overview/scheduling` - Scheduling algorithms
- :doc:`../overview/synchronization` - Synchronization details
- RTOS-specific cheatsheets (FreeRTOS, Zephyr, etc.)

Quick Links
===========

- Task priorities: Higher number often means higher priority (check your RTOS)
- Context switch: Typically 1-5 µs on modern MCUs
- Tick rate: Common values 100 Hz (10ms) to 1000 Hz (1ms)
- Stack size: Measure with watermarking, typical 128-2048 bytes
