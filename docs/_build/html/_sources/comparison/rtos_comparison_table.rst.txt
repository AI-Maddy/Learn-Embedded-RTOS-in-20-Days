========================
RTOS Comparison Table
========================

Introduction
============

This comprehensive comparison helps you understand the differences between major RTOS options covered in this course. Use this table along with the selection guide to choose the right RTOS for your project.

Feature Comparison Matrix
==========================

General Characteristics
-----------------------

+------------------+------------+-----------+-----------+------------+----------+-----------+----------+----------+
| Feature          | FreeRTOS   | Zephyr    | ChibiOS   | ThreadX    | embOS    | NuttX     | PX5      | eCos     |
+==================+============+===========+===========+============+==========+===========+==========+==========+
| **License**      | MIT        | Apache2.0 | Apache2.0,| Proprietary| Comm.    | Apache2.0 | Eclipse  | Modified |
|                  |            |           | GPL       | (MS owned) |          |           | Public   | GPL      |
+------------------+------------+-----------+-----------+------------+----------+-----------+----------+----------+
| **Maturity**     | Very High  | High      | High      | Very High  | Very High| High      | Medium   | High     |
+------------------+------------+-----------+-----------+------------+----------+-----------+----------+----------+
| **Community**    | Excellent  | Excellent | Good      | Good       | Commercial| Good     | Small    | Small    |
+------------------+------------+-----------+-----------+------------+----------+-----------+----------+----------+
| **Documentation**| Excellent  | Very Good | Good      | Very Good  | Excellent| Good      | Fair     | Good     |
+------------------+------------+-----------+-----------+------------+----------+-----------+----------+----------+
| **Support**      | Community+ | Community | Community | Microsoft+ | SEGGER   | Community | Community| Community|
|                  | Commercial |           | +Comm.    | Commercial | Support  |           |          |          |
+------------------+------------+-----------+-----------+------------+----------+-----------+----------+----------+

Platform Support
----------------

+------------------+------------+-----------+-----------+------------+----------+-----------+----------+----------+
| Architecture     | FreeRTOS   | Zephyr    | ChibiOS   | ThreadX    | embOS    | NuttX     | PX5      | eCos     |
+==================+============+===========+===========+============+==========+===========+==========+==========+
| **ARM Cortex-M** | ✓✓✓        | ✓✓✓       | ✓✓✓       | ✓✓✓        | ✓✓✓      | ✓✓        | ✓✓       | ✓✓       |
+------------------+------------+-----------+-----------+------------+----------+-----------+----------+----------+
| **ARM Cortex-A** | ✓          | ✓✓✓       | Limited   | ✓✓         | ✓✓       | ✓✓✓       | Limited  | ✓✓       |
+------------------+------------+-----------+-----------+------------+----------+-----------+----------+----------+
| **RISC-V**       | ✓✓         | ✓✓✓       | ✓         | ✓          | ✓        | ✓✓        | Limited  | Limited  |
+------------------+------------+-----------+-----------+------------+----------+-----------+----------+----------+
| **x86**          | Limited    | ✓✓        | Limited   | ✓          | ✓        | ✓✓        | ✓        | ✓✓       |
+------------------+------------+-----------+-----------+------------+----------+-----------+----------+----------+
| **AVR/8-bit**    | ✓          | Limited   | ✓         | Limited    | ✓        | Limited   | No       | ✓        |
+------------------+------------+-----------+-----------+------------+----------+-----------+----------+----------+

✓✓✓ = Excellent support, ✓✓ = Good support, ✓ = Basic support

Kernel Features
---------------

+----------------------+------------+-----------+-----------+------------+----------+-----------+----------+----------+
| Feature              | FreeRTOS   | Zephyr    | ChibiOS   | ThreadX    | embOS    | NuttX     | PX5      | eCos     |
+======================+============+===========+===========+============+==========+===========+==========+==========+
| **Preemptive**       | Yes        | Yes       | Yes       | Yes        | Yes      | Yes       | Yes      | Yes      |
+----------------------+------------+-----------+-----------+------------+----------+-----------+----------+----------+
| **Priority Levels**  | Unlimited  | 32        | 256       | 32         | 255      | 255       | 32       | 32       |
+----------------------+------------+-----------+-----------+------------+----------+-----------+----------+----------+
| **Round Robin**      | Yes        | Yes       | Yes       | Yes        | Yes      | Yes       | Yes      | Yes      |
+----------------------+------------+-----------+-----------+------------+----------+-----------+----------+----------+
| **Priority**         | Yes        | Yes       | Yes       | Yes        | Yes      | Yes       | Yes      | Yes      |
| **Inheritance**      |            |           |           |            |          |           |          |          |
+----------------------+------------+-----------+-----------+------------+----------+-----------+----------+----------+
| **Tick Rate**        | Config.    | Config.   | Config.   | Config.    | Config.  | Config.   | Config.  | Config.  |
+----------------------+------------+-----------+-----------+------------+----------+-----------+----------+----------+
| **Tickless Idle**    | Yes        | Yes       | Yes       | Yes        | Yes      | Yes       | Limited  | Yes      |
+----------------------+------------+-----------+-----------+------------+----------+-----------+----------+----------+

