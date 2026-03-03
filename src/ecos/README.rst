eCos Source Modules
===================

Purpose
-------
This directory contains eCos (Embedded Configurable Operating System)-focused
implementations for Day 18 concepts. eCos is an open-source, configurable,
portable RTOS designed for embedded applications. It provides a rich set of
components with extensive compile-time configuration.

eCos is known for its powerful configuration system (CDL), allowing precise
tailoring to application needs, eliminating unused features at compile time.
It supports eCos API, μITRON, and POSIX compatibility layers.

Structure
---------

**basics/**
  Core eCos thread and synchronization examples:
  
  - Thread creation and control (cyg_thread_create, cyg_thread_resume)
  - Kernel scheduling
  - Semaphores (cyg_semaphore_init, cyg_semaphore_wait, cyg_semaphore_post)
  - Mutexes (cyg_mutex_init, cyg_mutex_lock, cyg_mutex_unlock)
  - Condition variables (cyg_cond_wait, cyg_cond_signal)
  - Message boxes (cyg_mbox_put, cyg_mbox_get)
  - Event flags (cyg_flag_wait, cyg_flag_setbits)
  - Alarms and counters
  - Interrupts (cyg_interrupt_create, cyg_interrupt_attach)

**patterns/**
  Architecture patterns adapted to eCos:
  
  - Producer-consumer with message boxes
  - State machines with event flags
  - Deferred interrupt processing (DSRs)
  - Device driver patterns
  - Memory pool usage
  - Alarm-based periodic tasks

Coding Guidelines
-----------------

**Thread Management**
  - Allocate thread structures and stacks
  - Use cyg_thread_create() to initialize
  - Call cyg_thread_resume() to start execution
  - Set priorities: 0 (highest) to 31 (lowest)
  - Use cyg_thread_delay() for sleeping
  - Clean termination with cyg_thread_exit()

**Synchronization**
  - Binary semaphores: init with value 0 (wait) or 1 (signal)
  - Counting semaphores: track resource count
  - Mutexes: provide priority inheritance protocol
  - Condition variables: wait for complex conditions
  - Message boxes: FIFO or priority queues
  - Event flags: wait for pattern of bits

**Memory Management**
  - Fixed-size memory pools (cyg_mempool_fix)
  - Variable-size memory pools (cyg_mempool_var)
  - Static allocation preferred for determinism
  - malloc() available if configured
  - Memory allocators are thread-safe

**Interrupt Handling**
  - Two-level interrupt handling: ISR and DSR
  - ISR acknowledges interrupt, minimal processing
  - DSR (Deferred Service Routine) does main work
  - ISR can't block or use mutex/semaphore directly
  - Use cyg_interrupt_* API for interrupt control

**Configuration**
  - eCos uses Configuration Database Language (CDL)
  - Configure via ecosconfig tool or graphical configtool
  - Components can be included/excluded at compile time
  - Options have values, expressions, dependencies

**HAL Integration**
  - Hardware Abstraction Layer (HAL) for portability
  - HAL provides architecture and platform packages
  - Device drivers use HAL for hardware access
  - Custom HAL packages for new platforms

Build Instructions
------------------

eCos uses a two-phase build: configuration then compilation:

.. code-block:: bash

   # Create eCos configuration
   ecosconfig new stm32f4discovery
   ecosconfig tree
   
   # Build eCos library
   make
   # Output: install/lib/libtarget.a
   
   # Build application
   cd src/ecos/basics/hello_task
   arm-eabi-gcc -I$ECOS_INSTALL/include hello_task.c \
       -L$ECOS_INSTALL/lib -Ttarget.ld -nostdlib -o app.elf
   
   # Flash
   openocd -f board.cfg -c "program app.elf verify reset exit"

Configuration with ecosconfig:

.. code-block:: bash

   # List templates
   ecosconfig list
   
   # Create from template
   ecosconfig new stm32 default
   
   # Add/remove packages
   ecosconfig add CYGPKG_NET
   ecosconfig remove CYGPKG_LIBC_STDIO
   
   # Change option value
   ecosconfig value CYGNUM_HAL_COMMON_INTERRUPTS_STACK_SIZE 8192
   
   # Resolve dependencies
   ecosconfig resolve
   
   # Generate build tree
   ecosconfig tree

Configuration
-------------

Key configuration options in ``ecos.ecc``:

.. code-block:: text

   # Kernel scheduler type
   cdl_component CYGPKG_KERNEL_SCHED_MLQUEUE {  # Multi-level queue
       user_value 1
   }
   
   # Thread priorities
   cdl_option CYGNUM_KERNEL_SCHED_PRIORITIES {
       user_value 32
   }
   
   # Time slice
   cdl_option CYGSEM_KERNEL_SCHED_TIMESLICE {
       user_value 1
   }
   
   # Memory allocators
   cdl_component CYGPKG_MEMALLOC {
       user_value 1
   }

Debugging
---------

Debug with GDB:

.. code-block:: bash

   # Start OpenOCD
   openocd -f interface/stlink.cfg -f target/stm32f4x.cfg
   
   # Connect GDB
   arm-eabi-gdb app.elf
   (gdb) target remote :3333
   (gdb) monitor reset halt
   (gdb) load
   (gdb) break cyg_user_start
   (gdb) continue

Enable kernel assertions:

.. code-block:: bash

   ecosconfig value CYGPKG_KERNEL_INSTRUMENT 1
   ecosconfig value CYGDBG_KERNEL_DEBUG_GDB_THREAD_SUPPORT 1

Key Examples
------------

- **hello_task.c**: Basic thread with diagnostic output
- **multiple_tasks.c**: Multiple threads with priorities
- **queue_example.c**: Message box communication
- **semaphore_example.c**: Binary semaphore sync
- **producer_consumer.c**: Message box producer-consumer
- **state_machine.c**: Event flag-driven FSM

Advanced Features
-----------------

**Schedulers**:
  Bitmap, MLS (multi-level), lottery scheduling

**Compatibility Layers**:
  POSIX (threads, signals), μITRON

**Networking**:
  TCP/IP stack (FreeBSD-derived)

**File Systems**:
  ROM, RAM, Flash file systems

**USB**:
  Device-side USB support

**Instrumentation**:
  Kernel instrumentation for tracing

**RedBoot**:
  ROM monitor and bootloader

Performance
-----------

Typical eCos performance (ARM Cortex-M4):

- Context switch: ~0.5-1 μs
- Semaphore post/wait: ~0.3 μs
- Mutex lock/unlock: ~0.4 μs
- Message box put/get: ~0.8 μs
- Interrupt latency: <1 μs
- Memory footprint: Configurable, 10-300 KB

Porting Guide
-------------

To port eCos to new hardware:

1. Create HAL package (architecture + variant + platform)
2. Define memory layout (.ldi files)
3. Implement HAL functions (startup, context switch, etc.)
4. Provide interrupt handling
5. Configure CDL for platform
6. Test with kernel tests

Resources
---------

- eCos Homepage: https://ecos.sourceware.org/
- Documentation: https://ecos.sourceware.org/docs-latest/
- Source Code: https://sourceware.org/git/ecos.git
- Mailing List: https://sourceware.org/ml/ecos-discuss/
- Community Wiki: https://ecos.sourceware.org/wiki/

License
-------

eCos is open-source software licensed under a modified GPL with
linking exception, allowing proprietary applications.
