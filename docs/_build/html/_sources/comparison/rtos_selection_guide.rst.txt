====================
RTOS Selection Guide
====================

Introduction
============

Choosing the right RTOS is a critical decision that impacts your project's success, development timeline, and long-term maintainability. This guide provides a systematic approach to RTOS selection.

Selection Framework
===================

The RTOS Decision Tree
----------------------

.. code-block:: text

    Start
      │
      ├─→ Safety certification required?
      │   ├─→ Yes → embOS, ThreadX, SafeRTOS
      │   └─→ No ↓
      │
      ├─→ Commercial support essential?
      │   ├─→ Yes → embOS, ThreadX
      │   └─→ No ↓
      │
      ├─→ Azure IoT integration?
      │   ├─→ Yes → ThreadX (Azure RTOS)
      │   └─→ No ↓
      │
      ├─→ POSIX compatibility important?
      │   ├─→ Yes → NuttX, eCos
      │   └─→ No ↓
      │
      ├─→ IoT/Connectivity focus?
      │   ├─→ Yes → Zephyr
      │   └─→ No ↓
      │
      ├─→ Smallest footprint critical?
      │   ├─→ Yes → FreeRTOS, embOS
      │   └─→ No ↓
      │
      └─→ General embedded → FreeRTOS, ChibiOS, Zephyr

Key Decision Criteria
======================

1. Technical Requirements
-------------------------

Hardware Constraints
~~~~~~~~~~~~~~~~~~~~

**MCU Architecture:**

.. code-block:: text

    ┌─────────────────┬──────────────────────────────────┐
    │ Architecture    │ Recommended RTOS                 │
    ├─────────────────┼──────────────────────────────────┤
    │ ARM Cortex-M0/1 │ FreeRTOS, embOS, ChibiOS         │
    │ ARM Cortex-M3/4 │ Any RTOS (all well-supported)    │
    │ ARM Cortex-M7   │ Any RTOS + consider features     │
    │ ARM Cortex-A    │ Zephyr, NuttX, ThreadX           │
    │ RISC-V          │ Zephyr, FreeRTOS                 │
    │ 8-bit AVR       │ FreeRTOS, eCos                   │
    │ x86/x64         │ NuttX, Zephyr                    │
    └─────────────────┴──────────────────────────────────┘

**Memory Budget:**

.. code-block:: c

    // Ultra-constrained (< 16KB RAM, < 64KB Flash)
    // → FreeRTOS (minimal config) or embOS
    
    // Constrained (16-64KB RAM, 64-256KB Flash)
    // → FreeRTOS, ChibiOS, embOS, ThreadX
    
    // Moderate (64-512KB RAM, 256KB-2MB Flash)
    // → Any RTOS - choose based on features
    
    // Abundant (> 512KB RAM, > 2MB Flash)
    // → Consider feature-rich options: Zephyr, NuttX

**Real-Time Requirements:**

.. code-block:: text

    Hard Real-Time (< 10µs latency):
      → embOS, ChibiOS (optimized), FreeRTOS
    
    Firm Real-Time (10-100µs):
      → FreeRTOS, ThreadX, Zephyr
    
    Soft Real-Time (> 100µs):
      → Any RTOS acceptable

Connectivity Requirements
~~~~~~~~~~~~~~~~~~~~~~~~~~

**Bluetooth:**
- **Best:** Zephyr (native BLE stack)
- **Good:** ThreadX (Azure RTOS), third-party add-ons for others

**WiFi:**
- **TCP/IP integration:** All RTOSes support lwIP or native stacks
- **IoT protocols (MQTT, CoAP):** Zephyr, ThreadX, FreeRTOS

**Ethernet:**
- **Industrial protocols:** FreeRTOS, ChibiOS, embOS
- **Rich networking:** NuttX, Zephyr

**USB:**
- All major RTOSes have USB support (device and/or host)

2. Licensing and Cost
---------------------

License Comparison
~~~~~~~~~~~~~~~~~~

