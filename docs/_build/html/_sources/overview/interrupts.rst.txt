====================
Interrupt Handling
====================

Introduction
============

**Interrupts** are critical to real-time systems, enabling immediate response to hardware events. In an RTOS context, proper interrupt handling requires understanding:

- Interrupt priorities and nesting
- ISR constraints and best practices
- Deferred interrupt processing
- Integration with RTOS scheduler

This guide covers how to handle interrupts effectively in RTOS-based systems.

Interrupt Fundamentals
======================

What Are Interrupts?
--------------------

An **interrupt** is a hardware signal that temporarily halts normal program execution to handle a time-critical event. 

**Types:**
- **Hardware interrupts**: External events (UART, GPIO, timers)
- **Software interrupts**: Triggered by software (system calls, exceptions)
- **Exceptions**: Error conditions (bus faults, divide by zero)

Interrupt Service Routine (ISR)
--------------------------------

The **ISR** (or interrupt handler) is the function that executes when an interrupt occurs.

.. code-block:: c

    // Simple ISR example (ARM Cortex-M)
    void UART1_IRQHandler(void) {
        // Check interrupt source
        if (UART1->SR & UART_SR_RXNE) {
            // Read received character
            char c = UART1->DR;
            
            // Process or queue for later
            buffer[write_idx++] = c;
        }
    }

Interrupt Latency
-----------------

**Interrupt latency** is the time from interrupt assertion to ISR execution start.

.. code-block:: text

    Event occurs → Interrupt latency → ISR starts → ISR executes → ISR ends
                   ├─────────────────┤
                         Latency

**Components:**
1. Hardware detection time
2. Instruction completion time
3. Context saving time
4. Interrupt priority arbitration

Typical latency: **1-20 microseconds** depending on architecture and configuration.

Interrupt Priorities
====================

Priority Levels
---------------

Most architectures support multiple interrupt priority levels. Higher priority interrupts can preempt lower priority ones.

ARM Cortex-M Example:

.. code-block:: c

    // Configure interrupt priorities0-15 (0 = highest)
    NVIC_SetPriority(UART1_IRQn, 5);   // Medium priority
    NVIC_SetPriority(TIMER1_IRQn, 2);  // High priority
    NVIC_SetPriority(ADC1_IRQn, 10);   // Low priority
    
    // Enable interrupts
    NVIC_EnableIRQ(UART1_IRQn);
    NVIC_EnableIRQ(TIMER1_IRQn);
    NVIC_EnableIRQ(ADC1_IRQn);

Priority Guidelines
-------------------

**Assign higher priorities to:**
- Time-critical events (safety, control loops)
- Events with tight deadlines
- Events that occur rarely

**Assign lower priorities to:**
- Less time-critical events
- Events that can tolerate latency
- High-frequency events (to reduce overhead)

.. code-block:: c

    // Priority assignment example
    #define IRQ_PRIORITY_CRITICAL    0  // Safety interlock
    #define IRQ_PRIORITY_HIGH        3  // Motor control
    #define IRQ_PRIORITY_MEDIUM      5  // UART comms
    #define IRQ_PRIORITY_LOW         10 // Status LED
    #define IRQ_PRIORITY_RTOS        15 // RTOS tick/syscalls

RTOS Integration
================

Interrupt Priorities and RTOS
------------------------------

Most RTOSes use a **syscall priority threshold**: interrupts above this priority cannot call RTOS APIs.

.. code-block:: c

    // FreeRTOS configuration
    #define configMAX_SYSCALL_INTERRUPT_PRIORITY  5
    
    // Priority 0-4: Cannot call FreeRTOS APIs
    // Priority 5-15: Can call FreeRTOS "FromISR" APIs

.. code-block:: text

    Priority
    ────────
       0    ┐
       1    │ High-priority ISRs
       2    │ (Cannot use RTOS APIs)
       3    │ Minimal latency
       4    ┘
    ─────────── configMAX_SYSCALL_INTERRUPT_PRIORITY
       5    ┐
       6    │ RTOS-aware ISRs
       7    │ (Can call *FromISR APIs)
       8    ┘
    ─────────── RTOS kernel priority
      15        (SysTick, PendSV, etc.)

Safe ISR Calls
--------------

**From high-priority ISRs (above threshold):**
- Set flags
- Write to hardware registers
- Minimal processing only

**From RTOS-aware ISRs:**
- Call *FromISR APIs
- Signal semaphores
- Send to queues
- Notify tasks

.. code-block:: c

    // High-priority ISR (priority < configMAX_SYSCALL_INTERRUPT_PRIORITY)
    void CRITICAL_IRQHandler(void) {
        // Can ONLY do direct operations
        critical_flag = 1;
        GPIO_SET(EMERGENCY_PIN);
        // NO RTOS calls allowed!
    }
    
    // RTOS-aware ISR (priority >= configMAX_SYSCALL_INTERRUPT_PRIORITY)
    void UART_IRQHandler(void) {
        char c = UART->DR;
        BaseType_t higher_priority_woken = pdFALSE;
        
        // Safe to call FromISR APIs
        xQueueSendFromISR(uart_queue, &c, &higher_priority_woken);
        
        // Yield if higher-priority task woken
        portYIELD_FROM_ISR(higher_priority_woken);
    }

