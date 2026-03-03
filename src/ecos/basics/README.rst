eCos Basics Examples
====================

Purpose
-------
Fundamental eCos examples using the native Cyg kernel APIs. These demonstrate
threads, semaphores, mutexes, and message boxes.

Examples Included
-----------------

**hello_task.c**
  - Single thread with cyg_thread_create()
  - Thread structure and stack allocation
  - diag_printf() for output
  - ~110 lines

**multiple_tasks.c**
  - Multiple threads with priorities (0=highest)
  - cyg_thread_resume() to start
  - cyg_thread_delay() for sleeping
  - ~165 lines

**queue_example.c**
  - Message box communication
  - cyg_mbox_create(), cyg_mbox_put(), cyg_mbox_get()
  - Pointer-based messaging
  - ~205 lines

**semaphore_example.c**
  - Binary and counting semaphores
  - cyg_semaphore_init(), cyg_semaphore_wait(), cyg_semaphore_post()
  - cyg_mutex_t for mutual exclusion
  - ~180 lines

Build Instructions
------------------

.. code-block:: bash

   ecosconfig new stm32f4discovery
   ecosconfig tree
   make
   
   cd src/ecos/basics/hello_task
   arm-eabi-gcc -I$ECOS_INSTALL/include hello_task.c \
       -L$ECOS_INSTALL/lib -Ttarget.ld -nostdlib -o app.elf

Configuration
-------------

Use ecosconfig or configtool to configure eCos kernel.

Next Steps
----------

See patterns/ for advanced eCos designs.
