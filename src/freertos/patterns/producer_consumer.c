/**
 * @file producer_consumer.c
 * @brief Complete producer-consumer pattern implementation
 * 
 * This example demonstrates:
 * - Multiple producers and consumers
 * - Queue-based communication with backpressure
 * - Error handling and recovery
 * - Performance monitoring
 */

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include <stdio.h>
#include <stdlib.h>

/* Configuration */
#define QUEUE_LENGTH            20
#define NUM_PRODUCERS           2
#define NUM_CONSUMERS           2
#define PRODUCER_PRIORITY       (tskIDLE_PRIORITY + 2)
#define CONSUMER_PRIORITY       (tskIDLE_PRIORITY + 2)
#define MONITOR_PRIORITY        (tskIDLE_PRIORITY + 3)
#define TASK_STACK_SIZE         (configMINIMAL_STACK_SIZE * 3)

/* Data structure for queue items */
typedef struct {
    uint32_t producer_id;
    uint32_t sequence_num;
    uint32_t timestamp;
    uint32_t data;
} data_item_t;

/* Statistics structure */
typedef struct {
    uint32_t produced;
    uint32_t consumed;
    uint32_t queue_full_count;
    uint32_t queue_empty_count;
} stats_t;

/* Global variables */
static QueueHandle_t data_queue = NULL;
static stats_t stats = {0};
static SemaphoreHandle_t stats_mutex = NULL;

/**
 * @brief Update statistics (thread-safe)
 */
static void update_stats(stats_t *delta)
{
    xSemaphoreTake(stats_mutex, portMAX_DELAY);
    if (delta->produced) stats.produced += delta->produced;
    if (delta->consumed) stats.consumed += delta->consumed;
    if (delta->queue_full_count) stats.queue_full_count += delta->queue_full_count;
    if (delta->queue_empty_count) stats.queue_empty_count += delta->queue_empty_count;
    xSemaphoreGive(stats_mutex);
}

/**
 * @brief Producer task
 */
static void producer_task(void *pvParameters)
{
    uint32_t producer_id = (uint32_t)pvParameters;
    uint32_t sequence = 0;
    data_item_t item;
    stats_t local_stats = {0};
    
    printf("[Producer %lu] Started\n", producer_id);
    
    while (1) {
        /* Generate data item */
        item.producer_id = producer_id;
        item.sequence_num = sequence++;
        item.timestamp = xTaskGetTickCount();
        item.data = (producer_id << 16) | (sequence & 0xFFFF);
        
        /* Try to send to queue */
        if (xQueueSend(data_queue, &item, pdMS_TO_TICKS(100)) == pdPASS) {
            local_stats.produced++;
            
            if (sequence % 10 == 0) {
                printf("[Producer %lu] Sent item #%lu (data=0x%08lX)\n",
                       producer_id, sequence, item.data);
                update_stats(&local_stats);
                memset(&local_stats, 0, sizeof(local_stats));
            }
        } else {
            local_stats.queue_full_count++;
            printf("[Producer %lu] Queue full! Dropping item #%lu\n",
                   producer_id, sequence);
        }
        
        /* Variable production rate */
        vTaskDelay(pdMS_TO_TICKS(100 + (rand() % 200)));
    }
}

/**
 * @brief Consumer task
 */
static void consumer_task(void *pvParameters)
{
    uint32_t consumer_id = (uint32_t)pvParameters;
    data_item_t item;
    stats_t local_stats = {0};
    uint32_t processed_count = 0;
    
    printf("  [Consumer %lu] Started\n", consumer_id);
    
    while (1) {
        /* Try to receive from queue */
        if (xQueueReceive(data_queue, &item, pdMS_TO_TICKS(500)) == pdPASS) {
            /* Process data */
            uint32_t latency = xTaskGetTickCount() - item.timestamp;
            processed_count++;
            local_stats.consumed++;
            
            if (processed_count % 10 == 0) {
                printf("  [Consumer %lu] Processed item #%lu from Producer %lu "
                       "(latency=%lu ms)\n",
                       consumer_id, processed_count, item.producer_id, latency);
                update_stats(&local_stats);
                memset(&local_stats, 0, sizeof(local_stats));
            }
            
            /* Simulate variable processing time */
            vTaskDelay(pdMS_TO_TICKS(50 + (rand() % 150)));
        } else {
            local_stats.queue_empty_count++;
        }
    }
}

