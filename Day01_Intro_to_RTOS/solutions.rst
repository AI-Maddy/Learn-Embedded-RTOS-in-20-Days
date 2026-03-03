Day 01 Solutions - Introduction to RTOS
========================================

Solution 1 - Bare-Metal vs. RTOS Analysis
------------------------------------------

Part 1: Bare-Metal Super-Loop
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Typical Implementation:**

.. code-block:: c

   int main(void) {
       hw_init();
       while(1) {
           // Sequential execution
           read_temperature();        // 1ms
           check_motion_detector();   // <1ms (if no motion)
           update_lcd();              // 10ms
           check_button_press();      // <1ms (if not pressed)
           handle_wifi();             // 50-200ms variable!
       }
   }

**Main Challenge**: Button press responsiveness depends on where in the loop it occurs.

**Worst-Case Response Time Calculation:**

Assume button pressed right after ``check_button_press()`` completes:

.. code-block:: text

   Wait time = rest of loop
   = handle_wifi(max) + read_temperature + check_motion + update_lcd + check_button
   = 200ms + 1ms + 1ms + 10ms + 1ms
   = 213 ms worst case
   
   In best case (button pressed just before check): ~1ms

**Problem**: 213ms is too slow for good UI responsiveness (humans notice >100ms).

Part 2: RTOS Task Structure
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Task Design:**

.. list-table::
   :header-rows: 1
   :widths: 25 15 60

   * - Task Name
     - Priority
     - Justification
   * - Button Handler
     - 3 (High)
     - User interaction requires immediate feedback (<50ms)
   * - Motion Detector
     - 3 (High)
     - Security/energy feature, time-sensitive
   * - Temperature Sampler
     - 2 (Medium)
     - Periodic but not critical
   * - Wi-Fi Communications
     - 2 (Medium)
     - Important but can tolerate delays
   * - LCD Update
     - 1 (Low)
     - Cosmetic, no timing criticality

**RTOS Implementation Sketch:**

.. code-block:: c

   void button_task(void *p) {  // Priority 3
       while(1) {
           wait_for_button_interrupt();  // Blocked until press
           handle_button();              // 2ms
       }
   }
   
   void wifi_task(void *p) {  // Priority 2
       while(1) {
           wifi_communicate();  // 50-200ms
           vTaskDelay(pdMS_TO_TICKS(1000));  // 1Hz rate
       }
   }
   
   void lcd_task(void *p) {  // Priority 1
       TickType_t last_wake = xTaskGetTickCount();
       while(1) {
           update_lcd();  // 10ms
           vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(500));
       }
   }

**RTOS Worst-Case Response Time:**

When button ISR occurs:

1. ISR sets event → wakes button_task
2. If Wi-Fi task running (priority 2), it's preempted immediately
3. Button task runs (priority 3 > all others)

.. code-block:: text

   Response time = ISR latency + context switch + button handler
   = ~5μs + ~1μs + 2ms
   ≈ 2.01 ms
   
   Even if Wi-Fi is mid-communication, it's preempted!

**Comparison Table:**

.. list-table::
   :header-rows: 1
   :widths: 30 35 35

   * - Metric
     - Bare-Metal
     - RTOS
   * - Worst-case button response
     - 213 ms
     - ~2 ms
   * - Code complexity
     - All in one function; manual state machines for concurrency
     - Distributed across tasks; simpler per-task logic
   * - Memory overhead
     - ~1KB (single stack)
     - ~6KB (5 tasks × 512B stack + kernel)
   * - CPU utilization
     - Lower (polling wastes cycles)
     - Higher (sleep when idle)

**Bonus - Failure Mode Prevention:**

**Stack overflow**: Bare-metal has one stack—overflow corrupts globals silently. RTOS isolates each task's stack; overflow detected via guard patterns (``vApplicationStackOverflowHook``).

Solution 2 - Code Review: Critical Bugs
----------------------------------------

Bug #1: Unbounded Loop in packet_task
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: c

   for(int i = 0; i < get_packet_length(buffer); i++) {
       process_byte(buffer[i]);  // 10-50ms per byte!
   }

**Problem**: If packet length is 512 bytes × 50ms/byte = **25.6 seconds** max!  
Safety task (priority unknown) might not run for 25 seconds → emergency_shutdown delayed.

