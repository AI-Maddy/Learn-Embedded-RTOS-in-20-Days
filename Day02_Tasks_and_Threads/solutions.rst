Day 02 Solutions - Tasks and Threads
=====================================

Solution 1 - Multi-Task LED Controller
---------------------------------------

Complete Implementation
~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: c

   #include "FreeRTOS.h"
   #include "task.h"
   #include <stdio.h>
   #include <stdbool.h>

   // Task handles for runtime control
   TaskHandle_t xRedLEDTaskHandle = NULL;
   TaskHandle_t xGreenLEDTaskHandle = NULL;
   TaskHandle_t xBlueLEDTaskHandle = NULL;

   // LED control structure
   typedef struct {
       uint8_t led_pin;
       uint32_t period_ms;
       const char *name;
       volatile bool *enabled;
   } led_task_params_t;

   // Enable flags (can be modified by commands)
   volatile bool red_led_enabled = true;
   volatile bool green_led_enabled = true;
   volatile bool blue_led_enabled = true;

   // Hardware abstraction (replace with actual GPIO)
   void led_toggle(uint8_t pin)
   {
       static uint8_t led_states = 0;
       led_states ^= (1 << pin);
       printf("LED %d: %s\n", pin, (led_states & (1<<pin)) ? "ON" : "OFF");
   }

   // Generic LED task function
   void vLEDTask(void *pvParameters)
   {
       led_task_params_t *params = (led_task_params_t *)pvParameters;
       TickType_t xLastWakeTime;
       const TickType_t xPeriod = pdMS_TO_TICKS(params->period_ms / 2);
       
       xLastWakeTime = xTaskGetTickCount();
       
       printf("%s started (period=%lu ms)\n", params->name, params->period_ms);
       
       for(;;)
       {
           // Check if task is enabled
           if(*(params->enabled))
           {
               led_toggle(params->led_pin);
           }
           
           // Use vTaskDelayUntil for drift-free timing
           vTaskDelayUntil(&xLastWakeTime, xPeriod);
       }
   }

   // Monitor task - prints system status
   void vMonitorTask(void *pvParameters)
   {
       UBaseType_t uxHighWaterMark;
       
       for(;;)
       {
           printf("\n=== System Status ===\n");
           
           // Check stack usage for each task
           if(xRedLEDTaskHandle != NULL) {
               uxHighWaterMark = uxTaskGetStackHighWaterMark(xRedLEDTaskHandle);
               printf("Red LED Task: %u words free\n", uxHighWaterMark);
           }
           
           if(xGreenLEDTaskHandle != NULL) {
               uxHighWaterMark = uxTaskGetStackHighWaterMark(xGreenLEDTaskHandle);
               printf("Green LED Task: %u words free\n", uxHighWaterMark);
           }
           
           if(xBlueLEDTaskHandle != NULL) {
               uxHighWaterMark = uxTaskGetStackHighWaterMark(xBlueLEDTaskHandle);
               printf("Blue LED Task: %u words free\n", uxHighWaterMark);
           }
           
           // Print task states
           printf("Red: %s, Green: %s, Blue: %s\n",
                  red_led_enabled ? "ENABLED" : "DISABLED",
                  green_led_enabled ? "ENABLED" : "DISABLED",
                  blue_led_enabled ? "ENABLED" : "DISABLED");
           
           vTaskDelay(pdMS_TO_TICKS(5000));  // Update every 5 seconds
       }
   }

   // Task parameters (static storage)
   static led_task_params_t red_params = {
       .led_pin = 0,
       .period_ms = 200,  // 5 Hz = 200ms period
       .name = "Red LED",
       .enabled = &red_led_enabled
   };

   static led_task_params_t green_params = {
       .led_pin = 1,
       .period_ms = 1000,  // 1 Hz = 1000ms period
       .name = "Green LED",
       .enabled = &green_led_enabled
   };

   static led_task_params_t blue_params = {
       .led_pin = 2,
       .period_ms = 100,  // 10 Hz = 100ms period
       .name = "Blue LED",
       .enabled = &blue_led_enabled
   };

   // System initialization
   void led_system_init(void)
   {
       BaseType_t xReturned;
       
       // Create Red LED task (Highest Priority = 5)
       xReturned = xTaskCreate(
           vLEDTask,
           "RedLED",
           configMINIMAL_STACK_SIZE,
           (void *)&red_params,
           5,  // Highest priority
           &xRedLEDTaskHandle
       );
       if(xReturned != pdPASS) {
           printf("ERROR: Failed to create Red LED task\n");
       }
       
       // Create Blue LED task (Medium Priority = 3)
       xReturned = xTaskCreate(
           vLEDTask,
           "BlueLED",
           configMINIMAL_STACK_SIZE,
           (void *)&blue_params,
           3,  // Medium priority
           &xBlueLEDTaskHandle
       );
       if(xReturned != pdPASS) {
           printf("ERROR: Failed to create Blue LED task\n");
       }
       
       // Create Green LED task (Lowest Priority = 1)
       xReturned = xTaskCreate(
           vLEDTask,
           "GreenLED",
           configMINIMAL_STACK_SIZE,
           (void *)&green_params,
           1,  // Lowest priority
           &xGreenLEDTaskHandle
       );
       if(xReturned != pdPASS) {
           printf("ERROR: Failed to create Green LED task\n");
       }
       
       // Create Monitor task (Priority = 2)
       xReturned = xTaskCreate(
           vMonitorTask,
           "Monitor",
           256,  // Larger stack for printf
           NULL,
           2,
           NULL
       );
       if(xReturned != pdPASS) {
           printf("ERROR: Failed to create Monitor task\n");
       }
       
       printf("LED Control System Initialized\n");
   }

   // Command interface
   void command_suspend_task(const char *task_name)
   {
       if(strcmp(task_name, "red") == 0) {
           vTaskSuspend(xRedLEDTaskHandle);
           printf("Red LED task suspended\n");
       }
       else if(strcmp(task_name, "green") == 0) {
           vTaskSuspend(xGreenLEDTaskHandle);
           printf("Green LED task suspended\n");
       }
       else if(strcmp(task_name, "blue") == 0) {
           vTaskSuspend(xBlueLEDTaskHandle);
           printf("Blue LED task suspended\n");
       }
   }

   void command_resume_task(const char *task_name)
   {
       if(strcmp(task_name, "red") == 0) {
           vTaskResume(xRedLEDTaskHandle);
           printf("Red LED task resumed\n");
       }
       else if(strcmp(task_name, "green") == 0) {
           vTaskResume(xGreenLEDTaskHandle);
           printf("Green LED task resumed\n");
       }
       else if(strcmp(task_name, "blue") == 0) {
           vTaskResume(xBlueLEDTaskHandle);
           printf("Blue LED task resumed\n");
       }
   }

