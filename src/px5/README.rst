PX5 (PikeOS) Source Modules
===========================

Purpose
-------
This directory contains PX5 (PikeOS)-focused implementations for Day 17 concepts.
PikeOS is a commercial separation microkernel and real-time operating system from
SYSGO, designed for safety-critical and security-critical applications requiring
strong isolation and certification.

PikeOS provides spatial and temporal partitioning with hypervisor capabilities,
allowing multiple operating systems to coexist with guaranteed non-interference.
It's certified to the highest safety levels (DO-178C DAL A, IEC 61508 SIL 4,
ISO 26262 ASIL D).

Structure
---------

**basics/**
  Core PikeOS API demonstrations:
  
  - Thread creation and management (p4_thr_create)
  - Message passing (p4_msg_send, p4_msg_receive)
  - Synchronization primitives (p4_sem_init, p4_mutex_init)
  - Shared memory regions (p4_shm_create)
  - Inter-partition communication (IPC)
  - Time management (p4_sleep, p4_get_time)
  - Event handling
  - Resource partitioning examples

**patterns/**
  Reusable architecture examples:
  
  - Producer-consumer across partitions
  - State machines in partitioned environment
  - Resource sharing with isolation
  - Fault containment patterns
  - Time-triggered communication
  - Mixed-criticality system design

Coding Guidelines
-----------------

**Partition Design**
  - Each partition is an independent execution environment
  - Partitions have dedicated resources (memory, CPU time)
  - No shared memory between partitions unless explicitly configured
  - Partition configuration in XML files
  - Design for isolation and fault containment

**Thread Management**
  - Threads exist within partitions
  - Use p4_thr_create() to create threads
  - Set appropriate priorities and affinity
  - Threads share partition resources
  - Use POSIX threads API where available

**Inter-Partition Communication (IPC)**
  - Queuing ports for message passing
  - Sampling ports for state sharing
  - Shared memory with access control
  - Events for notification
  - All IPC configured in integration file
  - IPC follows ARINC 653 model

**Synchronization**
  - Semaphores (p4_sem_*) for signaling
  - Mutexes (p4_mutex_*) for mutual exclusion
  - Condition variables for complex waits
  - All within partition scope

**Time and Scheduling**
  - Temporal partitioning ensures determinism
  - Major time frame divided into time windows
  - Each partition has allocated time windows
  - Time-triggered and event-triggered scheduling
  - Watchdogs for monitoring

**Safety and Security**
  - Memory protection enforced by hypervisor
  - CPU time strictly partitioned
  - I/O devices assigned to partitions
  - Fault isolation between partitions
  - Secure boot and attestation

Build Instructions
------------------

PikeOS projects use SYSGO's build environment:

.. code-block:: bash

   # Setup PikeOS environment
   source /opt/pikeos/scripts/setup.sh
   
   # Build partition
   cd src/px5/basics/hello_task
   make ARCH=arm-v7a TARGET=qemu-arm
   
   # Integration build (all partitions)
   cd project
   make integration
   # Output: integration.elf

Configuration
-------------

**Project Configuration (proj.xml)**:

.. code-block:: xml

   <project>
     <partition name="app1" id="1">
       <memory>
         <region base="0x1000000" size="0x100000" />
       </memory>
       <schedule>
         <window offset="0ms" duration="10ms" />
       </schedule>
     </partition>
   </project>

**Partition Configuration**:

.. code-block:: c

   // In partition's main
   #include <p4.h>
   
   int main(void) {
       p4_partition_init();
       // Create threads, register handlers
       p4_partition_run();
   }

**IPC Port Configuration**:

.. code-block:: xml

   <port name="sensor_data" type="queuing" 
         direction="source" max_msg_size="128" max_msgs="8" />

Debugging
---------

PikeOS supports source-level debugging:

.. code-block:: bash

   # Start QEMU target
   make qemu-debug
   
   # Connect GDB
   p4gdb partition.elf
   (gdb) target remote :1234
   (gdb) break main
   (gdb) continue

Use CODEO IDE (Eclipse-based):

- Integrated debugging
- Partition monitoring
- Resource usage visualization
- Trace analysis

PikeOS Monitor:

.. code-block:: bash

   # Runtime monitoring
   p4mon --show-partitions
   p4mon --show-threads
   p4mon --show-cpu-usage

Key Examples
------------

- **hello_task.c**: Single partition, single thread
- **multiple_tasks.c**: Multiple threads in partition
- **queue_example.c**: Queuing port communication
- **semaphore_example.c**: Semaphore synchronization
- **producer_consumer.c**: Inter-partition messaging
- **state_machine.c**: Partitioned state machine

Ad Advanced Features
-------------------

**Partitioning**:
  Spatial (memory) and temporal (CPU) isolation

**Hypervisor**:
  Run Linux, RTOS, or bare-metal in partitions

**ARINC 653**:
  Full ARINC 653 APEX API support

**Multi-Core**:
  Symmetric and asymmetric multiprocessing

**Virtualization**:
  Paravirtualization for Linux guests

**Safety**:
  Pre-certified to highest safety levels

**Security**:
  Common Criteria EAL 5+, secure boot

Mixed-Criticality Example
-------------------------

.. code-block:: xml

   <!-- High-criticality partition: 70% CPU -->
   <partition name="safety_critical" criticality="high">
     <schedule>
       <window offset="0ms" duration="70ms" />
     </schedule>
   </partition>
   
   <!-- Low-criticality partition: 30% CPU -->
   <partition name="non_critical" criticality="low">
     <schedule>
       <window offset="70ms" duration="30ms" />
     </schedule>
   </partition>

Performance
-----------

Typical PikeOS performance (ARM Cortex-A9):

- Context switch: ~1 μs
- IPC (queuing port): ~2-5 μs
- Partition switch: ~3 μs
- Interrupt latency: <1 μs
- Memory footprint: 50-200 KB (kernel)

Certifications
--------------

- DO-178C DAL A (avionics)
- IEC 61508 SIL 4 (industrial)
- ISO 26262 ASIL D (automotive)
- IEC 62304 Class C (medical)
- EN 50128 SIL 4 (railway)
- Common Criteria EAL 5+

Resources
---------

- SYSGO Website: https://www.sysgo.com/
- PikeOS Documentation: https://www.sysgo.com/pikeos
- CODEO IDE: https://www.sysgo.com/codeo
- Support: support@sysgo.com

License
-------

PikeOS is commercial software requiring a license from SYSGO.
Evaluation licenses available for qualified projects.
