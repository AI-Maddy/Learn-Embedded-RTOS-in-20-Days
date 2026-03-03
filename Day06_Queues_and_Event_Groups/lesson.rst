Day 06 Lesson - Queues and Event Groups
========================================

Introduction
------------

Queues and event groups are IPC (Inter-Process Communication) mechanisms enabling tasks to exchange data and synchronize on multiple conditions. These are essential for building loosely-coupled, maintainable systems.

Message Queues
--------------

**Queue** is a FIFO buffer for passing data between tasks.

Key features:
- Thread-safe (no explicit locking needed)
- Blocking send/receive with timeout
- Copy-based (data is copied into/out of queue)

.. code-block:: c

   typedef struct {
       uint32_t sensor_id;
       float value;
       uint32_t timestamp;
   } sensor_reading_t;
   
   QueueHandle_t xSensorQueue;
   
   void init_sensor_queue(void)
   {
       xSensorQueue = xQueueCreate(10, sizeof(sensor_reading_t));
   }
   
   // Producer
   void sensor_task(void *pvParameters)
   {
       sensor_reading_t reading;
       for(;;)
       {
           reading.value = read_sensor();
           reading.timestamp = xTaskGetTickCount();
           
           if(xQueueSend(xSensorQueue, &reading, pdMS_TO_TICKS(100)) != pdPASS)
           {
               // Queue full - handle error
           }
           vTaskDelay(pdMS_TO_TICKS(100));
       }
   }
   
   // Consumer
   void processing_task(void *pvParameters)
   {
       sensor_reading_t reading;
       for(;;)
       {
           if(xQueueReceive(xSensorQueue, &reading, portMAX_DELAY) == pdPASS)
           {
               process_data(&reading);
           }
       }
   }

Queue Operations
----------------

.. code-block:: c

   // Send to back (normal)
   xQueueSend(queue, &item, timeout);
   
   // Send to front (priority)
   xQueueSendToFront(queue, &item, timeout);
   
   // Non-blocking check
   if(uxQueueMessagesWaiting(queue) > 0)
   {
       xQueueReceive(queue, &item, 0);
   }
   
   // Peek without removing
   xQueuePeek(queue, &item, timeout);
   
   // From ISR
   xQueueSendFromISR(queue, &item, &xHigherPriorityTaskWoken);

Event Groups
------------

**Event group** allows tasks to wait for multiple events (bits) simultaneously.

Use cases:
- Wait for multiple conditions before proceeding
- Broadcast events to multiple tasks
- Complex synchronization patterns

.. code-block:: c

   EventGroupHandle_t xEventGroup;
   
   #define EVENT_SENSOR_READY   (1 << 0)
   #define EVENT_COMM_READY     (1 << 1)
   #define EVENT_DATA_RECEIVED  (1 << 2)
   
   void init_events(void)
   {
       xEventGroup = xEventGroupCreate();
   }
   
   // Set events (from ISR or task)
   void sensor_isr(void)
   {
       BaseType_t xHigherPriorityTaskWoken = pdFALSE;
       xEventGroupSetBitsFromISR(xEventGroup, EVENT_SENSOR_READY, 
                                  &xHigherPriorityTaskWoken);
       portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
   }
   
   // Wait for multiple events
   void processing_task(void *pvParameters)
   {
       EventBits_t uxBits;
       const EventBits_t uxAllEvents = EVENT_SENSOR_READY | 
                                        EVENT_COMM_READY |
                                        EVENT_DATA_RECEIVED;
       
       for(;;)
       {
           // Wait for ALL three events
           uxBits = xEventGroupWaitBits(
               xEventGroup,
               uxAllEvents,
               pdTRUE,  // Clear bits on exit
               pdTRUE,  // Wait for ALL bits
               portMAX_DELAY
           );
           
           if((uxBits & uxAllEvents) == uxAllEvents)
           {
               // All events occurred
               process_complete_dataset();
           }
       }
   }

Queue Sets
----------

Monitor multiple queues simultaneously.

.. code-block:: c

   QueueSetHandle_t xQueueSet;
   QueueHandle_t xQueue1, xQueue2, xQueue3;
   
   void init_queue_set(void)
   {
       xQueue1 = xQueueCreate(10, sizeof(uint32_t));
       xQueue2 = xQueueCreate(10, sizeof(uint32_t));
       xQueue3 = xQueueCreate(10, sizeof(uint32_t));
       
       xQueueSet = xQueueCreateSet(30);  // Total items
       
       xQueueAddToSet(xQueue1, xQueueSet);
       xQueueAddToSet(xQueue2, xQueueSet);
       xQueueAddToSet(xQueue3, xQueueSet);
   }
   
   void monitor_task(void *pvParameters)
   {
       QueueSetMemberHandle_t xActiveMember;
       uint32_t value;
       
       for(;;)
       {
           xActiveMember = xQueueSelectFromSet(xQueueSet, portMAX_DELAY);
           
           if(xActiveMember == xQueue1)
           {
               xQueueReceive(xQueue1, &value, 0);
               handle_queue1_data(value);
           }
           else if(xActiveMember == xQueue2)
           {
               xQueueReceive(xQueue2, &value, 0);
               handle_queue2_data(value);
           }
           // ...
       }
   }

Best Practices
--------------

1. **Size queues appropriately**: Balance buffering vs. memory
2. **Always use timeouts**: Never block indefinitely in production
3. **Copy small data**: Queues copy data, use pointers for large items
4. **Use event groups for synchronization**: Not for data transfer
5. **Monitor queue depth**: Detect overflow conditions early

Summary
-------

- **Queues**: Thread-safe FIFO for data transfer between tasks
- **Event Groups**: Synchronize on multiple conditions
- **Queue Sets**: Monitor multiple queues with single task
