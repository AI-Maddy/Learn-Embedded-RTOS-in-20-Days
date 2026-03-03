==================
Command Dispatcher
==================

Introduction
============

The **command dispatcher pattern** serializes command execution through a centralized processing task. Commands are queued from multiple sources (user input, network, other tasks) and executed sequentially in a controlled manner.

**Benefits:**
- Centralized command processing
- Serialized execution (no race conditions)
- Priority-based command handling
- Easy command logging and debugging
- Undo/redo capability

Pattern Overview
================

Architecture
------------

.. code-block:: text

    Source 1 ──┐
               ├──► Command    ──► Dispatcher ──► Execute
    Source 2 ──┤      Queue          Task           Commands
               │
    Source 3 ──┘

**Components:**
- **Command Queue**: Holds pending commands
- **Dispatcher Task**: Processes commands sequentially
- **Command Handlers**: Execute specific commands
- **Result Queue** (optional): Return command results

Basic Implementation
====================

.. code-block:: c

    #include "FreeRTOS.h"
    #include "task.h"
    #include "queue.h"
    
    // Command types
    typedef enum {
        CMD_LED_ON,
        CMD_LED_OFF,
        CMD_SET_SPEED,
        CMD_READ_SENSOR,
        CMD_RESET_SYSTEM
    } command_type_t;
    
    // Command structure
    typedef struct {
        command_type_t type;
        uint32_t param1;
        uint32_t param2;
        void *data;
        TaskHandle_t response_task;  // For async responses
    } command_t;
    
    // Command queue
    QueueHandle_t command_queue;
    #define COMMAND_QUEUE_LENGTH 20
    
    // Command dispatcher task
    void command_dispatcher_task(void *param) {
        command_t cmd;
        
        while (1) {
            // Wait for command
            if (xQueueReceive(command_queue, &cmd, portMAX_DELAY) == pdTRUE) {
                // Dispatch based on command type
                switch (cmd.type) {
                    case CMD_LED_ON:
                        led_set(cmd.param1, true);
                        break;
                    
                    case CMD_LED_OFF:
                        led_set(cmd.param1, false);
                        break;
                    
                    case CMD_SET_SPEED:
                        motor_set_speed(cmd.param1);
                        break;
                    
                    case CMD_READ_SENSOR:
                        {
                            uint32_t value = sensor_read(cmd.param1);
                            // Send result if response task specified
                            if (cmd.response_task) {
                                xTaskNotify(cmd.response_task, value, eSetValueWithOverwrite);
                            }
                        }
                        break;
                    
                    case CMD_RESET_SYSTEM:
                        system_reset();
                        break;
                    
                    default:
                        printf("Unknown command: %d\n", cmd.type);
                        break;
                }
            }
        }
    }
    
    // Command submission function
    bool submit_command(command_type_t type, uint32_t param1, uint32_t param2) {
        command_t cmd = {
            .type = type,
            .param1 = param1,
            .param2 = param2,
            .data = NULL,
            .response_task = NULL
        };
        
        return xQueueSend(command_queue, &cmd, pdMS_TO_TICKS(100)) == pdTRUE;
    }
    
    // Initialization
    void init_command_dispatcher(void) {
        command_queue = xQueueCreate(COMMAND_QUEUE_LENGTH, sizeof(command_t));
        xTaskCreate(command_dispatcher_task, "CmdDispatch", 512, NULL, 3, NULL);
    }

Function Pointer-Based Dispatcher
==================================

More extensible and maintainable:

