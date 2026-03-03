Day 03 Exercises - Scheduling and Determinism
==============================================

Exercise 1 - Schedulability Analysis
-------------------------------------

Objective
~~~~~~~~~
Analyze a real-time system to determine if all tasks can meet their deadlines using Rate Monotonic Scheduling analysis.

Scenario
~~~~~~~~
You're designing an automotive sensor fusion system with the following periodic tasks:

+------------------+----------+------------+----------+
| Task             | Period   | Exec Time  | Deadline |
+==================+==========+============+==========+
| IMU Reading      | 5 ms     | 1.2 ms     | 5 ms     |
+------------------+----------+------------+----------+
| GPS Processing   | 100 ms   | 15 ms      | 100 ms   |
+------------------+----------+------------+----------+
| Radar Filtering  | 20 ms    | 4 ms       | 20 ms    |
+------------------+----------+------------+----------+
| Camera Image     | 50 ms    | 12 ms      | 50 ms    |
+------------------+----------+------------+----------+
| Fusion Algorithm | 10 ms    | 2.5 ms     | 10 ms    |
+------------------+----------+------------+----------+

Tasks
~~~~~

1. **Calculate CPU Utilization:**
   
   .. math::
   
      U = \sum_{i=1}^{n} \frac{C_i}{T_i}
   
   Compute utilization for each task and total system utilization.

2. **Determine RMS Priority Assignment:**
   
   Assign priorities based on Rate Monotonic Scheduling (shortest period = highest priority).

3. **Apply RMS Schedulability Test:**
   
   .. math::
   
      U \leq n(2^{1/n} - 1)
   
   Is this task set schedulable?

4. **Perform Response Time Analysis:**
   
   For each task, calculate worst-case response time:
   
   .. math::
   
      R_i = C_i + \sum_{j \in hp(i)} \left\lceil \frac{R_i}{T_j} \right\rceil \cdot C_j
   
   Does every task meet its deadline?

5. **What-If Analysis:**
   
   - Camera task execution time increases to 18 ms. Is system still schedulable?
   - A new low-priority logging task (Period=200ms, Exec=25ms) is added. Impact?
   - IMU reading must run at 1 kHz (1 ms period). Can you accommodate this?

Expected Output
~~~~~~~~~~~~~~~

.. code-block:: text

   CPU Utilization Analysis:
     IMU:    1.2ms / 5ms   = 24.0%
     Fusion: 2.5ms / 10ms  = 25.0%
     Radar:  4.0ms / 20ms  = 20.0%
     Camera: 12ms / 50ms   = 24.0%
     GPS:    15ms / 100ms  = 15.0%
     Total Utilization: 108.0%
   
   RMS Priority Assignment (5 tasks):
     IMU:    Period= 5ms  → Priority=5 (Highest)
     Fusion: Period=10ms  → Priority=4
     Radar:  Period=20ms  → Priority=3
     Camera: Period=50ms  → Priority=2
     GPS:    Period=100ms → Priority=1 (Lowest)
   
   RMS Schedulability Bound: 74.3%
   System Utilization: 108.0%
   RMS Test: FAIL (108% > 74.3%)
   
   Response Time Analysis:
     [Detailed calculations showing deadline misses]
   
   Conclusion: System is NOT schedulable. 
   Recommendations: Optimize GPS/Camera algorithms or offload to separate processor.

Deliverables
~~~~~~~~~~~~

1. Complete utilization calculations with work shown
2. Priority assignment table with rationale
3. Response time analysis for all tasks
4. What-if analysis results
5. Design recommendations for schedulability

Exercise 2 - Priority Inversion Resolution
-------------------------------------------

Objective
~~~~~~~~~
Demonstrate priority inversion problem and implement solutions using priority inheritance.

Scenario
~~~~~~~~
Three tasks share a hardware SPI peripheral:

- **High Priority (5)**: Emergency shutdown task (must respond within 10ms)
- **Medium Priority (3)**: Data logging task (CPU-intensive, 50ms execution)
- **Low Priority (1)**: Configuration task (accesses SPI, 20ms)

Without proper synchronization, priority inversion can cause the emergency task to miss its deadline.

Tasks
~~~~~

1. **Implement the Scenario (No Protection):**
   
   .. code-block:: c
   
      volatile bool spi_busy = false;  // Simple busy flag (bad!)
      
      void low_priority_task(void *pvParameters)
      {
          for(;;)
          {
              while(spi_busy);  // Busy wait
              spi_busy = true;
              
              use_spi_peripheral();  // 20ms
              
              spi_busy = false;
              vTaskDelay(pdMS_TO_TICKS(100));
          }
      }
      
      void medium_priority_task(void *pvParameters)
      {
          for(;;)
          {
              intensive_computation();  // 50ms, no SPI access
              vTaskDelay(pdMS_TO_TICKS(200));
          }
      }
      
      void high_priority_task(void *pvParameters)
      {
          for(;;)
          {
              wait_for_emergency_event();
              
              while(spi_busy);  // BLOCKED HERE!
              spi_busy = true;
              
              emergency_spi_operation();  // Must complete in 10ms
              
              spi_busy = false;
          }
      }
   
   Measure: How long is high-priority task blocked?

