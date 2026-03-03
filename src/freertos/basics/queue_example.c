/**
 * @file queue_example.c
 * @brief FreeRTOS queue example demonstrating producer-consumer pattern
 * 
 * This example demonstrates:
 * - Queue creation with xQueueCreate()
 * - Sending data with xQueueSend()
 * - Receiving data with xQueueReceive()
 * - Blocking and timeout behavior
 */

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include <stdio.h>

/* Queue parameters */
#define QUEUE_LENGTH     10
#define QUEUE_ITEM_SIZE  sizeof(uint32_t)

/* Task parameters */
#define PRODUCER_PRIORITY    (tskIDLE_PRIORITY + 2)
#define CONSUMER_PRIORITY    (tskIDLE_PRIORITY + 2)
#define TASK_STACK_SIZE      (configMINIMAL_STACK_SIZE * 2)

/* Global queue handle */
static QueueHandle_t data_queue = NULL;

/**
 * @brief Producer task - generates data and sends to queue
 */
static void producer_task(void *pvParameters)
{
    uint32_t counter = 0;
    BaseType_t ret;
    
    printf("Producer task started\n");
    
    while (1) {
        /* Generate data */
        uint32_t data = counter++;
        
        /* Try to send data to queue */
        ret = xQueueSend(data_queue, &data, pdMS_TO_TICKS(100));
        
        if (ret == pdPASS) {
            printf("[Producer] Sent: %lu\n", data);
        } else {
            printf("[Producer] Queue full! Could not send: %lu\n", data);
        }
        
        /* Delay before producing next item */
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

/**
 * @brief Consumer task - receives data from queue and processes it
 */
static void consumer_task(void *pvParameters)
{
    uint32_t received_data;
    BaseType_t ret;
    
    printf("Consumer task started\n");
    
    while (1) {
        /* Try to receive data from queue (block for 1 second) */
        ret = xQueueReceive(data_queue, &received_data, pdMS_TO_TICKS(1000));
        
        if (ret == pdPASS) {
            printf("  [Consumer] Received: %lu\n", received_data);
            
            /* Simulate processing time */
            vTaskDelay(pdMS_TO_TICKS(300));
        } else {
            printf("  [Consumer] Timeout - no data received\n");
        }
    }
}

/**
 * @brief Application entry point
 */
int main(void)
{
    /* Initialize hardware */
    board_init();
    
    printf("\nFreeRTOS Queue Example\n");
    printf("======================\n");
    printf("Producer sends data, Consumer receives and processes it.\n\n");
    
    /* Create queue */
    data_queue = xQueueCreate(QUEUE_LENGTH, QUEUE_ITEM_SIZE);
    
    if (data_queue == NULL) {
        printf("ERROR: Failed to create queue!\n");
        while (1);
    }
    
    printf("Queue created (length=%d, item_size=%d bytes)\n\n",
           QUEUE_LENGTH, QUEUE_ITEM_SIZE);
    
    /* Create producer task */
    if (xTaskCreate(producer_task, "Producer", TASK_STACK_SIZE,
                    NULL, PRODUCER_PRIORITY, NULL) != pdPASS) {
        printf("ERROR: Failed to create producer task!\n");
        while (1);
    }
    
    /* Create consumer task */
    if (xTaskCreate(consumer_task, "Consumer", TASK_STACK_SIZE,
                    NULL, CONSUMER_PRIORITY, NULL) != pdPASS) {
        printf("ERROR: Failed to create consumer task!\n");
        while (1);
    }
    
    printf("Starting scheduler...\n\n");
    
    /* Start scheduler */
    vTaskStartScheduler();
    
    /* Should never reach here */
    while (1);
    return 0;
}

/* Hook functions */
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