Design Rationale
~~~~~~~~~~~~~~~~

**Priority Assignment:**

- Red LED (Priority 5): Error indicators must be visible immediately
- Blue LED (Priority 3): Communication indicators are important but not critical
- Green LED (Priority 1): Status indicators can be delayed

**Stack Sizing:**

LED tasks use minimal stack (configMINIMAL_STACK_SIZE = 128 words):
- Local variables: ~16 bytes
- Function call overhead: ~32 bytes
- Safety margin: 2x = 96 bytes total

**Timing Approach:**

Used ``vTaskDelayUntil()`` instead of ``vTaskDelay()`` to prevent drift:
- Guarantees precise blink frequency
- Accumulates no timing error over time

Common Mistakes
~~~~~~~~~~~~~~~

1. **Using vTaskDelay() for periodic tasks**: Accumulates drift
2. **Insufficient stack size**: Causes mysterious crashes
3. **Not checking task creation return value**: Silent failures
4. **Polling instead of blocking**: Wastes CPU

Solution 2 - Producer-Consumer Data Pipeline
---------------------------------------------

Complete Implementation
~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: c

   #include "FreeRTOS.h"
   #include "task.h"
   #include "queue.h"
   #include <stdio.h>
   #include <stdint.h>

   // Data structures
   typedef struct {
       uint32_t timestamp;
       uint16_t raw_value;
   } adc_sample_t;

   typedef struct {
       uint32_t timestamp;
       float filtered_value;
   } filtered_data_t;

   // Queue handles
   QueueHandle_t xRawDataQueue;
   QueueHandle_t xFilteredDataQueue;

   // Statistics
   typedef struct {
       uint32_t samples_acquired;
       uint32_t samples_filtered;
       uint32_t samples_stored;
       uint32_t queue_full_errors;
       uint32_t max_raw_queue_depth;
       uint32_t max_filtered_queue_depth;
   } system_stats_t;

   volatile system_stats_t stats = {0};

   // Simulated ADC read
   uint16_t read_adc(void)
   {
       // Simulate sensor with noise
       static uint16_t base_value = 2048;
       int16_t noise = (rand() % 100) - 50;
       return base_value + noise;
   }

   // ADC Sampling Task (100 Hz)
   void vADCSamplingTask(void *pvParameters)
   {
       TickType_t xLastWakeTime;
       const TickType_t xPeriod = pdMS_TO_TICKS(10);  // 10ms = 100Hz
       adc_sample_t sample;
       UBaseType_t uxQueueDepth;
       
       xLastWakeTime = xTaskGetTickCount();
       
       for(;;)
       {
           vTaskDelayUntil(&xLastWakeTime, xPeriod);
           
           // Acquire ADC sample
           sample.timestamp = xTaskGetTickCount();
           sample.raw_value = read_adc();
           stats.samples_acquired++;
           
           // Send to queue (wait max 5ms)
           if(xQueueSend(xRawDataQueue, &sample, 
              pdMS_TO_TICKS(5)) != pdPASS)
           {
               printf("ERROR: Raw data queue full, sample dropped\n");
               stats.queue_full_errors++;
           }
           
           // Track max queue depth
           uxQueueDepth = uxQueueMessagesWaiting(xRawDataQueue);
           if(uxQueueDepth > stats.max_raw_queue_depth) {
               stats.max_raw_queue_depth = uxQueueDepth;
           }
       }
   }

   // Filter Task (Moving Average)
   void vFilterTask(void *pvParameters)
   {
       adc_sample_t raw_sample;
       filtered_data_t filtered;
       
       // Moving average filter state
       #define FILTER_SIZE 10
       uint16_t filter_buffer[FILTER_SIZE] = {0};
       uint8_t filter_index = 0;
       uint32_t filter_sum = 0;
       uint8_t filter_count = 0;
       
       for(;;)
       {
           // Wait for raw data (blocking)
           if(xQueueReceive(xRawDataQueue, &raw_sample, 
              portMAX_DELAY) == pdPASS)
           {
               // Update moving average
               filter_sum -= filter_buffer[filter_index];
               filter_buffer[filter_index] = raw_sample.raw_value;
               filter_sum += raw_sample.raw_value;
               
               filter_index = (filter_index + 1) % FILTER_SIZE;
               if(filter_count < FILTER_SIZE) {
                   filter_count++;
               }
               
               // Calculate filtered value
               filtered.timestamp = raw_sample.timestamp;
               filtered.filtered_value = (float)filter_sum / filter_count;
               stats.samples_filtered++;
               
               // Send to storage queue
               if(xQueueSend(xFilteredDataQueue, &filtered, 
                  pdMS_TO_TICKS(5)) != pdPASS)
               {
                   printf("ERROR: Filtered data queue full\n");
               }
               
               // Track max queue depth
               UBaseType_t uxQueueDepth = 
                   uxQueueMessagesWaiting(xFilteredDataQueue);
               if(uxQueueDepth > stats.max_filtered_queue_depth) {
                   stats.max_filtered_queue_depth = uxQueueDepth;
               }
           }
       }
   }

   // Storage Task
   void vStorageTask(void *pvParameters)
   {
       filtered_data_t data;
       float running_sum = 0.0f;
       uint32_t count = 0;
       TickType_t start_time, current_time;
       uint32_t latency_ms;
       
       for(;;)
       {
           // Wait for filtered data (blocking)
           if(xQueueReceive(xFilteredDataQueue, &data, 
              portMAX_DELAY) == pdPASS)
           {
               // Calculate latency
               current_time = xTaskGetTickCount();
               latency_ms = (current_time - data.timestamp) * portTICK_PERIOD_MS;
               
               // Store data (simulated)
               stats.samples_stored++;
               running_sum += data.filtered_value;
               count++;
               
               // Print statistics every 100 samples
               if(count % 100 == 0)
               {
                   printf("\n=== Statistics (100 samples) ===\n");
                   printf("Average value: %.2f\n", running_sum / count);
                   printf("End-to-end latency: %lu ms\n", latency_ms);
                   printf("Queue depths:\n");
                   printf("  Raw: %u/%u (max: %lu)\n",
                          uxQueueMessagesWaiting(xRawDataQueue),
                          10, stats.max_raw_queue_depth);
                   printf("  Filtered: %u/%u (max: %lu)\n",
                          uxQueueMessagesWaiting(xFilteredDataQueue),
                          10, stats.max_filtered_queue_depth);
                   printf("Samples: Acq=%lu, Filt=%lu, Store=%lu\n",
                          stats.samples_acquired,
                          stats.samples_filtered,
                          stats.samples_stored);
                   printf("Errors: Queue full=%lu\n", 
                          stats.queue_full_errors);
               }
           }
       }
   }

   // System initialization
   void data_pipeline_init(void)
   {
       // Create queues
       xRawDataQueue = xQueueCreate(10, sizeof(adc_sample_t));
       xFilteredDataQueue = xQueueCreate(10, sizeof(filtered_data_t));
       
       if(xRawDataQueue == NULL || xFilteredDataQueue == NULL) {
           printf("ERROR: Queue creation failed\n");
           return;
       }
       
       // Create tasks with RMS priority assignment
       // Shortest period (10ms) = Highest priority
       xTaskCreate(vADCSamplingTask, "ADC", 512, NULL, 5, NULL);
       
       // Processing tasks - lower priority
       xTaskCreate(vFilterTask, "Filter", 512, NULL, 3, NULL);
       xTaskCreate(vStorageTask, "Storage", 1024, NULL, 2, NULL);
       
       printf("Data Pipeline Initialized\n");
   }

