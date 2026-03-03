Zephyr Patterns
===============

Purpose
-------
Reusable architectural patterns for Zephyr applications. These demonstrate
production-quality designs using Zephyr's kernel APIs and device model.

Patterns Included
-----------------

**producer_consumer.c** (~250 lines)
  - Message queue-based data pipeline
  - Multiple producers and consumers
  - Work queue integration
  - Logging framework usage

**state_machine.c** (~270 lines)
  - Event-driven FSM with k_poll()
  - State transition logging
  - Timeout handling
  - Device tree integration

Build
-----

.. code-block:: bash

   cd src/zephyr/patterns/producer_consumer
   west build -b nucleo_f429zi
   west flash

Features
--------

- Work queues for deferred processing
- K_poll() for multi-event waiting
- Logging subsystem integration
- Device tree based configuration
- Low-power design considerations
