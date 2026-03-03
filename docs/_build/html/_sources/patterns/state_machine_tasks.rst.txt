====================
State Machine Tasks
====================

Introduction
============

**State machines** are fundamental to embedded systems, modeling behavior that changes over time based on events. Implementing state machines as RTOS tasks combines the clarity of states with the benefits of concurrent execution.

**Benefits:**
- Clear, maintainable code structure
- Event-driven behavior
- Timeout handling built-in
- Easy to test and debug

Pattern Overview
================

State Machine Basics
---------------------

A state machine consists of:

- **States**: Distinct modes of operation
- **Events**: Triggers that cause transitions
- **Transitions**: Changes from one state to another
- **Actions**: Code executed on transitions or in states

.. code-block:: text

    ┌─────────┐   Event A   ┌─────────┐
    │ State 1 ├────────────►│ State 2 │
    └────┬────┘             └────┬────┘
         │                       │
         │ Event B               │ Event C
         │                       │
         ▼                       ▼
    ┌─────────┐             ┌─────────┐
    │ State 3 │◄────────────│ State 4 │
    └─────────┘   Event D   └─────────┘

Simple Switch-Based Implementation
===================================

.. code-block:: c

    #include "FreeRTOS.h"
    #include "task.h"
    #include "queue.h"
    
    // States
    typedef enum {
        STATE_IDLE,
        STATE_RUNNING,
        STATE_PAUSED,
        STATE_ERROR
    } system_state_t;
    
    // Events
    typedef enum {
        EVENT_START,
        EVENT_STOP,
        EVENT_PAUSE,
        EVENT_RESUME,
        EVENT_ERROR
    } system_event_t;
    
    // State machine context
    typedef struct {
        system_state_t current_state;
        QueueHandle_t event_queue;
        uint32_t error_count;
    } state_machine_t;
    
    state_machine_t sm;
    
    // State machine task
    void state_machine_task(void *param) {
        system_event_t event;
        
        sm.current_state = STATE_IDLE;
        sm.event_queue = xQueueCreate(10, sizeof(system_event_t));
        
        while (1) {
            // Wait for event
            if (xQueueReceive(sm.event_queue, &event, portMAX_DELAY) == pdTRUE) {
                
                // Process event based on current state
                switch (sm.current_state) {
                    case STATE_IDLE:
                        if (event == EVENT_START) {
                            printf("IDLE -> RUNNING\n");
                            sm.current_state = STATE_RUNNING;
                            start_operation();
                        }
                        break;
                    
                    case STATE_RUNNING:
                        if (event == EVENT_STOP) {
                            printf("RUNNING -> IDLE\n");
                            sm.current_state = STATE_IDLE;
                            stop_operation();
                        }
                        else if (event == EVENT_PAUSE) {
                            printf("RUNNING -> PAUSED\n");
                            sm.current_state = STATE_PAUSED;
                            pause_operation();
                        }
                        else if (event == EVENT_ERROR) {
                            printf("RUNNING -> ERROR\n");
                            sm.current_state = STATE_ERROR;
                            handle_error();
                        }
                        break;
                    
                    case STATE_PAUSED:
                        if (event == EVENT_RESUME) {
                            printf("PAUSED -> RUNNING\n");
                            sm.current_state = STATE_RUNNING;
                            resume_operation();
                        }
                        else if (event == EVENT_STOP) {
                            printf("PAUSED -> IDLE\n");
                            sm.current_state = STATE_IDLE;
                            stop_operation();
                        }
                        break;
                    
                    case STATE_ERROR:
                        if (event == EVENT_STOP) {
                            printf("ERROR -> IDLE\n");
                            sm.current_state = STATE_IDLE;
                            clear_error();
                        }
                        break;
                }
            }
        }
    }
    
    // Event injection from other tasks/ISRs
    void send_event(system_event_t event) {
        xQueueSend(sm.event_queue, &event, 0);
    }

Function Pointer Table Implementation
======================================

More scalable for complex state machines:

