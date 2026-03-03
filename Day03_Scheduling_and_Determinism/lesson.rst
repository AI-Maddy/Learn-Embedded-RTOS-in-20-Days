Day 03 Lesson - Scheduling and Determinism
===========================================

Introduction
------------

**Scheduling** is the process by which an RTOS decides which task runs at any given moment. **Determinism** means the system's behavior is predictable and repeatable—the same inputs always produce the same outputs at the same times.

Together, these concepts form the foundation of real-time systems. Understanding scheduling algorithms, schedulability analysis, and how to achieve deterministic behavior is essential for building reliable embedded systems.

What is Scheduling?
-------------------

The **scheduler** is the RTOS component that:

1. **Decides** which ready task should execute next
2. **Preempts** the current task when a higher-priority task becomes ready
3. **Manages** CPU time allocation among tasks
4. **Maintains** fairness (for equal-priority tasks)

**Key Questions:**

- Which task runs when multiple tasks are ready?
- How long does each task run before switching?
- What happens when a higher-priority task becomes ready?
- How do we guarantee all tasks meet their deadlines?

Scheduling Algorithms
---------------------

Priority-Based Preemptive Scheduling
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Most common in RTOS** (FreeRTOS, Zephyr, ThreadX, etc.)

**Rules:**

1. Highest-priority ready task always runs
2. Lower-priority task is preempted immediately
3. Equal-priority tasks share CPU via round-robin (optional)

.. code-block:: text

   Time ─────────────────────────────────────►
   
   Task A (Priority 5): ████─────████████─────
   Task B (Priority 3): ─────███──────────█████
   Task C (Priority 1): ──────────────────────██
   
   Events:
   - Task A runs initially
   - Task B becomes ready → preempts A
   - Task A becomes ready again → preempts B
   - A completes → B resumes
   - B completes → C runs

**Example:**

.. code-block:: c

   // Three tasks with different priorities
   void high_priority_task(void *pvParameters)
   {
       for(;;)
       {
           // Becomes ready every 100ms
           do_urgent_work();
           vTaskDelay(pdMS_TO_TICKS(100));
       }
   }

   void medium_priority_task(void *pvParameters)
   {
       for(;;)
       {
           do_normal_work();
           vTaskDelay(pdMS_TO_TICKS(500));
       }
   }

   void low_priority_task(void *pvParameters)
   {
       for(;;)
       {
           do_background_work();
           vTaskDelay(pdMS_TO_TICKS(1000));
       }
   }

   void create_tasks(void)
   {
       xTaskCreate(high_priority_task, "High", 512, NULL, 5, NULL);
       xTaskCreate(medium_priority_task, "Med", 512, NULL, 3, NULL);
       xTaskCreate(low_priority_task, "Low", 512, NULL, 1, NULL);
   }

Round-Robin (Time-Slicing)
~~~~~~~~~~~~~~~~~~~~~~~~~~~

For **equal-priority tasks**, CPU time is divided into time slices.

.. code-block:: text

   Time Slice = 10ms
   
   Task A (Priority 2): ██████────██████────
   Task B (Priority 2): ──────████──────████
   
   Each task runs for one time slice before switching

**FreeRTOS Configuration:**

.. code-block:: c

   // In FreeRTOSConfig.h
   #define configUSE_PREEMPTION        1
   #define configUSE_TIME_SLICING      1
   #define configTICK_RATE_HZ          1000  // 1ms tick

**Behavior:**

- Tick interrupt occurs every 1ms
- After N ticks, scheduler switches to next equal-priority task
- If tasks have different priorities, priority always wins

Cooperative Scheduling
~~~~~~~~~~~~~~~~~~~~~~

Tasks must **explicitly yield** the CPU. No preemption.

.. code-block:: c

   void cooperative_task(void *pvParameters)
   {
       for(;;)
       {
           do_some_work();
           
           // Explicitly yield to other tasks
           taskYIELD();
           
           do_more_work();
           taskYIELD();
       }
   }

**Pros:**
- Simpler synchronization (no race conditions from preemption)
- Lower overhead

**Cons:**
- One misbehaving task blocks the entire system
- Poor responsiveness
- Not suitable for hard real-time

**Not recommended for most embedded systems.**