Synchronization Primitives
---------------------------

+----------------------+------------+-----------+-----------+------------+----------+-----------+----------+----------+
| Primitive            | FreeRTOS   | Zephyr    | ChibiOS   | ThreadX    | embOS    | NuttX     | PX5      | eCos     |
+======================+============+===========+===========+============+==========+===========+==========+==========+
| **Semaphores**       | Binary,    | Binary,   | Binary,   | Counting   | Binary,  | Binary,   | Counting | Counting |
|                      | Counting   | Counting  | Counting  |            | Counting | Counting  |          |          |
+----------------------+------------+-----------+-----------+------------+----------+-----------+----------+----------+
| **Mutexes**          | Yes        | Yes       | Yes       | Yes        | Yes      | Yes       | Yes      | Yes      |
+----------------------+------------+-----------+-----------+------------+----------+-----------+----------+----------+
| **Queues**           | Yes        | Yes       | Yes       | Yes        | Yes      | Yes       | Yes      | Yes      |
+----------------------+------------+-----------+-----------+------------+----------+-----------+----------+----------+
| **Event Flags**      | Yes        | Yes       | Yes       | Yes        | Yes      | Yes       | Yes      | Yes      |
+----------------------+------------+-----------+-----------+------------+----------+-----------+----------+----------+
| **Message Mailboxes**| No         | Yes       | Yes       | Yes        | Yes      | Yes       | Yes      | Yes      |
+----------------------+------------+-----------+-----------+------------+----------+-----------+----------+----------+
| **Condition**        | No         | Yes       | Yes       | No         | Limited  | Yes       | No       | Yes      |
| **Variables**        |            |           |           |            |          |           |          |          |
+----------------------+------------+-----------+-----------+------------+----------+-----------+----------+----------+

Memory Management
-----------------

+----------------------+------------+-----------+-----------+------------+----------+-----------+----------+----------+
| Feature              | FreeRTOS   | Zephyr    | ChibiOS   | ThreadX    | embOS    | NuttX     | PX5      | eCos     |
+======================+============+===========+===========+============+==========+===========+==========+==========+
| **Static**           | Yes        | Yes       | Yes       | Yes        | Yes      | Yes       | Yes      | Yes      |
| **Allocation**       |            |           |           |            |          |           |          |          |
+----------------------+------------+-----------+-----------+------------+----------+-----------+----------+----------+
| **Dynamic**          | Yes (5     | Yes       | Yes       | Yes        | Yes      | Yes       | Yes      | Yes      |
| **Allocation**       | schemes)   |           |           |            |          |           |          |          |
+----------------------+------------+-----------+-----------+------------+----------+-----------+----------+----------+
| **Memory Pools**     | No         | Yes       | Yes       | Yes        | Yes      | Yes       | Yes      | Yes      |
+----------------------+------------+-----------+-----------+------------+----------+-----------+----------+----------+
| **MPU Support**      | Yes        | Yes       | Yes       | Yes        | Yes      | Yes       | Limited  | Limited  |
+----------------------+------------+-----------+-----------+------------+----------+-----------+----------+----------+

Footprint (Typical)
-------------------

+----------------------+------------+-----------+-----------+------------+----------+-----------+----------+----------+
| Metric               | FreeRTOS   | Zephyr    | ChibiOS   | ThreadX    | embOS    | NuttX     | PX5      | eCos     |
+======================+============+===========+===========+============+==========+===========+==========+==========+
| **ROM (KB)**         | 4-10       | 8-15      | 6-12      | 5-15       | 4-10     | 30-100    | 10-30    | 20-50    |
+----------------------+------------+-----------+-----------+------------+----------+-----------+----------+----------+
| **RAM per task (B)** | 100-200    | 200-400   | 150-300   | 150-300    | 100-200  | 300-500   | 200-400  | 400-600  |
+----------------------+------------+-----------+-----------+------------+----------+-----------+----------+----------+
| **Min RAM (KB)**     | 2-4        | 8-16      | 4-8       | 4-10       | 2-4      | 16-32     | 8-16     | 16-32    |
+----------------------+------------+-----------+-----------+------------+----------+-----------+----------+----------+