**Real-Time Principle Violated**: **Bounded execution time**.

**Fix**: Process in chunks with yield points:

.. code-block:: c

   int offset = 0;
   while(offset < get_packet_length(buffer)) {
       int chunk = min(10, get_packet_length(buffer) - offset);
       for(int i = 0; i < chunk; i++) {
           process_byte(buffer[offset + i]);
       }
       offset += chunk;
       taskYIELD();  // Allow higher-priority tasks to run
   }

Bug #2: Priority Inversion Risk
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Safety task must run within 10ms but might be blocked if packet_task holds a shared resource.

**Problem**: If both tasks access shared data (e.g., sensor calibration table) without priority inheritance, packet_task at lower priority could block safety_task.

**Fix**: 
- Use mutex with priority inheritance: ``xSemaphoreCreateMutex()``
- Or redesign to avoid shared mutable state

Bug #3: Spin-Wait in log_task
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: c

   while(!sd_card_ready()) {
       // Spin-wait for SD card
   }

**Problem**: Wastes CPU while SD card is busy (could be 10-100ms). Task holds CPU even though it can't make progress → blocks lower-priority tasks needlessly.

**Fix**: Block on SD card ready event using semaphore:

.. code-block:: c

   xSemaphoreTake(sd_ready_semaphore, portMAX_DELAY);
   sd_card_write(msg);
   
   // In SD card ISR:
   xSemaphoreGiveFromISR(sd_ready_semaphore, NULL);

Bug #4: Missing Delay in log_task Loop
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: c

   while(1) {
       char* msg = get_next_log_message();
       // No yield if no message!
   }

**Problem**: If ``get_next_log_message()`` returns NULL immediately (no messages), loop spins at 100% CPU → starves lower-priority tasks (if any).

**Fix**: Block on queue with timeout:

.. code-block:: c

   char msg[128];
   while(1) {
       if(xQueueReceive(log_queue, msg, portMAX_DELAY) == pdTRUE) {
           wait_for_sd_card();
           sd_card_write(msg);
       }
   }

Bug #5: Safety Task False Sense of Security
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: c

   vTaskDelay(pdMS_TO_TICKS(5));  // Check every 5ms

**Problem**: If safety task misses a check cycle (preempted by long-running packet process), next check is 10ms later. Also, delay is **at least** 5ms, could be 6ms depending on tick timing.

**Fix**: Use ``vTaskDelayUntil`` for precise timing + increase priority:

.. code-block:: c

   TickType_t last_wake = xTaskGetTickCount();
   while(1) {
       read_pressure_sensor();
       if(pressure > CRITICAL_THRESHOLD) {
           emergency_shutdown();
       }
       vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(5));
   }

**Priority Assignment:**

Given packet_task is priority 2:

- **safety_task**: Priority 4-5 (highest) — Must preempt everything
- **packet_task**: Priority 2 (given)
- **log_task**: Priority 1 (lowest) — Not time-critical

**Justification**: Safety is hard real-time (life/property risk). Logging is best-effort.

Solution 3 - Data Logger Design
--------------------------------

Task Architecture
~~~~~~~~~~~~~~~~~

.. list-table::
   :header-rows: 1
   :widths: 20 10 15 55

   * - Task
     - Priority
     - Stack
     - Purpose
   * - ADC_Sampler
     - 5
     - 256B
     - Sample 4 ADC channels every 10ms; write to ring buffer
   * - SD_Writer
     - 2
     - 1024B
     - Batch 100 samples, write to SD card
   * - USB_Command
     - 4
     - 512B
     - Parse START/STOP/STATUS commands
   * - LED_Blinker
     - 1
     - 128B
     - Update LED pattern based on system state

**Timing Requirements:**

- ADC_Sampler: **10ms period, <1ms jitter** (hard real-time)
- USB_Command: **<50ms response** (soft real-time)
- SD_Writer: **No deadline** (best-effort)
- LED_Blinker: **~100-200ms periods** (cosmetic)

Communication Design
~~~~~~~~~~~~~~~~~~~~

**ADC → SD Writer**: Lock-free circular buffer

