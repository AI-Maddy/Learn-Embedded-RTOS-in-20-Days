Learn Embedded RTOS in 20 Days
==============================


A fast-track, hands-on learning path to build real-time embedded systems with modern RTOSes.

This repository is organized as a mini course: daily topics, practical examples, concise references, and exercises. It is designed for engineers who want to move from RTOS fundamentals to real project patterns in 20 focused days.

What You Will Learn
-------------------


- RTOS fundamentals: tasks, scheduling, context switching, ISRs
- Synchronization: semaphores, mutexes, queues, events
- Memory, timing, latency, jitter, and determinism
- How to evaluate and choose an RTOS for product constraints
- Practical architecture patterns for embedded systems

RTOS Coverage
-------------


- FreeRTOS
- Zephyr
- ChibiOS
- ThreadX (Azure RTOS ThreadX)
- embOS
- NuttX
- PX5 RTOS
- eCos

Planned 20-Day Roadmap
----------------------


Core Foundations (Days 1–10)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~


1. What is an RTOS?
2. Tasks and threads
3. Scheduling and determinism
4. Context switching
5. Semaphores and mutexes
6. Queues and event groups
7. Interrupt handling with RTOS
8. Memory and stack management
9. Latency, jitter, and timing analysis
10. Build systems, ports, and board bring-up

RTOS Deep Dives (Days 11–18)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~


11. FreeRTOS
12. Zephyr
13. ChibiOS
14. ThreadX
15. embOS
16. NuttX
17. PX5
18. eCos

Wrap-up (Days 19–20)
~~~~~~~~~~~~~~~~~~~~


19. RTOS comparison and selection strategy
20. Final project (same architecture on two RTOSes)

Suggested Repository Layout
---------------------------


.. code-block:: text

   Learn-Embedded-RTOS-in-20-Days/
   ├── README.rst
   ├── docs/
   │   ├── overview/
   │   ├── patterns/
   │   ├── comparison/
   │   └── cheatsheets/
      ├── Day01_Intro_to_RTOS/
      ├── ...
      ├── Day20_Final_Project/
   └── src/
       ├── common/
       ├── freertos/
       ├── zephyr/
       ├── chibios/
       ├── threadx/
       ├── embos/
       ├── nuttx/
       ├── px5/
       └── ecos/


Per-Day Template (Recommended)
------------------------------


Each day folder can follow a consistent structure:

.. code-block:: text

   DayXX_Topic/
   ├── lesson.rst
   ├── exercises.rst
   ├── solutions.rst
   └── README.rst


Final Project Concept
---------------------


**Smart Sensor Hub with Real-Time Telemetry**

- Periodic sampling task
- Queue-based processing pipeline
- Interrupt-driven communication (UART/SPI)
- Logging, watchdog, and fault handling
- Implemented in two RTOSes for API/behavior comparison

Who This Is For
---------------


- Embedded beginners with C fundamentals
- Firmware engineers transitioning from bare metal to RTOS
- Practitioners who want practical patterns, not just theory

Contributing
------------


Contributions are welcome. If you want to add examples, improve explanations, or fix issues, open a PR with clear scope and testing notes.

License
-------


This project is licensed under the MIT License. See ``LICENSE``.