ISR Constraints
===============

What NOT To Do in ISRs
----------------------

**NEVER:**
- Call blocking functions (``xQueueSend`` without ``FromISR``)
- Use floating-point operations (unless context saved)
- Call ``printf`` or complex library functions
- Allocate/free memory
- Take a long time (> 100 µs guideline)

**Why?**
- Blocks real-time response
- Increases jitter
- May corrupts RTOS state
- Causes priority inversion

Best Practices
--------------

1. **Keep ISRs short**: Defer work to tasks
2. **Use FromISR variants**: Always in RTOS context
3. **Check return values**: Handle failures gracefully
4. **Clear interrupt flags**: Prevent repeated triggering
5. **Use volatile**: For shared variables

Deferred Interrupt Processing
==============================

The Two-Stage Approach
-----------------------

**Stage 1: ISR (fast)**
- Acknowledge interrupt
- Read hardware registers
- Signal RTOS primitive
- Exit quickly

**Stage 2: Task (deferred)**
- Process data
- Perform complex operations
- Can block if needed

Pattern 1: Binary Semaphore
----------------------------

.. code-block:: c

    SemaphoreHandle_t isr_semaphore;
    
    void ISR_Handler(void) {
        BaseType_t higher_priority_woken = pdFALSE;
        
        // Quick: just signal the task
        xSemaphoreGiveFromISR(isr_semaphore, &higher_priority_woken);
        
        portYIELD_FROM_ISR(higher_priority_woken);
    }
    
    void deferred_task(void *param) {
        while (1) {
            // Wait for ISR signal
            xSemaphoreTake(isr_semaphore, portMAX_DELAY);
            
            // Do the heavy lifting here
            complex_processing();
            update_state_machine();
            log_event();
        }
    }

Pattern 2: Queue
----------------

.. code-block:: c

    QueueHandle_t isr_queue;
    
    typedef struct {
        uint32_t timestamp;
        uint16_t adc_value;
        uint8_t channel;
    } adc_reading_t;
    
    void ADC_IRQHandler(void) {
        adc_reading_t reading;
        BaseType_t higher_priority_woken = pdFALSE;
        
        // Read hardware
        reading.timestamp = DWT->CYCCNT;
        reading.adc_value = ADC1->DR;
        reading.channel = 0;
        
        // Queue it
        xQueueSendFromISR(isr_queue, &reading, &higher_priority_woken);
        
        portYIELD_FROM_ISR(higher_priority_woken);
    }
    
    void adc_processing_task(void *param) {
        adc_reading_t reading;
        
        while (1) {
            if (xQueueReceive(isr_queue, &reading, portMAX_DELAY) == pdTRUE) {
                // Process in task context
                float voltage = adc_to_voltage(reading.adc_value);
                apply_filtering(voltage);
                update_display(voltage);
            }
        }
    }

Pattern 3: Task Notification
-----------------------------

.. code-block:: c

    TaskHandle_t processing_task_handle;
    
    void IRQHandler(void) {
        BaseType_t higher_priority_woken = pdFALSE;
        
        // Lightweight notification
        vTaskNotifyGiveFromISR(processing_task_handle, &higher_priority_woken);
        
        portYIELD_FROM_ISR(higher_priority_woken);
    }
    
    void processing_task(void *param) {
        while (1) {
            // Wait for notification
            ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
            
            // Handle event
            process_interrupt_event();
        }
    }

Pattern 4: Deferred Interrupt Work Queue
-----------------------------------------

.. code-block:: c

    typedef void (*deferred_func_t)(void *);
    
    typedef struct {
        deferred_func_t function;
        void *param;
    } work_item_t;
    
    QueueHandle_t work_queue;
    
    void schedule_work(deferred_func_t func, void *param) {
        work_item_t item = { .function = func, .param = param };
        BaseType_t higher_priority_woken = pdFALSE;
        
        xQueueSendFromISR(work_queue, &item, &higher_priority_woken);
        portYIELD_FROM_ISR(higher_priority_woken);
    }
    
    void worker_task(void *param) {
        work_item_t item;
        
        while (1) {
            if (xQueueReceive(work_queue, &item, portMAX_DELAY) == pdTRUE) {
                item.function(item.param);
            }
        }
    }
    
    // Usage in ISR
    void process_uart(void *param) {
        char c = *(char *)param;
        // Complex processing here
    }
    
    void UART_IRQHandler(void) {
        static char received;
        received = UART->DR;
        schedule_work(process_uart, &received);
    }

Interrupt Nesting
=================

Nested interrupts occur when a higher-priority interrupt preempts a lower-priority ISR.

.. code-block:: text

    Task running
        │
        ├──► Low-priority ISR starts
        │        │
        │        ├──► High-priority ISR preempts
        │        │        │
        │        │        └──► High-priority ISR completes
        │        │
        │        └──► Low-priority ISR resumes and completes
        │
        └──► Task resumes

Configuration
-------------

