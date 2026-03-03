embOS Source Modules
====================

Purpose
-------
This directory contains embOS-focused implementations for Day 15 concepts.
embOS is a commercial real-time operating system from SEGGER, optimized for
minimum memory footprint and maximum performance. It's widely used in
resource-constrained embedded systems requiring deterministic behavior.

embOS is known for its tiny kernel size (1-2KB), fast context switching,
and comprehensive timing analysis tools. It's particularly popular in
industrial, medical, and IoT devices.

Structure
---------

**basics/**
  Core embOS API examples and fundamental concepts:
  
  - Task creation and management (OS_CREATETASK, OS_Terminate)
  - Mailboxes for message passing (OS_CreateMB, OS_PutMail, OS_GetMail)
  - Semaphores (OS_CreateCSema, OS_Wait, OS_Signal)
  - Resource semaphores (mutexes with priority inversion avoidance)
  - Event objects (OS_CreateEvent, OS_SetEvent, OS_WaitEvent)
  - Timers (OS_CreateTimer, OS_StartTimer)
  - Software timers and callbacks
  - Time management (OS_Delay, OS_GetTime)

**patterns/**
  Architecture-level reusable patterns:
  
  - Producer-consumer with mailboxes
  - State machines with event objects
  - Interrupt handling and deferred processing
  - Resource management with resource semaphores
  - Timer-based periodic tasks
  - Power-aware task design

Coding Guidelines
-----------------

**Task Management**
  - Use OS_CREATETASK macro for static task creation
  - Define stack as array: OS_STACKPTR int Stack[128];
  - Set appropriate task priorities (0 = highest, 255 = lowest)
  - Use OS_Delay() instead of busy-waiting
  - Always initialize embOS with OS_InitKern() and OS_Start()

**Synchronization**
  - Use semaphores for signaling (OS_CSEMA for counting)
  - Use resource semaphores for mutual exclusion (priority ceiling)
  - Use event objects for complex conditions
  - Mailboxes for data exchange (zero-copy message passing)
  - OS_Q for queues with variable-size data

**Memory Management**
  - Static task creation preferred for determinism
  - Fixed memory pools available (OS_MEMF)
  - Dynamic allocation possible but not recommended for real-time
  - Monitor stack usage with embOSView
  - Use OS_STACKPTR for stack definitions

**Interrupt Integration**
  - Use OS_EnterInterrupt()/OS_LeaveInterrupt() in ISRs
  - Or use naked ISR with OS_EnterNestableInterrupt()
  - Signal tasks from ISR using OS_SignalFromInt()
  - Keep ISR execution time minimal
  - embOS handles nested interrupts properly

**Time Management**
  - Configure tick rate in RTOS.h (typically 1000 Hz)
  - Use OS_Delay() for relative delays
  - Use OS_DelayUntil() for periodic tasks
  - Query system time with OS_GetTime()
  - High-resolution timers available on supported hardware

Build Instructions
------------------

embOS projects typically use SEGGER Embedded Studio or Makefile:

.. code-block:: bash

   cd src/embos/basics/hello_task
   make BOARD=STM32F429_Discovery
   # Output: build/app.elf
   
   # Flash with J-Link
   make flash

Or open project in SEGGER Embedded Studio:

.. code-block:: bash

   # Open .emProject file
   segger-embedded-studio hello_task.emProject

Configuration
-------------

embOS configuration in ``RTOS.h``:

.. code-block:: c

   #define OS_FSYS                     168000000  // CPU frequency
   #define OS_TICK_FREQ                1000       // Tick rate (Hz)
   #define OS_SUPPORT_TIMER            1          // Enable timers
   #define OS_SUPPORT_STAT             1          // Enable statistics
   #define OS_TRACKNAME                1          // Task names for debugging
   #define OS_STACKCHECK               1          // Stack overflow checking

Debugging with embOSView
------------------------

embOSView is SEGGER's profiling tool:

.. code-block:: c

   // Enable system view
   #include "SEGGER_SYSVIEW.h"
   
   void main(void) {
       OS_InitKern();
       OS_InitHW();
       SEGGER_SYSVIEW_Conf();
       // Create tasks...
       OS_Start();
   }

Connect with embOSView:

- Real-time task list and state
- CPU load per task
- Timeline visualization
- Context switch analysis
- Interrupt timing
- Semaphore and mailbox state

Key Examples
------------

- **hello_task.c**: Minimal single-task example
- **multiple_tasks.c**: Multiple tasks with priorities
- **queue_example.c**: Mailbox communication
- **semaphore_example.c**: Resource semaphore usage
- **producer_consumer.c**: Mailbox producer-consumer
- **state_machine.c**: Event-driven state machine

Performance
-----------

Typical embOS performance (ARM Cortex-M4 @ 168 MHz):

- Kernel size: ~2 KB
- Context switch: ~50 cycles
- Interrupt latency: ~10 cycles
- OS_Signal(): ~50 cycles
- OS_Wait(): ~60 cycles
- Task activation: ~70 cycles

Advanced Features
-----------------

**Software Timers**:
  Callback-based timers for periodic or one-shot events

**Mailboxes**:
  Efficient zero-copy message passing

**Queues**:
  Variable-size data exchange

**Memory Pools**:
  Fixed-size block allocation (OS_MEMF)

**Event Objects**:
  Multiple condition wait/signal

**Profiling**:
  Built-in profiling with embOSView

**Safety**:
  Available as embOS-Safe with certifications

Porting Notes
-------------

When porting to a new MCU:

1. Port files in embOS/Start/<MCU>/SEGGER/
2. Configure RTOS.h for CPU frequency
3. Adapt RTOSInit.c for hardware init
4. Configure interrupt priorities
5. Test with basic task example

License
-------

embOS is commercial software requiring a license from SEGGER.
Evaluation licenses available for development.

Resources
---------

- SEGGER embOS: https://www.segger.com/products/rtos/embos/
- User Manual: https://www.segger.com/downloads/embos/UM01001
- embOSView: https://www.segger.com/products/rtos/embos/tools/embosview/
- Support: https://forum.segger.com/