.. code-block:: c

    // Command handler function type
    typedef void (*command_handler_t)(command_t *cmd);
    
    // Command handler implementations
    void handle_led_on(command_t *cmd) {
        printf("LED %u ON\n", (unsigned)cmd->param1);
        led_set(cmd->param1, true);
    }
    
    void handle_led_off(command_t *cmd) {
        printf("LED %u OFF\n", (unsigned)cmd->param1);
        led_set(cmd->param1, false);
    }
    
    void handle_set_speed(command_t *cmd) {
        printf("Motor speed: %u\n", (unsigned)cmd->param1);
        motor_set_speed(cmd->param1);
    }
    
    void handle_read_sensor(command_t *cmd) {
        uint32_t value = sensor_read(cmd->param1);
        printf("Sensor %u: %u\n", (unsigned)cmd->param1, (unsigned)value);
        
        if (cmd->response_task) {
            xTaskNotify(cmd->response_task, value, eSetValueWithOverwrite);
        }
    }
    
    void handle_reset(command_t *cmd) {
        printf("System reset\n");
        system_reset();
    }
    
    // Command handler table
    command_handler_t command_handlers[] = {
        [CMD_LED_ON]      = handle_led_on,
        [CMD_LED_OFF]     = handle_led_off,
        [CMD_SET_SPEED]   = handle_set_speed,
        [CMD_READ_SENSOR] = handle_read_sensor,
        [CMD_RESET_SYSTEM] = handle_reset
    };
    
    // Improved dispatcher
    void command_dispatcher_task(void *param) {
        command_t cmd;
        
        while (1) {
            if (xQueueReceive(command_queue, &cmd, portMAX_DELAY) == pdTRUE) {
                // Validate command type
                if (cmd.type < (sizeof(command_handlers) / sizeof(command_handler_t))) {
                    if (command_handlers[cmd.type] != NULL) {
                        // Execute handler
                        command_handlers[cmd.type](&cmd);
                    } else {
                        printf("No handler for command %d\n", cmd.type);
                    }
                } else {
                    printf("Invalid command type: %d\n", cmd.type);
                }
            }
        }
    }

Priority-Based Command Handling
================================

.. code-block:: c

    typedef enum {
        CMD_PRIORITY_LOW = 0,
        CMD_PRIORITY_NORMAL = 1,
        CMD_PRIORITY_HIGH = 2,
        CMD_PRIORITY_CRITICAL = 3
    } command_priority_t;
    
    typedef struct {
        command_t command;
        command_priority_t priority;
        uint32_t sequence_number;  // For ordering within same priority
    } prioritized_command_t;
    
    // Separate queues for each priority
    QueueHandle_t priority_queues[4];
    
    void init_priority_dispatcher(void) {
        priority_queues[CMD_PRIORITY_LOW] = xQueueCreate(10, sizeof(command_t));
        priority_queues[CMD_PRIORITY_NORMAL] = xQueueCreate(10, sizeof(command_t));
        priority_queues[CMD_PRIORITY_HIGH] = xQueueCreate(10, sizeof(command_t));
        priority_queues[CMD_PRIORITY_CRITICAL] = xQueueCreate(5, sizeof(command_t));
    }
    
    bool submit_priority_command(command_t *cmd, command_priority_t priority) {
        return xQueueSend(priority_queues[priority], cmd, pdMS_TO_TICKS(100)) == pdTRUE;
    }
    
    void priority_dispatcher_task(void *param) {
        command_t cmd;
        
        while (1) {
            // Check queues from highest to lowest priority
            for (int pri = CMD_PRIORITY_CRITICAL; pri >= CMD_PRIORITY_LOW; pri--) {
                if (xQueueReceive(priority_queues[pri], &cmd, 0) == pdTRUE) {
                    // Process command
                    command_handlers[cmd.type](&cmd);
                    break;  // Check from top again after processing
                }
            }
            
            // If no commands, block on normal priority queue
            if (xQueueReceive(priority_queues[CMD_PRIORITY_NORMAL], 
                             &cmd, pdMS_TO_TICKS(10)) == pdTRUE) {
                command_handlers[cmd.type](&cmd);
            }
        }
    }

Synchronous Command Execution
==============================

Wait for command completion:

.. code-block:: c

    typedef struct {
        command_t command;
        SemaphoreHandle_t completion_sem;
        int result;
    } sync_command_t;
    
    int execute_command_sync(command_type_t type, uint32_t param1, uint32_t param2) {
        sync_command_t sync_cmd;
        
        sync_cmd.command.type = type;
        sync_cmd.command.param1 = param1;
        sync_cmd.command.param2 = param2;
        sync_cmd.completion_sem = xSemaphoreCreateBinary();
        sync_cmd.result = -1;
        
        // Submit command
        if (xQueueSend(command_queue, &sync_cmd.command, pdMS_TO_TICKS(100)) == pdTRUE) {
            // Wait for completion
            if (xSemaphoreTake(sync_cmd.completion_sem, pdMS_TO_TICKS(5000)) == pdTRUE) {
                int result = sync_cmd.result;
                vSemaphoreDelete(sync_cmd.completion_sem);
                return result;
            } else {
                // Timeout
                vSemaphoreDelete(sync_cmd.completion_sem);
                return -1;
            }
        }
        
        vSemaphoreDelete(sync_cmd.completion_sem);
        return -1;
    }
    
    // Modified handler
    void handle_read_sensor_sync(command_t *cmd) {
        sync_command_t *sync_cmd = (sync_command_t *)cmd;
        
        uint32_t value = sensor_read(cmd->param1);
        sync_cmd->result = value;
        
        // Signal completion
        xSemaphoreGive(sync_cmd->completion_sem);
    }

