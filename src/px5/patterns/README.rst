PX5 (PikeOS) Patterns
=====================

Purpose
-------
Safety-critical patterns demonstrating partition isolation, mixed-criticality
design, and fault containment in PikeOS.

Patterns Included
-----------------

**producer_consumer.c** (~290 lines)
  - Inter-partition queuing ports
  - ARINC 653 messaging
  - Health monitoring
  - Fault isolation demonstration

**state_machine.c** (~295 lines)
  - Time-triggered FSM
  - Partition event handling
  - Watchdog integration
  - Mixed-criticality scheduling

Build
-----

.. code-block:: bash

   source /opt/pikeos/scripts/setup.sh
   cd src/px5/patterns/producer_consumer
   make ARCH=arm-v7a
   make integration

Features
--------

- Spatial/temporal partitioning
- ARINC 653 IPC
- Mixed-criticality scheduling
- Fault containment
- Watchdog and health monitoring
