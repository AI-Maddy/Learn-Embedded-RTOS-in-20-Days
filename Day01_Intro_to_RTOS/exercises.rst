Day 01 Exercises - Introduction to RTOS
========================================

Exercise 1 - Concept Check: Bare-Metal vs. RTOS
-----------------------------------------------

Objective
~~~~~~~~~
Understand when an RTOS is beneficial compared to bare-metal firmware.

Scenario
~~~~~~~~
You're designing firmware for a smart thermostat with these requirements:

- Read temperature sensor every 100ms
- Check motion detector for occupancy (irregular timing)
- Update LCD display every 500ms
- Handle button presses (immediate response required)
- Communicate with Wi-Fi module (variable duration, 50-200ms)

Tasks
~~~~~
1. **Bare-Metal Analysis**: Sketch a super-loop implementation. Identify the main challenge in handling urgent button presses.

2. **RTOS Analysis**: Design a task structure for an RTOS solution. Assign priorities (1=low, 3=high) with justification.

3. **Timing Analysis**: Calculate the worst-case response time for a button press in both approaches, assuming:
   - Temperature read: 1ms
   - LCD update: 10ms
   - Wi-Fi communication: 200ms (max)
   - Button handler: 2ms

Expected Output
~~~~~~~~~~~~~~~
A comparison table showing:

.. list-table::
   :header-rows: 1
   :widths: 30 35 35

   * - Metric
     - Bare-Metal
     - RTOS
   * - Worst-case button response
     - ??? ms (calculate)
     - ??? ms (calculate)
   * - Code complexity
     - (your assessment)
     - (your assessment)
   * - Memory overhead
     - (estimate)
     - (estimate)

**Bonus**: Identify one failure mode that RTOS makes easier to prevent.

Exercise 2 - Code Review: Critical Bugs
----------------------------------------

Objective
~~~~~~~~~
Identify common mistakes in early RTOS code that violate determinism.

Buggy Code
~~~~~~~~~~

.. code-block:: c

   // Task 1: Process incoming packets
   void packet_task(void *param) {
       uint8_t buffer[512];
       
       while(1) {
           // Wait for packet
           if(receive_packet(buffer, 512, TIMEOUT_FOREVER)) {
               // Process all data in packet
               for(int i = 0; i < get_packet_length(buffer); i++) {
                   process_byte(buffer[i]);  // Takes 10-50ms per byte
               }
           }
       }
   }
   
   // Task 2: Critical safety monitor
   void safety_task(void *param) {
       while(1) {
           read_pressure_sensor();
           
           if(pressure > CRITICAL_THRESHOLD) {
               emergency_shutdown();  // MUST run within 10ms of detection
           }
           
           vTaskDelay(pdMS_TO_TICKS(5));  // Check every 5ms
       }
   }
   
   // Task 3: Logging task
   void log_task(void *param) {
       while(1) {
           char* msg = get_next_log_message();
           
           // Write to SD card
           while(!sd_card_ready()) {
               // Spin-wait for SD card
           }
           sd_card_write(msg);
       }
   }

Tasks
~~~~~
1. **Identify 3-5 bugs** that could violate timing requirements or cause system failures.

2. **For each bug**, explain:
   - What goes wrong under what conditions?
   - What real-time principle is violated?
   - How to fix it?

3. **Priority Assignment**: If Task 1 has priority 2, what priorities should Tasks 2 and 3 have? Justify your answer.

Expected Output
~~~~~~~~~~~~~~~
A bug report table:

.. code-block:: text

   Bug #1: [Bug description]
   Location: [Task name, line description]
   Failure Scenario: [When does this fail?]
   Fix: [Specific code change]
   
   Bug #2: ...

Exercise 3 - Design Practice: Multi-Task System
-----------------------------------------------

Objective
~~~~~~~~~
Design a multi-task RTOS system from scratch with proper synchronization.

Scenario
~~~~~~~~
Design firmware for a **data logger** that:

- **Samples 4 ADC channels** at 100 Hz (every 10ms, all channels)
- **Writes data to SD card** in batches of 100 samples (avoid SD wear)
- **Monitors USB port** for commands: "START", "STOP", "STATUS"
- **Blinks status LED**: 
  - Slow (1 Hz) = idle
  - Fast (5 Hz) = logging
  - Off = error

**Hard Requirements:**