Rate Monotonic Scheduling (RMS)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Optimal fixed-priority algorithm** for periodic tasks.

**Rule:** Assign priority inversely to period:
- Shortest period → Highest priority
- Longest period → Lowest priority

**Example:**

+------------------+-------------+-----------+
| Task             | Period      | Priority  |
+==================+=============+===========+
| Motor Control    | 10 ms       | 5 (HIGH)  |
+------------------+-------------+-----------+
| Sensor Reading   | 50 ms       | 3 (MED)   |
+------------------+-------------+-----------+
| Display Update   | 1000 ms     | 1 (LOW)   |
+------------------+-------------+-----------+

**Why RMS Works:**

Frequent tasks need to run often → should preempt infrequent tasks.

.. code-block:: c

   void create_rms_tasks(void)
   {
       // Priority assigned by period (shortest = highest)
       xTaskCreate(motor_control_task, "Motor", 512, NULL, 5, NULL);  // 10ms
       xTaskCreate(sensor_read_task, "Sensor", 512, NULL, 3, NULL);   // 50ms
       xTaskCreate(display_update_task, "Display", 512, NULL, 1, NULL); // 1000ms
   }

Earliest Deadline First (EDF)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Optimal dynamic-priority algorithm.**

**Rule:** Task with earliest deadline runs first. Priorities change dynamically.

**Example:**

.. code-block:: text

   Time: 0ms
   - Task A: Deadline at 100ms → Priority HIGH
   - Task B: Deadline at 150ms → Priority LOW
   
   Time: 50ms
   - Task A completes
   - Task B: Deadline at 150ms (100ms away)
   - Task C arrives: Deadline at 80ms (30ms away)
   - Task C runs (earliest deadline)

**In Practice:**

- Requires OS support (not common in typical RTOSes)
- Higher overhead due to dynamic priority changes
- Better CPU utilization than RMS
- Used in research and some advanced systems

Schedulability Analysis
-----------------------

**Schedulability**: Will all tasks meet their deadlines?

Utilization-Based Test (Sufficient Condition)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**For Rate Monotonic Scheduling:**

.. math::

   U = \sum_{i=1}^{n} \frac{C_i}{T_i} \leq n(2^{1/n} - 1)

Where:
- $C_i$ = Task execution time (WCET)
- $T_i$ = Task period
- $n$ = Number of tasks

**Bounds:**

+-------------------+------------------+
| Number of Tasks   | Utilization Bound|
+===================+==================+
| 1                 | 100%             |
+-------------------+------------------+
| 2                 | 82.8%            |
+-------------------+------------------+
| 3                 | 78.0%            |
+-------------------+------------------+
| 4                 | 75.7%            |
+-------------------+------------------+
| ∞                 | 69.3%            |
+-------------------+------------------+

**Example:**

.. code-block:: python

   # Task set:
   # Task A: Period=10ms, Execution=2ms
   # Task B: Period=20ms, Execution=4ms
   # Task C: Period=50ms, Execution=5ms
   
   U = 2/10 + 4/20 + 5/50
   U = 0.2 + 0.2 + 0.1 = 0.5 = 50%
   
   # RMS bound for 3 tasks:
   U_bound = 3 * (2^(1/3) - 1) = 0.78 = 78%
   
   # Since 50% < 78%, task set is schedulable!

**Implementation:**

.. code-block:: c

   typedef struct {
       const char *name;
       uint32_t period_ms;
       uint32_t execution_ms;
       uint8_t priority;
   } task_spec_t;

   task_spec_t tasks[] = {
       {"Motor",   10,  2, 5},
       {"Sensor",  20,  4, 3},
       {"Display", 50,  5, 1}
   };

   bool check_schedulability(task_spec_t *tasks, int num_tasks)
   {
       float utilization = 0.0f;
       float rms_bound;
       
       // Calculate total utilization
       for(int i = 0; i < num_tasks; i++)
       {
           utilization += (float)tasks[i].execution_ms / tasks[i].period_ms;
       }
       
       // Calculate RMS bound
       rms_bound = num_tasks * (pow(2.0, 1.0/num_tasks) - 1.0);
       
       printf("Total Utilization: %.1f%%\\n", utilization * 100);
       printf("RMS Bound: %.1f%%\\n", rms_bound * 100);
       
       if(utilization <= rms_bound) {
           printf("SCHEDULABLE by RMS test\\n");
           return true;
       } else {
           printf("Cannot guarantee schedulability\\n");
           return false;
       }
   }

