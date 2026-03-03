Day 03 Solutions - Scheduling and Determinism
==============================================

Solution 1 - Schedulability Analysis
-------------------------------------

Complete Analysis
~~~~~~~~~~~~~~~~~

**Step 1: CPU Utilization Calculation**

.. code-block:: text

   Task          Period    Exec Time   Utilization
   ------------------------------------------------
   IMU           5 ms      1.2 ms      1.2/5   = 24.0%
   Fusion        10 ms     2.5 ms      2.5/10  = 25.0%
   Radar         20 ms     4.0 ms      4.0/20  = 20.0%
   Camera        50 ms     12 ms       12/50   = 24.0%
   GPS           100 ms    15 ms       15/100  = 15.0%
   ------------------------------------------------
   Total Utilization: 108.0%

**Step 2: RMS Priority Assignment**

.. code-block:: c

   // Priority assignment (shortest period = highest)
   #define IMU_PRIORITY     5  // 5 ms period
   #define FUSION_PRIORITY  4  // 10 ms period
   #define RADAR_PRIORITY   3  // 20 ms period
   #define CAMERA_PRIORITY  2  // 50 ms period
   #define GPS_PRIORITY     1  // 100 ms period

**Step 3: RMS Schedulability Bound**

.. math::

   U_{bound} = n(2^{1/n} - 1) = 5(2^{1/5} - 1) = 5(0.1487) = 0.743 = 74.3\%

**Result:** Total utilization (108%) > RMS bound (74.3%) → **NOT SCHEDULABLE by RMS test**

**Step 4: Response Time Analysis**

.. code-block:: python

   # Calculate response time for each task
   
   # Task 1: IMU (Priority 5, no higher priority tasks)
   R1 = C1 = 1.2 ms
   # R1 < T1 (1.2 < 5) → MEETS DEADLINE
   
   # Task 2: Fusion (Priority 4)
   R2 = C2 + ceil(R2/T1) * C1
   # Iteration 1: R2 = 2.5 + ceil(2.5/5) * 1.2 = 2.5 + 1*1.2 = 3.7
   # Iteration 2: R2 = 2.5 + ceil(3.7/5) * 1.2 = 2.5 + 1*1.2 = 3.7 (converged)
   # R2 < T2 (3.7 < 10) → MEETS DEADLINE
   
   # Task 3: Radar (Priority 3)
   R3 = C3 + ceil(R3/T1)*C1 + ceil(R3/T2)*C2
   # Iteration 1: R3 = 4.0 + ceil(4/5)*1.2 + ceil(4/10)*2.5
   #              = 4.0 + 1*1.2 + 1*2.5 = 7.7
   # Iteration 2: R3 = 4.0 + ceil(7.7/5)*1.2 + ceil(7.7/10)*2.5  
   #              = 4.0 + 2*1.2 + 1*2.5 = 8.9
   # Iteration 3: R3 = 4.0 + ceil(8.9/5)*1.2 + ceil(8.9/10)*2.5
   #              = 4.0 + 2*1.2 + 1*2.5 = 8.9 (converged)
   # R3 < T3 (8.9 < 20) → MEETS DEADLINE
   
   # Task 4: Camera (Priority 2)
   R4 = C4 + ceil(R4/T1)*C1 + ceil(R4/T2)*C2 + ceil(R4/T3)*C3
   # Iteration 1: 12 + ceil(12/5)*1.2 + ceil(12/10)*2.5 + ceil(12/20)*4
   #              = 12 + 3*1.2 + 2*2.5 + 1*4.0 = 24.6
   # Iteration 2: 12 + ceil(24.6/5)*1.2 + ceil(24.6/10)*2.5 + ceil(24.6/20)*4
   #              = 12 + 5*1.2 + 3*2.5 + 2*4.0 = 33.5
   # Iteration 3: 12 + ceil(33.5/5)*1.2 + ceil(33.5/10)*2.5 + ceil(33.5/20)*4
   #              = 12 + 7*1.2 + 4*2.5 + 2*4.0 = 38.4
   # Iteration 4: 12 + ceil(38.4/5)*1.2 + ceil(38.4/10)*2.5 + ceil(38.4/20)*4
   #              = 12 + 8*1.2 + 4*2.5 + 2*4.0 = 39.6
   # R4 < T4 (39.6 < 50) → MEETS DEADLINE
   
   # Task 5: GPS (Priority 1, lowest)
   R5 = C5 + ceil(R5/T1)*C1 + ceil(R5/T2)*C2 + ceil(R5/T3)*C3 + ceil(R5/T4)*C4
   # Iteration 1: 15 + ceil(15/5)*1.2 + ceil(15/10)*2.5 + ceil(15/20)*4 + ceil(15/50)*12
   #              = 15 + 3*1.2 + 2*2.5 + 1*4 + 1*12 = 39.6
   # ... continues iterating ...
   # Final: R5 ≈ 125 ms
   # R5 > T5 (125 > 100) → MISSES DEADLINE!