.. code-block:: c

   #define BUFFER_SIZE 1000
   struct {
       adc_sample_t samples[BUFFER_SIZE];
       volatile uint32_t write_idx;  // Modified by ADC task only
       volatile uint32_t read_idx;   // Modified by SD task only
   } sample_buffer;
   
   // ADC task (producer):
   void adc_task(void *p) {
       TickType_t last_wake = xTaskGetTickCount();
       while(1) {
           adc_sample_t sample = read_all_channels();
           
           uint32_t next = (sample_buffer.write_idx + 1) % BUFFER_SIZE;
           if(next != sample_buffer.read_idx) {  // Not full
               sample_buffer.samples[sample_buffer.write_idx] = sample;
               sample_buffer.write_idx = next;
           } else {
               error_buffer_overrun++;
           }
           
           vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(10));
       }
   }

**USB → Control**: Command queue

.. code-block:: c

   QueueHandle_t cmd_queue;
   
   typedef enum { CMD_START, CMD_STOP, CMD_STATUS } command_t;
   
   // USB task sends:
   command_t cmd = parse_usb_input();
   xQueueSend(cmd_queue, &cmd, pdMS_TO_TICKS(100));
   
   // ADC/SD tasks check:
   command_t cmd;
   if(xQueueReceive(cmd_queue, &cmd, 0) == pdTRUE) {
       handle_command(cmd);
   }

**State → LED**: Event group flags

.. code-block:: c

   EventGroupHandle_t state_events;
   #define STATE_LOGGING   (1 << 0)
   #define STATE_ERROR     (1 << 1)
   
   // SD task sets:
   xEventGroupSetBits(state_events, STATE_LOGGING);
   
   // LED task waits:
   EventBits_t bits = xEventGroupWaitBits(state_events, 
       STATE_LOGGING | STATE_ERROR, pdFALSE, pdFALSE, portMAX_DELAY);

Error Handling
~~~~~~~~~~~~~~

**Scenario 1: SD Card Full**

- **Detection**: ``sd_write()`` returns error code
- **Action**:
  1. Stop consuming from ring buffer
  2. Ring buffer reaches capacity → ADC task detects overrun
  3. Set ``STATE_ERROR`` flag
  4. Respond to STATUS command with "MEMORY_FULL"
- **Recovery**: User sends STOP → system returns to idle

**Scenario 2: ADC Sampling Overrun**

- **Detection**: ``vTaskDelayUntil`` returns pdFALSE (missed deadline) -OR- ring buffer full
- **Action**:
  1. Increment ``stats.overruns`` counter
  2. Log timestamp of overrun event
  3. Set error flag (don't stop sampling)
- **Prevention**: 
  - Ensure ADC task priority > all others
  - Minimize ISR duration
  - Increase ring buffer size

**Scenario 3: Command During SD Write**

- **Not a problem**: USB task (priority 4) preempts SD Writer (priority 2) immediately
- Command processed within <50ms even if SD write takes 500ms

Validation Plan
~~~~~~~~~~~~~~~

**Jitter Measurement:**

.. code-block:: c

   void adc_task(void *p) {
       TickType_t last_wake = xTaskGetTickCount();
       TickType_t expected_wake = last_wake;
       
       while(1) {
           TickType_t actual_wake = xTaskGetTickCount();
           int32_t jitter = (actual_wake - expected_wake) * portTICK_PERIOD_MS;
           
           if(abs(jitter) > 1) {
               log_error("Jitter violation: %d ms", jitter);
           }
           
           // GPIO toggle for oscilloscope measurement:
           GPIO_Toggle(DEBUG_PIN);
           
           read_all_channels();
           
           vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(10));
           expected_wake += pdMS_TO_TICKS(10);
       }
   }

**Production Test**:
- Oscilloscope on DEBUG_PIN: measure period variation → must be 10.00 ± 0.1ms
- Stress test: Run for 24 hours, verify stats.overruns == 0

Solution 4 - Timing Analysis (Advanced)
----------------------------------------

Part 1: CPU Utilization
~~~~~~~~~~~~~~~~~~~~~~~~

.. math::

   U = \sum \frac{C_i}{T_i}

Where $C_i$ = execution time, $T_i$ = period.

