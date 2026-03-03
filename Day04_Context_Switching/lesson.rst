Day 04 Lesson - Context Switching
==================================

Introduction
------------

**Context switching** is the process of saving the state of one task and restoring the state of another. It's the fundamental mechanism that enables multitasking in an RTOS. Understanding context switching—what happens, when it happens, and how much it costs—is essential for building efficient real-time systems.

What is a Context Switch?
--------------------------

A **context** is the complete CPU state of a task:

- **Program Counter (PC)**: Current instruction address
- **Stack Pointer (SP)**: Top of task's stack
- **CPU Registers**: General-purpose and special registers
- **Status Flags**: Processor status register (PSR)

**Context Switch**: Saving one task's context and loading another's.

.. code-block:: text

   Task A Running:
     PC = 0x08001234
     SP = 0x20003F00
     R0-R12 = [task A values]
   
   [Context Switch Occurs]
   
   Task B Running:
     PC = 0x08005678
     SP = 0x20007E00
     R0-R12 = [task B values]

When Does Context Switching Occur?
-----------------------------------

1. **Tick Interrupt**
   
   RTOS scheduler runs periodically (typically 1ms)
   
   .. code-block:: c
   
      void SysTick_Handler(void)
      {
          // Save current task context
          // Check if higher priority task is ready
          // If yes, perform context switch
          // Restore new task context
      }

2. **Blocking API Call**
   
   Task voluntarily yields CPU
   
   .. code-block:: c
   
      vTaskDelay(100);              // Delay
      xSemaphoreTake(sem, timeout); // Wait for semaphore
      xQueueReceive(queue, &data, timeout);  // Wait for data

3. **Preemption**
   
   Higher-priority task becomes ready
   
   .. code-block:: c
   
      void UART_IRQHandler(void)
      {
          // ISR signals high-priority task
          BaseType_t xHigherPriorityTaskWoken = pdFALSE;
          xSemaphoreGiveFromISR(xUARTSemaphore, &xHigherPriorityTaskWoken);
          
          // Context switch if higher priority task now ready
          portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
      }

ARM Cortex-M Context Switch
----------------------------

Cortex-M Register Set
~~~~~~~~~~~~~~~~~~~~~~

**Core Registers (R0-R15):**

.. code-block:: text

   R0-R3:   Argument/scratch registers
   R4-R11:  Saved registers (preserved across calls)
   R12:     Intra-procedure-call scratch register
   R13 (SP): Stack Pointer
   R14 (LR): Link Register (return address)
   R15 (PC): Program Counter

**Special Registers:**

- **PSR**: Program Status Register (flags, execution mode)
- **PRIMASK**: Interrupt mask
- **CONTROL**: Stack selection, privilege level

Hardware-Assisted Context Switch
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Cortex-M processors have **automatic stacking** for interrupts:

.. code-block:: text

   Interrupt Entry (Hardware):
     1. Push R0-R3, R12, LR, PC, PSR to stack
     2. Set LR to special return value (EXC_RETURN)
     3. Jump to ISR
   
   Interrupt Exit (Hardware):
     1. Pop R0-R3, R12, LR, PC, PSR from stack
     2. Resume execution

**Software saves remaining registers (R4-R11):**

.. code-block:: c

   // PendSV_Handler: Context switch ISR
   void PendSV_Handler(void)
   {
       // Disable interrupts
       __asm volatile("CPSID I");
       
       // Save current task context
       // Get current task SP
       __asm volatile("MRS R0, PSP");  // Get Process Stack Pointer
       
       // Save R4-R11 (hardware already saved R0-R3, R12, LR, PC, PSR)
       __asm volatile(
           "STMDB R0!, {R4-R11}\\n"  // Push R4-R11 onto stack
           "MOV %0, R0"              // Save new SP
           : "=r"(pxCurrentTCB->pxTopOfStack)
       );
       
       // Select next task
       vTaskSwitchContext();
       
       // Restore new task context
       __asm volatile(
           "MOV R0, %0\\n"           // Load new task SP
           "LDMIA R0!, {R4-R11}\\n"  // Pop R4-R11 from stack
           "MSR PSP, R0"             // Set Process Stack Pointer
           : : "r"(pxCurrentTCB->pxTopOfStack)
       );
       
       // Enable interrupts
       __asm volatile("CPSIE I");
       
       // Return (hardware restores R0-R3, R12, LR, PC, PSR)
       __asm volatile("BX LR");
   }

FreeRTOS Context Switch Implementation
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: c

   // Trigger context switch (sets PendSV interrupt)
   #define portYIELD()  do { \\
       portNVIC_INT_CTRL_REG = portNVIC_PENDSVSET_BIT; \\
       __asm volatile("DSB"); \\
       __asm volatile("ISB"); \\
   } while(0)

   // PendSV has lowest interrupt priority
   // Ensures context switch happens after all other ISRs complete

**Why PendSV?**