.. code-block:: c

    // State handler function type
    typedef system_state_t (*state_handler_t)(system_event_t event);
    
    // State handler declarations
    system_state_t idle_handler(system_event_t event);
    system_state_t running_handler(system_event_t event);
    system_state_t paused_handler(system_event_t event);
    system_state_t error_handler(system_event_t event);
    
    // State table
    state_handler_t state_table[] = {
        [STATE_IDLE]    = idle_handler,
        [STATE_RUNNING] = running_handler,
        [STATE_PAUSED]  = paused_handler,
        [STATE_ERROR]   = error_handler
    };
    
    // State handlers
    system_state_t idle_handler(system_event_t event) {
        switch (event) {
            case EVENT_START:
                printf("Starting system\n");
                start_operation();
                return STATE_RUNNING;
            
            default:
                return STATE_IDLE;  // Ignore other events
        }
    }
    
    system_state_t running_handler(system_event_t event) {
        switch (event) {
            case EVENT_STOP:
                stop_operation();
                return STATE_IDLE;
            
            case EVENT_PAUSE:
                pause_operation();
                return STATE_PAUSED;
            
            case EVENT_ERROR:
                handle_error();
                return STATE_ERROR;
            
            default:
                return STATE_RUNNING;
        }
    }
    
    system_state_t paused_handler(system_event_t event) {
        switch (event) {
            case EVENT_RESUME:
                resume_operation();
                return STATE_RUNNING;
            
            case EVENT_STOP:
                stop_operation();
                return STATE_IDLE;
            
            default:
                return STATE_PAUSED;
        }
    }
    
    system_state_t error_handler(system_event_t event) {
        switch (event) {
            case EVENT_STOP:
                clear_error();
                return STATE_IDLE;
            
            default:
                // Stay in error until explicitly cleared
                return STATE_ERROR;
        }
    }
    
    // State machine task
    void state_machine_task(void *param) {
        system_state_t state = STATE_IDLE;
        system_event_t event;
        QueueHandle_t event_queue = xQueueCreate(10, sizeof(system_event_t));
        
        while (1) {
            if (xQueueReceive(event_queue, &event, portMAX_DELAY) == pdTRUE) {
                // Call current state handler
                system_state_t new_state = state_table[state](event);
                
                // Log transition if state changed
                if (new_state != state) {
                    printf("State transition: %d -> %d\n", state, new_state);
                    state = new_state;
                }
            }
        }
    }

Hierarchical State Machine
===========================

For complex systems with nested states:

.. code-block:: c

    typedef enum {
        // Top-level states
        STATE_SYSTEM_OFF,
        STATE_SYSTEM_ON,
        
        // Sub-states of SYSTEM_ON
        STATE_ON_INITIALIZING,
        STATE_ON_READY,
        STATE_ON_ACTIVE,
        STATE_ON_SHUTTING_DOWN
    } hierarchical_state_t;
    
    typedef struct {
        hierarchical_state_t state;
        hierarchical_state_t parent_state;
    } hsm_context_t;
    
    hsm_context_t hsm;
    
    void hsm_task(void *param) {
        system_event_t event;
        QueueHandle_t event_queue = xQueueCreate(10, sizeof(system_event_t));
        
        hsm.state = STATE_SYSTEM_OFF;
        hsm.parent_state = STATE_SYSTEM_OFF;
        
        while (1) {
            if (xQueueReceive(event_queue, &event, portMAX_DELAY) == pdTRUE) {
                
                // Handle event based on current state
                switch (hsm.state) {
                    case STATE_SYSTEM_OFF:
                        if (event == EVENT_START) {
                            hsm.state = STATE_ON_INITIALIZING;
                            hsm.parent_state = STATE_SYSTEM_ON;
                            initialize_system();
                        }
                        break;
                    
                    case STATE_ON_INITIALIZING:
                        if (event == EVENT_INIT_COMPLETE) {
                            hsm.state = STATE_ON_READY;
                        }
                        else if (event == EVENT_STOP) {
                            // Parent state handler
                            goto handle_system_on;
                        }
                        break;
                    
                    case STATE_ON_READY:
                        if (event == EVENT_START) {
                            hsm.state = STATE_ON_ACTIVE;
                            start_processing();
                        }
                        else if (event == EVENT_STOP) {
                            goto handle_system_on;
                        }
                        break;
                    
                    case STATE_ON_ACTIVE:
                        if (event == EVENT_PAUSE) {
                            hsm.state = STATE_ON_READY;
                            pause_processing();
                        }
                        else if (event == EVENT_STOP) {
                            goto handle_system_on;
                        }
                        break;
                    
                    case STATE_ON_SHUTTING_DOWN:
                        if (event == EVENT_SHUTDOWN_COMPLETE) {
                            hsm.state = STATE_SYSTEM_OFF;
                            hsm.parent_state = STATE_SYSTEM_OFF;
                        }
                        break;
                }
                
                continue;
                
handle_system_on:
                // Common handler for all SYSTEM_ON sub-states
                if (event == EVENT_STOP) {
                    hsm.state = STATE_ON_SHUTTING_DOWN;
                    shutdown_system();
                }
            }
        }
    }

State Machine with Timeouts
============================

