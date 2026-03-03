embOS Patterns
==============

Purpose
-------
Architectural patterns demonstrating embOS efficiency and minimal footprint
while maintaining production quality.

Patterns Included
-----------------

**producer_consumer.c** (~235 lines)
  - Mailbox zero-copy messaging
  - Multiple producers/consumers
  - Resource semaphore usage
  - embOSView instrumentation

**state_machine.c** (~255 lines)
  - Event object driven FSM
  - Software timer callbacks
  - Minimal memory footprint
  - SEGGER RTT logging

Build
-----

.. code-block:: bash

   cd src/embos/patterns/producer_consumer
   make BOARD=STM32F429_Discovery
   make flash

Features
--------

- Mailboxes for zero-copy IPC
- Event objects for complex conditions
- Resource semaphores (priority ceiling)
- Software timers with callbacks
- embOSView profiling support