- **Tail-chaining**: Multiple interrupts can complete before context switch
- **Efficient**: Context switch deferred until all ISRs handled
- **Clean**: Consistent stack state

Context Switch Cost
-------------------

Measuring Overhead
~~~~~~~~~~~~~~~~~

.. code-block:: c

   void measure_context_switch_time(void)
   {
       uint32_t start, end, duration_cycles;
       
       // Disable interrupts for accurate measurement
       taskENTER_CRITICAL();
       
       start = DWT->CYCCNT;  // Cycle counter
       
       // Force context switch
       taskYIELD();
       
       end = DWT->CYCCNT;
       
       taskEXIT_CRITICAL();
       
       duration_cycles = end - start;
       float duration_us = (float)duration_cycles / (SystemCoreClock / 1000000);
       
       printf("Context switch: %lu cycles (%.2f µs)\\n", 
              duration_cycles, duration_us);
   }

**Typical Costs:**

+----------------------+------------------+------------------------+
| Platform             | Cycles           | Time @ 100MHz          |
+======================+==================+========================+
| Cortex-M0+           | 40-60            | 0.4-0.6 µs            |
+----------------------+------------------+------------------------+
| Cortex-M3/M4         | 12-25            | 0.12-0.25 µs          |
+----------------------+------------------+------------------------+
| Cortex-M7            | 8-15             | 0.08-0.15 µs          |
+----------------------+------------------+------------------------+

**Factors Affecting Cost:**

1. **Number of registers** to save/restore
2. **Floating-point unit (FPU)**: +34 registers if FPU used
3. **Cache/MPU configuration**
4. **Interrupt latency**

