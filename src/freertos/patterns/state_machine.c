/**
 * @file state_machine.c
 * @brief Finite State Machine (FSM) in RTOS task
 * 
 * This example demonstrates:
 * - Event-driven state machine design
 * - Queue for event delivery
 * - State transition table
 * - Entry/exit actions
 */

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include <stdio.h>
#include <string.h>

/* State definitions */
typedef enum {
    STATE_IDLE,
    STATE_ACTIVE,
    STATE_PROCESSING,
    STATE_ERROR,
    STATE_MAX
} state_t;

/* Event definitions */
typedef enum {
    EVENT_START,
    EVENT_STOP,
    EVENT_DATA_READY,
    EVENT_PROCESS_COMPLETE,
    EVENT_ERROR,
    EVENT_RESET,
    EVENT_TIMEOUT,
    EVENT_MAX
} event_t;

/* Event structure */
typedef struct {
    event_t type;
    uint32_t data;
    uint32_t timestamp;
} event_data_t;

/* State names for logging */
static const char *state_names[] = {
    "IDLE", "ACTIVE", "PROCESSING", "ERROR"
};

/* Event names for logging */
static const char *event_names[] = {
    "START", "STOP", "DATA_READY", "PROCESS_COMPLETE",
    "ERROR", "RESET", "TIMEOUT"
};

/* Configuration */
#define EVENT_QUEUE_LENGTH      10
#define FSM_TASK_PRIORITY       (tskIDLE_PRIORITY + 2)
#define GEN_TASK_PRIORITY       (tskIDLE_PRIORITY + 1)
#define TASK_STACK_SIZE         (configMINIMAL_STACK_SIZE * 2)

/* Global variables */
static QueueHandle_t event_queue = NULL;
static state_t current_state = STATE_IDLE;
static uint32_t state_entry_time = 0;
static uint32_t transition_count = 0;

/**
 * @brief Send event to state machine
 */
static BaseType_t fsm_send_event(event_t event, uint32_t data)
{
    event_data_t evt = {
        .type = event,
        .data = data,
        .timestamp = xTaskGetTickCount()
    };
    
    return xQueueSend(event_queue, &evt, pdMS_TO_TICKS(100));
}

/**
 * @brief State entry action
 */
static void state_entry(state_t state)
{
    state_entry_time = xTaskGetTickCount();
    printf("  [FSM] >>> Entered state: %s\n", state_names[state]);
    
    /* State-specific entry actions */
    switch (state) {
        case STATE_IDLE:
            /* Turn off LED */
            board_set_led(0, false);
            break;
            
        case STATE_ACTIVE:
            /* Turn on LED */
            board_set_led(0, true);
            break;
            
        case STATE_PROCESSING:
            /* Blink LED */
            board_toggle_led(1);
            break;
            
        case STATE_ERROR:
            /* Turn on error LED */
            board_set_led(2, true);
            break;
            
        default:
            break;
    }
}

/**
 * @brief State exit action
 */
static void state_exit(state_t state)
{
    uint32_t time_in_state = xTaskGetTickCount() - state_entry_time;
    printf("  [FSM] <<< Exited state: %s (duration: %lu ms)\n",
           state_names[state], time_in_state);
    
    /* State-specific exit actions */
    switch (state) {
        case STATE_ERROR:
            /* Turn off error LED */
            board_set_led(2, false);
            break;
            
        default:
            break;
    }
}

/**
 * @brief Perform state transition
 */
static void transition_to(state_t new_state)
{
    if (new_state != current_state) {
        printf("[FSM] Transition: %s -> %s\n",
               state_names[current_state], state_names[new_state]);
        
        state_exit(current_state);
        current_state = new_state;
        state_entry(current_state);
        transition_count++;
    }
}

/**
 * @brief Handle event in current state
 */