.. code-block:: c

    typedef struct {
        system_state_t state;
        TickType_t state_entry_time;
        uint32_t timeout_ms;
    } timed_state_machine_t;
    
    timed_state_machine_t tsm;
    
    void timed_state_machine_task(void *param) {
        system_event_t event;
        QueueHandle_t event_queue = xQueueCreate(10, sizeof(system_event_t));
        
        tsm.state = STATE_IDLE;
        tsm.state_entry_time = xTaskGetTickCount();
        tsm.timeout_ms = 0;
        
        while (1) {
            // Calculate timeout for current state
            TickType_t timeout = portMAX_DELAY;
            if (tsm.timeout_ms > 0) {
                TickType_t elapsed = xTaskGetTickCount() - tsm.state_entry_time;
                TickType_t timeout_ticks = pdMS_TO_TICKS(tsm.timeout_ms);
                
                if (elapsed < timeout_ticks) {
                    timeout = timeout_ticks - elapsed;
                } else {
                    // Timeout occurred
                    event = EVENT_TIMEOUT;
                    goto process_event;
                }
            }
            
            // Wait for event or timeout
            if (xQueueReceive(event_queue, &event, timeout) != pdTRUE) {
                event = EVENT_TIMEOUT;
            }
            
process_event:
            system_state_t old_state = tsm.state;
            
            switch (tsm.state) {
                case STATE_IDLE:
                    if (event == EVENT_START) {
                        tsm.state = STATE_RUNNING;
                        tsm.timeout_ms = 5000;  // 5 second timeout
                    }
                    break;
                
                case STATE_RUNNING:
                    if (event == EVENT_STOP) {
                        tsm.state = STATE_IDLE;
                        tsm.timeout_ms = 0;
                    }
                    else if (event == EVENT_TIMEOUT) {
                        printf("Running state timeout - moving to error\n");
                        tsm.state = STATE_ERROR;
                        tsm.timeout_ms = 10000;  // 10 second error timeout
                    }
                    break;
                
                case STATE_ERROR:
                    if (event == EVENT_STOP) {
                        tsm.state = STATE_IDLE;
                        tsm.timeout_ms = 0;
                    }
                    else if (event == EVENT_TIMEOUT) {
                        printf("Error timeout - attempting recovery\n");
                        attempt_recovery();
                        tsm.state = STATE_IDLE;
                        tsm.timeout_ms = 0;
                    }
                    break;
            }
            
            // Update entry time if state changed
            if (tsm.state != old_state) {
                tsm.state_entry_time = xTaskGetTickCount();
                printf("State: %d -> %d\n", old_state, tsm.state);
            }
        }
    }

Guard Conditions and Actions
=============================

.. code-block:: c

    typedef struct {
        system_state_t from_state;
        system_state_t to_state;
        system_event_t event;
        bool (*guard)(void);        // Condition to allow transition
        void (*action)(void);       // Action to perform on transition
    } transition_t;
    
    // Guard functions
    bool battery_ok(void) {
        return get_battery_voltage() > 3.3f;
    }
    
    bool temperature_normal(void) {
        return get_temperature() < 80;
    }
    
    // Action functions
    void start_motor(void) {
        printf("Starting motor\n");
        motor_enable(true);
    }
    
    void stop_motor(void) {
        printf("Stopping motor\n");
        motor_enable(false);
    }
    
    void log_error(void) {
        printf("Error occurred\n");
        error_logger_add_entry();
    }
    
    // Transition table
    transition_t transitions[] = {
        // from,         to,            event,       guard,            action
        {STATE_IDLE,     STATE_RUNNING, EVENT_START, battery_ok,       start_motor},
        {STATE_RUNNING,  STATE_IDLE,    EVENT_STOP,  NULL,             stop_motor},
        {STATE_RUNNING,  STATE_ERROR,   EVENT_ERROR, temperature_normal, log_error},
        {STATE_ERROR,    STATE_IDLE,    EVENT_STOP,  NULL,             NULL},
    };
    
    #define NUM_TRANSITIONS (sizeof(transitions) / sizeof(transition_t))
    
    void table_driven_state_machine_task(void *param) {
        system_state_t state = STATE_IDLE;
        system_event_t event;
        QueueHandle_t event_queue = xQueueCreate(10, sizeof(system_event_t));
        
        while (1) {
            if (xQueueReceive(event_queue, &event, portMAX_DELAY) == pdTRUE) {
                
                // Search for matching transition
                for (int i = 0; i < NUM_TRANSITIONS; i++) {
                    transition_t *t = &transitions[i];
                    
                    if (t->from_state == state && t->event == event) {
                        // Check guard condition
                        if (t->guard == NULL || t->guard()) {
                            // Transition allowed
                            printf("Transition: %d -> %d\n", state, t->to_state);
                            
                            // Execute action
                            if (t->action != NULL) {
                                t->action();
                            }
                            
                            // Change state
                            state = t->to_state;
                            break;
                        } else {
                            printf("Transition blocked by guard\n");
                        }
                    }
                }
            }
        }
    }

State Entry/Exit Actions
=========================

