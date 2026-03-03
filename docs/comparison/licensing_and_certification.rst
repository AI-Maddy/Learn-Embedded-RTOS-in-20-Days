================================
Licensing and Certification
================================

Introduction
============

Understanding RTOS licensing and certification is crucial for commercial projects, especially in safety-critical domains. This guide explains license types, commercial implications, and certification requirements.

License Types
=============

Open Source Licenses
--------------------

MIT License (FreeRTOS)
~~~~~~~~~~~~~~~~~~~~~~

**Key Points:**
- Most permissive
- Commercial use allowed
- No requirement to share source
- Can modify and redistribute
- No warranty

.. code-block:: text

    Permission is hereby granted, free of charge, to any person
    obtaining a copy of this software...
    
    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND

**Commercial Impact:**
- ✓ Use in proprietary products
- ✓ No per-unit royalties
- ✓ Can modify source without disclosure
- ✓ Can statically link
- ✗ No warranty or liability protection

**Best For:**
- Commercial IoT devices
- Consumer electronics
- Cost-sensitive products
- Rapid prototyping

Apache 2.0 (Zephyr, NuttX, ChibiOS option)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Key Points:**
- Permissive like MIT
- Explicit patent grant
- Requires attribution
- Can combine with proprietary code

.. code-block:: text

    Licensed under the Apache License, Version 2.0
    
    Grant of Patent License: ... a perpetual, worldwide, non-exclusive,
    no-charge, royalty-free, irrevocable patent license...

**Commercial Impact:**
- ✓ Use in proprietary products
- ✓ Patent protection
- ✓ Modification allowed
- ✓ Commercial redistribution OK
- ⚠ Must include license notice
- ⚠ Must state changes made

**Best For:**
- Projects concerned about patents
- Corporate environments
- When contributing back is optional

GPL (GNU Public License - ChibiOS option, eCos modified)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Key Points:**
- "Copyleft" license
- Distributed modifications must share source
- "Viral" - extends to linked code
- Modified GPL (eCos) less restrictive

.. code-block:: text

    If you distribute copies of such a program... you must give
    the recipients all the rights that you have.

**Commercial Impact:**
- ⚠ Proprietary use requires careful isolation
- ⚠ Linking may require source disclosure
- ⚠ Distribution triggers obligations
- ✓ Can use internally without disclosure
- ✓ ChibiOS offers commercial license option

**Strategies:**
1. **Dual licensing**: ChibiOS offers Apache 2.0 paid option
2. **Static exception**: eCos modified GPL allows linking
3. **Dynamic linking**: May avoid GPL (consult lawyer)

**Best For:**
- Open source products
- Internal/research use
- When comfortable sharing modifications

Proprietary Licenses
--------------------

Commercial RTOS (embOS, ThreadX)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**embOS Licensing:**

.. code-block:: text

    Licensing Models:
    - Per-project license
    - Per-developer license
    - Volume discounts available
    - Includes support and updates

**Typical Costs:**
- Single project: $2,000 - $5,000
- Per developer: $3,000 - $8,000/year
- Volume: Negotiable
- Safety-certified: +$20,000 - $100,000

**ThreadX (Azure RTOS) Licensing:**

.. code-block:: text

    - Free with Azure IoT services
    - Commercial license otherwise
    - Now Microsoft-owned (2019)
    - Azure tie-in increasingly expected

**Commercial Impact:**
- ✓ Professional support included
- ✓ No GPL concerns
- ✓ Certified versions available
- ✗ Per-unit or per-project fees
- ✗ Vendor lock-in risk

License Comparison Table
========================

+---------------+------------+----------------+------------------+-------------------+
| License       | RTOSes     | Commercial Use | Source Required  | Patent Protection |
+===============+============+================+==================+===================+
| **MIT**       | FreeRTOS   | ✓ Unrestricted | ✗ No             | ⚠ Not explicit    |
+---------------+------------+----------------+------------------+-------------------+
| **Apache 2.0**| Zephyr,    | ✓ Unrestricted | ⚠ Attribution    | ✓ Yes             |
|               | NuttX      |                | required         |                   |
+---------------+------------+----------------+------------------+-------------------+
| **GPL v2/v3** | ChibiOS    | ⚠ Conditional  | ✓ If distributed | ⚠ Implicit        |
|               | (option),  |                |                  |                   |
|               | eCos (mod) |                |                  |                   |
+---------------+------------+----------------+------------------+-------------------+
| **Commercial**| embOS,     | ✓ Licensed     | ✗ No             | ⚠ Vendor-specific |
|               | ThreadX    |                |                  |                   |
+---------------+------------+----------------+------------------+-------------------+