.. math::

   U = \frac{2}{10} + \frac{5}{20} + \frac{8}{50}

   U = 0.20 + 0.25 + 0.16 = 0.61 = 61\%

**Result**: System is schedulable (U < 100%, leaves 39% margin).

Part 2: Worst-Case Response Time (Task B)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Task B (priority 2, execution 5ms, period 20ms).

**Analysis window**: 20ms (one period of Task B).

**Interference from higher-priority Task A**:

- Task A: period 10ms, execution 2ms
- In 20ms, Task A runs **2 times**: at t=0 and t=10ms

**Preemptions**:
- Task B starts at t=0 (worst case: Task A also ready)
- Task A preempts (2ms) + context switch (0.01ms)
- Task B resumes, runs until t=10ms
- Task A preempts again (2ms) + context switch (0.01ms)
- Task B finishes

**Calculation**:

.. code-block:: text

   Response Time (Task B) = Execution + Interference
   
   Interference = (Task A executions in 20ms) × (Execution + Context Switch)
   = 2 × (2ms + 0.01ms) = 4.02ms
   
   Total Response Time = 5ms + 4.02ms = 9.02ms
   
   Deadline: 20ms
   Meets deadline?: YES (9.02ms < 20ms)

Part 3: Priority Inversion
~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Scenario**:

1. Task C (priority 1) acquires mutex
2. Task C holds mutex for max 1ms
3. Task B (priority 2) preempts Task C (mutex still held by C)
4. Task B runs its 5ms
5. Task C resumes, holds mutex (1ms remaining)
6. Task A (priority 3) needs the mutex → **blocked by Task C**

**Blocking Time for Task A**:

- Task C has 1ms of work while holding mutex
- BUT Task B extended Task C's blocking period by 5ms!
- Total blocking = 5ms (Task B) + 1ms (Task C) = **6ms**

This is **unbounded priority inversion** (medium-priority task blocks high-priority).

**Fix**: Priority Inheritance Protocol (PIP)

- When Task A blocks on mutex held by Task C:
  - **Boost Task C's priority to 3** (same as Task A)
  - Task C now preempts Task B
  - Task C finishes mutex section (1ms), releases mutex
  - Task C returns to priority 1
  - Task A acquires mutex

**New Blocking Time**: 1ms (only Task C's critical section)

Part 4: New Task D Headroom
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Target**: U < 80% (20% safety margin for analysis uncertainty).

Current: U = 61%  
Available: 80% - 61% = 19%

Task D: Priority 2, period 100ms

.. math::

   \frac{C_D}{100} \leq 0.19

   C_D \leq 19 \text{ ms}

**Answer**: Task D can execute for up to **19ms** per 100ms period.

**Bonus: Rate Monotonic Analysis (RMA)**

RMA optimal priority assignment: **shorter period = higher priority**

Current assignment:

- Task A: period 10ms, priority 3 ✓
- Task B: period 20ms, priority 2 ✓
- Task C: period 50ms, priority 1 ✓

**Result**: Priorities are optimal per RMA.

Common Mistakes to Avoid
-------------------------

1. **Assuming vTaskDelay is precise**: It's a minimum delay; actual delay is ceil(delay/tick_period). Use vTaskDelayUntil for periodic tasks.

2. **Ignoring stack size**: Stack overflow is silent killer. Over-provision initially (2x estimate), then measure with watermark APIs.

3. **Priority inversion neglect**: Always use mutexes (not binary semaphores) for shared resources to enable priority inheritance.

4. **Unbounded operations in high-priority tasks**: Every loop needs a maximum iteration count or yield point.

5. **Polling in tasks**: Use blocking IPC (queues, semaphores) to avoid wasting CPU and enable lower-priority tasks to run.

6. **Missing deadline analysis**: Don't just code and hope; calculate WCRT before implementation.

Key Takeaways
-------------

- **RTOS shines when** concurrent activities have different urgencies
- **Preemption provides responsiveness** but requires careful priority assignment
- **Determinism comes from** bounded execution, priority management, and avoiding locks
- **Always analyze** CPU utilization and worst-case response times before declaring success
- **Measure in production**: Use instrumentation (GPIO toggles, timestamps) to validate timing assumptions

**Next**: Day 02 dives into task lifecycle, stack management, and priority pitfalls.
