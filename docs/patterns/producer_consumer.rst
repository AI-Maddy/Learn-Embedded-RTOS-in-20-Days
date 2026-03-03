==================
Producer-Consumer
==================

Introduction
============

The **producer-consumer pattern** is one of the most common patterns in RTOS applications. It decouples data production from data consumption, allowing tasks to operate independently at different rates.

**Key Benefits:**
- Decouples producers from consumers
- Handles rate mismatches
- Provides buffering for bursty data
- Simplifies concurrent design

Pattern Overview
================

Concept
-------

.. code-block:: text

    Producer Task(s)       Queue/Buffer       Consumer Task(s)
    ───────────────      ────────────────    ────────────────
         │                     │                    │
         │  Produce data       │                    │
         ├────────────────────►│                    │
         │                     │                    │
         │                     │   Consume data     │
         │                     │◄───────────────────┤
         │                     │                    │

**Components:**
- **Producer**: Generates data (sensors, network, user input)
- **Buffer/Queue**: Temporary storage between producer and consumer
- **Consumer**: Processes data (compute, display, storage)

Basic Implementation
====================

Using FreeRTOS Queue
--------------------

.. code-block:: c

    #include "FreeRTOS.h"
    #include "queue.h"
    #include "task.h"
    
    // Message structure
    typedef struct {
        uint8_t sensor_id;
        int16_t temperature;
        uint32_t timestamp;
    } sensor_data_t;
    
    // Queue handle
    QueueHandle_t data_queue;
    #define QUEUE_LENGTH 10
    
    // Producer task
    void producer_task(void *param) {
        sensor_data_t data;
        
        while (1) {
            // Produce data
            data.sensor_id = 1;
            data.temperature = read_temperature_sensor();
            data.timestamp = xTaskGetTickCount();
            
            // Send to queue (block if full)
            if (xQueueSend(data_queue, &data, pdMS_TO_TICKS(100)) != pdTRUE) {
                // Queue full - handle error
                error_count++;
            }
            
            // Wait before next sample
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }
    
    // Consumer task
    void consumer_task(void *param) {
        sensor_data_t data;
        
        while (1) {
            // Wait for data (block until available)
            if (xQueueReceive(data_queue, &data, portMAX_DELAY) == pdTRUE) {
                // Process data
                printf("Sensor %d: %d°C at %ums\n",
                       data.sensor_id,
                       data.temperature,
                       (unsigned)data.timestamp);
                
                // Store to flash, send over network, etc.
                log_sensor_data(&data);
            }
        }
    }
    
    // Initialization
    void init_producer_consumer(void) {
        // Create queue
        data_queue = xQueueCreate(QUEUE_LENGTH, sizeof(sensor_data_t));
        
        // Create tasks
        xTaskCreate(producer_task, "Producer", 256, NULL, 2, NULL);
        xTaskCreate(consumer_task, "Consumer", 512, NULL, 1, NULL);
    }

Multiple Producers, Single Consumer
====================================

.. code-block:: c

    // Multiple sensors producing data
    void sensor1_task(void *param) {
        sensor_data_t data = { .sensor_id = 1 };
        
        while (1) {
            data.temperature = read_sensor_1();
            data.timestamp = xTaskGetTickCount();
            xQueueSend(data_queue, &data, pdMS_TO_TICKS(100));
            vTaskDelay(pdMS_TO_TICKS(500));
        }
    }
    
    void sensor2_task(void *param) {
        sensor_data_t data = { .sensor_id = 2 };
        
        while (1) {
            data.temperature = read_sensor_2();
            data.timestamp = xTaskGetTickCount();
            xQueueSend(data_queue, &data, pdMS_TO_TICKS(100));
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }
    
    void sensor3_task(void *param) {
        sensor_data_t data = { .sensor_id = 3 };
        
        while (1) {
            data.temperature = read_sensor_3();
            data.timestamp = xTaskGetTickCount();
            xQueueSend(data_queue, &data, pdMS_TO_TICKS(100));
            vTaskDelay(pdMS_TO_TICKS(750));
        }
    }
    
    // Single consumer handles all data
    void consumer_task(void *param) {
        sensor_data_t data;
        
        while (1) {
            if (xQueueReceive(data_queue, &data, portMAX_DELAY) == pdTRUE) {
                // Handle data from any sensor
                process_sensor_data(&data);
            }
        }
    }

Single Producer, Multiple Consumers
====================================

.. code-block:: c

    // Broadcast using multiple queues
    QueueHandle_t display_queue;
    QueueHandle_t logger_queue;
    QueueHandle_t network_queue;
    
    void producer_task(void *param) {
        sensor_data_t data;
        
        while (1) {
            data.temperature = read_temperature_sensor();
            data.timestamp = xTaskGetTickCount();
            
            // Send to all consumers
            xQueueSend(display_queue, &data, 0);
            xQueueSend(logger_queue, &data, 0);
            xQueueSend(network_queue, &data, 0);
            
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }
    
    void display_consumer_task(void *param) {
        sensor_data_t data;
        while (1) {
            if (xQueueReceive(display_queue, &data, portMAX_DELAY) == pdTRUE) {
                update_display(data.temperature);
            }
        }
    }
    
    void logger_consumer_task(void *param) {
        sensor_data_t data;
        while (1) {
            if (xQueueReceive(logger_queue, &data, portMAX_DELAY) == pdTRUE) {
                log_to_flash(&data);
            }
        }
    }
    
    void network_consumer_task(void *param) {
        sensor_data_t data;
        while (1) {
            if (xQueueReceive(network_queue, &data, portMAX_DELAY) == pdTRUE) {
                send_over_network(&data);
            }
        }
    }

Buffering Strategies
====================

Fixed-Size Queue
----------------

.. code-block:: c

    // Simple fixed-size queue
    #define QUEUE_SIZE 16
    QueueHandle_t queue = xQueueCreate(QUEUE_SIZE, sizeof(message_t));
    
    // When full, oldest data is lost (drop head) or newest is rejected

Ring Buffer (Zero-Copy)
------------------------

.. code-block:: c

    #define BUFFER_SIZE 256
    
    typedef struct {
        uint8_t buffer[BUFFER_SIZE];
        volatile uint32_t write_idx;
        volatile uint32_t read_idx;
        SemaphoreHandle_t mutex;
        SemaphoreHandle_t items_available;
    } ring_buffer_t;
    
    ring_buffer_t rb;
    
    void ring_buffer_init(ring_buffer_t *rb) {
        rb->write_idx = 0;
        rb->read_idx = 0;
        rb->mutex = xSemaphoreCreateMutex();
        rb->items_available = xSemaphoreCreateCounting(BUFFER_SIZE, 0);
    }
    
    bool ring_buffer_write(ring_buffer_t *rb, uint8_t data) {
        xSemaphoreTake(rb->mutex, portMAX_DELAY);
        
        uint32_t next_write = (rb->write_idx + 1) % BUFFER_SIZE;
        
        if (next_write == rb->read_idx) {
            xSemaphoreGive(rb->mutex);
            return false;  // Buffer full
        }
        
        rb->buffer[rb->write_idx] = data;
        rb->write_idx = next_write;
        
        xSemaphoreGive(rb->mutex);
        xSemaphoreGive(rb->items_available);
        
        return true;
    }
    
    bool ring_buffer_read(ring_buffer_t *rb, uint8_t *data, TickType_t timeout) {
        if (xSemaphoreTake(rb->items_available, timeout) != pdTRUE) {
            return false;  // Timeout
        }
        
        xSemaphoreTake(rb->mutex, portMAX_DELAY);
        
        *data = rb->buffer[rb->read_idx];
        rb->read_idx = (rb->read_idx + 1) % BUFFER_SIZE;
        
        xSemaphoreGive(rb->mutex);
        
        return true;
    }

Dynamic Priority Queue
----------------------

.. code-block:: c

    typedef struct {
        uint8_t priority;  // 0 = highest
        uint8_t data[64];
        uint16_t length;
    } priority_message_t;
    
    // Consumer processes high-priority messages first
    void priority_consumer_task(void *param) {
        priority_message_t messages[QUEUE_LENGTH];
        uint32_t count = 0;
        
        while (1) {
            // Receive message
            priority_message_t msg;
            if (xQueueReceive(msg_queue, &msg, pdMS_TO_TICKS(100)) == pdTRUE) {
                messages[count++] = msg;
            }
            
            // Process all pending messages by priority
            if (count > 0) {
                // Sort by priority
                qsort(messages, count, sizeof(priority_message_t), compare_priority);
                
                // Process in order
                for (uint32_t i = 0; i < count; i++) {
                    process_message(&messages[i]);
                }
                
                count = 0;
            }
        }
    }

Rate Mismatch Handling
======================

Fast Producer, Slow Consumer
-----------------------------

.. code-block:: c

    // Producer: 100Hz, Consumer: 10Hz
    // Use larger queue or throttle producer
    
    void fast_producer_task(void *param) {
        const TickType_t period = pdMS_TO_TICKS(10);  // 100Hz
        TickType_t last_wake = xTaskGetTickCount();
        uint32_t dropped = 0;
        
        while (1) {
            data_t data = generate_data();
            
            // Non-blocking send (drop if full)
            if (xQueueSend(queue, &data, 0) != pdTRUE) {
                dropped++;
                // Optionally: keep only latest data
                xQueueReset(queue);
                xQueueSend(queue, &data, 0);
            }
            
            vTaskDelayUntil(&last_wake, period);
        }
    }
    
    void slow_consumer_task(void *param) {
        const TickType_t period = pdMS_TO_TICKS(100);  // 10Hz
        
        while (1) {
            data_t data;
            if (xQueueReceive(queue, &data, portMAX_DELAY) == pdTRUE) {
                // Time-consuming processing
                complex_processing(&data);
            }
            
            vTaskDelay(period);
        }
    }

Slow Producer, Fast Consumer
-----------------------------

.. code-block:: c

    // Consumer blocks waiting for data
    void slow_producer_task(void *param) {
        while (1) {
            // Slow data acquisition
            data_t data = read_slow_sensor();  // Takes 5 seconds
            xQueueSend(queue, &data, portMAX_DELAY);
        }
    }
    
    void fast_consumer_task(void *param) {
        data_t data;
        
        while (1) {
            // Blocks until data available
            if (xQueueReceive(queue, &data, portMAX_DELAY) == pdTRUE) {
                // Process quickly
                quick_processing(&data);
            }
        }
    }

Batch Processing
================

.. code-block:: c

    #define BATCH_SIZE 32
    
    void batching_consumer_task(void *param) {
        data_t batch[BATCH_SIZE];
        uint32_t count = 0;
        TickType_t batch_timeout = pdMS_TO_TICKS(1000);
        
        while (1) {
            // Collect batch
            while (count < BATCH_SIZE) {
                if (xQueueReceive(queue, &batch[count], batch_timeout) == pdTRUE) {
                    count++;
                } else {
                    // Timeout - process partial batch
                    break;
                }
            }
            
            // Process entire batch efficiently
            if (count > 0) {
                process_batch(batch, count);
                count = 0;
            }
        }
    }

Error Handling
==============

Queue Full
----------

.. code-block:: c

    void robust_producer_task(void *param) {
        data_t data;
        
        while (1) {
            data = generate_data();
            
            // Try to send with timeout
            if (xQueueSend(queue, &data, pdMS_TO_TICKS(500)) != pdTRUE) {
                // Queue full - handle gracefully
                
                // Option 1: Drop oldest, insert newest
                data_t dummy;
                xQueueReceive(queue, &dummy, 0);  // Remove oldest
                xQueueSend(queue, &data, 0);      // Insert newest
                
                // Option 2: Log error and continue
                error_log("Queue full, data dropped");
                
                // Option 3: Increase producer priority temporarily
                // to drain queue faster
            }
            
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }

Memory Management
-----------------

.. code-block:: c

    // Using memory pools for zero-allocation queueing
    typedef struct data_block {
        struct data_block *next;
        uint8_t data[256];
        uint16_t length;
    } data_block_t;
    
    #define POOL_SIZE 16
    data_block_t memory_pool[POOL_SIZE];
    QueueHandle_t free_blocks;
    QueueHandle_t filled_blocks;
    
    void init_memory_pool(void) {
        free_blocks = xQueueCreate(POOL_SIZE, sizeof(data_block_t *));
        filled_blocks = xQueueCreate(POOL_SIZE, sizeof(data_block_t *));
        
        // Add all blocks to free list
        for (int i = 0; i < POOL_SIZE; i++) {
            data_block_t *block = &memory_pool[i];
            xQueueSend(free_blocks, &block, 0);
        }
    }
    
    void producer_with_pool(void *param) {
        while (1) {
            // Get free block
            data_block_t *block;
            if (xQueueReceive(free_blocks, &block, pdMS_TO_TICKS(100)) == pdTRUE) {
                // Fill block
                block->length = read_data(block->data, sizeof(block->data));
                
                // Pass to consumer
                xQueueSend(filled_blocks, &block, portMAX_DELAY);
            }
            
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }
    
    void consumer_with_pool(void *param) {
        while (1) {
            data_block_t *block;
            if (xQueueReceive(filled_blocks, &block, portMAX_DELAY) == pdTRUE) {
                // Process block
                process_data(block->data, block->length);
                
                // Return to free pool
                xQueueSend(free_blocks, &block, portMAX_DELAY);
            }
        }
    }

Best Practices
==============

1. **Size queues appropriately**: Balance memory vs overflow risk
2. **Handle queue full/empty**: Always check return values
3. **Consider priorities**: Higher priority consumers drain faster
4. **Use timeouts**: Prevent indefinite blocking
5. **Monitor queue usage**: Track high-water mark
6. **Consider zero-copy**: Use pointers/pools for large data
7. **Avoid priority inversion**: Use appropriate synchronization
8. **Document assumptions**: Producer/consumer rates and constraints

Performance Monitoring
======================

.. code-block:: c

    void monitor_queue_health(void) {
        UBaseType_t items = uxQueueMessagesWaiting(data_queue);
        UBaseType_t spaces = uxQueueSpacesAvailable(data_queue);
        UBaseType_t length = items + spaces;
        
        uint32_t usage_percent = (items * 100) / length;
        
        printf("Queue: %u/%u (%u%% full)\n",
               (unsigned)items,
               (unsigned)length,
               (unsigned)usage_percent);
        
        if (usage_percent > 80) {
            printf("WARNING: Queue nearly full!\n");
        }
    }

See Also
========

- :doc:`../days/day06` - Queues and Event Groups
- :doc:`../overview/synchronization` - Queue details
- :doc:`command_dispatcher` - Command queue pattern
- :doc:`periodic_scheduler` - Periodic data generation

Further Reading
===============

- Operating System Concepts (Silberschatz et al.)
- The Little Book of Semaphores (Downey)
- FreeRTOS queue implementation documentation
