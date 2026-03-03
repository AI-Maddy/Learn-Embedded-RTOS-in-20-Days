ThreadX Source Modules
======================

Purpose
-------
This directory contains ThreadX-focused implementations for Day 14 concepts.
ThreadX is a high-performance RTOS from Microsoft (formerly Express Logic)
designed for deeply embedded applications requiring real-time deterministic
behavior and extensive safety certifications.

ThreadX is certified for use in safety-critical applications (IEC 61508,
IEC 62304, ISO 26262) and offers pre-emption threshold™ scheduling for
optimal determinism and reduced overhead.

Structure
---------

**basics/**
  Fundamental ThreadX kernel primitive usage:
  
  - Thread creation and control (tx_thread_create, tx_thread_resume)
  - Message queues (tx_queue_create, tx_queue_send, tx_queue_receive)
  - Semaphores (tx_semaphore_create, tx_semaphore_get, tx_semaphore_put)
  - Mutexes with priority inheritance (tx_mutex_create, tx_mutex_get)
  - Event flags (tx_event_flags_create, tx_event_flags_set)
  - Block memory pools
  - Byte memory pools
  - Application timers

**patterns/**
  Deterministic design pattern implementations:
  
  - Producer-consumer with message queue
  - Priority inversion prevention with mutexes
  - State machines with event flags
  - Interrupt service routine integration
  - Preemption-threshold scheduling examples
  - Memory pool management patterns

Coding Guidelines
-----------------

**Thread Creation**
  - Define thread entry functions with signature: void func(ULONG input)
  - Allocate stack memory (typically from static arrays)
  - Set priorities: 0 (highest) to 31 (lowest) or custom range
  - Use preemption-threshold to reduce context switches
  - Configure time-slicing if needed

**Synchronization**
  - Use semaphores for simple event signaling
  - Use mutexes for resource protection (includes priority inheritance)
  - Use event flags for multiple condition synchronization
  - Message queues for data passing (supports priorities)
  - Use tx_thread_sleep() for delays

**Memory Management**
  - Block pools for fixed-size allocations (fast, no fragmentation)
  - Byte pools for variable-size allocations
  - Memory is never automatically freed
  - Monitor pool usage with tx_*_info_get() calls
  - Use TX_NO_WAIT vs TX_WAIT_FOREVER appropriately

**Error Handling**
  - Check return values (TX_SUCCESS, TX_QUEUE_FULL, etc.)
  - Use tx_thread_identify() to get current thread
  - Leverage tx_*_info_get() for diagnostics
  - Implement application-defined error handlers

**Interrupt Handling**
  - Minimize ISR execution time
  - Use tx_interrupt_control() to disable/enable interrupts
  - No blocking calls in ISRs
  - Signal threads via tx_semaphore_put() or tx_event_flags_set()

**Preemption-Threshold™**
  - Unique ThreadX feature for determinism
  - Set threshold between thread priority and max allowed preemption
  - Reduces context switches while maintaining responsiveness
  - Useful for mid-priority threads that shouldn't be preempted

Build Instructions
------------------

ThreadX projects use provided build files:

.. code-block:: bash

   cd src/threadx/basics/hello_task
   
   # Using provided Makefile
   make BOARD=stm32f4
   
   # Or with CMake
   mkdir build && cd build
   cmake -DBOARD=stm32f4 ..
   make
   
   # Flash
   make flash

Configuration
-------------

ThreadX configuration in ``tx_user.h``:

.. code-block:: c

   #define TX_MAX_PRIORITIES                   32
   #define TX_MINIMUM_STACK                    200
   #define TX_TIMER_THREAD_STACK_SIZE          1024
   #define TX_TIMER_THREAD_PRIORITY            0
   
   // Enable features
   #define TX_ENABLE_STACK_CHECKING
   #define TX_ENABLE_EVENT_TRACE
   #define TX_THREAD_USER_EXTENSION            my_extension_t

Port-specific configuration in ``tx_port.h``:

.. code-block:: c

   #define TX_TIMER_TICKS_PER_SECOND           1000
   #define TX_INT_DISABLE                      0xC0

Debugging
---------

Enable ThreadX debug features:

.. code-block:: c

   #define TX_ENABLE_STACK_CHECKING
   #define TX_ENABLE_PERFORMANCE_INFO

Use TraceX for visualization (Microsoft tool):

.. code-block:: c

   #define TX_ENABLE_EVENT_TRACE
   tx_trace_enable(&trace_buffer, sizeof(trace_buffer), 32);

Debug with GDB:

.. code-block:: bash

   arm-none-eabi-gdb build/app.elf
   (gdb) target remote localhost:3333
   (gdb) break main
   (gdb) continue

Key Examples
------------

- **hello_task.c**: Single thread with basic output
- **multiple_tasks.c**: Multiple threads with priorities
- **queue_example.c**: Message queue communication
- **semaphore_example.c**: Semaphore synchronization
- **producer_consumer.c**: Queue-based producer-consumer
- **state_machine.c**: Event flag-driven FSM

Advanced Features
-----------------

**Preemption-Threshold Scheduling**:
  Allows a thread to disable preemption by higher-priority threads
  up to a specified threshold.

**FileX Integration**:
  FAT-compatible file system designed for ThreadX.

**NetX/NetX Duo**:
  Embedded TCP/IP stack (IPv4/IPv6).

**USBX**:
  USB host and device stack.

**Safety Certification**:
  Pre-certified packages available for IEC 61508 SIL 4, IEC 62304 Class C.

Performance
-----------

Typical ThreadX performance metrics:

- Context switch: ~100 cycles
- Interrupt latency: minimal (few instructions)
- Semaphore put/get: ~70 cycles
- Queue send/receive: ~100 cycles
- Thread creation: ~200 cycles

Resources
---------

- Azure RTOS Documentation: https://docs.microsoft.com/en-us/azure/rtos/
- ThreadX User Guide: https://github.com/azure-rtos/threadx
- TraceX Tool: https://github.com/azure-rtos/tracex
- GitHub: https://github.com/azure-rtos/threadx
