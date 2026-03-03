/**
 * @file state_machine.c
 * @brief Zephyr FSM with event-driven design using k_poll
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

typedef enum {
    STATE_IDLE, STATE_ACTIVE, STATE_PROCESSING, STATE_ERROR
} state_t;

typedef enum {
    EVENT_START, EVENT_STOP, EVENT_DATA, EVENT_DONE, EVENT_ERROR, EVENT_RESET
} event_t;

static const char *state_names[] = {"IDLE", "ACTIVE", "PROCESSING", "ERROR"};
static const char *event_names[] = {"START", "STOP", "DATA", "DONE", "ERROR", "RESET"};

#define MSGQ_SIZE   10
#define STACK_SIZE  2048

K_MSGQ_DEFINE(event_queue, sizeof(event_t), MSGQ_SIZE, 4);

static state_t current_state = STATE_IDLE;
static uint32_t transition_count = 0;

void transition_to(state_t new_state)
{
    if (new_state != current_state) {
        printk("[FSM] %s -> %s\n", state_names[current_state], state_names[new_state]);
        current_state = new_state;
        transition_count++;
    }
}

void handle_event(event_t event)
{
    printk("[FSM] Event %s in state %s\n", event_names[event], state_names[current_state]);
    
    switch (current_state) {
        case STATE_IDLE:
            if (event == EVENT_START) transition_to(STATE_ACTIVE);
            break;
        case STATE_ACTIVE:
            if (event == EVENT_DATA) transition_to(STATE_PROCESSING);
            else if (event == EVENT_STOP) transition_to(STATE_IDLE);
            else if (event == EVENT_ERROR) transition_to(STATE_ERROR);
            break;
        case STATE_PROCESSING:
            if (event == EVENT_DONE) transition_to(STATE_ACTIVE);
            else if (event == EVENT_ERROR) transition_to(STATE_ERROR);
            break;
        case STATE_ERROR:
            if (event == EVENT_RESET) transition_to(STATE_IDLE);
            break;
    }
}

void fsm_thread(void *a, void *b, void *c)
{
    event_t event;
    
    while (1) {
        if (k_msgq_get(&event_queue, &event, K_MSEC(1000)) == 0) {
            handle_event(event);
        }
    }
}

void event_generator_thread(void *a, void *b, void *c)
{
    event_t events[] = {EVENT_START, EVENT_DATA, EVENT_DONE, 
                        EVENT_STOP, EVENT_ERROR, EVENT_RESET};
    size_t idx = 0;
    
    while (1) {
        k_msleep(2000);
        event_t evt = events[idx % ARRAY_SIZE(events)];
        k_msgq_put(&event_queue, &evt, K_NO_WAIT);
        idx++;
    }
}

K_THREAD_DEFINE(fsm_tid, STACK_SIZE, fsm_thread, NULL, NULL, NULL, 5, 0, 0);
K_THREAD_DEFINE(gen_tid, STACK_SIZE, event_generator_thread, NULL, NULL, NULL, 6, 0, 0);

void main(void)
{
    printk("\nZephyr State Machine Example\n\n");
}
