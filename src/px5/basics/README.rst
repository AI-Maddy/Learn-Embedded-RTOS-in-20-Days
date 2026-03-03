PX5 (PikeOS) Basics Examples
============================

Purpose
-------
Fundamental PikeOS partition and thread examples. These demonstrate spatial and
temporal partitioning with safety-critical RTOS features.

Examples Included
-----------------

**hello_task.c**
  - Single thread in partition
  - p4_thr_create() API
  - Partition initialization
  - ~100 lines

**multiple_tasks.c**
  - Multiple threads in partition
  - Thread priorities and affinity
  - Time-triggered scheduling
  - ~170 lines

**queue_example.c**
  - Queuing port communication
  - ARINC 653 style messaging
  - Inter-partition IPC
  - ~220 lines

**semaphore_example.c**
  - Semaphores within partition
  - p4_sem_init(), p4_sem_wait(), p4_sem_post()
  - Mutex for resource protection
  - ~190 lines

Build Instructions
------------------

.. code-block:: bash

   source /opt/pikeos/scripts/setup.sh
   cd src/px5/basics/hello_task
   make ARCH=arm-v7a
   make integration  # Build all partitions

Configuration
-------------

See proj.xml for partition and IPC configuration.

Next Steps
----------

Explore patterns/ for mixed-criticality designs.
