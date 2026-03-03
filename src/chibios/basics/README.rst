ChibiOS Basics Examples
=======================

Purpose
-------
Fundamental ChibiOS/RT examples demonstrating threads, synchronization, and messaging.
These examples use the native ChibiOS kernel APIs and are designed for learning.

Examples Included
-----------------

**hello_task.c**
  - Single thread with serial output
  - Uses chThdCreateStatic()
  - THD_WORKING_AREA for stack
  - ~100 lines

**multiple_tasks.c**
  - Multiple threads with priorities
  - Higher number = higher priority
  - chThdSleepMilliseconds() for delays
  - ~140 lines

**queue_example.c**
  - Mailbox communication
  - chMBObjectInit(), chMBPost(), chMBFetch()
  - Message passing pattern
  - ~190 lines

**semaphore_example.c**
  - Binary semaphore synchronization
  - chSemObjectInit(), chSemWait(), chSemSignal()
  - Thread coordination
  - ~170 lines

Build Instructions
------------------

.. code-block:: bash

   cd src/chibios/basics/hello_task
   make
   make flash

Configuration Files
-------------------

- chconf.h: Kernel configuration
- halconf.h: HAL configuration  
- mcuconf.h: MCU-specific settings

Next Steps
----------

See patterns/ for production designs.