Command History and Logging
============================

.. code-block:: c

    #define HISTORY_SIZE 100
    
    typedef struct {
        command_t command;
        TickType_t timestamp;
        uint32_t execution_time_us;
        int result;
    } command_history_entry_t;
    
    command_history_entry_t command_history[HISTORY_SIZE];
    uint32_t history_index = 0;
    SemaphoreHandle_t history_mutex;
    
    void log_command(command_t *cmd, int result, uint32_t execution_time_us) {
        xSemaphoreTake(history_mutex, portMAX_DELAY);
        
        command_history[history_index].command = *cmd;
        command_history[history_index].timestamp = xTaskGetTickCount();
        command_history[history_index].execution_time_us = execution_time_us;
        command_history[history_index].result = result;
        
        history_index = (history_index + 1) % HISTORY_SIZE;
        
        xSemaphoreGive(history_mutex);
    }
    
    void print_command_history(void) {
        xSemaphoreTake(history_mutex, portMAX_DELAY);
        
        printf("\n=== Command History ===\n");
        for (int i = 0; i < HISTORY_SIZE; i++) {
            int idx = (history_index + i) % HISTORY_SIZE;
            command_history_entry_t *entry = &command_history[idx];
            
            if (entry->timestamp != 0) {
                printf("[%u] Type:%d Params:(%u,%u) Result:%d Time:%uus\n",
                       (unsigned)entry->timestamp,
                       entry->command.type,
                       (unsigned)entry->command.param1,
                       (unsigned)entry->command.param2,
                       entry->result,
                       (unsigned)entry->execution_time_us);
            }
        }
        
        xSemaphoreGive(history_mutex);
    }

Command Validation
==================

.. code-block:: c

    typedef bool (*command_validator_t)(command_t *cmd);
    
    bool validate_led_command(command_t *cmd) {
        return cmd->param1 < MAX_LEDS;
    }
    
    bool validate_motor_speed(command_t *cmd) {
        return cmd->param1 <= MAX_MOTOR_SPEED;
    }
    
    bool validate_sensor_id(command_t *cmd) {
        return cmd->param1 < NUM_SENSORS;
    }
    
    command_validator_t command_validators[] = {
        [CMD_LED_ON]      = validate_led_command,
        [CMD_LED_OFF]     = validate_led_command,
        [CMD_SET_SPEED]   = validate_motor_speed,
        [CMD_READ_SENSOR] = validate_sensor_id,
        [CMD_RESET_SYSTEM] = NULL  // No validation needed
    };
    
    void validated_dispatcher_task(void *param) {
        command_t cmd;
        
        while (1) {
            if (xQueueReceive(command_queue, &cmd, portMAX_DELAY) == pdTRUE) {
                // Validate command
                bool valid = true;
                if (cmd.type < (sizeof(command_validators) / sizeof(command_validator_t))) {
                    if (command_validators[cmd.type] != NULL) {
                        valid = command_validators[cmd.type](&cmd);
                    }
                }
                
                if (valid) {
                    // Execute command
                    uint32_t start = get_microseconds();
                    command_handlers[cmd.type](&cmd);
                    uint32_t duration = get_microseconds() - start;
                    
                    // Log successful execution
                    log_command(&cmd, 0, duration);
                } else {
                    printf("Command validation failed: type=%d\n", cmd.type);
                    log_command(&cmd, -1, 0);
                }
            }
        }
    }

Undo/Redo Support
=================