+------------+---------------+------------------+-------------------------+
| RTOS       | License       | Cost             | Commercial Restrictions |
+============+===============+==================+=========================+
| FreeRTOS   | MIT           | Free             | None                    |
+------------+---------------+------------------+-------------------------+
| Zephyr     | Apache 2.0    | Free             | None (permissive)       |
+------------+---------------+------------------+-------------------------+
| ChibiOS    | Apache 2.0/GPL| Free or paid     | GPL requires source     |
+------------+---------------+------------------+-------------------------+
| ThreadX    | Proprietary   | Free (MS Azure)  | Azure integration       |
+------------+---------------+------------------+-------------------------+
| embOS      | Commercial    | Paid per unit    | License fees apply      |
+------------+---------------+------------------+-------------------------+
| NuttX      | Apache 2.0    | Free             | None                    |
+------------+---------------+------------------+-------------------------+
| PX5        | Eclipse Public| Free/paid        | Complex licensing       |
+------------+---------------+------------------+-------------------------+
| eCos       | Modified GPL  | Free             | GPL restrictions        |
+------------+---------------+------------------+-------------------------+

**Cost Considerations:**

.. code-block:: text

    Open Source (No fees):
      ✓ FreeRTOS, Zephyr, NuttX
      ✓ No per-unit royalties
      ✓ Community support
      
    Commercial (Licensing fees):
      ✓ embOS: Per-project licensing
      ✓ ThreadX: Free with Azure
      ✓ Professional support included
      ✓ Certified versions available

3. Safety and Certification
----------------------------

Certification Matrix
~~~~~~~~~~~~~~~~~~~~

.. code-block:: text

    ┌────────────────────┬──────────────────────────────────────┐
    │ Industry           │ Recommended RTOS                      │
    ├────────────────────┼──────────────────────────────────────┤
    │ Automotive         │ embOS (ASIL D), ThreadX (ASIL D),    │
    │ (ISO 26262)        │ SafeRTOS                             │
    ├────────────────────┼──────────────────────────────────────┤
    │ Industrial         │ embOS (SIL 3), ThreadX (SIL 4),      │
    │ (IEC 61508)        │ SafeRTOS                             │
    ├────────────────────┼──────────────────────────────────────┤
    │ Aviation           │ embOS (DO-178C Level A),             │
    │ (DO-178B/C)        │ ThreadX (Level A)                    │
    ├────────────────────┼──────────────────────────────────────┤
    │ Medical            │ embOS (IEC 62304 Class C),           │
    │ (IEC 62304)        │ ThreadX (Class C)                    │
    ├────────────────────┼──────────────────────────────────────┤
    │ Non-certified      │ FreeRTOS, Zephyr, ChibiOS, NuttX     │
    └────────────────────┴──────────────────────────────────────┘

**Certification Costs:**

.. code-block:: text

    SafeRTOS (FreeRTOS derivative): $10k-50k
    embOS Safety: $20k-100k (depends on SIL level)
    ThreadX Safety: Contact Microsoft

4. Development Ecosystem
------------------------

Toolchain Support
~~~~~~~~~~~~~~~~~

**Best IDE Integration:**
- FreeRTOS: Excellent (all major IDEs)
- Zephyr: VS Code-centric, excellent
- embOS: Excellent (Segger tools)
- ThreadX: Good (Visual Studio integration)

**Debugging Tools:**

.. code-block:: text

    Runtime Trace Analysis:
      ✓ FreeRTOS → Tracealyzer, Segger SystemView
      ✓ Zephyr → Segger SystemView
      ✓ embOS → embOS/View (professional)
      ✓ ThreadX → TraceX
      
    Thread-Aware Debugging:
      ✓ All major RTOSes support GDB, IAR, Keil, Segger

Community and Support
~~~~~~~~~~~~~~~~~~~~~

**Community Size (GitHub stars as proxy):**

.. code-block:: text

    FreeRTOS: ★★★★★ (15k+ stars, huge community)
    Zephyr:   ★★★★★ (8k+ stars, growing rapidly)
    NuttX:    ★★★  (2k+ stars, active)
    ChibiOS:  ★★★  (Active forums)
    ThreadX:  ★★   (Microsoft backing)
    embOS:    ★★   (Professional support only)

**Documentation Quality:**

.. code-block:: text

    Excellent: FreeRTOS, embOS, ThreadX
    Very Good: Zephyr
    Good: ChibiOS, NuttX, eCos
    Fair: PX5

5. Long-Term Viability
----------------------

Project Maturity
~~~~~~~~~~~~~~~~

