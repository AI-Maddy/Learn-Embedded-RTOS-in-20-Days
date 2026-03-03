Zephyr Basics Examples
======================

Purpose
-------
This directory contains minimal, runnable examples demonstrating fundamental Zephyr RTOS
concepts using the native Zephyr kernel APIs. Each example is designed to teach a specific
feature through clear, well-commented code.

Examples Included
-----------------

**hello_task.c**
  - Single thread printing to console
  - Uses k_thread_create() API
  - Demonstrates K_THREAD_DEFINE macro
  - Shows printk() usage
  - ~80 lines

**multiple_tasks.c**
  - Three threads with different priorities
  - Demonstrates preemptive scheduling
  - Shows cooperative vs preemptive threads
  - Priority levels: -1 (coop), 0+ (preemptive)
  - ~150 lines

**queue_example.c**
  - Message queue communication
  - k_msgq_init(), k_msgq_put(), k_msgq_get()
  - Producer-consumer pattern
  - Timeout handling
  - ~180 lines

**semaphore_example.c**
  - Binary and counting semaphores
  - k_sem_init(), k_sem_take(), k_sem_give()
  - Task synchronization
  - ISR to thread signaling
  - ~160 lines

Build Instructions
------------------

.. code-block:: bash

   cd src/zephyr/basics/hello_task
   west build -b nucleo_f429zi
   west flash
   
   # View output
   west espressif monitor  # For ESP32
   # Or: picocom -b 115200 /dev/ttyACM0

Configuration
-------------

Each example includes prj.conf:

.. code-block:: ini

   CONFIG_PRINTK=y
   CONFIG_THREAD_MONITOR=y
   CONFIG_THREAD_NAME=y

Common Features
---------------

- Device tree configuration
- Kconfig build system
- West meta-tool integration
- QEMU testing support
- Logging framework integration

Next Steps
----------

Explore patterns/ for more complex examples.