.. code-block:: c

    // Enable interrupt nesting (ARM Cortex-M)
    void init_interrupts(void) {
        // Set priority grouping (4 bits for preempt, 0 for sub)
        NVIC_SetPriorityGrouping(3);
        
        // Configure priorities
        NVIC_SetPriority(HIGH_PRIORITY_IRQ, 2);
        NVIC_SetPriority(LOW_PRIORITY_IRQ, 5);
        
        NVIC_EnableIRQ(HIGH_PRIORITY_IRQ);
        NVIC_EnableIRQ(LOW_PRIORITY_IRQ);
    }

Shared Data Between ISRs and Tasks
===================================

Volatile Keyword
----------------

Use ``volatile`` for variables shared between ISRs and tasks:

.. code-block:: c

    volatile uint32_t interrupt_counter = 0;
    volatile bool data_ready = false;
    
    void ISR_Handler(void) {
        interrupt_counter++;  // Compiler won't optimize away
        data_ready = true;
    }
    
    void task(void *param) {
        while (1) {
            if (data_ready) {
                data_ready = false;
                // Process
            }
        }
    }

Atomic Operations
-----------------

For multi-byte accesses, use critical sections:

.. code-block:: c

    volatile uint32_t shared_value;
    
    // In task
    void task(void *param) {
        taskENTER_CRITICAL();
        uint32_t local_copy = shared_value;
        taskEXIT_CRITICAL();
        
        // Use local_copy safely
    }
    
    // In ISR
    void ISR_Handler(void) {
        // ISRs automatically run with appropriate masking
        shared_value = new_value;
    }

Circular Buffers
----------------

Efficient ISR-to-task communication:

.. code-block:: c

    #define BUFFER_SIZE 128
    
    typedef struct {
        volatile uint8_t buffer[BUFFER_SIZE];
        volatile uint32_t head;
        volatile uint32_t tail;
    } circular_buffer_t;
    
    circular_buffer_t uart_buffer = {0};
    
    // In ISR: add data
    void UART_IRQHandler(void) {
        uint8_t c = UART->DR;
        uint32_t next_head = (uart_buffer.head + 1) % BUFFER_SIZE;
        
        if (next_head != uart_buffer.tail) {
            uart_buffer.buffer[uart_buffer.head] = c;
            uart_buffer.head = next_head;
        }
    }
    
    // In task: read data
    bool buffer_read(uint8_t *data) {
        if (uart_buffer.head == uart_buffer.tail) {
            return false;  // Empty
        }
        
        *data = uart_buffer.buffer[uart_buffer.tail];
        uart_buffer.tail = (uart_buffer.tail + 1) % BUFFER_SIZE;
        return true;
    }

Performance Optimization
========================

Minimize ISR Duration
---------------------

Measure ISR execution time:

.. code-block:: c

    void ISR_Handler(void) {
        GPIO_SET(DEBUG_PIN);  // Set at ISR start
        
        // ISR code here
        
        GPIO_CLEAR(DEBUG_PIN);  // Clear at ISR end
    }
    // Measure pulse width with scope/logic analyzer

Batch Processing
----------------

.. code-block:: c

    #define BATCH_SIZE 16
    uint8_t batch_buffer[BATCH_SIZE];
    uint32_t batch_count = 0;
    
    void ISR_Handler(void) {
        batch_buffer[batch_count++] = read_hardware();
        
        if (batch_count >= BATCH_SIZE) {
            // Signal task to process batch
            xSemaphoreGiveFromISR(batch_ready_sem, NULL);
            batch_count = 0;
        }
    }

Common Pitfalls
===============

1. **Forgetting to clear interrupt flag** → Repeated ISR calls
2. **Using non-FromISR RTOS calls** → System crash
3. **Long ISRs** → Increased jitter and latency
4. **Missing volatile** → Compiler optimization bugs
5. **Priority inversion** → High-priority ISR blocked

Debugging ISRs
==============

.. code-block:: c

    // ISR execution counter
    volatile uint32_t isr_count = 0;
    volatile uint32_t isr_max_duration = 0;
    
    void ISR_Handler(void) {
        uint32_t start = DWT->CYCCNT;
        
        isr_count++;
        
        // ISR work here
        
        uint32_t duration = DWT->CYCCNT - start;
        if (duration > isr_max_duration) {
            isr_max_duration = duration;
        }
    }

Best Practices Summary
======================

1. **Keep ISRs minimal** (< 10-20 µs)
2. **Defer processing** to tasks
3. **Use FromISR APIs** exclusively
4. **Configure priorities** carefully
5. **Protect shared data** with volatile/critical sections
6. **Test under load** to find worst-case behavior
7. **Measure ISR timing** with diagnostic pins

See Also
========

- :doc:`../days/day07` - Interrupt Handling with RTOS
- :doc:`synchronization` - ISR-safe synchronization
- :doc:`timing_analysis` - Measuring interrupt latency
- :doc:`rtos_basics` - Task scheduling fundamentals
- :doc:`../patterns/watchdog_fault_handling` - Fault handling

Further Reading
===============

- ARM Cortex-M Programming Guide
- "Embedded Systems Architecture" by Tammy Noergaard
- Interrupt handling in specific RTOS documentation
