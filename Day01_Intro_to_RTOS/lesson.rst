Day 01 Lesson - Introduction to Real-Time Operating Systems
===========================================================

What is an RTOS?
----------------

A **Real-Time Operating System (RTOS)** is a specialized operating system designed for embedded systems that must respond to events within guaranteed time constraints. Unlike general-purpose operating systems (like Linux or Windows), an RTOS prioritizes **determinism** and **predictability** over throughput.

**Key Characteristics:**

- **Deterministic Scheduling**: Tasks execute in predictable order based on priority
- **Low Latency**: Minimal delay between event occurrence and response
- **Preemption**: Higher-priority tasks can interrupt lower-priority ones immediately
- **Small Footprint**: Designed for resource-constrained environments (KB of RAM/ROM)
- **Hard Real-Time Guarantees**: Deadlines must be met; missing them is system failure

Why Use an RTOS?
----------------

**Bare-Metal vs. RTOS Trade-offs:**

Bare-metal firmware (super-loop architecture):

.. code-block:: c

   // Typical bare-metal super-loop
   int main(void) {
       hardware_init();
       while(1) {
           check_uart();
           read_sensors();
           update_display();
           process_commands();
           // Everything sequential, no priority
       }
   }

**Problems:**

- All tasks share one execution flow
- High-priority urgent events must wait for low-priority work
- Complex state machines needed for concurrent activities
- Difficult to reason about timing

**RTOS Solution:**

.. code-block:: c

   // RTOS approach with task priorities
   void uart_task(void *param) {      // Priority: HIGH
       while(1) {
           wait_for_uart_event();
           process_urgent_command();
       }
   }

   void sensor_task(void *param) {    // Priority: MEDIUM
       while(1) {
           read_sensors();
           delay(100ms);
       }
   }

   void display_task(void *param) {   // Priority: LOW
       while(1) {
           update_display();
           delay(500ms);
       }
   }

**Benefits:**

- Urgent UART commands preempt display updates
- Each task maintains its own stack and state
- Cleaner code with implicit priority management
- Scheduler handles timing and context switching

Core RTOS Concepts
------------------

1. Tasks (Threads)
~~~~~~~~~~~~~~~~~~

A task is an independent execution context with its own:

- **Program counter (PC)**: Current instruction address
- **Stack**: Local variables and return addresses
- **Priority**: Determines scheduling order
- **State**: Running, Ready, Blocked, or Suspended

.. code-block:: c

   TaskHandle_t task_handle;
   
   void my_task(void *parameters) {
       int local_var = 0;  // Stored on task's private stack
       
       while(1) {
           // Task body runs forever
           do_work();
           vTaskDelay(pdMS_TO_TICKS(100));  // Yield CPU for 100ms
       }
   }
   
   // Create task with 512-byte stack, priority 2
   xTaskCreate(my_task, "MyTask", 512, NULL, 2, &task_handle);

2. Scheduler
~~~~~~~~~~~~

The **scheduler** decides which ready task runs next based on:

- **Priority-based**: Highest-priority ready task always runs (preemptive)
- **Round-robin**: Equal-priority tasks share CPU time slices
- **Cooperative**: Tasks must explicitly yield (rare in modern RTOS)

**Scheduling Algorithm (Simplified):**

.. code-block:: text

   while(system_running) {
       highest_priority_task = find_highest_ready();
       if (current_task != highest_priority_task) {
           save_context(current_task);
           restore_context(highest_priority_task);
           current_task = highest_priority_task;
       }
       run_task_until_blocked_or_preempted();
   }

3. Context Switching
~~~~~~~~~~~~~~~~~~~~

**Context switch** is the process of saving one task's state and restoring another's.

**What gets saved/restored:**

- CPU registers (R0-R15 on ARM Cortex-M)
- Stack pointer
- Program counter
- Status flags

**Cost**: Typically 10-50 CPU cycles on modern MCUs (e.g., ~1-2 μs on Cortex-M4 @ 80MHz)

4. Determinism
~~~~~~~~~~~~~~

**Determinism** means the system's behavior is predictable and repeatable.

**Key Principles:**