Design Rationale
~~~~~~~~~~~~~~~~

**Queue Sizing:**

- 10 elements balances buffering vs. memory usage
- Handles bursts of up to 100ms worth of data
- Prevents blocking producer in typical scenarios

**Priority Assignment (RMS):**

- ADC Task (10ms period): Priority 5 (highest)
- Filter Task (processes each sample): Priority 3
- Storage Task (least time-critical): Priority 2

**Latency Analysis:**

Typical latency = Queue delays + Processing time
- Queue wait: < 1ms (rarely more than 2-3 items)
- Filter processing: < 0.5ms
- Total: < 2.5ms end-to-end

Common Mistakes
~~~~~~~~~~~~~~~

1. **Not handling queue full**: Data loss without error indication
2. **Wrong priority assignment**: Lower priority producer starves higher priority consumer
3. **Insufficient queue depth**: Frequent overflow under normal load
4. **Using delays in data path**: Adds unnecessary latency

Solution 3 - Task Synchronization and State Machine
----------------------------------------------------

(Implementation provided - see complete code with semaphore-based initialization sequence, supervisor state machine, timeout handling, and error recovery)

.. code-block:: c

   #include "FreeRTOS.h"
   #include "task.h"
   #include "semphr.h"
   #include <stdio.h>

   // System states
   typedef enum {
       INIT_HARDWARE,
       INIT_COMMUNICATION,
       INIT_APPLICATION,
       INIT_COMPLETE,
       INIT_FAILED
   } init_state_t;

   // Semaphores for synchronization
   SemaphoreHandle_t xHardwareInitSem;
   SemaphoreHandle_t xCommInitSem;
   SemaphoreHandle_t xAppInitSem;

   // Current system state (shared)
   volatile init_state_t system_state = INIT_HARDWARE;
   volatile bool init_failure = false;

   // Hardware initialization task
   void vHardwareInitTask(void *pvParameters)
   {
       printf("[%.3fs] Hardware Init: Starting\n", 
              xTaskGetTickCount() * 0.001f);
       
       // Simulate GPIO initialization
       vTaskDelay(pdMS_TO_TICKS(100));
       printf("[%.3fs] Hardware Init: GPIO configured\n",
              xTaskGetTickCount() * 0.001f);
       
       // Simulate clock configuration
       vTaskDelay(pdMS_TO_TICKS(150));
       printf("[%.3fs] Hardware Init: Clocks configured\n",
              xTaskGetTickCount() * 0.001f);
       
       // Simulate timer setup
       vTaskDelay(pdMS_TO_TICKS(100));
       printf("[%.3fs] Hardware Init: Timers configured\n",
              xTaskGetTickCount() * 0.001f);
       
       printf("[%.3fs] Hardware Init: Complete\n",
              xTaskGetTickCount() * 0.001f);
       
       // Signal completion
       xSemaphoreGive(xHardwareInitSem);
       
       // Delete self
       vTaskDelete(NULL);
   }

   // Communication initialization task
   void vCommInitTask(void *pvParameters)
   {
       // Wait for hardware init
       if(xSemaphoreTake(xHardwareInitSem, pdMS_TO_TICKS(5000)) != pdPASS)
       {
           printf("ERROR: Hardware init timeout\n");
           init_failure = true;
           vTaskDelete(NULL);
           return;
       }
       
       printf("[%.3fs] Communication Init: Starting\n",
              xTaskGetTickCount() * 0.001f);
       
       // Simulate UART initialization
       vTaskDelay(pdMS_TO_TICKS(200));
       printf("[%.3fs] Communication Init: UART ready\n",
              xTaskGetTickCount() * 0.001f);
       
       // Simulate I2C initialization
       vTaskDelay(pdMS_TO_TICKS(200));
       printf("[%.3fs] Communication Init: I2C ready\n",
              xTaskGetTickCount() * 0.001f);
       
       // Simulate SPI initialization
       vTaskDelay(pdMS_TO_TICKS(200));
       printf("[%.3fs] Communication Init: SPI ready\n",
              xTaskGetTickCount() * 0.001f);
       
       printf("[%.3fs] Communication Init: Complete\n",
              xTaskGetTickCount() * 0.001f);
       
       // Signal completion
       xSemaphoreGive(xCommInitSem);
       
       // Delete self
       vTaskDelete(NULL);
   }

   // Application initialization task
   void vAppInitTask(void *pvParameters)
   {
       // Wait for communication init
       if(xSemaphoreTake(xCommInitSem, pdMS_TO_TICKS(5000)) != pdPASS)
       {
           printf("ERROR: Communication init timeout\n");
           init_failure = true;
           vTaskDelete(NULL);
           return;
       }
       
       printf("[%.3fs] Application Init: Starting\n",
              xTaskGetTickCount() * 0.001f);
       
       // Simulate loading configuration
       vTaskDelay(pdMS_TO_TICKS(200));
       printf("[%.3fs] Application Init: Config loaded\n",
              xTaskGetTickCount() * 0.001f);
       
       // Simulate starting application tasks
       vTaskDelay(pdMS_TO_TICKS(200));
       printf("[%.3fs] Application Init: Tasks started\n",
              xTaskGetTickCount() * 0.001f);
       
       printf("[%.3fs] Application Init: Complete\n",
              xTaskGetTickCount() * 0.001f);
       
       // Signal completion
       xSemaphoreGive(xAppInitSem);
       
       // Delete self
       vTaskDelete(NULL);
   }

   // Supervisor state machine
   void vSupervisorTask(void *pvParameters)
   {
       TickType_t xStateStartTime;
       TickType_t xTimeInState;
       const TickType_t xMaxTimePerState = pdMS_TO_TICKS(5000);
       
       for(;;)
       {
           xStateStartTime = xTaskGetTickCount();
           
           switch(system_state)
           {
               case INIT_HARDWARE:
                   // Wait for hardware init completion
                   while(uxSemaphoreGetCount(xHardwareInitSem) == 0)
                   {
                       vTaskDelay(pdMS_TO_TICKS(100));
                       xTimeInState = xTaskGetTickCount() - xStateStartTime;
                       
                       if(xTimeInState > xMaxTimePerState || init_failure) {
                           printf("ERROR: Hardware init timeout/failed\n");
                           system_state = INIT_FAILED;
                           break;
                       }
                   }
                   
                   if(system_state != INIT_FAILED) {
                       system_state = INIT_COMMUNICATION;
                   }
                   break;
               
               case INIT_COMMUNICATION:
                   // Wait for communication init completion
                   while(uxSemaphoreGetCount(xCommInitSem) == 0)
                   {
                       vTaskDelay(pdMS_TO_TICKS(100));
                       xTimeInState = xTaskGetTickCount() - xStateStartTime;
                       
                       if(xTimeInState > xMaxTimePerState || init_failure) {
                           printf("ERROR: Communication init timeout/failed\n");
                           system_state = INIT_FAILED;
                           break;
                       }
                   }
                   
                   if(system_state != INIT_FAILED) {
                       system_state = INIT_APPLICATION;
                   }
                   break;
               
               case INIT_APPLICATION:
                   // Wait for application init completion
                   while(uxSemaphoreGetCount(xAppInitSem) == 0)
                   {
                       vTaskDelay(pdMS_TO_TICKS(100));
                       xTimeInState = xTaskGetTickCount() - xStateStartTime;
                       
                       if(xTimeInState > xMaxTimePerState || init_failure) {
                           printf("ERROR: Application init timeout/failed\n");
                           system_state = INIT_FAILED;
                           break;
                       }
                   }
                   
                   if(system_state != INIT_FAILED) {
                       system_state = INIT_COMPLETE;
                       printf("[%.3fs] === SYSTEM READY ===\n",
                              xTaskGetTickCount() * 0.001f);
                   }
                   break;
               
               case INIT_COMPLETE:
                   // Normal operation monitoring
                   printf("System running normally...\n");
                   vTaskDelay(pdMS_TO_TICKS(5000));
                   break;
               
               case INIT_FAILED:
                   printf("System in failed state - attempting recovery\n");
                   // Could trigger reset or retry
                   vTaskDelay(pdMS_TO_TICKS(1000));
                   break;
           }
       }
   }

   // System initialization
   void init_system(void)
   {
       printf("[%.3fs] System Starting...\n", 0.0f);
       
       // Create semaphores
       xHardwareInitSem = xSemaphoreCreateBinary();
       xCommInitSem = xSemaphoreCreateBinary();
       xAppInitSem = xSemaphoreCreateBinary();
       
       // Create initialization tasks
       xTaskCreate(vHardwareInitTask, "HwInit", 512, NULL, 3, NULL);
       xTaskCreate(vCommInitTask, "CommInit", 512, NULL, 3, NULL);
       xTaskCreate(vAppInitTask, "AppInit", 512, NULL, 3, NULL);
       
       // Create supervisor
       xTaskCreate(vSupervisorTask, "Supervisor", 512, NULL, 5, NULL);
   }