Response Time Analysis (Exact Test)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**More precise** than utilization test. Calculates worst-case response time for each task.

**For task $i$ with priority higher than or equal to all $j < i$:**

.. math::

   R_i = C_i + \sum_{j: priority(j) > priority(i)} \left\lceil \frac{R_i}{T_j} \right\rceil \cdot C_j

**Iterative Calculation:**

.. code-block:: c

   uint32_t calculate_response_time(task_spec_t *tasks, int num_tasks, int task_idx)
   {
       uint32_t R_prev = 0;
       uint32_t R = tasks[task_idx].execution_ms;
       uint32_t max_iterations = 100;
       
       // Iterate until convergence
       for(uint32_t iter = 0; iter < max_iterations; iter++)
       {
           R_prev = R;
           R = tasks[task_idx].execution_ms;
           
           // Add interference from higher-priority tasks
           for(int j = 0; j < num_tasks; j++)
           {
               if(tasks[j].priority > tasks[task_idx].priority)
               {
                   uint32_t interference = 
                       ((R_prev + tasks[j].period_ms - 1) / tasks[j].period_ms) 
                       * tasks[j].execution_ms;
                   R += interference;
               }
           }
           
           // Check convergence
           if(R == R_prev) {
               break;
           }
           
           // Check if deadline missed
           if(R > tasks[task_idx].period_ms) {
               printf("Task %s: Response time (%lu ms) exceeds deadline (%lu ms)\\n",
                      tasks[task_idx].name, R, tasks[task_idx].period_ms);
               return R;
           }
       }
       
       printf("Task %s: Response time = %lu ms (Deadline = %lu ms)\\n",
              tasks[task_idx].name, R, tasks[task_idx].period_ms);
       
       return R;
   }

Determinism
-----------

What is Determinism?
~~~~~~~~~~~~~~~~~~~~

**Deterministic System:**
- Same inputs → Same outputs
- Same timing → Same execution order
- Predictable worst-case behavior

**Non-Deterministic Factors:**

1. **Unbounded operations**: Dynamic memory allocation (malloc/free)
2. **Variable-time operations**: String operations, floating-point division
3. **Caching effects**: Cache hits/misses vary
4. **Interrupts**: Variable interrupt arrival times
5. **Branch prediction**: CPU speculation varies

Achieving Determinism
~~~~~~~~~~~~~~~~~~~~~

**1. Use Static Memory Allocation**

.. code-block:: c

   // BAD: Non-deterministic (malloc may fail or take variable time)
   void process_data_nondeterministic(void)
   {
       uint8_t *buffer = malloc(1024);  // Variable execution time!
       if(buffer == NULL) {
           // Handle error
           return;
       }
       // Process...
       free(buffer);
   }

   // GOOD: Deterministic (fixed allocation time)
   void process_data_deterministic(void)
   {
       static uint8_t buffer[1024];  // Allocated at compile time
       // Process...
   }

**2. Avoid Unbounded Loops**

.. code-block:: c

   // BAD: Unbounded search
   int find_value_unbounded(int *array, int size, int target)
   {
       for(int i = 0; i < size; i++)  // Could take 1 or 1000 iterations
       {
           if(array[i] == target)
               return i;
       }
       return -1;
   }

   // GOOD: Bounded search with timeout
   int find_value_bounded(int *array, int size, int target, int max_iterations)
   {
       int iterations = (size < max_iterations) ? size : max_iterations;
       for(int i = 0; i < iterations; i++)
       {
           if(array[i] == target)
               return i;
       }
       return -1;  // Not found within budget
   }

**3. Use Fixed-Point Instead of Floating-Point**

.. code-block:: c

   // Floating-point (variable execution time on some MCUs)
   float calculate_average_float(float *samples, int count)
   {
       float sum = 0.0f;
       for(int i = 0; i < count; i++)
       {
           sum += samples[i];
       }
       return sum / count;  // Division may be slow/variable
   }

   // Fixed-point (deterministic)
   typedef int32_t fixed_t;  // 16.16 fixed-point
   
   fixed_t calculate_average_fixed(fixed_t *samples, int count)
   {
       int64_t sum = 0;
       for(int i = 0; i < count; i++)
       {
           sum += samples[i];
       }
       return (fixed_t)(sum / count);  // Integer division is fast and deterministic
   }

