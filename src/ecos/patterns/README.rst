eCos Patterns
=============

Purpose
-------
Architectural patterns demonstrating eCos configurability and flexibility
with native Cyg APIs and HAL integration.

Patterns Included
-----------------

**producer_consumer.c** (~255 lines)
  - Message box pipeline
  - Memory pool allocation
  - Alarm-based scheduling
  - HAL interrupt integration

**state_machine.c** (~270 lines)
  - Event flag driven FSM
  - Counter/alarm timers
  - Condition variable usage
  - Device driver integration

Build
-----

.. code-block:: bash

   ecosconfig new stm32f4discovery
   ecosconfig tree && make
   
   cd src/ecos/patterns/producer_consumer
   arm-eabi-gcc -I$ECOS_INSTALL/include producer_consumer.c \
       -L$ECOS_INSTALL/lib -Ttarget.ld -nostdlib -o app.elf

Features
--------

- Message boxes for IPC
- Event flags for sync
- Memory pools (fixed/variable)
- Alarm/counter timers
- DSR (Deferred Service Routine) pattern
