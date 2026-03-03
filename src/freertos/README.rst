FreeRTOS Source Modules
=======================

Purpose
-------
This directory contains FreeRTOS-specific examples, reference implementations, and
patterns that demonstrate real-time system design using FreeRTOS APIs. These examples
are designed to accompany Day 11 lessons and provide hands-on experience with one of
the most popular embedded RTOS platforms.

FreeRTOS is a market-leading real-time operating system (RTOS) for microcontrollers
and small microprocessors. It is lightweight, portable, and has extensive community
support.

Structure
---------

**basics/**
  Minimal examples demonstrating core FreeRTOS concepts:
  
  - Task creation and management (xTaskCreate, vTaskDelete)
  - Queue-based inter-task communication (xQueueCreate, xQueueSend)
  - Semaphores for synchronization (xSemaphoreCreateBinary, xSemaphoreTake)
  - Mutexes for resource protection (xSemaphoreCreateMutex)
  - Software timers (xTimerCreate)
  - Event groups for task synchronization
  - Direct-to-task notifications

**patterns/**
  Architectural patterns and best practices:
  
  - Producer-consumer with queue
  - Priority-based task scheduling
  - State machine implementations
  - Deferred interrupt processing
  - Resource management patterns
  - Power-aware task design

**final_project/**
  Integrated Day 20 implementation variant:
  
  - Complete multi-task application
  - Multiple communication mechanisms
  - Real-world sensor data processing
  - Error handling and recovery
  - Power management integration

Coding Guidelines
-----------------

**Task Design**
  - Keep task functions focused and single-purpose
  - Use appropriate task priorities (0 = lowest, configMAX_PRIORITIES-1 = highest)
  - Avoid busy-waiting; use vTaskDelay or blocking operations
  - Design for deterministic timing when required

**Memory Management**
  - Be aware of heap scheme (heap_1.c through heap_5.c)
  - Monitor stack usage with uxTaskGetStackHighWaterMark()
  - Use pvPortMalloc/vPortFree for RTOS-aware allocation
  - Configure stack sizes conservatively

**Synchronization**
  - Prefer queues for data passing
  - Use binary semaphores for synchronization
  - Use mutexes for resource protection (includes priority inheritance)
  - Use counting semaphores for resource counting
  - Use event groups for multiple event flags

**ISR Integration**
  - Use FromISR variants in interrupt handlers
  - Keep ISRs short; defer work to tasks
  - Use xHigherPriorityTaskWoken for context switching
  - Configure interrupt priorities correctly

Build Instructions
------------------

FreeRTOS examples can be built using the provided Makefile or CMake:

.. code-block:: bash

   cd src/freertos
   make BOARD=stm32f4_discovery
   # Output: build/freertos_examples.elf

Or with CMake:

.. code-block:: bash

   mkdir build && cd build
   cmake -DBOARD=stm32f4_discovery ..
   make

Configuration
-------------

FreeRTOS configuration in ``FreeRTOSConfig.h``:

.. code-block:: c

   #define configUSE_PREEMPTION              1
   #define configUSE_IDLE_HOOK               0
   #define configUSE_TICK_HOOK               0
   #define configCPU_CLOCK_HZ                168000000UL
   #define configTICK_RATE_HZ                1000
   #define configMAX_PRIORITIES              5
   #define configMINIMAL_STACK_SIZE          128
   #define configTOTAL_HEAP_SIZE             20480

Flashing and Debugging
----------------------

Flash to target:

.. code-block:: bash

   make flash
   # Or: openocd -f board.cfg -c "program build/app.elf verify reset exit"

Debug with GDB:

.. code-block:: bash

   arm-none-eabi-gdb build/app.elf
   (gdb) target remote localhost:3333
   (gdb) monitor reset halt
   (gdb) load
   (gdb) continue

Key Examples
------------

See the ``basics/`` and ``patterns/`` directories for:

- **hello_task.c**: Minimal single-task "Hello World" example
- **multiple_tasks.c**: Multiple tasks with different priorities
- **queue_example.c**: Producer-consumer using queues
- **semaphore_example.c**: Task synchronization with binary semaphore
- **producer_consumer.c**: Full producer-consumer pattern
- **state_machine.c**: FSM in FreeRTOS task context

Porting Notes
-------------

When porting to a new MCU:

1. Select appropriate FreeRTOS port from portable/ directory
2. Configure FreeRTOSConfig.h for your hardware
3. Implement vApplicationStackOverflowHook() for debugging
4. Configure SysTick or alternative tick source
5. Set interrupt priorities correctly (lower numeric = higher priority)

Resources
---------

- FreeRTOS Kernel Documentation: https://www.freertos.org/
- API Reference: https://www.freertos.org/a00106.html
- Community Forums: https://forums.freertos.org/