+------------+----------------+------------------+--------------------+
| RTOS       | First Release  | Maturity         | Future Outlook     |
+============+================+==================+====================+
| FreeRTOS   | 2003          | Very Mature      | Excellent (AWS)    |
+------------+----------------+------------------+--------------------+
| Zephyr     | 2016          | Mature           | Excellent (Linux F)|
+------------+----------------+------------------+--------------------+
| ChibiOS    | 2006          | Mature           | Stable             |
+------------+----------------+------------------+--------------------+
| ThreadX    | 1997          | Very Mature      | Good (Microsoft)   |
+------------+----------------+------------------+--------------------+
| embOS      | 1992          | Very Mature      | Excellent (SEGGER) |
+------------+----------------+------------------+--------------------+
| NuttX      | 2007          | Mature           | Growing            |
+------------+----------------+------------------+--------------------+
| PX5        | 2005          | Mature           | Niche (SYSGO)      |
+------------+----------------+------------------+--------------------+
| eCos       | 1998          | Legacy           | Declining          |
+------------+----------------+------------------+--------------------+

Vendor Lock-in Risk
~~~~~~~~~~~~~~~~~~~

**Low Risk (Open, portable):**
- FreeRTOS (MIT license, portable)
- Zephyr (Linux Foundation, broad support)
- ChibiOS (Apache 2.0 option, portable)
- NuttX (Apache 2.0, POSIX-like)

**Medium Risk:**
- ThreadX (Microsoft tie-in, Azure focus)
- embOS (Proprietary but stable vendor)

**Higher Risk:**
- PX5 (Single vendor, niche)
- eCos (Declining community)

Use Case Scenarios
==================

Scenario 1: Consumer IoT Device
--------------------------------

**Requirements:**
- WiFi or Bluetooth connectivity
- Cloud integration
- Cost-sensitive
- Moderate performance

**Recommendation:** **Zephyr** or **FreeRTOS**

**Rationale:**
- Both have excellent connectivity support
- Free, permissive licenses
- Large communities
- Good cloud ecosystem support

.. code-block:: c

    // Quick decision:
    if (bluetooth_required && modern_dev_tools_preferred) {
        choose_zephyr();
    } else if (widest_hw_support_needed) {
        choose_freertos();
    }

Scenario 2: Industrial Control System
--------------------------------------

**Requirements:**
- Hard real-time (< 10µs)
- Safety certification (SIL 2)
- Ethernet/Industrial protocols
- Long-term support

**Recommendation:** **embOS** or **ThreadX (certified)**

**Rationale:**
- Certified options available
- Excellent real-time performance
- Professional support
- Proven in industrial applications

Scenario 3: Automotive ECU
---------------------------

**Requirements:**
- ISO 26262 ASIL D certification
- CAN/LIN/FlexRay support
- Deterministic behavior
- Commercial support essential

**Recommendation:** **embOS** or **ThreadX**

**Rationale:**
- Both offer ASIL D certified versions
- Automotive-grade support
- Proven track record
- Extensive tooling

Scenario 4: Battery-Powered Sensor Node
----------------------------------------

**Requirements:**
- Ultra-low power
- Small footprint (< 32KB RAM)
- Simple application
- Low cost

**Recommendation:** **FreeRTOS** (minimal config)

**Rationale:**
- Smallest footprint possible
- Excellent tickless idle support
- Simple, proven
- No licensing costs

Scenario 5: Linux-Like Embedded System
---------------------------------------

**Requirements:**
- POSIX APIs
- File system (FAT, Flash FS)
- Rich networking
- Team familiar with Linux

**Recommendation:** **NuttX**

**Rationale:**
- Most POSIX-compliant RTOS
- Rich feature set
- VFS support
- Familiar development model

Migration Considerations
========================

Porting Between RTOSes
-----------------------

**Easiest Migrations:**

.. code-block:: text

    FreeRTOS ↔ ChibiOS    (Similar APIs, wrappers available)
    NuttX ↔ eCos          (Both POSIX-like)
    Zephyr ← FreeRTOS     (Zephyr has FreeRTOS compatibility)

**Hardest Migrations:**

.. code-block:: text

    Any → PX5             (Unique architecture)
    Any → embOS           (Proprietary APIs)
    POSIX → Non-POSIX     (Different programming model)

**Abstraction Layer Approach:**

.. code-block:: c

    // Create RTOS abstraction layer
    typedef void (*task_func_t)(void *);
    
    // Generic API
    void os_task_create(task_func_t func, ...);
    void os_mutex_lock(os_mutex_t *mutex);
    void os_queue_send(os_queue_t *queue, void *item);
    
    // Implement for each RTOS
    #ifdef USE_FREERTOS
        #include "FreeRTOS.h"
    #elif USE_ZEPHYR
        #include <zephyr.h>
    #endif