.. code-block:: c

    typedef struct {
        void (*on_entry)(void);
        void (*on_execute)(void);
        void (*on_exit)(void);
    } state_actions_t;
    
    // State-specific actions
    void idle_on_entry(void) {
        printf("Entering IDLE\n");
        leds_off();
    }
    
    void idle_on_execute(void) {
        // Periodic work while in IDLE
        check_battery_level();
    }
    
    void idle_on_exit(void) {
        printf("Leaving IDLE\n");
    }
    
    void running_on_entry(void) {
        printf("Entering RUNNING\n");
        start_motor();
        led_set(LED_GREEN);
    }
    
    void running_on_execute(void) {
        // Periodic work while RUNNING
        monitor_motor_speed();
        monitor_temperature();
    }
    
    void running_on_exit(void) {
        printf("Leaving RUNNING\n");
        stop_motor();
    }
    
    // Action table
    state_actions_t state_actions[] = {
        [STATE_IDLE] = {
            .on_entry = idle_on_entry,
            .on_execute = idle_on_execute,
            .on_exit = idle_on_exit
        },
        [STATE_RUNNING] = {
            .on_entry = running_on_entry,
            .on_execute = running_on_execute,
            .on_exit = running_on_exit
        }
    };
    
    void state_machine_with_actions_task(void *param) {
        system_state_t state = STATE_IDLE;
        system_event_t event;
        QueueHandle_t event_queue = xQueueCreate(10, sizeof(system_event_t));
        const TickType_t execute_period = pdMS_TO_TICKS(100);
        
        // Call entry action for initial state
        if (state_actions[state].on_entry) {
            state_actions[state].on_entry();
        }
        
        while (1) {
            // Wait for event or timeout for execute action
            if (xQueueReceive(event_queue, &event, execute_period) == pdTRUE) {
                // Process event
                system_state_t new_state = process_event(state, event);
                
                if (new_state != state) {
                    // Call exit action
                    if (state_actions[state].on_exit) {
                        state_actions[state].on_exit();
                    }
                    
                    // Change state
                    state = new_state;
                    
                    // Call entry action
                    if (state_actions[state].on_entry) {
                        state_actions[state].on_entry();
                    }
                }
            }
            
            // Call execute action periodically
            if (state_actions[state].on_execute) {
                state_actions[state].on_execute();
            }
        }
    }

Best Practices
==============

1. **Keep states simple**: Each state should have a clear purpose
2. **Document state diagram**: Visual representation aids understanding
3. **Use enums for states/events**: Type safety and readability
4. **Validate transitions**: Log invalid event/state combinations
5. **Handle all events**: Define behavior for every state/event pair
6. **Use timeouts**: Recover from stuck states
7. **Test all paths**: Cover all state transitions
8. **Avoid deep nesting**: Keep hierarchies manageable
9. **Log transitions**: Aids debugging
10. **Make it deterministic**: Same input = same output

Debugging State Machines
=========================

.. code-block:: c

    // State machine debugging helper
    const char *state_names[] = {
        [STATE_IDLE] = "IDLE",
        [STATE_RUNNING] = "RUNNING",
        [STATE_PAUSED] = "PAUSED",
        [STATE_ERROR] = "ERROR"
    };
    
    const char *event_names[] = {
        [EVENT_START] = "START",
        [EVENT_STOP] = "STOP",
        [EVENT_PAUSE] = "PAUSE",
        [EVENT_RESUME] = "RESUME",
        [EVENT_ERROR] = "ERROR"
    };
    
    void log_transition(system_state_t from, system_state_t to, system_event_t event) {
        printf("[%u] %s + %s -> %s\n",
               (unsigned)xTaskGetTickCount(),
               state_names[from],
               event_names[event],
               state_names[to]);
    }
    
    // History buffer for debugging
    #define HISTORY_SIZE 32
    typedef struct {
        system_state_t from_state;
        system_state_t to_state;
        system_event_t event;
        TickType_t timestamp;
    } transition_history_t;
    
    transition_history_t history[HISTORY_SIZE];
    uint32_t history_idx = 0;
    
    void record_transition(system_state_t from, system_state_t to, system_event_t event) {
        history[history_idx].from_state = from;
        history[history_idx].to_state = to;
        history[history_idx].event = event;
        history[history_idx].timestamp = xTaskGetTickCount();
        history_idx = (history_idx + 1) % HISTORY_SIZE;
    }

See Also
========

- :doc:`../days/day02` - Tasks and Threads
- :doc:`../overview/synchronization` - Event queues
- :doc:`command_dispatcher` - Command processing patterns
- :doc:`periodic_scheduler` - Periodic state machine execution

Further Reading
===============

- "UML State Machines" by Miro Samek
- "Practical Statecharts in C/C++" by Miro Samek
- Design Patterns (Gang of Four)
