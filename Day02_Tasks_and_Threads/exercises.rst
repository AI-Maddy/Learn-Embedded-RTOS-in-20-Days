Day 02 Exercises - Tasks and Threads
=====================================

Exercise 1 - Multi-Task LED Controller
---------------------------------------

Objective
~~~~~~~~~
Create a system with three tasks that control different LEDs with varying blink rates, demonstrating task priorities and timing.

Scenario
~~~~~~~~
You're building a status indicator system for an embedded device that uses three LEDs:

- **Red LED**: Error indicator (fast blink at 5 Hz when active)
- **Green LED**: Normal operation (slow blink at 1 Hz)
- **Blue LED**: Communication activity (rapid blink at 10 Hz during transfers)

Each LED must be controlled by a separate task with appropriate priority.

Tasks
~~~~~

1. **Create three tasks** for LED control:
   
   a. Design task functions for Red, Green, and Blue LED control
   b. Assign appropriate priorities:
      
      - Error indicator (Red) = Highest priority
      - Communication indicator (Blue) = Medium priority
      - Status indicator (Green) = Lowest priority
   
   c. Implement proper timing using ``vTaskDelay()`` or ``vTaskDelayUntil()``

2. **Implement task creation** in your main/init function:
   
   a. Calculate appropriate stack sizes for each task
   b. Use descriptive task names for debugging
   c. Store task handles for later control

3. **Add runtime control**:
   
   a. Implement a command interface to suspend/resume individual LED tasks
   b. Add ability to change blink rates dynamically
   c. Implement graceful task deletion

4. **Monitor system behavior**:
   
   a. Print task state information periodically
   b. Monitor stack high-water marks
   c. Measure actual blink timing accuracy

Expected Output
~~~~~~~~~~~~~~~

- Three LEDs blinking at specified rates
- Red LED always responds immediately (never blocked by lower priority tasks)
- Console output showing task states and stack usage
- Ability to control tasks via commands

Deliverables
~~~~~~~~~~~~

1. Complete source code with all three tasks
2. Initialization function with proper task creation
3. Documentation of priority assignments and rationale
4. Test results showing timing accuracy and priority behavior

Exercise 2 - Producer-Consumer Data Pipeline
---------------------------------------------

Objective
~~~~~~~~~
Implement a multi-task data processing pipeline using queues to pass data between producer and consumer tasks.

Scenario
~~~~~~~~
Design a sensor data acquisition system:

- **ADC Sampling Task**: Reads analog sensor at 100 Hz
- **Filter Task**: Applies moving average filter
- **Storage Task**: Logs filtered data to memory

Data flows through queues: ADC → Filter → Storage

Tasks
~~~~~

1. **Implement ADC Sampling Task**:
   
   .. code-block:: c
   
      // Define data structure
      typedef struct {
          uint32_t timestamp;
          uint16_t raw_value;
      } adc_sample_t;
   
      void adc_sampling_task(void *pvParameters)
      {
          // TODO: Implement periodic ADC sampling
          // - Use vTaskDelayUntil for precise 10ms timing
          // - Read ADC value (or simulate)
          // - Send to raw_data_queue
          // - Handle queue full condition
      }

2. **Implement Filter Task**:
   
   .. code-block:: c
   
      typedef struct {
          uint32_t timestamp;
          float filtered_value;
      } filtered_data_t;
   
      void filter_task(void *pvParameters)
      {
          // TODO: Implement moving average filter
          // - Receive from raw_data_queue (blocking)
          // - Apply 10-sample moving average
          // - Send to filtered_data_queue
      }

3. **Implement Storage Task**:
   
   .. code-block:: c
   
      void storage_task(void *pvParameters)
      {
          // TODO: Implement data logging
          // - Receive from filtered_data_queue (blocking)
          // - Write to circular buffer or file
          // - Print statistics every 100 samples
      }

4. **System Integration**:
   
   a. Create two queues with appropriate depths
   b. Assign priorities using Rate Monotonic Scheduling principles
   c. Size stacks appropriately for each task
   d. Add error handling for queue send/receive failures