**4. Disable Interrupts for Critical Sections**

.. code-block:: c

   volatile uint32_t shared_counter = 0;

   void update_counter_deterministic(void)
   {
       taskENTER_CRITICAL();  // Disable interrupts
       
       // This operation is now atomic and deterministic
       shared_counter++;
       
       taskEXIT_CRITICAL();   // Re-enable interrupts
   }

**5. Use Bounded Queues and Timeouts**

.. code-block:: c

   void send_data_deterministic(QueueHandle_t queue, data_t *data)
   {
       // Always use timeout, never block indefinitely
       if(xQueueSend(queue, data, pdMS_TO_TICKS(100)) != pdPASS)
       {
           // Handle timeout - known worst case
           log_error("Queue send timeout");
       }
   }

Jitter and Latency
------------------

Jitter
~~~~~~

**Jitter** is the variation in timing from the expected value.

.. code-block:: text

   Expected: Task runs every 10.000ms
   
   Actual measurements:
   - 10.002ms
   - 9.998ms
   - 10.015ms
   - 9.991ms
   
   Jitter = max(|actual - expected|) = 0.015ms

**Measuring Jitter:**

.. code-block:: c

   void measure_jitter(void)
   {
       static TickType_t last_time = 0;
       static TickType_t min_period = UINT32_MAX;
       static TickType_t max_period = 0;
       TickType_t current_time, period;
       
       current_time = xTaskGetTickCount();
       
       if(last_time != 0)
       {
           period = current_time - last_time;
           
           if(period < min_period) min_period = period;
           if(period > max_period) max_period = period;
           
           TickType_t expected = pdMS_TO_TICKS(10);
           TickType_t jitter = max_period - min_period;
           
           printf("Period: %lu ms (expected: %lu ms)\\n", 
                  period, expected);
           printf("Jitter: %lu ms (min: %lu, max: %lu)\\n",
                  jitter, min_period, max_period);
       }
       
       last_time = current_time;
   }

**Reducing Jitter:**

1. Use ``vTaskDelayUntil()`` instead of ``vTaskDelay()``
2. Increase task priority if being preempted too often
3. Minimize interrupt processing time
4. Disable time-slicing for critical tasks

Latency
~~~~~~~

**Latency** is the delay between an event and the system's response.

.. code-block:: text

   Event: Button pressed at time T
   Response: LED turns on at time T + L
   
   Latency = L

**Components of Latency:**

1. **Interrupt Latency**: Time from interrupt signal to ISR execution
2. **Scheduling Latency**: Time from task becoming ready to actually running
3. **Processing Latency**: Time spent executing the response

**Measuring Interrupt Latency:**

.. code-block:: c

   volatile uint32_t interrupt_time = 0;
   volatile uint32_t handler_start_time = 0;
   volatile uint32_t latency_us = 0;

   void button_interrupt_handler(void)
   {
       handler_start_time = get_microseconds();
       latency_us = handler_start_time - interrupt_time;
       
       // Signal task
       BaseType_t xHigherPriorityTaskWoken = pdFALSE;
       xSemaphoreGiveFromISR(xButtonSemaphore, &xHigherPriorityTaskWoken);
       portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
   }

   void button_task(void *pvParameters)
   {
       uint32_t task_start_time, total_latency;
       
       for(;;)
       {
           xSemaphoreTake(xButtonSemaphore, portMAX_DELAY);
           task_start_time = get_microseconds();
           
           // Total latency from interrupt to task
           total_latency = task_start_time - interrupt_time;
           
           printf("Interrupt latency: %lu µs\\n", latency_us);
           printf("Total latency: %lu µs\\n", total_latency);
           
           // Respond to event
           turn_on_led();
       }
   }

Priority Inversion
------------------

**Priority inversion** occurs when a high-priority task waits for a resource held by a low-priority task, while a medium-priority task runs.

.. code-block:: text

   Timeline:
   
   High Priority:  [WAIT]─────────────────────[RUN]
   Medium Priority: ────[RUN]─────[RUN]──────
   Low Priority:    [LOCK]─────────────[UNLOCK]
   
   Problem: High priority waits while Medium runs!