/**
 * @brief Monitor task - prints statistics periodically
 */
static void monitor_task(void *pvParameters)
{
    TickType_t last_wake_time = xTaskGetTickCount();
    uint32_t last_produced = 0, last_consumed = 0;
    
    printf("    [Monitor] Started\n\n");
    
    while (1) {
        /* Wait for 5 seconds */
        vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(5000));
        
        /* Read statistics */
        xSemaphoreTake(stats_mutex, portMAX_DELAY);
        uint32_t produced = stats.produced;
        uint32_t consumed = stats.consumed;
        uint32_t full_count = stats.queue_full_count;
        uint32_t empty_count = stats.queue_empty_count;
        xSemaphoreGive(stats_mutex);
        
        /* Calculate rates */
        uint32_t prod_rate = (produced - last_produced) / 5;
        uint32_t cons_rate = (consumed - last_consumed) / 5;
        last_produced = produced;
        last_consumed = consumed;
        
        /* Get queue status */
        UBaseType_t items_waiting = uxQueueMessagesWaiting(data_queue);
        UBaseType_t spaces_available = uxQueueSpacesAvailable(data_queue);
        
        /* Print report */
        printf("\n=== Statistics Report ===\n");
        printf("  Total Produced: %lu (rate: %lu items/sec)\n",
               produced, prod_rate);
        printf("  Total Consumed: %lu (rate: %lu items/sec)\n",
               consumed, cons_rate);
        printf("  Queue Status:   %lu/%d items (%.1f%% full)\n",
               items_waiting, QUEUE_LENGTH,
               (items_waiting * 100.0) / QUEUE_LENGTH);
        printf("  Queue Full:     %lu times\n", full_count);
        printf("  Queue Empty:    %lu times\n", empty_count);
        printf("========================\n\n");
    }
}

/**
 * @brief Application entry point
 */
int main(void)
{
    /* Initialize hardware */
    board_init();
    
    printf("\nFreeRTOS Producer-Consumer Pattern\n");
    printf("===================================\n");
    printf("Configuration:\n");
    printf("  Producers: %d\n", NUM_PRODUCERS);
    printf("  Consumers: %d\n", NUM_CONSUMERS);
    printf("  Queue Length: %d\n\n", QUEUE_LENGTH);
    
    /* Create queue */
    data_queue = xQueueCreate(QUEUE_LENGTH, sizeof(data_item_t));
    if (data_queue == NULL) {
        printf("ERROR: Failed to create queue!\n");
        while (1);
    }
    
    /* Create mutex for statistics */
    stats_mutex = xSemaphoreCreateMutex();
    if (stats_mutex == NULL) {
        printf("ERROR: Failed to create mutex!\n");
        while (1);
    }
    
    /* Create producer tasks */
    for (uint32_t i = 0; i < NUM_PRODUCERS; i++) {
        char name[16];
        snprintf(name, sizeof(name), "Prod%lu", i + 1);
        xTaskCreate(producer_task, name, TASK_STACK_SIZE,
                    (void *)(i + 1), PRODUCER_PRIORITY, NULL);
    }
    
    /* Create consumer tasks */
    for (uint32_t i = 0; i < NUM_CONSUMERS; i++) {
        char name[16];
        snprintf(name, sizeof(name), "Cons%lu", i + 1);
        xTaskCreate(consumer_task, name, TASK_STACK_SIZE,
                    (void *)(i + 1), CONSUMER_PRIORITY, NULL);
    }
    
    /* Create monitor task */
    xTaskCreate(monitor_task, "Monitor", TASK_STACK_SIZE,
                NULL, MONITOR_PRIORITY, NULL);
    
    printf("Starting scheduler...\n\n");
    
    /* Start scheduler */
    vTaskStartScheduler();
    
    while (1);
    return 0;
}

void vApplicationMallocFailedHook(void)
{
    taskDISABLE_INTERRUPTS();
    while (1);
}

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    (void)xTask;
    taskDISABLE_INTERRUPTS();
    while (1);
}
