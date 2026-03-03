ThreadX Patterns
================

Purpose
-------
Deterministic design patterns showcasing ThreadX unique features like
preemption-threshold scheduling and performance analysis.

Patterns Included
-----------------

**producer_consumer.c** (~260 lines)
  - Message queue with priorities
  - Block memory pool usage
  - Preemption-threshold example
  - Performance counters

**state_machine.c** (~275 lines)
  - Event flags driven FSM
  - Application timer integration
  - Priority inversion prevention
  - TraceX instrumentation

Build
-----

.. code-block:: bash

   cd src/threadx/patterns/producer_consumer
   make BOARD=stm32f4
   make flash

Features
--------

- Preemption-threshold for determinism
- Block pools for fast allocation
- Event flags for complex sync
- Performance info APIs
- TraceX integration for profiling
