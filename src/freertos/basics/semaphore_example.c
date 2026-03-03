/**
 * @file semaphore_example.c
 * @brief FreeRTOS semaphore synchronization example
 * 
 * This example demonstrates:
 * - Binary semaphore creation and usage
 * - Task synchronization with semaphores
 * - Mutex for resource protection
 */

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include <stdio.h>

/* Task parameters */
#define TASK_PRIORITY       (tskIDLE_PRIORITY + 2)
#define TASK_STACK_SIZE     (configMINIMAL_STACK_SIZE * 2)

/* Global semaphore and mutex handles */
static SemaphoreHandle_t binary_sem = NULL;
static SemaphoreHandle_t mutex = NULL;

/* Shared resource */
static uint32_t shared_counter = 0;

/**
 * @brief Task that signals via semaphore
 */
static void signaling_task(void *pvParameters)
{
    uint32_t counter = 0;
    
    printf("Signaling task started\n");
    
    while (1) {
        /* Do some work */
        vTaskDelay(pdMS_TO_TICKS(1000));
        
        printf("[Signal] Giving semaphore (count=%lu)\n", counter++);
        
        /* Signal the semaphore */
        xSemaphoreGive(binary_sem);
    }
}

/**
 * @brief Task that waits on semaphore
 */
static void waiting_task(void *pvParameters)
{
    printf("Waiting task started\n");
    
    while (1) {
        printf("  [Wait] Waiting for semaphore...\n");
        
        /* Wait for semaphore (block indefinitely) */
        if (xSemaphoreTake(binary_sem, portMAX_DELAY) == pdTRUE) {
            printf("  [Wait] Semaphore received! Processing...\n");
            
            /* Simulate processing */
            vTaskDelay(pdMS_TO_TICKS(200));
        }
    }
}

/**
 * @brief Task that accesses shared resource with mutex protection
 * @param id Task ID for identification
 */
static void resource_task(void *pvParameters)
{
    uint32_t task_id = (uint32_t)pvParameters;
    
    printf("Resource task %lu started\n", task_id);
    
    while (1) {
        /* Take mutex to protect shared resource */
        if (xSemaphoreTake(mutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
            printf("    [Task %lu] Locked mutex, accessing shared resource\n",
                   task_id);
            
            /* Critical section - access shared resource */
            uint32_t local_copy = shared_counter;
            vTaskDelay(pdMS_TO_TICKS(100));  /* Simulate work */
            shared_counter = local_copy + 1;
            
            printf("    [Task %lu] Shared counter = %lu\n",
                   task_id, shared_counter);
            
            /* Release mutex */
            xSemaphoreGive(mutex);
            printf("    [Task %lu] Released mutex\n", task_id);
        } else {
            printf("    [Task %lu] Failed to take mutex (timeout)\n", task_id);
        }
        
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

/**
 * @brief Application entry point
 */
int main(void)
{
    /* Initialize hardware */
    board_init();
    
    printf("\nFreeRTOS Semaphore Example\n");
    printf("===========================\n\n");
    
    /* Create binary semaphore */
    binary_sem = xSemaphoreCreateBinary();
    if (binary_sem == NULL) {
        printf("ERROR: Failed to create binary semaphore!\n");
        while (1);
    }
    printf("Binary semaphore created\n");
    
    /* Create mutex */
    mutex = xSemaphoreCreateMutex();
    if (mutex == NULL) {
        printf("ERROR: Failed to create mutex!\n");
        while (1);
    }
    printf("Mutex created\n\n");
    
    /* Create signaling and waiting tasks */
    xTaskCreate(signaling_task, "Signal", TASK_STACK_SIZE,
                NULL, TASK_PRIORITY, NULL);
    xTaskCreate(waiting_task, "Wait", TASK_STACK_SIZE,
                NULL, TASK_PRIORITY, NULL);
    
    /* Create two tasks that share a resource */
    xTaskCreate(resource_task, "Resource1", TASK_STACK_SIZE,
                (void *)1, TASK_PRIORITY, NULL);
    xTaskCreate(resource_task, "Resource2", TASK_STACK_SIZE,
                (void *)2, TASK_PRIORITY, NULL);
    
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

void vApplicationStackOverflowHook( TaskHandle_t xTask, char *pcTaskName)
{
    (void)xTask;
    taskDISABLE_INTERRUPTS();
    while (1);
}