Context Switch Frequency
~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: c

   void monitor_context_switches(void)
   {
       static uint32_t last_count = 0;
       static TickType_t last_time = 0;
       
       uint32_t current_count = uxTaskGetNumberOfTasks();
       TickType_t current_time = xTaskGetTickCount();
       
       uint32_t switches = current_count - last_count;
       uint32_t time_delta_ms = current_time - last_time;
       
       float switches_per_sec = (float)switches / (time_delta_ms / 1000.0f);
       
       printf("Context switches: %.1f per second\\n", switches_per_sec);
       
       // Calculate CPU overhead
       float cs_time_us = 0.2;  // Measured context switch time
       float overhead_percent = (switches_per_sec * cs_time_us / 1000000) * 100;
       
       printf("Context switch overhead: %.2f%%\\n\", overhead_percent);
       
       last_count = current_count;
       last_time = current_time;
   }

**Example:**

.. code-block:: text

   10 tasks, 1ms tick rate
   Worst case: 1000 context switches/second
   @ 0.2µs per switch = 0.2ms/second = 0.02% CPU overhead
   
   Negligible!

Lazy Context Switching
-----------------------

**Lazy FPU Context Save:**

Cortex-M4F/M7 can defer saving FPU registers until necessary:

.. code-block:: c

   // FreeRTOS FPU configuration
   #define configENABLE_FPU                1
   #define configENABLE_TRUSTZONE          0

   // Only save FPU context if task actually used FPU
   // Saves 34 registers (136 bytes) if FPU not used

**Implementation:**

.. code-block:: text

   Task A (uses FPU):
     - Full context save (CPU + FPU registers)
   
   Task B (no FPU):
     - Minimal context save (CPU registers only)
     - 34 fewer registers = 70% faster context switch!

Voluntary vs. Involuntary Context Switch
-----------------------------------------

Voluntary (Cooperative)
~~~~~~~~~~~~~~~~~~~~~~~

Task explicitly yields:

.. code-block:: c

   void cooperative_task(void *pvParameters)
   {
       for(;;)
       {
           do_some_work();
           
           // Explicitly yield CPU
           taskYIELD();  // or vTaskDelay(0);
           
           do_more_work();
       }
   }

**Pros:**
- Task completes "atomic" operations
- No race conditions within task
- Lower overhead (no tick interrupt needed)

**Cons:**
- One misbehaving task blocks system
- Poor real-time responsiveness

Involuntary (Preemptive)
~~~~~~~~~~~~~~~~~~~~~~~~~

Scheduler forces context switch:

.. code-block:: c

   void preemptive_task(void *pvParameters)
   {
       for(;;)
       {
           // This may be preempted at ANY time
           complex_calculation();
           
           // No explicit yield needed
       }
   }

**Pros:**
- Guaranteed responsiveness
- Higher-priority tasks never blocked by lower
- Better for real-time

**Cons:**
- Need synchronization (mutexes, critical sections)
- Slightly higher overhead

FreeRTOS Default: **Preemptive with cooperative option**

Critical Sections and Context Switching
----------------------------------------

Disabling Context Switches
~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: c

   void atomic_operation(void)
   {
       // Method 1: Disable interrupts (fast but blocks ISRs!)
       taskENTER_CRITICAL();
       
       // Atomic operation (no context switch possible)
       global_counter++;
       
       taskEXIT_CRITICAL();
       
       
       // Method 2: Suspend scheduler (ISRs still run)
       vTaskSuspendAll();
       
       // Multiple operations, no context switch
       update_shared_data();
       
       xTaskResumeAll();
   }

**Critical Section Maximum Duration:**

- Keep < 100µs on most systems
- Longer durations affect interrupt latency
- Affects system responsiveness

.. code-block:: c

   // BAD: Critical section too long
   taskENTER_CRITICAL();
   for(int i = 0; i < 10000; i++) {  // May take milliseconds!
       process_item(i);
   }
   taskEXIT_CRITICAL();
   
   // GOOD: Minimal critical section
   for(int i = 0; i < 10000; i++) {
       process_item(i);  // Outside critical section
   }
   taskENTER_CRITICAL();
   update_shared_result();  // Only shared access protected
   taskEXIT_CRITICAL();

Real-World Example: Data Logger with Context Switch Optimization
-----------------------------------------------------------------

.. code-block:: c

   #include "FreeRTOS.h"
   #include "task.h"
   #include "queue.h"

   // High-frequency data acquisition
   void sensor_task(void *pvParameters)
   {
       TickType_t xLastWakeTime = xTaskGetTickCount();
       const TickType_t xPeriod = pdMS_TO_TICKS(1);  // 1ms = 1kHz
       
       for(;;)
       {
           vTaskDelayUntil(&xLastWakeTime, xPeriod);
           
           // Read sensor (fast, DMA-based)
           uint16_t value = adc_read_dma();
           
           // Send to queue (may trigger context switch)
           xQueueSend(xDataQueue, &value, 0);  // Non-blocking
       }
   }

   // Lower-priority processing
   void processing_task(void *pvParameters)
   {
       uint16_t raw_values[100];
       uint8_t count = 0;
       
       for(;;)
       {
           // Batch processing to reduce context switches
           while(count < 100 && 
                 xQueueReceive(xDataQueue, &raw_values[count], 
                               pdMS_TO_TICKS(10)) == pdPASS)
           {
               count++;
           }
           
           if(count > 0)
           {
               // Process batch
               process_batch(raw_values, count);
               count = 0;
           }
       }
   }

**Optimization:**

- Sensor task minimizes work (just read & send)
- Processing task batches data: fewer context switches
- 100 samples processed in one context vs. 100 contexts

**Result:**

.. code-block:: text

   Before batching:
     - 1000 sensor reads/sec
     - 1000 context switches/sec
     - 0.2% CPU overhead
   
   After batching:
     - 1000 sensor reads/sec
     - 10 context switches/sec (100x reduction!)
     - 0.002% CPU overhead

Stack Traces and Debugging
---------------------------

Inspecting Task Context
~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: c

   void print_task_stack_trace(TaskHandle_t xTask)
   {
       TaskStatus_t xTaskDetails;
       
       vTaskGetInfo(xTask, &xTaskDetails, pdTRUE, eRunning);
       
       printf("Task: %s\\n", xTaskDetails.pcTaskName);
       printf("  Stack High Water Mark: %u words\\n",
              xTaskDetails.usStackHighWaterMark);
       printf("  Stack Base: 0x%08X\\n", 
              (uint32_t)xTaskDetails.pxStackBase);
       
       // Walk stack (platform-specific)
       uint32_t *sp = (uint32_t*)xTaskDetails.pxStackBase;
       printf("  Stack contents:\\n");
       for(int i = 0; i < 16; i++) {
           printf("    [0x%08X] = 0x%08X\\n", 
                  (uint32_t)&sp[i], sp[i]);
       }
   }

Context Switch Hooks
~~~~~~~~~~~~~~~~~~~~

.. code-block:: c

   // FreeRTOSConfig.h
   #define configUSE_TRACE_FACILITY  1

   // Application hook (called on every context switch)
   void vApplicationTaskSwitchHook(void)
   {
       static uint32_t switch_count = 0;
       switch_count++;
       
       TaskHandle_t xNewTask = xTaskGetCurrentTaskHandle();
       const char *pcTaskName = pcTaskGetName(xNewTask);
       
       printf("[%lu] Context switch to: %s\\n", 
              switch_count, pcTaskName);
   }

Summary
-------

**Key Concepts:**

1. **Context switching** saves one task's state and restores another's
2. **Triggered by**: Tick interrupts, blocking calls, preemption
3. **Hardware assistance** (Cortex-M): Automatic stacking of R0-R3, R12, LR, PC, PSR
4. **Cost**: Typically 0.1-0.5µs on modern MCUs (negligible overhead)
5. **Optimization**: Batch operations, minimize unnecessary switches

**Best Practices:**

- Don't fear context switches—overhead is minimal on modern MCUs
- Use preemptive scheduling for responsiveness
- Keep critical sections short (<100µs)
- Batch operations when possible to reduce switch frequency
- Enable lazy FPU context saving if applicable
- Monitor context switch frequency during development

Understanding context switching demystifies RTOS behavior and enables building efficient multitasking systems.