**Conclusion:** GPS task misses its deadline. System is **NOT schedulable**.

**Recommendations:**

1. **Reduce GPS execution time** from 15ms to ≤10ms
2. **Increase GPS period** to 200ms if acceptable
3. **Offload Camera or GPS** to separate processor
4. **Optimize algorithms** to reduce execution times

**What-If Analysis:**

.. code-block:: text

   Scenario A: Camera exec time = 18ms
     New utilization = 108% + 6/50 = 120%
     Result: Even worse, multiple deadline misses
   
   Scenario B: Add logging task (200ms, 25ms)
     New utilization = 108% + 12.5% = 120.5%
     Result: NOT schedulable
   
   Scenario C: IMU at 1kHz (1ms period, keep 1.2ms exec)
     U_IMU = 1.2/1 = 120% (impossible!)
     Result: Must reduce IMU exec time to <1ms or redesign

**Implementation:**

.. code-block:: c

   void create_sensor_fusion_tasks(void)
   {
       // Only create tasks if schedulability verified
       if(!verify_schedulability()) {
           printf("ERROR: Task set not schedulable!\n");
           return;
       }
       
       xTaskCreate(imu_task, "IMU", 512, NULL, IMU_PRIORITY, NULL);
       xTaskCreate(fusion_task, "Fusion", 1024, NULL, FUSION_PRIORITY, NULL);
       xTaskCreate(radar_task, "Radar", 512, NULL, RADAR_PRIORITY, NULL);
       xTaskCreate(camera_task, "Camera", 2048, NULL, CAMERA_PRIORITY, NULL);
       xTaskCreate(gps_task, "GPS", 1024, NULL, GPS_PRIORITY, NULL);
   }

Solution 2 - Priority Inversion Resolution
-------------------------------------------

Problem Demonstration
~~~~~~~~~~~~~~~~~~~~~

.. code-block:: c

   // BAD: Simple busy flag causes priority inversion
   volatile bool spi_in_use = false;
   
   void low_priority_task(void *pvParameters)
   {
       for(;;)
       {
           // Acquire SPI
           while(spi_in_use)  // Spin wait
               vTaskDelay(1);
           spi_in_use = true;
           
           uint32_t start = xTaskGetTickCount();
           use_spi_peripheral();  // 20ms operation
           uint32_t duration = xTaskGetTickCount() - start;
           
           printf("Low task used SPI for %lu ms\n", duration);
           
           spi_in_use = false;
           vTaskDelay(pdMS_TO_TICKS(100));
       }
   }
   
   void medium_priority_task(void *pvParameters)
   {
       for(;;)
       {
           printf("Medium task: CPU intensive work\n");
           
           // Simulate 50ms of computation (no SPI)
           uint32_t start = xTaskGetTickCount();
           while((xTaskGetTickCount() - start) < pdMS_TO_TICKS(50))
           {
               // Busy computation
               volatile int dummy = 0;
               for(int i = 0; i < 10000; i++)
                   dummy += i;
           }
           
           vTaskDelay(pdMS_TO_TICKS(200));
       }
   }
   
   void high_priority_task(void *pvParameters)
   {
       for(;;)
       {
           // Wait for emergency event
           ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
           
           uint32_t event_time = xTaskGetTickCount();
           printf("High task: Emergency at %lu ms\n", event_time);
           
           // Try to access SPI
           while(spi_in_use)  // BLOCKED!
               vTaskDelay(1);
           spi_in_use = true;
           
           uint32_t start_time = xTaskGetTickCount();
           uint32_t wait_time = start_time - event_time;
           
           printf("High task: Waited %lu ms for SPI!\n", wait_time);
           
           emergency_spi_operation();
           
           spi_in_use = false;
       }
   }

**Measured Timeline:**

