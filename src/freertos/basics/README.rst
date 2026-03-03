FreeRTOS Basics Examples
========================

Purpose
-------
This directory contains minimal, runnable examples demonstrating fundamental FreeRTOS
concepts. Each example is self-contained and focuses on a specific kernel feature,
making it easy to understand and adapt for your own projects.

These examples are designed for learning and experimentation. They include comprehensive
comments explaining the FreeRTOS APIs and best practices.

Examples Included
-----------------

**hello_task.c**
  Minimal "Hello World" example:
  
  - Creates a single task
  - Uses vTaskDelay() for periodic execution
  - Demonstrates basic task structure
  - Shows UART printf integration
  - ~100 lines of code

**multiple_tasks.c**
  Multiple tasks with different priorities:
  
  - Creates 3 tasks with low, medium, high priorities
  - Demonstrates preemptive scheduling
  - Shows priority-based task execution order
  - Includes timing measurements
  - Logs task execution to console
  - ~150 lines of code

**queue_example.c**
  Producer-consumer pattern using queues:
  
  - Producer task generates data
  - Consumer task processes data from queue
  - Demonstrates xQueueSend() and xQueueReceive()
  - Shows blocking and timeout behavior
  - Queue full/empty handling
  - ~200 lines of code

**semaphore_example.c**
  Task synchronization with semaphores:
  
  - Binary semaphore for task synchronization
  - One task signals, another waits
  - Demonstrates xSemaphoreTake() and xSemaphoreGive()
  - Shows FromISR variants for interrupt context
  - Mutex example for resource protection
  - ~180 lines of code

Build Instructions
------------------

Each example can be built individually:

.. code-block:: bash

   cd src/freertos/basics/hello_task
   make BOARD=stm32f4_discovery
   make flash
   
   # View output
   picocom -b 115200 /dev/ttyUSB0

Or build all basics examples:

.. code-block:: bash

   cd src/freertos/basics
   make all

Expected Output
---------------

**hello_task**: Prints "Hello from FreeRTOS!" every second

**multiple_tasks**: Shows interleaved execution of tasks based on priority

**queue_example**: Producer sends, consumer receives in order

**semaphore_example**: Synchronized task execution

Coding Guidelines
-----------------

**Task Functions**:
  - Infinite loop with vTaskDelay() or blocking call
  - Never return from task function
  - Use vTaskDelete(NULL) to self-terminate if needed

**Stack Sizes**:
  - Minimum: 128 words (configMINIMAL_STACK_SIZE)
  - With printf: 256-512 words
  - Monitor with uxTaskGetStackHighWaterMark()

**Priorities**:
  - 0 = idle priority (lowest)
  - configMAX_PRIORITIES-1 = highest
  - Assign based on deadline and importance

**Error Handling**:
  - Check return values from xTaskCreate(), xQueueCreate(), etc.
  - NULL return = creation failed (out of memory)
  - pdPASS = success, pdFAIL = failure

Timing and Performance
----------------------

Examples include timing measurements using:

- xTaskGetTickCount() for relative timing
- DWT cycle counter for precise measurements
- Task execution time profiling
- Stack usage monitoring

Portability Notes
-----------------

- Uses standard FreeRTOS APIs (portable across MCUs)
- Board-specific code isolated in board_init.c
- UART implementation in common/drivers/
- Tested on: STM32F4, STM32F7, ESP32, NXP i.MX RT
  
Common Issues
-------------

**Task not running**:
  - Check vTaskStartScheduler() is called
  - Verify sufficient heap (configTOTAL_HEAP_SIZE)
  - Check task priority isn't 0 (idle priority)

**Stack overflow**:
  - Increase stack size in xTaskCreate()
  - Enable configCHECK_FOR_STACK_OVERFLOW
  - Implement vApplicationStackOverflowHook()

**Queue full/empty**:
  - Increase queue depth
  - Use timeouts instead of blocking forever
  - Check sender/receiver rates

Next Steps
----------

After mastering basics, explore:

1. **patterns/** - Real-world design patterns
2. **final_project/** - Integrated application
3. FreeRTOS advanced features (software timers, event groups)
4. Optimize for your specific use case