Safety Certification
====================

Why Certification Matters
--------------------------

**Safety-critical systems** require formal verification that the RTOS meets specific safety standards:

.. code-block:: text

    Automotive → ISO 26262 (ASIL A/B/C/D)
    Industrial → IEC 61508 (SIL 1/2/3/4)
    Aviation → DO-178B/C (Level A/B/C/D/E)
    Medical → IEC 62304 (Class A/B/C)
    Railway → EN 50128 (SIL 0-4)

**Certification Process:**
1. Define safety requirements
2. Implement with traceability
3. Rigorous testing and verification
4. Independent assessment
5. Certificate issued

Certified RTOS Options
-----------------------

SafeRTOS (FreeRTOS Derivative)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Certifications:**
- IEC 61508 SIL 3
- IEC 62304 Class C (Medical)
- Available for multiple standards

**Features:**
- API similar to FreeRTOS
- Formally verified
- Full documentation package
- Source code with certificates

**Cost:**
- Typically $10,000 - $50,000
- Includes certification evidence
- Per-project licensing

.. code-block:: c

    // SafeRTOS API (similar to FreeRTOS)
    xTaskCreate(vTaskFunction, "Task", STACK_SIZE, NULL, PRIORITY, NULL);
    xQueueSend(xQueue, &data, TIMEOUT);

embOS (SEGGER)
~~~~~~~~~~~~~~

**Certifications:**
- IEC 61508 SIL 3
- ISO 26262 ASIL D
- DO-178B/C Level A
- IEC 62304 Class C
- EN 50128 SIL 4

**Features:**
- Proven track record (30+ years)
- Excellent tooling (embOS/View)
- Very small footprint
- Comprehensive support

**Cost:**
- Base license: $2,000 - $5,000
- Safety package: +$20,000 - $100,000
- Depends on SIL/ASIL level

.. code-block:: c

    // embOS API
    OS_CreateTask(&TCB, "Task", Task, PRIORITY, Stack);
    OS_Q_Put(&Queue, &data, sizeof(data));

ThreadX (Microsoft Azure RTOS)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Certifications:**
- IEC 61508 SIL 4
- IEC 62304 Class C
- ISO 26262 ASIL D
- DO-178B Level A

**Features:**
- Enterprise backing (Microsoft)
- Azure integration
- Complete ecosystem (NetX, FileX, etc.)
- Professional support

**Cost:**
- Free with Azure IoT
- Otherwise: Contact Microsoft
- Safety certification: $$$ (inquire)

.. code-block:: c

    // ThreadX API
    tx_thread_create(&thread, "Task", task_func, input, 
                     stack, STACK_SIZE, PRIORITY, PRIORITY, 
                     TX_NO_TIME_SLICE, TX_AUTO_START);
    tx_queue_send(&queue, &data, TX_WAIT_FOREVER);

Certification Levels Explained
================================

IEC 61508 (Industrial)
-----------------------

**Safety Integrity Levels (SIL):**

.. code-block:: text

    SIL 4: 10⁻⁵ to 10⁻⁴ probability of failure per hour
           (Most critical - nuclear, rail signaling)
           
    SIL 3: 10⁻⁴ to 10⁻³ probability of failure per hour
           (High integrity - industrial safety systems)
           
    SIL 2: 10⁻³ to 10⁻² probability of failure per hour
           (Medium integrity - protective equipment)
           
    SIL 1: 10⁻² to 10⁻¹ probability of failure per hour
           (Low integrity - low demand systems)

**RTOS Requirements:**
- Systematic fault avoidance
- Safety manual
- Proven-in-use argument
- Or formal certification

ISO 26262 (Automotive)
-----------------------

**Automotive Safety Integrity Levels (ASIL):**

.. code-block:: text

    ASIL D: Highest integrity (airbags, brake-by-wire)
    ASIL C: High integrity (ABS, power steering)
    ASIL B: Medium integrity (rear lights)
    ASIL A: Low integrity (convenience features)
    QM: Quality managed (no safety impact)

**RTOS Requirements:**
- Freedom from interference
- Timing analysis
- Memory protection
- Resource monitoring

DO-178B/C (Aviation)
--------------------

**Software Levels:**

.. code-block:: text

    Level A: Catastrophic failure (flight-critical)
    Level B: Hazardous failure (significant impact)
    Level C: Major failure (substantial reduction)
    Level D: Minor failure (slight reduction)
    Level E: No effect (no safety impact)

**RTOS Requirements:**
- Full traceability
- Extensive testing (MC/DC coverage)
- Configuration management
- Verification activities