Unbounded Priority Inversion
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: c

   SemaphoreHandle_t xSharedResourceSemaphore;

   // Low priority task
   void low_priority_task(void *pvParameters)
   {
       for(;;)
       {
           xSemaphoreTake(xSharedResourceSemaphore, portMAX_DELAY);
           
           // Access shared resource
           access_shared_resource();
           
           xSemaphoreGive(xSharedResourceSemaphore);
           
           vTaskDelay(pdMS_TO_TICKS(100));
       }
   }

   // Medium priority task (causes priority inversion)
   void medium_priority_task(void *pvParameters)
   {
       for(;;)
       {
           // Long-running computation
           do_extensive_calculations();
           vTaskDelay(pdMS_TO_TICKS(50));
       }
   }

   // High priority task (blocked!)
   void high_priority_task(void *pvParameters)
   {
       for(;;)
       {
           // Needs shared resource
           xSemaphoreTake(xSharedResourceSemaphore, portMAX_DELAY);
           
           // Critical operation
           critical_operation();
           
           xSemaphoreGive(xSharedResourceSemaphore);
           
           vTaskDelay(pdMS_TO_TICKS(10));
       }
   }

Priority Inheritance
~~~~~~~~~~~~~~~~~~~~

**Solution:** Low-priority task temporarily inherits the priority of the high-priority task waiting for its resource.

.. code-block:: c

   // Create mutex with priority inheritance
   SemaphoreHandle_t xMutex;

   void create_priority_inheritance_mutex(void)
   {
       xMutex = xSemaphoreCreateMutex();
       // FreeRTOS mutexes automatically use priority inheritance
   }

   // Timeline with priority inheritance:
   //
   // High Priority:  [WAIT]───[RUN]
   // Medium Priority: ────[WAIT]─────
   // Low Priority:    [LOCK at HIGH priority]─[UNLOCK]
   //
   // Low task runs at High priority until it releases the mutex!

Priority Ceiling Protocol
~~~~~~~~~~~~~~~~~~~~~~~~~~

**Alternative solution:** Mutex has a priority ceiling. Any task locking it runs at that priority.

.. code-block:: c

   // Configure mutex with priority ceiling
   // (Implementation varies by RTOS)
   
   typedef struct {
       SemaphoreHandle_t mutex;
       UBaseType_t ceiling_priority;
   } priority_ceiling_mutex_t;

   void take_priority_ceiling_mutex(priority_ceiling_mutex_t *pcm)
   {
       TaskHandle_t xCurrentTask = xTaskGetCurrentTaskHandle();
       UBaseType_t uxOriginalPriority = uxTaskPriorityGet(xCurrentTask);
       
       // Raise priority to ceiling
       vTaskPrioritySet(xCurrentTask, pcm->ceiling_priority);
       
       // Take mutex
       xSemaphoreTake(pcm->mutex, portMAX_DELAY);
       
       // Task now runs at ceiling priority
   }

Real-World Example: Motor Control System
-----------------------------------------