5. **Performance Analysis**:
   
   a. Measure end-to-end latency (ADC sample to storage)
   b. Monitor queue occupancy levels
   c. Check for data loss or queue overflows
   d. Verify 100 Hz sampling rate is maintained

Expected Output
~~~~~~~~~~~~~~~

.. code-block:: text

   ADC Task: Sample 100, Raw=2048
   Filter Task: Filtered value = 2045.3
   Storage Task: Logged 100 samples, avg=2044.2
   
   Queue Stats:
     Raw Queue: 2/10 used (max: 5)
     Filtered Queue: 1/10 used (max: 3)
   
   Latency: 2.5 ms (ADC to Storage)
   Stack Usage:
     ADC Task: 180/512 bytes
     Filter Task: 240/512 bytes
     Storage Task: 320/1024 bytes

Deliverables
~~~~~~~~~~~~

1. Complete source code for all three tasks
2. Queue definitions and creation code
3. Performance measurements and analysis
4. Documentation of design decisions

Exercise 3 - Task Synchronization and State Machine
----------------------------------------------------

Objective
~~~~~~~~~
Implement a multi-task system initialization sequence with proper synchronization and a state machine task.

Scenario
~~~~~~~~
Build a system startup controller that:

1. Performs initialization in stages (Hardware → Communication → Application)
2. Each stage is a separate task
3. Tasks must execute in order with proper synchronization
4. A supervisor task monitors overall system state

Tasks
~~~~~

1. **Create Initialization Tasks**:
   
   .. code-block:: c
   
      typedef enum {
          INIT_HARDWARE,
          INIT_COMMUNICATION,
          INIT_APPLICATION,
          INIT_COMPLETE,
          INIT_FAILED
      } init_state_t;
   
      // Hardware init task
      void hardware_init_task(void *pvParameters)
      {
          // TODO:
          // - Initialize GPIO, clocks, timers
          // - Signal completion via semaphore
          // - Delete self when done
      }
   
      // Communication init task
      void comm_init_task(void *pvParameters)
      {
          // TODO:
          // - Wait for hardware init semaphore
          // - Initialize UART, I2C, SPI
          // - Signal completion
          // - Delete self
      }
   
      // Application init task
      void app_init_task(void *pvParameters)
      {
          // TODO:
          // - Wait for communication init
          // - Load configuration
          // - Start application tasks
          // - Signal completion
          // - Delete self
      }

2. **Implement Supervisor State Machine**:
   
   .. code-block:: c
   
      void supervisor_task(void *pvParameters)
      {
          init_state_t state = INIT_HARDWARE;
          uint32_t timeout_counter;
          
          for(;;)
          {
              switch(state) {
                  case INIT_HARDWARE:
                      // TODO: Monitor hardware init
                      // Transition to next state or handle timeout
                      break;
                  
                  case INIT_COMMUNICATION:
                      // TODO: Monitor comm init
                      break;
                  
                  case INIT_APPLICATION:
                      // TODO: Monitor app init
                      break;
                  
                  case INIT_COMPLETE:
                      // TODO: Normal operation monitoring
                      break;
                  
                  case INIT_FAILED:
                      // TODO: Error recovery
                      break;
              }
              
              vTaskDelay(pdMS_TO_TICKS(100));
          }
      }

3. **Add Synchronization Mechanisms**:
   
   a. Create binary semaphores for each init stage
   b. Implement timeout detection (max 5 seconds per stage)
   c. Add error handling and retry logic
   d. Provide visual feedback (LEDs or console)

4. **Implement Watchdog Monitoring**:
   
   a. Each task must "pet" the watchdog during execution
   b. Supervisor detects stuck tasks
   c. System resets if any task hangs

5. **Test Failure Scenarios**:
   
   a. Simulate timeout in communication init
   b. Test recovery mechanisms
   c. Verify proper cleanup on failure

Expected Output
~~~~~~~~~~~~~~~