.. code-block:: c

    typedef struct {
        command_t forward_command;
        command_t undo_command;
    } undoable_command_t;
    
    #define UNDO_STACK_SIZE 20
    undoable_command_t undo_stack[UNDO_STACK_SIZE];
    int undo_stack_top = -1;
    
    void execute_undoable_command(undoable_command_t *undoable_cmd) {
        // Execute forward command
        command_handlers[undoable_cmd->forward_command.type](&undoable_cmd->forward_command);
        
        // Push to undo stack
        if (undo_stack_top < UNDO_STACK_SIZE - 1) {
            undo_stack_top++;
            undo_stack[undo_stack_top] = *undoable_cmd;
        }
    }
    
    void undo_last_command(void) {
        if (undo_stack_top >= 0) {
            undoable_command_t *undoable_cmd = &undo_stack[undo_stack_top];
            
            // Execute undo command
            command_handlers[undoable_cmd->undo_command.type](&undoable_cmd->undo_command);
            
            undo_stack_top--;
        } else {
            printf("Nothing to undo\n");
        }
    }
    
    // Example usage
    void toggle_led_with_undo(uint32_t led_id) {
        undoable_command_t undoable_cmd;
        
        // Forward: turn LED on
        undoable_cmd.forward_command.type = CMD_LED_ON;
        undoable_cmd.forward_command.param1 = led_id;
        
        // Undo: turn LED off
        undoable_cmd.undo_command.type = CMD_LED_OFF;
        undoable_cmd.undo_command.param1 = led_id;
        
        execute_undoable_command(&undoable_cmd);
    }

Batch Command Processing
=========================

.. code-block:: c

    typedef struct {
        command_t commands[10];
        uint32_t count;
        bool atomic;  // All or nothing
    } command_batch_t;
    
    void execute_batch(command_batch_t *batch) {
        if (batch->atomic) {
            // Validate all commands first
            for (uint32_t i = 0; i < batch->count; i++) {
                if (command_validators[batch->commands[i].type] != NULL) {
                    if (!command_validators[batch->commands[i].type](&batch->commands[i])) {
                        printf("Batch validation failed at command %u\n", (unsigned)i);
                        return;
                    }
                }
            }
        }
        
        // Execute all commands
        for (uint32_t i = 0; i < batch->count; i++) {
            command_handlers[batch->commands[i].type](&batch->commands[i]);
        }
    }

Remote Command Execution
========================

.. code-block:: c

    // Network command packet
    typedef struct __attribute__((packed)) {
        uint8_t command_type;
        uint32_t param1;
        uint32_t param2;
        uint16_t crc;
    } network_command_t;
    
    void network_rx_task(void *param) {
        network_command_t net_cmd;
        command_t local_cmd;
        
        while (1) {
            // Receive command from network
            if (network_receive(&net_cmd, sizeof(net_cmd)) > 0) {
                // Validate CRC
                uint16_t calculated_crc = calculate_crc(&net_cmd, 
                                                        sizeof(net_cmd) - sizeof(uint16_t));
                if (calculated_crc == net_cmd.crc) {
                    // Convert to local command
                    local_cmd.type = net_cmd.command_type;
                    local_cmd.param1 = net_cmd.param1;
                    local_cmd.param2 = net_cmd.param2;
                    local_cmd.response_task = xTaskGetCurrentTaskHandle();
                    
                    // Submit to command dispatcher
                    if (submit_command(local_cmd.type, local_cmd.param1, local_cmd.param2)) {
                        // Wait for response
                        uint32_t result;
                        if (xTaskNotifyWait(0, 0xFFFFFFFF, &result, pdMS_TO_TICKS(1000)) == pdTRUE) {
                            // Send response back over network
                            network_send_response(result);
                        }
                    }
                } else {
                    printf("CRC error in network command\n");
                }
            }
        }
    }

Best Practices
==============

1. **Validate all commands**: Check parameters before execution
2. **Log command execution**: For debugging and auditing
3. **Handle errors gracefully**: Don't crash on bad commands
4. **Use timeouts**: Prevent indefinite blocking
5. **Consider priorities**: Critical commands first
6. **Monitor queue depth**: Detect command backlog
7. **Protect command data**: Use mutexes for shared state
8. **Document command protocol**: Especially for remote commands
9. **Test edge cases**: Invalid commands, queue full, timeouts
10. **Measure performance**: Track command execution times

See Also
========

- :doc:`../days/day06` - Queues and Event Groups
- :doc:`../overview/synchronization` - Queue patterns
- :doc:`producer_consumer` - Queue-based patterns
- :doc:`state_machine_tasks` - State-driven command handling

Further Reading
===============

- "Design Patterns" - Command Pattern (Gang of Four)
- "Pattern-Oriented Software Architecture" - Command Processor pattern
- FreeRTOS queue documentation