- **Bounded Execution Time**: Every operation has known maximum duration
- **Priority Inheritance**: Prevents priority inversion (covered Day 5)
- **No Unbounded Loops**: Always have exit conditions
- **Interrupt Management**: Minimize ISR duration, defer work to tasks

.. code-block:: c

   // BAD: Unbounded loop
   while(uart_data_available()) {
       process_byte();  // Could run forever if data streams continuously
   }
   
   // GOOD: Bounded burst processing
   int count = 0;
   while(uart_data_available() && count < MAX_BURST) {
       process_byte();
       count++;
   }

Real-World RTOS Applications
-----------------------------

1. **Medical Devices**
   - Pacemakers, insulin pumps (hard real-time)
   - Missing deadline = patient harm
   
2. **Automotive**
   - Engine control units (ECU), anti-lock braking (ABS)
   - Response time: sub-millisecond
   
3. **Industrial Automation**
   - PLCs, robotics, motor control
   - Coordinating multiple sensors/actuators
   
4. **Aerospace**
   - Flight control systems, avionics
   - Certification (DO-178C) requires RTOS

5. **Consumer Electronics**
   - Smartwatches, IoT devices, drones
   - Power efficiency + responsiveness

Common RTOS Options
-------------------

.. list-table::
   :header-rows: 1
   :widths: 20 20 20 40

   * - RTOS
     - License
     - Footprint
     - Use Cases
   * - FreeRTOS
     - MIT
     - ~10KB
     - Most popular, AWS IoT integration
   * - Zephyr
     - Apache 2.0
     - ~20KB+
     - Modern, BLE/networking rich
   * - ThreadX
     - MIT
     - ~6KB
     - Certified (IEC 61508, ISO 26262)
   * - ChibiOS
     - GPL/Commercial
     - ~12KB
     - High performance, HAL included
   * - embOS
     - Commercial
     - ~4KB
     - Ultra-compact, automotive-grade

Your First RTOS Program
------------------------

**Minimal FreeRTOS Example (ARM Cortex-M):**

.. code-block:: c

   #include "FreeRTOS.h"
   #include "task.h"
   
   // LED blink task (Priority 1)
   void vLEDTask(void *pvParameters) {
       TickType_t xLastWakeTime = xTaskGetTickCount();
       
       while(1) {
           GPIO_ToggleLED();
           // Precise 500ms delay (maintains period even if task delayed)
           vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(500));
       }
   }
   
   // UART monitor task (Priority 2 - higher)
   void vUARTTask(void *pvParameters) {
       char buffer[128];
       
       while(1) {
           // Block until data available (yields CPU to lower priority tasks)
           UART_Receive(buffer, sizeof(buffer), portMAX_DELAY);
           
           if(strncmp(buffer, "CRITICAL", 8) == 0) {
               handle_critical_command();  // Preempts LED task if needed
           }
       }
   }
   
   int main(void) {
       SystemClock_Config();
       GPIO_Init();
       UART_Init();
       
       // Create tasks
       xTaskCreate(vLEDTask, "LED", 128, NULL, 1, NULL);
       xTaskCreate(vUARTTask, "UART", 256, NULL, 2, NULL);
       
       // Start scheduler (never returns)
       vTaskStartScheduler();
       
       // Should never reach here
       while(1);
   }

**Execution Flow:**

1. ``main()`` creates tasks (both start in Ready state)
2. ``vTaskStartScheduler()`` starts RTOS
3. **UART task runs** (priority 2 > 1), blocks waiting for data
4. **LED task runs** (only ready task), toggles LED, delays
5. When UART data arrives, **UART task preempts** LED task immediately
6. After UART processing, LED task resumes where it left off

Key Takeaways
-------------

1. **RTOS provides structure** for managing concurrent activities with priorities
2. **Determinism is critical** for real-time systems (predictable timing)
3. **Preemption enables responsiveness** - urgent tasks run immediately
4. **Overhead is minimal** (context switch < 2 μs on modern MCUs)
5. **Use RTOS when**:
   - Multiple concurrent activities with different urgencies
   - Hard timing requirements
   - Complex state management becomes unwieldy

**Next Steps:**

Tomorrow (Day 02) we'll dive deep into task creation, priorities, and stack management.