Networking
----------

+----------------------+------------+-----------+-----------+------------+----------+-----------+----------+----------+
| Feature              | FreeRTOS   | Zephyr    | ChibiOS   | ThreadX    | embOS    | NuttX     | PX5      | eCos     |
+======================+============+===========+===========+============+==========+===========+==========+==========+
| **TCP/IP Stack**     | FreeRTOS+  | Native    | lwIP      | NetX Duo   | embOS/IP | Native    | Limited  | Native   |
|                      | TCP, lwIP  | +lwIP     | plugin    |            |          | (NuttX)   |          |          |
+----------------------+------------+-----------+-----------+------------+----------+-----------+----------+----------+
| **IPv6**             | Yes (lwIP) | Yes       | Yes       | Yes        | Yes      | Yes       | No       | Yes      |
+----------------------+------------+-----------+-----------+------------+----------+-----------+----------+----------+
| **TLS/SSL**          | mbedTLS    | mbedTLS   | mbedTLS   | NetX       | embOS/IP | mbedTLS   | No       | OpenSSL  |
|                      |            |           |           | Secure     | TLS      |           |          |          |
+----------------------+------------+-----------+-----------+------------+----------+-----------+----------+----------+
| **Bluetooth**        | Third-     | Native    | ChibiOS   | Azure      | Limited  | Third-    | No       | No       |
|                      | party      | (BLE)     | extension | RTOS       |          | party     |          |          |
+----------------------+------------+-----------+-----------+------------+----------+-----------+----------+----------+

File Systems
------------

+----------------------+------------+-----------+-----------+------------+----------+-----------+----------+----------+
| Feature              | FreeRTOS   | Zephyr    | ChibiOS   | ThreadX    | embOS    | NuttX     | PX5      | eCos     |
+======================+============+===========+===========+============+==========+===========+==========+==========+
| **FAT**              | FreeRTOS+  | Yes       | FatFS     | FileX      | emFile   | Yes       | No       | Yes      |
|                      | FAT        |           |           |            |          |           |          |          |
+----------------------+------------+-----------+-----------+------------+----------+-----------+----------+----------+
| **Flash FS**         | Third-     | littlefs, | Yes       | LevelX     | emFile   | NXFFS,    | No       | JFFS2    |
|                      | party      | NFFS      |           | (wear-lev.)|          | littlefs  |          |          |
+----------------------+------------+-----------+-----------+------------+----------+-----------+----------+----------+
| **VFS**              | No         | Yes       | Limited   | No         | emFile   | Yes       | No       | Yes      |
+----------------------+------------+-----------+-----------+------------+----------+-----------+----------+----------+

Safety Certification
--------------------

+----------------------+------------+-----------+-----------+------------+----------+-----------+----------+----------+
| Certification        | FreeRTOS   | Zephyr    | ChibiOS   | ThreadX    | embOS    | NuttX     | PX5      | eCos     |
+======================+============+===========+===========+============+==========+===========+==========+==========+
| **IEC 61508**        | Yes        | In prog.  | No        | Yes        | Yes      | No        | No       | No       |
|                      | (SafeRTOS) |           |           | (SIL 4)    | (SIL 3)  |           |          |          |
+----------------------+------------+-----------+-----------+------------+----------+-----------+----------+----------+
| **ISO 26262**        | Yes        | No        | No        | Yes        | Yes      | No        | No       | No       |
|                      | (Auto)     |           |           | (ASIL D)   | (ASIL D) |           |          |          |
+----------------------+------------+-----------+-----------+------------+----------+-----------+----------+----------+
| **DO-178B/C**        | Yes        | No        | No        | Yes        | Yes      | No        | No       | No       |
|                      | (Aviation) |           |           | (Level A)  | (Level A)|           |          |          |
+----------------------+------------+-----------+-----------+------------+----------+-----------+----------+----------+
| **IEC 62304**        | Yes        | No        | No        | Yes        | Yes      | No        | No       | No       |
|                      | (Medical)  |           |           | (Class C)  | (Class C)|           |          |          |
+----------------------+------------+-----------+-----------+------------+----------+-----------+----------+----------+