static void handle_event(const event_data_t *evt)
{
    printf("[FSM] Event: %s (data=0x%08lX) in state %s\n",
           event_names[evt->type], evt->data, state_names[current_state]);
    
    /* State machine logic */
    switch (current_state) {
        case STATE_IDLE:
            if (evt->type == EVENT_START) {
                transition_to(STATE_ACTIVE);
            }
            break;
            
        case STATE_ACTIVE:
            if (evt->type == EVENT_DATA_READY) {
                transition_to(STATE_PROCESSING);
            } else if (evt->type == EVENT_STOP) {
                transition_to(STATE_IDLE);
            } else if (evt->type == EVENT_ERROR) {
                transition_to(STATE_ERROR);
            }
            break;
            
        case STATE_PROCESSING:
            if (evt->type == EVENT_PROCESS_COMPLETE) {
                transition_to(STATE_ACTIVE);
            } else if (evt->type == EVENT_ERROR) {
                transition_to(STATE_ERROR);
            } else if (evt->type == EVENT_TIMEOUT) {
                printf("  [FSM] Warning: Processing timeout!\n");
                transition_to(STATE_ERROR);
            }
            break;
            
        case STATE_ERROR:
            if (evt->type == EVENT_RESET) {
                transition_to(STATE_IDLE);
            }
            break;
            
        default:
            printf("  [FSM] Unhandled event in state!\n");
            break;
    }
}

/**
 * @brief FSM task - processes events
 */
static void fsm_task(void *pvParameters)
{
    event_data_t event;
    
    printf("[FSM] State machine task started\n");
    
    /* Initial state */
    state_entry(STATE_IDLE);
    
    while (1) {
        /* Wait for event */
        if (xQueueReceive(event_queue, &event, pdMS_TO_TICKS(5000)) == pdPASS) {
            /* Handle event */
            handle_event(&event);
            
            /* Periodic statistics */
            if (transition_count % 10 == 0 && transition_count > 0) {
                printf("  [FSM] Statistics: %lu transitions\n", transition_count);
            }
        } else {
            /* Timeout - no events */
            if (current_state == STATE_PROCESSING) {
                /* Send timeout event */
                fsm_send_event(EVENT_TIMEOUT, 0);
            }
        }
    }
}

/**
 * @brief Event generator task - simulates external events
 */
static void event_generator_task(void *pvParameters)
{
    uint32_t counter = 0;
    
    printf("[Generator] Event generator task started\n\n");
    
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(2000));
        
        /* Generate events based on current state */
        switch (counter % 6) {
            case 0:
                fsm_send_event(EVENT_START, counter);
                break;
            case 1:
                fsm_send_event(EVENT_DATA_READY, 0x1234 + counter);
                break;
            case 2:
                fsm_send_event(EVENT_PROCESS_COMPLETE, counter);
                break;
            case 3:
                fsm_send_event(EVENT_STOP, counter);
                break;
            case 4:
                fsm_send_event(EVENT_ERROR, 0xDEAD);
                break;
            case 5:
                fsm_send_event(EVENT_RESET, counter);
                break;
        }
        
        counter++;
    }
}

/**
 * @brief Application entry point
 */
int main(void)
{
    /* Initialize hardware */
    board_init();
    
    printf("\nFreeRTOS State Machine Example\n");
    printf("===============================\n\n");
    
    /* Create event queue */
    event_queue = xQueueCreate(EVENT_QUEUE_LENGTH, sizeof(event_data_t));
    if (event_queue == NULL) {
        printf("ERROR: Failed to create event queue!\n");
        while (1);
    }
    
    /* Create FSM task */
    xTaskCreate(fsm_task, "FSM", TASK_STACK_SIZE,
                NULL, FSM_TASK_PRIORITY, NULL);
    
    /* Create event generator task */
    xTaskCreate(event_generator_task, "Generator", TASK_STACK_SIZE,
                NULL, GEN_TASK_PRIORITY, NULL);
    
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