- ADC sampling timing jitter < 1ms (strict timing)
- USB commands must respond within 50ms
- SD write must not block ADC sampling
- Handle SD card errors gracefully (full card, removed card)

Tasks
~~~~~
1. **Task Decomposition**: Define 3-5 tasks with:
   - Task name and purpose
   - Priority (1=low, 5=high)
   - Stack size estimate
   - Timing requirements

2. **Communication Design**: Choose IPC mechanism(s) for:
   - Passing ADC samples to SD writer
   - Sending commands from USB task to ADC task
   - Notifying LED task of state changes

3. **Error Handling**: Define behavior when:
   - SD card is full (can't write)
   - ADC sampling overruns (missed deadline)
   - USB command arrives while SD writing

4. **Validation Plan**: How will you verify the 1ms jitter requirement in production?

Expected Output
~~~~~~~~~~~~~~~
A design document including:

.. code-block:: text

   ** Task Architecture **
   
   Task 1: ADC Sampler
   - Priority: 5 (highest - hard real-time)
   - Stack: 256 bytes
   - Period: 10ms (100 Hz)
   - IPC: Writes to circular buffer (lock-free)
   
   Task 2: ...
   
   ** Communication **
   
   ADC → SD: Circular buffer (1000 samples capacity)
   USB → Control: Queue for commands
   ...
   
   ** Error Scenarios **
   
   1. SD Full:
      - Action: Stop writing, keep latest 1000 samples in RAM
      - LED: Blink error pattern
      - USB: Report "MEMORY_FULL" status
   ...
   
   ** Verification **
   
   - Use GPIO toggle + oscilloscope to measure ADC task jitter
   - Log timestamp deltas: assert(delta < 11ms)
   ...

Exercise 4 - Timing Analysis Challenge (Advanced)
-------------------------------------------------

Objective
~~~~~~~~~
Perform worst-case execution time (WCET) analysis.

System Details
~~~~~~~~~~~~~~

Three tasks on a system with preemptive priority scheduling:

.. list-table::
   :header-rows: 1

   * - Task
     - Priority
     - Execution Time
     - Period
   * - Task A (Control)
     - 3 (High)
     - 2 ms
     - 10 ms
   * - Task B (Processing)
     - 2 (Medium)
     - 5 ms
     - 20 ms
   * - Task C (Logging)
     - 1 (Low)
     - 8 ms
     - 50 ms

Context switch overhead: 0.01 ms (10 μs)

Tasks
~~~~~
1. **Calculate CPU utilization**: Is this system schedulable? (Rule: U < 100%)

2. **Worst-case response time for Task B**: Consider:
   - Task B starts executing
   - Task A preempts it (how many times within 20ms?)
   - Include context switch overhead

3. **Priority Inversion scenario**: Task C runs and acquires a mutex that Task A later needs. Calculate the blocking time if:
   - Task C holds mutex for 1ms max
   - Task B preempts Task C while mutex held

4. **Design Rule**: What's the maximum execution time you'd allow for a new Task D (priority 2, period 100ms) while maintaining U < 80% (safety margin)?

Expected Output
~~~~~~~~~~~~~~~

.. code-block:: text

   ** CPU Utilization **
   U = (2/10) + (5/20) + (8/50) = ____%
   Schedulable: YES/NO
   
   ** Task B Response Time **
   - Base execution: 5ms
   - Task A preemptions: ___ times × 2ms = ___ ms
   - Context switches: ___ times × 0.01ms = ___ ms
   - Total WCRT: ___ ms
   - Meets deadline (20ms)?: YES/NO
   
   ** Priority Inversion **
   Max blocking for Task A: ___ ms
   Explain scenario: ...
   
   ** New Task D **
   Max execution time: ___ ms
   Calculation: ...

**Bonus**: Research "Rate Monotonic Analysis" and check if priority assignments are optimal.

Summary
-------

These exercises cover:

1. **When to use RTOS** (bare-metal comparison)
2. **Identifying real-time bugs** (code review)
3. **System design skills** (task decomposition, IPC, error handling)
4. **Timing analysis** (schedulability, WCET, response time)

After completing these, you should be able to:

- Justify RTOS adoption for a project
- Spot common timing violations
- Design a multi-task system architecture
- Perform basic schedulability analysis

**Solutions and deeper explanations in ``solutions.rst``**.