Certification Costs
===================

Typical Certification Expenses
-------------------------------

.. code-block:: text

    ┌──────────────────────┬─────────────────────────┐
    │ Component            │ Estimated Cost Range    │
    ├──────────────────────┼─────────────────────────┤
    │ Certified RTOS       │ $10,000 - $100,000      │
    │ Safety Consultant    │ $200 - $500 / hour      │
    │ Certification Body   │ $50,000 - $500,000      │
    │ Testing/Verification │ $100,000 - $1,000,000   │
    │ Documentation        │ $50,000 - $200,000      │
    │ Training             │ $5,000 - $50,000        │
    ├──────────────────────┼─────────────────────────┤
    │ **Total Project**    │ **$200k - $2M+**        │
    └──────────────────────┴─────────────────────────┘

**Cost Factors:**
- Safety level (SIL/ASIL/Level)
- System complexity
- Domain (aviation > automotive > industrial)
- Team experience
- Prior certification evidence

Alternative: Qualification
--------------------------

For less critical systems:

.. code-block:: text

    Qualification (vs Certification):
    - Vendor provides evidence package
    - Customer verifies for their application
    - Lower cost: $10k - $100k
    - Less rigorous but often sufficient

**Qualified RTOSes:**
- Most RTOSes can be qualified
- Vendor documentation + testing
- May satisfy some standards

Pre-Certified vs DIY Certification
===================================

Pre-Certified RTOS
------------------

**Advantages:**
- ✓ Certification evidence included
- ✓ Proven compliance
- ✓ Lower project risk
- ✓ Faster time to market
- ✓ Expert support available

**Disadvantages:**
- ✗ Higher upfront cost
- ✗ Vendor lock-in
- ✗ May include unused features
- ✗ Specific version required

Certifying Non-Certified RTOS
------------------------------

**Process:**
1. Select suitable RTOS (FreeRTOS, Zephyr, etc.)
2. Perform gap analysis vs standard
3. Add required documentation
4. Implement safety mechanisms
5. Conduct verification and testing
6. Engage certification body
7. Obtain certificate

**Advantages:**
- ✓ Lower RTOS license cost
- ✓ Full control over implementation
- ✓ Can use latest features
- ✓ No vendor lock-in

**Disadvantages:**
- ✗ Much longer timeline (1-2 years)
- ✗ Requires safety expertise
- ✗ Higher overall cost
- ✗ Greater risk

Practical Considerations
=========================

License Compliance
------------------

**Best Practices:**

.. code-block:: text

    1. Document ALL third-party components
       - RTOS version and license
       - Middleware licenses
       - Driver licenses
       
    2. Include required notices
       - LICENSE files
       - Copyright notices
       - Attribution where required
       
    3. Manage source disclosure
       - Identify GPL components
       - Prepare source packages
       - Setup public repo if needed
       
    4. Regular license audits
       - Use FOSS license scanners
       - Review before releases
       - Update documentation

Certification Planning
----------------------

**Timeline Considerations:**

.. code-block:: text

    Pre-certified RTOS: 6-12 months
      - Requirements definition: 2 months
      - Implementation with evidence: 4-6 months
      - Verification: 2-3 months
      - Assessment: 1-2 months
      
    DIY Certification: 12-24 months
      - Gap analysis: 3 months
      - RTOS adaptation: 6-12 months
      - Documentation: 4-6 months
      - Testing: 4-6 months
      - Assessment: 2-3 months

Cost-Benefit Analysis
---------------------

.. code-block:: text

    Decision Factors:
    
    Use Pre-Certified RTOS if:
      □ Safety-critical domain (ASIL C/D, SIL 3/4)
      □ Limited safety expertise
      □ Tight schedule
      □ Budget allows $20k-$100k for RTOS
      
    Certify Standard RTOS if:
      □ Lower safety level (ASIL A/B, SIL 1/2)
      □ Strong internal safety team
      □ Long-term project (> 2 years)
      □ Multiple products can amortize cost
      
    No Certification if:
      □ Non-safety-critical
      □ Customer doesn't require it
      □ Can use qualification instead

See Also
========

- :doc:`rtos_comparison_table` - License column
- :doc:`rtos_selection_guide` - Selection criteria including certification
- :doc:`../days/day19` - RTOS Comparison and Selection
- Safety standard documentation (IEC 61508, ISO 26262, etc.)

Further Reading
===============

- "Open Source Licensing" by Heather Meeker
- IEC 61508 standard (parts 1-7)
- ISO 26262 standard
- DO-178C standard
- RTOS vendor certification guides
- FOSS license compatibility matrix
