embOS Basics Examples
=====================

Purpose
-------
Fundamental embOS examples demonstrating SEGGER's minimal-footprint RTOS APIs.
These examples showcase tasks, mailboxes, semaphores, and events.

Examples Included
-----------------

**hello_task.c**
  - Single task with OS_CREATETASK
  - OS_Delay() for periodic execution
  - ~85 lines

**multiple_tasks.c**
  - Multiple tasks with priorities (0=highest)
  - OS_CREATETASK macro usage
  - Static stack allocation
  - ~145 lines

**queue_example.c**
  - Mailbox communication
  - OS_CreateMB(), OS_PutMail(), OS_GetMail()
  - Zero-copy message passing
  - ~195 lines

**semaphore_example.c**
  - Counting semaphores
  - OS_CreateCSema(), OS_Wait(), OS_Signal()
  - Resource semaphores for mutex behavior
  - ~175 lines

Build Instructions
------------------

.. code-block:: bash

   cd src/embos/basics/hello_task
   make BOARD=STM32F429_Discovery
   make flash

Debugging
---------

Use SEGGER Embedded Studio or embOSView for profiling.

Next Steps
----------

See patterns/ for production examples.