.. code-block:: c

   #include "FreeRTOS.h"
   #include "task.h"
   #include "semphr.h"

   // System configuration
   #define MOTOR_CONTROL_PERIOD_MS    10   // 100 Hz
   #define SENSOR_READ_PERIOD_MS      20   // 50 Hz
   #define DISPLAY_UPDATE_PERIOD_MS   100  // 10 Hz

   // Execution times (measured)
   #define MOTOR_CONTROL_EXEC_MS      2
   #define SENSOR_READ_EXEC_MS        5
   #define DISPLAY_UPDATE_EXEC_MS     15

   // Shared resource protection
   SemaphoreHandle_t xSensorDataMutex;

   typedef struct {
       float position;
       float velocity;
       uint32_t timestamp;
   } sensor_data_t;

   sensor_data_t sensor_data;

   // Highest priority: Motor control (shortest period)
   void motor_control_task(void *pvParameters)
   {
       TickType_t xLastWakeTime = xTaskGetTickCount();
       const TickType_t xPeriod = pdMS_TO_TICKS(MOTOR_CONTROL_PERIOD_MS);
       
       for(;;)
       {
           vTaskDelayUntil(&xLastWakeTime, xPeriod);
           
           // Read sensor data (protected by mutex with priority inheritance)
           xSemaphoreTake(xSensorDataMutex, portMAX_DELAY);
           float current_position = sensor_data.position;
           float current_velocity = sensor_data.velocity;
           xSemaphoreGive(xSensorDataMutex);
           
           // PID control calculation
           float control_output = calculate_pid(current_position, current_velocity);
           
           // Update PWM
           set_motor_pwm(control_output);
       }
   }

   // Medium priority: Sensor reading
   void sensor_read_task(void *pvParameters)
   {
       TickType_t xLastWakeTime = xTaskGetTickCount();
       const TickType_t xPeriod = pdMS_TO_TICKS(SENSOR_READ_PERIOD_MS);
       
       for(;;)
       {
           vTaskDelayUntil(&xLastWakeTime, xPeriod);
           
           // Read sensors (deterministic I2C transaction)
           sensor_data_t new_data;
           new_data.position = read_position_sensor();
           new_data.velocity = read_velocity_sensor();
           new_data.timestamp = xTaskGetTickCount();
           
           // Update shared data (mutex protects against priority inversion)
           xSemaphoreTake(xSensorDataMutex, pdMS_TO_TICKS(5));
           sensor_data = new_data;
           xSemaphoreGive(xSensorDataMutex);
       }
   }

   // Lowest priority: Display update
   void display_update_task(void *pvParameters)
   {
       TickType_t xLastWakeTime = xTaskGetTickCount();
       const TickType_t xPeriod = pdMS_TO_TICKS(DISPLAY_UPDATE_PERIOD_MS);
       
       for(;;)
       {
           vTaskDelayUntil(&xLastWakeTime, xPeriod);
           
           // Read sensor data for display
           xSemaphoreTake(xSensorDataMutex, pdMS_TO_TICKS(10));
           sensor_data_t display_data = sensor_data;
           xSemaphoreGive(xSensorDataMutex);
           
           // Update display (slow operation, but low priority is OK)
           update_lcd(display_data.position, display_data.velocity);
       }
   }

   void motor_system_init(void)
   {
       // Create mutex with priority inheritance
       xSensorDataMutex = xSemaphoreCreateMutex();
       
       // Check schedulability:
       // U = 2/10 + 5/20 + 15/100 = 0.2 + 0.25 + 0.15 = 0.6 = 60%
       // RMS bound for 3 tasks = 78%
       // 60% < 78% → SCHEDULABLE!
       
       // Create tasks with RMS priorities
       xTaskCreate(motor_control_task, \"Motor\", 512, NULL, 5, NULL);
       xTaskCreate(sensor_read_task, \"Sensor\", 512, NULL, 3, NULL);
       xTaskCreate(display_update_task, \"Display\", 512, NULL, 1, NULL);
       
       printf(\"Motor control system initialized\\n\");
       printf(\"CPU Utilization: 60%%\\n\");
       printf(\"System is schedulable (60%% < 78%% RMS bound)\\n\");
   }

Summary
-------

**Key Concepts:**

1. **Scheduling Algorithms:**
   
   - Priority-based preemptive (most common)
   - Round-robin for equal priority
   - RMS for periodic tasks
   - EDF for maximum utilization

2. **Schedulability Analysis:**
   
   - Utilization test (sufficient condition)
   - Response time analysis (exact test)
   - Always verify before deployment

3. **Determinism:**
   
   - Use static allocation
   - Avoid unbounded operations
   - Use timeouts everywhere
   - Prefer fixed-point over floating-point

4. **Priority Inversion:**
   
   - Use mutexes (not binary semaphores) for shared resources
   - Enable priority inheritance
   - Consider priority ceiling protocol

5. **Timing Analysis:**
   
   - Measure jitter and latency
   - Use ``vTaskDelayUntil()`` for periodic tasks
   - Instrument critical paths

**Best Practices:**

- Calculate CPU utilization before implementation
- Assign priorities using RMS for periodic tasks
- Always use bounded operations with timeouts
- Protect shared resources with priority inheritance mutexes
- Measure actual timing behavior in the target system
- Leave margin for interrupt handling and context switching

Mastering scheduling and determinism enables building real-time systems that **always** meet their deadlines.
