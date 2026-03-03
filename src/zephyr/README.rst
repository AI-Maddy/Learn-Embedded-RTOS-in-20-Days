Zephyr Source Modules
=====================

Purpose
-------
This directory contains Zephyr RTOS-specific examples and reference implementations
that demonstrate modern embedded development using the Zephyr Project. These examples
complement Day 12 lessons with practical, runnable code.

Zephyr is a scalable, open-source RTOS designed for resource-constrained devices
with extensive hardware support, networking stacks, and a sophisticated build system
based on CMake and Kconfig.

Structure
---------

**basics/**
  Core Zephyr primitives and usage examples:
  
  - Thread creation and management (k_thread_create)
  - Message queues (k_msgq_init, k_msgq_put, k_msgq_get)
  - Semaphores (k_sem_init, k_sem_take, k_sem_give)
  - Mutexes (k_mutex_init, k_mutex_lock)
  - Work queues for deferred processing
  - Timers (k_timer_init, k_timer_start)
  - Kernel events and polling

**patterns/**
  Reusable architecture patterns in Zephyr:
  
  - Producer-consumer with message queues
  - State machines with event-driven design
  - Device driver integration patterns
  - Power management strategies
  - Logging and diagnostics

**final_project/**
  Integrated Day 20 Zephyr variant:
  
  - Multi-threaded application architecture
  - Sensor data acquisition and processing
  - Network communication (optional)
  - Shell interface for debugging
  - Device tree integration

Coding Guidelines
-----------------

**Thread Design**
  - Use Zephyr's priority model (-1 = cooperative, 0+ = preemptive)
  - Define threads statically with K_THREAD_DEFINE when possible
  - Use appropriate stack sizes (K_THREAD_STACK_SIZEOF)
  - Prefer work queues for non-time-critical deferred work

**Kernel APIs**
  - Use k_* prefix for kernel calls
  - Respect timeout semantics (K_NO_WAIT, K_FOREVER, K_MSEC())
  - Check return values for errors
  - Use kernel objects correctly (init before use)

**Device Model**
  - Use device tree for hardware description
  - Access devices via device_get_binding() or DEVICE_DT_GET()
  - Respect device initialization dependencies
  - Implement proper power management hooks

**Memory Management**
  - Use k_malloc/k_free or memory slabs
  - Prefer stack allocation for small objects
  - Use memory pools for fixed-size allocations
  - Monitor heap usage

**Configuration**
  - Use Kconfig for feature selection
  - Define project-specific options in prj.conf
  - Use overlays for board-specific configs
  - Document configuration dependencies

Build Instructions
------------------

Zephyr uses west as its meta-tool:

.. code-block:: bash

   # Setup Zephyr environment
   source zephyr-env.sh
   
   # Build for specific board
   cd src/zephyr/basics/hello_task
   west build -b nucleo_f429zi
   
   # Flash to device
   west flash
   
   # Clean build
   west build -t clean

Alternatively, with CMake directly:

.. code-block:: bash

   mkdir build && cd build
   cmake -DBOARD=nucleo_f429zi ..
   make

Configuration
-------------

Project configuration in ``prj.conf``:

.. code-block:: ini

   CONFIG_KERNEL_DEBUG=y
   CONFIG_THREAD_MONITOR=y
   CONFIG_THREAD_NAME=y
   CONFIG_PRINTK=y
   CONFIG_CONSOLE=y
   CONFIG_UART_CONSOLE=y
   CONFIG_LOG=y
   CONFIG_LOG_DEFAULT_LEVEL=3

Board-specific overlay in ``boards/nucleo_f429zi.overlay``:

.. code-block:: dts

   &usart3 {
       status = "okay";
       current-speed = <115200>;
   };

Debugging
---------

Debug with west:

.. code-block:: bash

   west debug

Or attach GDB manually:

.. code-block:: bash

   west debugserver &
   arm-none-eabi-gdb build/zephyr/zephyr.elf
   (gdb) target remote :3333

Enable runtime logging:

.. code-block:: c

   #include <logging/log.h>
   LOG_MODULE_REGISTER(my_module, LOG_LEVEL_DBG);
   
   LOG_INF("Starting application");
   LOG_DBG("Debug value: %d", value);

Key Examples
------------

- **hello_task.c**: Minimal thread example with printk
- **multiple_tasks.c**: Multiple threads with priorities
- **queue_example.c**: Message queue demonstration
- **semaphore_example.c**: Binary semaphore synchronization
- **producer_consumer.c**: Producer-consumer with msgq
- **state_machine.c**: Event-driven state machine

Testing
-------

Run in QEMU for testing:

.. code-block:: bash

   west build -b qemu_cortex_m3
   west build -t run

Unit tests with Zephyr's test framework:

.. code-block:: bash

   cd tests/kernel
   west build -b native_posix -t run

Resources
---------

- Zephyr Documentation: https://docs.zephyrproject.org/
- API Reference: https://docs.zephyrproject.org/latest/reference/
- Device Tree Guide: https://docs.zephyrproject.org/latest/guides/dts/
- Community: https://github.com/zephyrproject-rtos/zephyr
