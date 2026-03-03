/**
 * @file hello_task.c
 * @brief Minimal FreeRTOS "Hello World" example
 * 
 * This example demonstrates:
 * - Basic task creation with xTaskCreate()
 * - Periodic execution with vTaskDelay()
 * - Simple console output
 * 
 * Expected output: "Hello from FreeRTOS!" printed every second
 */

#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>

/* Task parameters */
#define HELLO_TASK_PRIORITY     (tskIDLE_PRIORITY + 1)
#define HELLO_TASK_STACK_SIZE   (configMINIMAL_STACK_SIZE * 2)

/**
 * @brief Hello task function
 * 
 * This task runs in an infinite loop, printing a message
 * and then sleeping for 1 second.
 */
static void hello_task(void *pvParameters)
{
    uint32_t counter = 0;
    
    /* Task infinite loop */
    while (1) {
        /* Print hello message with counter */
        printf("[%lu] Hello from FreeRTOS!\n", counter++);
        
        /* Delay for 1000 milliseconds (1 second) */
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    
    /* Tasks should never return */
}

/**
 * @brief Application entry point
 */
int main(void)
{
    BaseType_t ret;
    
    /* Initialize hardware (board-specific) */
    board_init();
    
    printf("\nFreeRTOS Hello Task Example\n");
    printf("============================\n\n");
    
    /* Create the hello task */
    ret = xTaskCreate(
        hello_task,              /* Task function */
        "Hello",                 /* Task name (for debugging) */
        HELLO_TASK_STACK_SIZE,   /* Stack size in words */
        NULL,                    /* Task parameter */
        HELLO_TASK_PRIORITY,     /* Task priority */
        NULL                     /* Task handle (not needed) */
    );
    
    if (ret != pdPASS) {
        printf("ERROR: Failed to create hello task!\n");
        while (1);  /* Hang on error */
    }
    
    printf("Starting FreeRTOS scheduler...\n\n");
    
    /* Start the scheduler - this call never returns */
    vTaskStartScheduler();
    
    /* Should never reach here */
    while (1);
    
    return 0;
}

/**
 * @brief Hook function called when malloc fails
 */
void vApplicationMallocFailedHook(void)
{
    printf("ERROR: Malloc failed!\n");
    taskDISABLE_INTERRUPTS();
    while (1);
}

/**
 * @brief Hook function called on stack overflow
 */
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    printf("ERROR: Stack overflow in task: %s\n", pcTaskName);
    taskDISABLE_INTERRUPTS();
    while (1);
}

/**
 * @brief Idle hook (called when no tasks are ready to run)
 */
void vApplicationIdleHook(void)
{
    /* Optional: Enter low-power mode here */
}