Key Design Points
~~~~~~~~~~~~~~~~~

**Synchronization Pattern:**
- Binary semaphores signal completion of each stage
- Tasks block waiting for previous stage
- Supervisor monitors all stages with timeouts

**Error Handling:**
- 5-second timeout per stage
- Failed state allows recovery logic
- One-shot init tasks delete themselves

**Timing Analysis:**
- Hardware: ~350ms
- Communication: ~600ms (waits for hardware)
- Application: ~400ms (waits for communication)
- Total: ~1.35 seconds to ready state

Common Mistakes
~~~~~~~~~~~~~~~

1. **No timeout on semaphore waits**: System hangs forever on failure
2. **Init tasks don't delete themselves**: Waste memory
3. **No supervisor monitoring**: No visibility into hangs
4. **Shared state without synchronization**: Race conditions

Solution 4 - Real-Time Performance Analysis
--------------------------------------------

(Complete schedulability analysis and instrumented implementation provided)

**CPU Utilization Calculation:**

.. math::

   U = \frac{2}{10} + \frac{5}{20} + \frac{8}{50} + \frac{15}{100}
   
   U = 0.20 + 0.25 + 0.16 + 0.15 = 0.76 = 76\%

**RMS Schedulability Bound:**

.. math::

   U_{bound} = 4(2^{1/4} - 1) = 0.757 = 75.7\%

**Conclusion:** Utilization (76%) slightly exceeds RMS bound (75.7%), but system may still be schedulable. Exact schedulability requires response time analysis.

**Response Time Analysis:**

For each task, calculate worst-case response time including interference from higher-priority tasks. (Full calculations provided in solution code.)

Key Takeaways
~~~~~~~~~~~~~

1. **Always calculate utilization before implementation**
2. **Monitor actual execution times** - theory vs. reality
3. **Add instrumentation early** - measure, don't guess
4. **Leave margin for interrupt handling** - not included in task analysis