Evaluation Checklist
====================

Before Committing
-----------------

.. code-block:: text

    ☐ Hardware platform supported?
    ☐ Memory footprint acceptable?
    ☐ Real-time requirements met?
    ☐ Connectivity needs covered?
    ☐ License compatible with product?
    ☐ Certification available if needed?
    ☐ Team has expertise or training available?
    ☐ Development tools adequate?
    ☐ Community/support sufficient?
    ☐ Example code available for similar application?
    ☐ Long-term viability assured?
    ☐ Migration path if needed?

Proof of Concept
----------------

.. code-block:: c

    // Minimum viable RTOS evaluation
    void evaluate_rtos(void) {
        // 1. Get it building on your hardware
        hardware_init();
        rtos_init();
        
        // 2. Create simple test tasks
        create_task(fast_task, 1ms_period);
        create_task(slow_task, 100ms_period);
        
        // 3. Measure performance
        measure_context_switch_time();
        measure_interrupt_latency();
        measure_memory_usage();
        
        // 4. Test connectivity if needed
        test_network_stack();
        
        // 5. Evaluate debugging experience
        test_debugging_tools();
        
        // 6. Review documentation quality
        // 7. Check community responsiveness
    }

Quick Reference Decision Matrix
================================

+-------------------+----------+---------+---------+---------+--------+--------+-----+------+
| Criteria          | FreeRTOS | Zephyr  | ChibiOS | ThreadX | embOS  | NuttX  | PX5 | eCos |
+===================+==========+=========+=========+=========+========+========+=====+======+
| **Ease of Use**   | ★★★★★    | ★★★★    | ★★★★    | ★★★★    | ★★★★★  | ★★★    | ★★  | ★★   |
+-------------------+----------+---------+---------+---------+--------+--------+-----+------+
| **Documentation** | ★★★★★    | ★★★★    | ★★★     | ★★★★    | ★★★★★  | ★★★    | ★★  | ★★★  |
+-------------------+----------+---------+---------+---------+--------+--------+-----+------+
| **Community**     | ★★★★★    | ★★★★★   | ★★★     | ★★★     | ★★     | ★★★    | ★   | ★    |
+-------------------+----------+---------+---------+---------+--------+--------+-----+------+
| **Performance**   | ★★★★     | ★★★     | ★★★★★   | ★★★★    | ★★★★★  | ★★★    | ★★★ | ★★★  |
+-------------------+----------+---------+---------+---------+--------+--------+-----+------+
| **Footprint**     | ★★★★★    | ★★★     | ★★★★    | ★★★★    | ★★★★★  | ★★     | ★★  | ★★★  |
+-------------------+----------+---------+---------+---------+--------+--------+-----+------+
| **Connectivity**  | ★★★      | ★★★★★   | ★★★     | ★★★★    | ★★★    | ★★★★   | ★★  | ★★   |
+-------------------+----------+---------+---------+---------+--------+--------+-----+------+
| **Safety Cert**   | ★★★      | ★       | ☐       | ★★★★★   | ★★★★★  | ☐      | ★   | ☐    |
+-------------------+----------+---------+---------+---------+--------+--------+-----+------+

Summary Recommendation
======================

**For Most Projects:** Start with **FreeRTOS**
- Widest support, largest community, MIT license
- Easy to learn, well-documented
- Can always migrate later if needed

**For IoT/Modern Workflow:** Choose **Zephyr**
- Best connectivity support
- Modern development tools
- Growing ecosystem

**For Safety-Critical:** Choose **embOS** or **ThreadX**
- Certified versions available
- Professional support
- Proven in regulated industries

**For POSIX Compatibility:** Choose **NuttX**
- Most Linux-like RTOS
- Rich feature set
- Good for complex applications

See Also
========

- :doc:`rtos_comparison_table` - Detailed feature comparison
- :doc:`licensing_and_certification` - License and certification details
- :doc:`../days/day19` - RTOS Comparison and Selection day
- Individual RTOS day lessons (Days 11-18)

Further Reading
===============

- RTOS vendor selection guides
- Industry-specific RTOS requirements
- Safety certification standards documentation
- Community surveys and user experiences