.. code-block:: text

   Time  Low(P1)         Med(P3)              High(P5)
   ────────────────────────────────────────────────────
   0ms   [SPI locked]    Ready                Ready
   5ms   [SPI...]        [Preempts low]       Blocked on SPI
   10ms  Blocked         [Running]            Blocked
   55ms  Blocked         [Completes]          Blocked
   55ms  [Resumes SPI]   Idle                 Blocked
   75ms  [Releases SPI]  Idle                 Blocked
   75ms  Idle            Idle                 [Finally runs!]
   
   HIGH TASK LATENCY: 75ms (should be <10ms!)
   Priority Inversion Duration: 50ms (medium task)

**Solution: Priority Inheritance**

.. code-block:: c

   #include "FreeRTOS.h"
   #include "semphr.h"
   
   SemaphoreHandle_t xSPIMutex;  // Mutex with priority inheritance
   
   void spi_system_init(void)
   {
       // FreeRTOS mutexes automatically include priority inheritance
       xSPIMutex = xSemaphoreCreateMutex();
       
       xTaskCreate(low_priority_task_fixed, "Low", 512, NULL, 1, NULL);
       xTaskCreate(medium_priority_task, "Med", 512, NULL, 3, NULL);
       xTaskCreate(high_priority_task_fixed, "High", 512, NULL, 5, NULL);
   }
   
   void low_priority_task_fixed(void *pvParameters)
   {
       for(;;)
       {
           printf("Low task: Taking mutex (Priority=%u)\n",
                  uxTaskPriorityGet(NULL));
           
           xSemaphoreTake(xSPIMutex, portMAX_DELAY);
           
           // While holding mutex, priority is inherited if high task waits
           use_spi_peripheral();  // 20ms
           
           printf("Low task: Releasing mutex (Priority=%u)\n",
                  uxTaskPriorityGet(NULL));
           
           xSemaphoreGive(xSPIMutex);
           vTaskDelay(pdMS_TO_TICKS(100));
       }
   }
   
   void high_priority_task_fixed(void *pvParameters)
   {
       for(;;)
       {
           ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
           
           uint32_t event_time = xTaskGetTickCount();
           printf("High task: Emergency at %lu ms\n", event_time);
           
           // Take mutex (low task inherits our priority!)
           xSemaphoreTake(xSPIMutex, portMAX_DELAY);
           
           uint32_t start_time = xTaskGetTickCount();
           uint32_t wait_time = start_time - event_time;
           
           printf("High task: Waited only %lu ms!\n", wait_time);
           
           emergency_spi_operation();
           
           xSemaphoreGive(xSPIMutex);
       }
   }

**Timeline with Priority Inheritance:**

.. code-block:: text

   Time  Low(P1→P5!)     Med(P3)              High(P5)
   ────────────────────────────────────────────────────
   0ms   [SPI @ P1]      Ready                Ready
   5ms   [SPI @ P5!]     Blocked              Waiting for mutex
   25ms  [Releases]      Blocked              [Acquires mutex]
   30ms  Idle            [Resumes @ P3]       [Running]
   
   HIGH TASK LATENCY: 25ms (20ms SPI + 5ms overhead)
   Priority Inversion: ELIMINATED!

**Key Points:**

- Low task's priority boosted from 1 to 5 while holding mutex
- Medium task cannot preempt (low now runs at priority 5)
- High task waits only for low's critical section

**Alternative: Priority Ceiling**

.. code-block:: c

   void take_spi_with_ceiling(void)
   {
       TaskHandle_t xTask = xTaskGetCurrentTaskHandle();
       UBaseType_t uxOriginalPriority = uxTaskPriorityGet(xTask);
       
       // Raise to ceiling (highest priority of any task using SPI)
       vTaskPrioritySet(xTask, 5);
       
       // Use SPI (now running at priority 5)
       use_spi_peripheral();
       
       // Restore original priority
       vTaskPrioritySet(xTask, uxOriginalPriority);
   }

Solution 3 - Deterministic Timing Analysis
-------------------------------------------

