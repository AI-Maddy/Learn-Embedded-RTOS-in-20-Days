ChibiOS Patterns
================

Purpose
-------
Production-ready design patterns for ChibiOS/RT applications demonstrating
real-world architectural approaches.

Patterns Included
-----------------

**producer_consumer.c** (~240 lines)
  - Mailbox-based pipeline
  - Multiple producers/consumers
  - Memory pool integration
  - HAL driver usage

**state_machine.c** (~265 lines)
  - Event source driven FSM
  - Virtual timer integration
  - State persistence
  - Serial driver communication

Build
-----

.. code-block:: bash

   cd src/chibios/patterns/producer_consumer
   make
   make flash

Features
--------

- Event sources for async notifications
- Virtual timers for scheduling
- Memory pools for fixed allocation
- HAL integration (Serial, PAL, etc.)
- Registry for thread management