.. code-block:: text

   [0.000s] System Starting...
   [0.100s] Hardware Init: Starting
   [0.250s] Hardware Init: GPIO configured
   [0.400s] Hardware Init: Clocks configured
   [0.500s] Hardware Init: Complete
   [0.600s] Communication Init: Starting
   [0.800s] Communication Init: UART ready
   [1.000s] Communication Init: I2C ready
   [1.200s] Communication Init: Complete
   [1.300s] Application Init: Starting
   [1.500s] Application Init: Config loaded
   [1.700s] Application Init: Tasks started
   [1.800s] Application Init: Complete
   [1.900s] System Ready - All tasks operational
   
   Task List:
     Supervisor    : Running, Priority=5
     SensorTask    : Ready, Priority=3
     ControlTask   : Ready, Priority=4
     DisplayTask   : Ready, Priority=2

Deliverables
~~~~~~~~~~~~

1. Complete multi-stage initialization code
2. Supervisor state machine implementation
3. Synchronization design documentation
4. Test results for nominal and failure scenarios
5. Timing diagram showing task execution sequence

Exercise 4 - Real-Time Performance Analysis
--------------------------------------------

Objective
~~~~~~~~~
Analyze and optimize task performance in a constrained system.

Scenario
~~~~~~~~
You have a system with four tasks running on a 48 MHz Cortex-M4:

- Task A: 10 ms period, 2 ms execution time, Priority 4 (highest)
- Task B: 20 ms period, 5 ms execution time, Priority 3
- Task C: 50 ms period, 8 ms execution time, Priority 2
- Task D: 100 ms period, 15 ms execution time, Priority 1 (lowest)

Tasks
~~~~~

1. **Calculate CPU Utilization**:
   
   Use the formula:
   
   .. math::
   
      U = \sum_{i=1}^{n} \frac{C_i}{T_i}
   
   where C_i is execution time and T_i is period.

2. **Determine Schedulability**:
   
   Apply Rate Monotonic Scheduling analysis:
   
   .. math::
   
      U \leq n(2^{1/n} - 1)
   
   Is this task set schedulable?

3. **Implement the Task Set**:
   
   Create all four tasks with simulated execution times using delay loops or actual work.

4. **Add Instrumentation**:
   
   .. code-block:: c
   
      // Measure actual execution times
      void task_a(void *pvParameters)
      {
          TickType_t xStartTime, xEndTime, xExecutionTime;
          
          for(;;)
          {
              xStartTime = xTaskGetTickCount();
              
              // Do work
              perform_task_a_work();
              
              xEndTime = xTaskGetTickCount();
              xExecutionTime = xEndTime - xStartTime;
              
              printf("Task A: Exec time = %lu ms\n", xExecutionTime);
              
              vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(10));
          }
      }

5. **Analyze Results**:
   
   a. Measure actual vs. theoretical CPU utilization
   b. Check for deadline misses
   c. Identify context switch overhead
   d. Measure worst-case response times

6. **Optimization Challenge**:
   
   Reduce Task D's execution time to fit in available CPU time if system is overloaded.

Expected Output
~~~~~~~~~~~~~~~

.. code-block:: text

   CPU Utilization Analysis:
     Task A: 2ms / 10ms = 20.0%
     Task B: 5ms / 20ms = 25.0%
     Task C: 8ms / 50ms = 16.0%
     Task D: 15ms / 100ms = 15.0%
     Total Utilization: 76.0%
   
   Schedulability: PASS (76% < 75.7% RMS bound)
   
   Runtime Measurements:
     Task A: Avg=2.1ms, Max=2.3ms, Misses=0
     Task B: Avg=5.2ms, Max=5.8ms, Misses=0
     Task C: Avg=8.1ms, Max=8.9ms, Misses=0
     Task D: Avg=15.5ms, Max=16.2ms, Misses=0
   
   Context Switches: 450 per second
   Idle Time: 22%

Deliverables
~~~~~~~~~~~~

1. Schedulability analysis calculations
2. Instrumented task implementations
3. Performance measurement data
4. Analysis report with optimization recommendations
