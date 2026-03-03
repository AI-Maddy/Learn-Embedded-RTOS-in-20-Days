/**
 * @file multiple_tasks.c
 * @brief FreeRTOS example with multiple tasks at different priorities
 * 
 * This example demonstrates:
 * - Creating multiple tasks with xTaskCreate()
 * - Task priorities and preemptive scheduling
 * - Task execution order based on priority
 * 
 * Three tasks are created with low, medium, and high priorities.
 * The scheduler will preempt lower-priority tasks when higher-priority
 * tasks become ready.
 */

#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>

/* Task priorities (higher number = higher priority) */
#define LOW_PRIORITY_TASK       (tskIDLE_PRIORITY + 1)
#define MEDIUM_PRIORITY_TASK    (tskIDLE_PRIORITY + 2)
#define HIGH_PRIORITY_TASK      (tskIDLE_PRIORITY + 3)

#define TASK_STACK_SIZE         (configMINIMAL_STACK_SIZE * 2)

/**
 * @brief Low priority task
 */
static void low_priority_task(void *pvParameters)
{
    uint32_t counter = 0;
    
    while (1) {
        printf("[LOW]    Task running (count=%lu)\n", counter++);
        
        /* Simulate some work */
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

/**
 * @brief Medium priority task
 */
static void medium_priority_task(void *pvParameters)
{
    uint32_t counter = 0;
    
    while (1) {
        printf("  [MEDIUM] Task running (count=%lu)\n", counter++);
        
        /* Simulate some work */
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

/**
 * @brief High priority task
 */
static void high_priority_task(void *pvParameters)
{
    uint32_t counter = 0;
    
    while (1) {
        printf("    [HIGH]   Task running (count=%lu)\n", counter++);
        
        /* Simulate some work */
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

/**
 * @brief Application entry point
 */
int main(void)
{
    /* Initialize hardware */
    board_init();
    
    printf("\nFreeRTOS Multiple Tasks Example\n");
    printf("================================\n");
    printf("This example creates three tasks with different priorities.\n");
    printf("Watch how the scheduler preempts lower-priority tasks.\n\n");
    
    /* Create low priority task */
    if (xTaskCreate(low_priority_task, "Low", TASK_STACK_SIZE,
                    NULL, LOW_PRIORITY_TASK, NULL) != pdPASS) {
        printf("ERROR: Failed to create low priority task!\n");
        while (1);
    }
    
    /* Create medium priority task */
    if (xTaskCreate(medium_priority_task, "Medium", TASK_STACK_SIZE,
                    NULL, MEDIUM_PRIORITY_TASK, NULL) != pdPASS) {
        printf("ERROR: Failed to create medium priority task!\n");
        while (1);
    }
    
    /* Create high priority task */
    if (xTaskCreate(high_priority_task, "High", TASK_STACK_SIZE,
                    NULL, HIGH_PRIORITY_TASK, NULL) != pdPASS) {
        printf("ERROR: Failed to create high priority task!\n");
        while (1);
    }
    
    printf("All tasks created. Starting scheduler...\n\n");
    
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
    printf("Stack overflow in %s\n", pcTaskName);
    taskDISABLE_INTERRUPTS();
    while (1);
}
