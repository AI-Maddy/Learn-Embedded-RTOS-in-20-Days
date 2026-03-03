NuttX Basics Examples
=====================

Purpose
-------
Fundamental NuttX examples using POSIX-compliant APIs. These examples demonstrate
tasks, message queues, semaphores, and  mutexes in Unix-like RTOS environment.

Examples Included
-----------------

**hello_task.c**
  - Simple task with task_create()
  - POSIX thread alternative
  - printf() for output
  - ~95 lines

**multiple_tasks.c**
  - Multiple tasks with priorities (0-255)
  - task_create() and pthread_create() comparison
  - sched_setparam() for priority
  - ~155 lines

**queue_example.c**
  - POSIX message queues
  - mq_open(), mq_send(), mq_receive()
  - Named and  unnamed queues
  - ~210 lines

**semaphore_example.c**
  - POSIX semaphores
  - sem_init(), sem_wait(), sem_post()
  - pthread_mutex_t for mutual exclusion
  - ~185 lines

Build Instructions
------------------

.. code-block:: bash

   cd nuttx
   ./tools/configure.sh stm32f4discovery:nsh
   cd ../apps
   # Add example to Makefile
   cd ../nuttx
   make
   make flash

NuttShell Integration
---------------------

Examples can be run as NSH built-in commands.

Next Steps
----------

Explore patterns/ and networking/ directories.