2. **Demonstrate Priority Inversion:**
   
   Create a timeline showing:
   - Low task acquires SPI
   - High task becomes ready, blocked on SPI
   - Medium task preempts low task
   - High task waits while medium runs!

3. **Implement Priority Inheritance Solution:**
   
   .. code-block:: c
   
      SemaphoreHandle_t xSPIMutex;
      
      void create_spi_mutex(void)
      {
          // FreeRTOS mutex automatically has priority inheritance
          xSPIMutex = xSemaphoreCreateMutex();
      }
      
      void low_priority_task_fixed(void *pvParameters)
      {
          for(;;)
          {
              xSemaphoreTake(xSPIMutex, portMAX_DELAY);
              
              // While holding mutex, runs at HIGH priority!
              use_spi_peripheral();
              
              xSemaphoreGive(xSPIMutex);
              vTaskDelay(pdMS_TO_TICKS(100));
          }
      }

4. **Measure and Compare:**
   
   | Scenario              | High Task Wait Time |
   |-----------------------|---------------------|
   | No Protection         | 70+ ms (MISSED!)    |
   | Priority Inheritance  | 20 ms (OK)          |

5. **Implement Alternative: Priority Ceiling:**
   
   Manually elevate task priority when accessing SPI.

Expected Output
~~~~~~~~~~~~~~~

Timeline showing priority inversion and fix, plus measured latencies.

Deliverables
~~~~~~~~~~~~

1. Complete code for all three scenarios
2. Timeline diagrams showing task execution
3. Measured wait times
4. Analysis comparing solutions

Exercise 3 - Deterministic Timing Analysis
-------------------------------------------

Objective
~~~~~~~~~
Measure and reduce jitter in a periodic control task.

Scenario
~~~~~~~~
A motor control task must run every 1ms with minimal jitter (< 50µs). Current implementation shows excessive jitter due to various non-deterministic factors.

Tasks
~~~~~

1. **Implement Baseline (With Jitter):**
   
   .. code-block:: c
   
      void motor_control_task_v1(void *pvParameters)
      {
          for(;;)
          {
              vTaskDelay(pdMS_TO_TICKS(1));  // BAD: accumulates drift
              
              // Random processing variations
              float setpoint = read_setpoint();    // Variable I2C time
              float current = read_current_sensor(); // Variable ADC conversion
              
              // Floating point (may be slow on some MCUs)
              float error = setpoint - current;
              float output = pid_controller(error);
              
              set_pwm(output);
          }
      }

2. **Instrument to Measure Jitter:**
   
   .. code-block:: c
   
      typedef struct {
          uint32_t min_period_us;
          uint32_t max_period_us;
          uint32_t avg_period_us;
          uint32_t jitter_us;
          uint32_t sample_count;
      } timing_stats_t;
      
      void measure_jitter(timing_stats_t *stats)
      {
          static uint32_t last_time_us = 0;
          uint32_t current_time_us = get_microseconds();
          
          if(last_time_us != 0)
          {
              uint32_t period = current_time_us - last_time_us;
              // Update min, max, average, jitter
          }
          
          last_time_us = current_time_us;
      }

3. **Identify Jitter Sources:**
   
   - Use of ``vTaskDelay()`` instead of ``vTaskDelayUntil()``
   - Variable-time I2C/ADC operations
   - Floating-point computation
   - Interrupt interference
   - Context switch overhead

4. **Implement Optimized Version:**
   
   .. code-block:: c
   
      void motor_control_task_v2(void *pvParameters)
      {
          TickType_t xLastWakeTime = xTaskGetTickCount();
          const TickType_t xPeriod = pdMS_TO_TICKS(1);
          
          // Pre-allocated buffers (no dynamic allocation)
          static int32_t setpoint_fixed;  // Fixed-point
          static int32_t current_fixed;
          
          for(;;)
          {
              vTaskDelayUntil(&xLastWakeTime, xPeriod);  // Drift-free!
              
              // Deterministic sensor reads with timeout
              current_fixed = read_current_sensor_dma();  // DMA, no blocking
              
              // Fixed-point arithmetic (deterministic)
              int32_t error = setpoint_fixed - current_fixed;
              int32_t output = pid_controller_fixed(error);
              
              set_pwm_register_direct(output);  // Direct register write
          }
      }

5. **Measure Improvement:**
   
   Compare jitter before and after optimization.

Expected Output
~~~~~~~~~~~~~~~

.. code-block:: text

   Baseline Implementation:
     Min Period: 850 µs
     Max Period: 1250 µs
     Avg Period: 1015 µs
     Jitter: 400 µs (EXCESSIVE!)
   
   Optimized Implementation:
     Min Period: 980 µs
     Max Period: 1020 µs
     Avg Period: 1000 µs
     Jitter: 40 µs (ACCEPTABLE)
   
   Improvements:
     - Used vTaskDelayUntil: Eliminated drift
     - DMA sensors: Removed blocking I/O
     - Fixed-point math: Deterministic execution
     - Direct register access: Removed function call overhead

Deliverables
~~~~~~~~~~~~

1. Baseline and optimized implementations
2. Jitter measurement code
3. Before/after timing statistics
4. Analysis of jitter sources and fixes
