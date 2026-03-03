ChibiOS Source Modules
======================

Purpose
-------
This directory contains ChibiOS-focused implementations demonstrating Day 13 concepts.
ChibiOS is a complete, portable, open-source RTOS with a rich set of features including
a real-time kernel (Nil or RT), HAL, and various protocol stacks.

ChibiOS is known for its efficiency, small footprint, and comprehensive HAL that
supports a wide range of microcontrollers. It's particularly popular in robotics
and industrial applications.

Structure
---------

**basics/**
  Core ChibiOS threading and synchronization concepts:
  
  - Thread creation (chThdCreateStatic, chThdCreateFromHeap)
  - Static and dynamic threads
  - Thread priorities and scheduling
  - Semaphores (chSemObjectInit, chSemWait, chSemSignal)
  - Mutexes (chMtxObjectInit, chMtxLock, chMtxUnlock)
  - Condition variables
  - Message passing (chMsgSend, chMsgGet, chMsgRelease)
  - Mailboxes for buffered messaging
  - Event flags and sources

**patterns/**
  Practical reusable design patterns:
  
  - Producer-consumer using mailboxes
  - Event-driven architectures with event sources
  - State machines in thread context
  - HAL-based peripheral access
  - Virtual timers
  - Memory pool usage

Coding Guidelines
-----------------

**Thread Management**
  - Use static thread creation for deterministic behavior
  - Set appropriate working area sizes
  - Higher numeric priority = higher actual priority
  - Use chThdSleepMilliseconds() instead of busy-waiting
  - Properly terminate threads with chThdExit()

**Synchronization**
  - Prefer semaphores for simple synchronization
  - Use mutexes when priority inheritance is needed
  - Use condition variables for complex wait conditions
  - Message passing is efficient for thread communication
  - Event flags good for multiple condition signaling

**Memory Management**
  - Use static allocation where possible
  - Memory pools for fixed-size objects
  - Core heap for dynamic allocation
  - Monitor working area usage
  - Use THD_WORKING_AREA macro for stack definition

**HAL Integration**
  - Include appropriate HAL headers (hal.h)
  - Initialize HAL with halInit()
  - Use PAL for GPIO (palSetPad, palReadPad)
  - Use Serial driver for UART communication
  - Leverage driver event callbacks

**System Design**
  - Initialize system with chSysInit()
  - Configure system tick rate in halconf.h/chconf.h
  - Use proper configuration files
  - Enable statistics if debugging

Build Instructions
------------------

ChibiOS projects typically use GNU Make:

.. code-block:: bash

   cd src/chibios/basics/hello_task
   make
   # Output in build/
   
   # Flash with OpenOCD
   make flash
   
   # Clean build
   make clean

Configuration in ``Makefile``:

.. code-block:: make

   PROJECT = hello_task
   CHIBIOS = ../../../../ChibiOS
   include $(CHIBIOS)/os/rt/rt.mk
   include $(CHIBIOS)/os/hal/hal.mk
   include $(CHIBIOS)/os/common/startup/ARMCMx/compilers/GCC/mk/startup_stm32f4xx.mk

Configuration
-------------

**chconf.h** - Kernel configuration:

.. code-block:: c

   #define CH_CFG_ST_FREQUENCY                 10000
   #define CH_CFG_INTERVALS_SIZE               32
   #define CH_CFG_TIME_QUANTUM                 20
   #define CH_CFG_MEMCORE_SIZE                 0
   #define CH_CFG_USE_MUTEXES                  TRUE
   #define CH_CFG_USE_SEMAPHORES               TRUE
   #define CH_CFG_USE_MESSAGES                 TRUE

**halconf.h** - HAL configuration:

.. code-block:: c

   #define HAL_USE_PAL                         TRUE
   #define HAL_USE_SERIAL                      TRUE
   #define HAL_USE_SPI                         TRUE
   #define HAL_USE_I2C                         TRUE

**mcuconf.h** - MCU-specific configuration:

.. code-block:: c

   #define STM32_PLLM_VALUE                    8
   #define STM32_PLLN_VALUE                    336
   #define STM32_PLLP_VALUE                    2
   #define STM32_SERIAL_USE_USART2             TRUE

Debugging
---------

Enable kernel statistics:

.. code-block:: c

   #define CH_DBG_STATISTICS                   TRUE
   #define CH_DBG_THREADS_PROFILING            TRUE

Debug with GDB:

.. code-block:: bash

   openocd -f board/stm32f4discovery.cfg &
   arm-none-eabi-gdb build/app.elf
   (gdb) target extended-remote :3333
   (gdb) monitor reset halt
   (gdb) load

Key Examples
------------

- **hello_task.c**: Basic thread with serial output
- **multiple_tasks.c**: Multiple threads with priorities
- **queue_example.c**: Mailbox-based communication
- **semaphore_example.c**: Binary semaphore usage
- **producer_consumer.c**: Mailbox producer-consumer
- **state_machine.c**: Event-driven FSM

Advanced Features
-----------------

**Virtual Timers**:
  Lightweight one-shot or periodic timers

**Registry**:
  Named thread registration and lookup

**Memory Pools**:
  Fast fixed-size block allocation

**Heap**:
  Dynamic memory allocation with fragments coalescing

Resources
---------

- ChibiOS Website: https://www.chibios.org/
- Documentation: https://www.chibios.org/dokuwiki/doku.php
- API Reference: http://chibios.sourceforge.net/docs3/rt/
- Community Forums: https://www.chibios.com/forum/