Development Tools
-----------------

+----------------------+------------+-----------+-----------+------------+----------+-----------+----------+----------+
| Tool                 | FreeRTOS   | Zephyr    | ChibiOS   | ThreadX    | embOS    | NuttX     | PX5      | eCos     |
+======================+============+===========+===========+============+==========+===========+==========+==========+
| **IDE Support**      | Most       | VS Code,  | Most      | VS, VS     | Most     | Most      | Eclipse  | Eclipse, |
|                      | popular    | Eclipse   | popular   | Code       | popular  | popular   |          | limited  |
+----------------------+------------+-----------+-----------+------------+----------+-----------+----------+----------+
| **Trace Tools**      | Tracealyzer| Systemview| ChibiStud.|TraceX,     | SystemVw.| Limited   | Limited  | Limited  |
|                      | SystemView |           | Segger    | Systemview | embOS/Vw.|           |          |          |
+----------------------+------------+-----------+-----------+------------+----------+-----------+----------+----------+
| **Stack Analysis**   | Yes        | Yes       | Yes       | Yes        | Yes      | Yes       | Limited  | Limited  |
+----------------------+------------+-----------+-----------+------------+----------+-----------+----------+----------+
| **RTOS Awareness**   | GDB, IAR,  | GDB, IAR, | GDB, IAR, | GDB, IAR,  | All      | GDB, IAR, | GDB      | GDB      |
| **Debug**            | Keil, etc. | Keil, etc.| Keil, etc.| Keil, etc. | major    | Keil, etc.|          |          |
+----------------------+------------+-----------+-----------+------------+----------+-----------+----------+----------+

Performance (Typical Context Switch Time on ARM Cortex-M4 @ 100MHz)
--------------------------------------------------------------------

+-----------------+------------+-----------+-----------+------------+----------+-----------+----------+----------+
| Metric          | FreeRTOS   | Zephyr    | ChibiOS   | ThreadX    | embOS    | NuttX     | PX5      | eCos     |
+=================+============+===========+===========+============+==========+===========+==========+==========+
| **Context**     | ~1-2 µs    | ~2-3 µs   | ~1-2 µs   | ~1-2 µs    | ~0.8 µs  | ~3-5 µs   | ~2-3 µs  | ~2-4 µs  |
| **Switch**      |            |           |           |            |          |           |          |          |
+-----------------+------------+-----------+-----------+------------+----------+-----------+----------+----------+
| **Interrupt**   | <1 µs      | ~1 µs     | <1 µs     | <1 µs      | <1 µs    | ~1-2 µs   | ~1 µs    | ~1-2 µs  |
| **Latency**     |            |           |           |            |          |           |          |          |
+-----------------+------------+-----------+-----------+------------+----------+-----------+----------+----------+

Use Case Recommendations
=========================

Best For...
-----------

**FreeRTOS:**
- General embedded applications
- Cost-sensitive projects
- Quick prototyping
- Wide hardware support needed
- Large community/resources important

**Zephyr:**
- IoT devices
- Modern development workflow
- Bluetooth/connectivity focus
- Multiple vendor support
- Future-proof projects

**ChibiOS:**
- Real-time performance critical
- Robotics and control systems
- HAL abstraction needed
- GPL-compatible projects

**ThreadX:**
- Microsoft Azure IoT integration
- Enterprise support required
- Safety-critical (certified version)
- Windows development team

**embOS:**
- Commercial support essential
- Safety certification required
- Smallest footprint needed
- Professional tools important

**NuttX:**
- POSIX compatibility needed
- Complex applications
- Experienced Linux developers
- Rich feature set required

**PX5:**
- PikeOS integration
- Hypervisor environment
- Mixed-criticality systems
- Aerospace/defense

**eCos:**
- Legacy project maintenance
- Specific hardware with eCos HAL
- Research/academic use
- POSIX-like API needed

See Also
========

- :doc:`rtos_selection_guide` - Detailed selection criteria
- :doc:`licensing_and_certification` - License and certification details
- :doc:`../days/day11` through :doc:`../days/day18` - Individual RTOS deep-dives
- :doc:`../days/day19` - RTOS Comparison and Selection

Further Reading
===============

- Individual RTOS vendor documentation
- Performance benchmarking studies
- Safety certification guides
- Community forums and wikis
