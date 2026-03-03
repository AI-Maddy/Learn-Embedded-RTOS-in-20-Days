NuttX Patterns
==============

Purpose
-------
POSIX-compliant design patterns for NuttX, demonstrating Unix-like
approaches in embedded RTOS context.

Patterns Included
-----------------

**producer_consumer.c** (~270 lines)
  - POSIX message queue pipeline
  - pthread-based workers
  - Standard file I/O integration
  - NSH command interface

**state_machine.c** (~280 lines)
  - Signal-driven FSM
  - Condition variable usage
  - Timer (timer_create) integration
  - VFS file logging

Build
-----

.. code-block:: bash

   cd nuttx
   ./tools/configure.sh stm32f4discovery:nsh
   # Add to apps/examples
   make

Features
--------

- POSIX threads and message queues
- Standard signal handling
- VFS for file and device access
- NSH built-in command integration
- pthread condition variables
