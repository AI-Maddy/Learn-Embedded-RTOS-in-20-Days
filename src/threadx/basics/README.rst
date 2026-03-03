ThreadX Basics Examples
=======================

Purpose
-------
Fundamental ThreadX kernel examples demonstrating threads, queues, semaphores, and
mutexes using Azure RTOS ThreadX APIs.

Examples Included
-----------------

**hello_task.c**
  - Single thread example
  - tx_thread_create() API
  - Stack allocation
  - ~90 lines

**multiple_tasks.c**
  - Multiple threads with priorities (0=highest)
  - Preemption-threshold demonstration
  - tx_thread_sleep() for delays
  - ~160 lines

**queue_example.c**
  - Message queue communication
  - tx_queue_create(), tx_queue_send(), tx_queue_receive()
  - Fixed-size messages
  - ~200 lines

**semaphore_example.c**
  - Binary and counting semaphores
  - tx_semaphore_create(), tx_semaphore_get(), tx_semaphore_put()
  - Mutex with priority inheritance
  - ~180 lines

Build Instructions
------------------

.. code-block:: bash

   cd src/threadx/basics/hello_task
   make BOARD=stm32f4
   make flash

Configuration
-------------

tx_user.h for compile-time configuration

Performance
-----------

ThreadX offers deterministic performance with minimal overhead.
Enable TX_ENABLE_EVENT_TRACE for TraceX profiling.

Next Steps
----------

Explore patterns/ for advanced designs.