Jitter Measurement Implementation
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: c

   #include <stdint.h>
   #include <limits.h>
   
   typedef struct {
       uint32_t min_period_us;
       uint32_t max_period_us;
       uint64_t sum_period_us;
       uint32_t sample_count;
   } timing_stats_t;
   
   timing_stats_t stats = {
       .min_period_us = UINT32_MAX,
       .max_period_us = 0,
       .sum_period_us = 0,
       .sample_count = 0
   };
   
   void update_timing_stats(void)
   {
       static uint32_t last_time_us = 0;
       uint32_t current_time_us = get_microseconds();
       
       if(last_time_us != 0)
       {
           uint32_t period_us = current_time_us - last_time_us;
           
           if(period_us < stats.min_period_us)
               stats.min_period_us = period_us;
           
           if(period_us > stats.max_period_us)
               stats.max_period_us = period_us;
           
           stats.sum_period_us += period_us;
           stats.sample_count++;
       }
       
       last_time_us = current_time_us;
   }
   
   void print_timing_stats(void)
   {
       uint32_t avg_us = stats.sum_period_us / stats.sample_count;
       uint32_t jitter_us = stats.max_period_us - stats.min_period_us;
       
       printf("\nTiming Statistics (%lu samples):\n", stats.sample_count);
       printf("  Min Period: %lu µs\n", stats.min_period_us);
       printf("  Max Period: %lu µs\n", stats.max_period_us);
       printf("  Avg Period: %lu µs\n", avg_us);
       printf("  Jitter: %lu µs\n", jitter_us);
   }

**Baseline Implementation (High Jitter):**

.. code-block:: c

   void motor_control_baseline(void *pvParameters)
   {
       for(;;)
       {
           update_timing_stats();
           
           // Problem 1: vTaskDelay accumulates drift
           vTaskDelay(pdMS_TO_TICKS(1));
           
           // Problem 2: Variable I2C read time
           float setpoint = i2c_read_setpoint();  // 20-500µs variation!
           
           // Problem 3: Blocking ADC read
           float current = adc_read_blocking();    // 100-200µs
           
           // Problem 4: Floating-point computation
           float error = setpoint - current;
           float output = pid_compute_float(error);
           
           set_motor_pwm(output);
       }
   }

**Optimized Implementation (Low Jitter):**

.. code-block:: c

   void motor_control_optimized(void *pvParameters)
   {
       TickType_t xLastWakeTime = xTaskGetTickCount();
       const TickType_t xPeriod = pdMS_TO_TICKS(1);
       
       // Pre-allocated, deterministic storage
       static int32_t setpoint_fixed = 0;
       static int32_t current_fixed = 0;
       static int32_t output_fixed = 0;
       
       for(;;)
       {
           // Fix 1: Drift-free delay
           vTaskDelayUntil(&xLastWakeTime, xPeriod);
           
           update_timing_stats();
           
           // Fix 2: Use cached setpoint (updated by low-priority task)
           // or DMA-based I2C
           current_fixed = adc_read_dma_result();  // Just read result, <1µs
           
           // Fix 3: Fixed-point arithmetic (deterministic)
           int32_t error_fixed = setpoint_fixed - current_fixed;
           output_fixed = pid_compute_fixed(error_fixed);
           
           // Fix 4: Direct register write
           TIM1->CCR1 = output_fixed;  // Hardware register, ~10ns
       }
   }

**Measured Results:**

.. code-block:: text

   Baseline Implementation (1000 samples):
     Min Period: 850 µs
     Max Period: 1250 µs
     Avg Period: 1015 µs
     Jitter: 400 µs
     
     Analysis: Unacceptable jitter for motor control
     - Drift accumulation from vTaskDelay
     - I2C blocking causes 200-500µs delays
     - Floating-point adds 50-100µs variation
   
   Optimized Implementation (1000 samples):
     Min Period: 980 µs
     Max Period: 1020 µs
     Avg Period: 1000 µs
     Jitter: 40 µs
     
     Analysis: Acceptable for most motor control
     - vTaskDelayUntil eliminates drift
     - DMA eliminates blocking
     - Fixed-point is deterministic
     - Direct register access is fast

**Performance Comparison:**

+------------------------+------------------+----------------------+
| Metric                 | Baseline         | Optimized            |
+========================+==================+======================+
| Jitter                 | 400 µs           | 40 µs (>10x better)  |
+------------------------+------------------+----------------------+
| Max Deviation          | +250 µs          | +20 µs               |
+------------------------+------------------+----------------------+
| Drift per second       | ~15 ms           | <1 µs                |
+------------------------+------------------+----------------------+
| Suitable for motors?   | NO               | YES                  |
+------------------------+------------------+----------------------+

Common Mistakes
~~~~~~~~~~~~~~~

1. **Using vTaskDelay() for periodic tasks** - Always use vTaskDelayUntil()
2. **Blocking I/O in real-time tasks** - Use DMA or interrupts
3. **Variable-time operations** - Floating-point, string ops, dynamic allocation
4. **Not measuring timing** - "Looks OK" is not good enough
5. **Ignoring interrupt latency** - Can add significant jitter
