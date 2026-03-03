============================
Watchdog and Fault Handling
============================

Introduction
============

**Watchdog and fault handling** patterns ensure system reliability and recovery from errors. These patterns detect failures, log diagnostic information, and restore normal operation.

**Key Goals:**
- Detect system failures
- Recover automatically when possible
- Preserve diagnostic information
- Prevent cascading failures
- Maintain safety in critical systems

Watchdog Timers
===============

Hardware Watchdog Basics
------------------------

A **hardware watchdog** resets the system if not periodically refreshed, protecting against software hangs.

.. code-block:: c

    #include "stm32f4xx.h"
    
    // Initialize independent watchdog (IWDG)
    void watchdog_init(void) {
        // Enable write access to IWDG registers
        IWDG->KR = 0x5555;
        
        // Set prescaler (40 kHz / 32 = 1.25 kHz)
        IWDG->PR = IWDG_PR_PR_2;
        
        // Set reload value (1.25 kHz / 1250 = 1 Hz = 1 second timeout)
        IWDG->RLR = 1250;
        
        // Wait for registers to update
        while (IWDG->SR != 0);
        
        // Start watchdog
        IWDG->KR = 0xCCCC;
    }
    
    // Feed the watchdog (must be called periodically)
    void watchdog_refresh(void) {
        IWDG->KR = 0xAAAA;
    }

Watchdog Task Pattern
---------------------

.. code-block:: c

    #include "FreeRTOS.h"
    #include "task.h"
    
    #define WATCHDOG_TIMEOUT_MS 1000
    
    void watchdog_task(void *param) {
        const TickType_t refresh_period = pdMS_TO_TICKS(WATCHDOG_TIMEOUT_MS / 2);
        TickType_t last_wake = xTaskGetTickCount();
        
        while (1) {
            // Refresh hardware watchdog
            watchdog_refresh();
            
            // Wait for half the timeout period
            vTaskDelayUntil(&last_wake, refresh_period);
        }
    }
    
    void init_watchdog_system(void) {
        watchdog_init();
        xTaskCreate(watchdog_task, "Watchdog", 128, NULL, 
                    configMAX_PRIORITIES - 1,  // Highest priority
                    NULL);
    }

Multi-Task Watchdog Monitoring
===============================

Track multiple tasks to ensure all are responsive:

.. code-block:: c

    #define MAX_MONITORED_TASKS 8
    
    typedef struct {
        const char *name;
        TaskHandle_t handle;
        TickType_t last_checkin;
        TickType_t timeout_ms;
        bool alive;
    } monitored_task_t;
    
    monitored_task_t monitored_tasks[MAX_MONITORED_TASKS];
    uint32_t num_monitored_tasks = 0;
    SemaphoreHandle_t monitor_mutex;
    
    // Register task for monitoring
    void watchdog_register_task(const char *name, TickType_t timeout_ms) {
        xSemaphoreTake(monitor_mutex, portMAX_DELAY);
        
        if (num_monitored_tasks < MAX_MONITORED_TASKS) {
            monitored_tasks[num_monitored_tasks].name = name;
            monitored_tasks[num_monitored_tasks].handle = xTaskGetCurrentTaskHandle();
            monitored_tasks[num_monitored_tasks].last_checkin = xTaskGetTickCount();
            monitored_tasks[num_monitored_tasks].timeout_ms = timeout_ms;
            monitored_tasks[num_monitored_tasks].alive = true;
            num_monitored_tasks++;
        }
        
        xSemaphoreGive(monitor_mutex);
    }
    
    // Task checks in periodically
    void watchdog_checkin(void) {
        TaskHandle_t current_task = xTaskGetCurrentTaskHandle();
        
        xSemaphoreTake(monitor_mutex, portMAX_DELAY);
        
        for (uint32_t i = 0; i < num_monitored_tasks; i++) {
            if (monitored_tasks[i].handle == current_task) {
                monitored_tasks[i].last_checkin = xTaskGetTickCount();
                monitored_tasks[i].alive = true;
                break;
            }
        }
        
        xSemaphoreGive(monitor_mutex);
    }
    
    // Watchdog monitor task
    void watchdog_monitor_task(void *param) {
        const TickType_t check_period = pdMS_TO_TICKS(100);
        
        while (1) {
            TickType_t now = xTaskGetTickCount();
            bool all_alive = true;
            
            xSemaphoreTake(monitor_mutex, portMAX_DELAY);
            
            for (uint32_t i = 0; i < num_monitored_tasks; i++) {
                TickType_t elapsed = now - monitored_tasks[i].last_checkin;
                TickType_t timeout = pdMS_TO_TICKS(monitored_tasks[i].timeout_ms);
                
                if (elapsed > timeout) {
                    monitored_tasks[i].alive = false;
                    all_alive = false;
                    
                    printf("ERROR: Task '%s' timeout (last checkin: %u ms ago)\n",
                           monitored_tasks[i].name,
                           (unsigned)(elapsed * portTICK_PERIOD_MS));
                }
            }
            
            xSemaphoreGive(monitor_mutex);
            
            // Refresh hardware watchdog only if all tasks alive
            if (all_alive) {
                watchdog_refresh();
            } else {
                // Don't refresh - let system reset
                printf("System will reset due to task timeout\n");
            }
            
            vTaskDelay(check_period);
        }
    }
    
    // Example monitored task
    void critical_task(void *param) {
        watchdog_register_task("CriticalTask", 1000);  // 1 second timeout
        
        while (1) {
            // Do work
            perform_critical_operation();
            
            // Check in with watchdog
            watchdog_checkin();
            
            vTaskDelay(pdMS_TO_TICKS(500));
        }
    }

Fault Detection
===============

Stack Overflow Detection
------------------------

.. code-block:: c

    // FreeRTOS stack overflow hook
    void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName) {
        // Log error
        printf("FATAL: Stack overflow in task '%s'\n", pcTaskName);
        
        // Save diagnostic info before reset
        save_crash_log("Stack overflow", pcTaskName);
        
        // Reset system
        NVIC_SystemReset();
    }

Malloc Failure Detection
-------------------------

.. code-block:: c

    void vApplicationMallocFailedHook(void) {
        printf("FATAL: Heap allocation failed\n");
        printf("Free heap: %u bytes\n", (unsigned)xPortGetFreeHeapSize());
        
        // Log and reset
        save_crash_log("Malloc failed", "HeapExhausted");
        NVIC_SystemReset();
    }

Assert Handling
---------------

.. code-block:: c

    #define ASSERT(condition) \
        do { \
            if (!(condition)) { \
                assert_failed(__FILE__, __LINE__, #condition); \
            } \
        } while(0)
    
    void assert_failed(const char *file, int line, const char *expr) {
        // Disable interrupts
        __disable_irq();
        
        // Log assertion failure
        printf("\n*** ASSERTION FAILED ***\n");
        printf("File: %s\nLine: %d\nExpression: %s\n", file, line, expr);
        
        // Save crash log
        save_crash_log("Assertion failed", expr);
        
        // Optionally: enter debug mode or reset
        #ifdef DEBUG
            __BKPT(0);  // Trigger debugger breakpoint
        #else
            NVIC_SystemReset();
        #endif
        
        while (1);  // Should never reach here
    }

Hard Fault Handler
------------------

.. code-block:: c

    typedef struct {
        uint32_t r0;
        uint32_t r1;
        uint32_t r2;
        uint32_t r3;
        uint32_t r12;
        uint32_t lr;
        uint32_t pc;
        uint32_t psr;
    } fault_stack_frame_t;
    
    void HardFault_Handler_C(fault_stack_frame_t *frame) {
        printf("\n*** HARD FAULT ***\n");
        printf("R0:  0x%08X\n", (unsigned)frame->r0);
        printf("R1:  0x%08X\n", (unsigned)frame->r1);
        printf("R2:  0x%08X\n", (unsigned)frame->r2);
        printf("R3:  0x%08X\n", (unsigned)frame->r3);
        printf("R12: 0x%08X\n", (unsigned)frame->r12);
        printf("LR:  0x%08X\n", (unsigned)frame->lr);
        printf("PC:  0x%08X\n", (unsigned)frame->pc);
        printf("PSR: 0x%08X\n", (unsigned)frame->psr);
        
        // Check fault status registers
        uint32_t cfsr = SCB->CFSR;
        uint32_t hfsr = SCB->HFSR;
        uint32_t mmfar = SCB->MMFAR;
        uint32_t bfar = SCB->BFAR;
        
        printf("\nCFSR: 0x%08X\n", (unsigned)cfsr);
        printf("HFSR: 0x%08X\n", (unsigned)hfsr);
        printf("MMFAR: 0x%08X\n", (unsigned)mmfar);
        printf("BFAR: 0x%08X\n", (unsigned)bfar);
        
        // Save to non-volatile storage
        save_hardfault_info(frame, cfsr, hfsr, mmfar, bfar);
        
        // Reset
        NVIC_SystemReset();
    }
    
    // Assembly wrapper to extract stack frame
    __attribute__((naked))
    void HardFault_Handler(void) {
        __asm volatile (
            "TST LR, #4                \n"  // Test bit 2 of LR
            "ITE EQ                    \n"
            "MRSEQ R0, MSP             \n"  // Main stack
            "MRSNE R0, PSP             \n"  // Process stack
            "B HardFault_Handler_C     \n"
        );
    }

Crash Log System
================

.. code-block:: c

    #define CRASH_LOG_MAGIC 0xDEADBEEF
    
    typedef struct {
        uint32_t magic;
        uint32_t timestamp;
        char reason[64];
        char task_name[32];
        uint32_t pc;
        uint32_t lr;
        uint32_t psr;
        uint32_t cfsr;
        uint32_t hfsr;
    } crash_log_t;
    
    // Store in special RAM section that survives reset
    __attribute__((section(".noinit")))
    crash_log_t crash_log;
    
    void save_crash_log(const char *reason, const char *task_name) {
        crash_log.magic = CRASH_LOG_MAGIC;
        crash_log.timestamp = xTaskGetTickCount();
        strncpy(crash_log.reason, reason, sizeof(crash_log.reason) - 1);
        strncpy(crash_log.task_name, task_name, sizeof(crash_log.task_name) - 1);
        
        // Could also save to flash or EEPROM
    }
    
    void check_crash_log(void) {
        if (crash_log.magic == CRASH_LOG_MAGIC) {
            printf("\n*** Previous crash detected ***\n");
            printf("Reason: %s\n", crash_log.reason);
            printf("Task: %s\n", crash_log.task_name);
            printf("Timestamp: %u ms\n", (unsigned)crash_log.timestamp);
            printf("PC: 0x%08X\n", (unsigned)crash_log.pc);
            printf("LR: 0x%08X\n", (unsigned)crash_log.lr);
            
            // Send crash report over network/serial
            send_crash_report(&crash_log);
            
            // Clear log
            crash_log.magic = 0;
        }
    }

Error Recovery Strategies
=========================

Graceful Degradation
--------------------

.. code-block:: c

    typedef enum {
        MODE_FULL_FUNCTION,
        MODE_REDUCED,
        MODE_SAFE,
        MODE_EMERGENCY
    } system_mode_t;
    
    system_mode_t current_mode = MODE_FULL_FUNCTION;
    uint32_t error_count = 0;
    
    void handle_error(error_type_t error) {
        error_count++;
        
        // Log error
        log_error(error);
        
        // Adjust system mode based on error severity
        if (error == ERROR_CRITICAL) {
            current_mode = MODE_EMERGENCY;
            disable_non_essential_functions();
        } else if (error_count > 10 && current_mode == MODE_FULL_FUNCTION) {
            current_mode = MODE_REDUCED;
            reduce_functionality();
        }
        
        printf("System mode: %d (errors: %u)\n", current_mode, (unsigned)error_count);
    }
    
    void control_task(void *param) {
        while (1) {
            switch (current_mode) {
                case MODE_FULL_FUNCTION:
                    run_all_features();
                    break;
                
                case MODE_REDUCED:
                    run_essential_features();
                    break;
                
                case MODE_SAFE:
                    run_safe_mode();
                    break;
                
                case MODE_EMERGENCY:
                    emergency_shutdown();
                    break;
            }
            
            vTaskDelay(pdMS_TO_TICKS(100));
        }
    }

Task Restart
------------

.. code-block:: c

    typedef struct {
        TaskHandle_t handle;
        TaskFunction_t function;
        const char *name;
        uint32_t stack_size;
        void *parameters;
        UBaseType_t priority;
        uint32_t restart_count;
        uint32_t max_restarts;
    } restartable_task_t;
    
    restartable_task_t tasks[MAX_TASKS];
    uint32_t num_tasks = 0;
    
    void register_restartable_task(TaskFunction_t func, const char *name,
                                   uint32_t stack_size, void *param,
                                   UBaseType_t priority, uint32_t max_restarts) {
        restartable_task_t *task = &tasks[num_tasks++];
        
        task->function = func;
        task->name = name;
        task->stack_size = stack_size;
        task->parameters = param;
        task->priority = priority;
        task->restart_count = 0;
        task->max_restarts = max_restarts;
        
        xTaskCreate(func, name, stack_size, param, priority, &task->handle);
    }
    
    void restart_task(restartable_task_t *task) {
        if (task->restart_count >= task->max_restarts) {
            printf("Task '%s' reached max restarts (%u), not restarting\n",
                   task->name, (unsigned)task->max_restarts);
            return;
        }
        
        printf("Restarting task '%s' (restart #%u)\n",
               task->name, (unsigned)task->restart_count + 1);
        
        // Delete old task
        if (task->handle != NULL) {
            vTaskDelete(task->handle);
        }
        
        // Create new task
        xTaskCreate(task->function,
                    task->name,
                    task->stack_size,
                    task->parameters,
                    task->priority,
                    &task->handle);
        
        task->restart_count++;
    }
    
    // Supervisor task monitors and restarts failed tasks
    void supervisor_task(void *param) {
        while (1) {
            for (uint32_t i = 0; i < num_tasks; i++) {
                restartable_task_t *task = &tasks[i];
                
                // Check if task is still running
                if (eTaskGetState(task->handle) == eDeleted) {
                    printf("Task '%s' has died\n", task->name);
                    restart_task(task);
                }
            }
            
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }

Redundancy and Failover
========================

Dual-Task Redundancy
--------------------

.. code-block:: c

    typedef struct {
        TaskHandle_t primary_task;
        TaskHandle_t backup_task;
        bool primary_active;
        SemaphoreHandle_t switch_mutex;
    } redundant_system_t;
    
    redundant_system_t redundant_system;
    
    void primary_control_task(void *param) {
        while (1) {
            if (redundant_system.primary_active) {
                // Primary is active - do work
                perform_control();
                watchdog_checkin();
            } else {
                // Backup has taken over - go dormant
                vTaskDelay(pdMS_TO_TICKS(100));
            }
        }
    }
    
    void backup_control_task(void *param) {
        while (1) {
            if (!redundant_system.primary_active) {
                // Primary failed - backup takes over
                perform_control();
                watchdog_checkin();
            } else {
                // Primary is healthy - standby
                vTaskDelay(pdMS_TO_TICKS(100));
            }
        }
    }
    
    void failover_monitor_task(void *param) {
        while (1) {
            // Monitor primary task health
            if (!is_task_healthy(redundant_system.primary_task)) {
                printf("Primary task failed - switching to backup\n");
                
                xSemaphoreTake(redundant_system.switch_mutex, portMAX_DELAY);
                redundant_system.primary_active = false;
                xSemaphoreGive(redundant_system.switch_mutex);
            }
            
            vTaskDelay(pdMS_TO_TICKS(100));
        }
    }

Checkpointing and Recovery
===========================

.. code-block:: c

    typedef struct {
        uint32_t state_variable_1;
        uint32_t state_variable_2;
        float coefficients[10];
        uint32_t crc;
    } checkpoint_t;
    
    checkpoint_t checkpoint;
    
    void save_checkpoint(void) {
        checkpoint.state_variable_1 = get_state_1();
        checkpoint.state_variable_2 = get_state_2();
        memcpy(checkpoint.coefficients, get_coefficients(), sizeof(checkpoint.coefficients));
        
        // Calculate CRC
        checkpoint.crc = calculate_crc(&checkpoint, 
                                       sizeof(checkpoint) - sizeof(uint32_t));
        
        // Save to non-volatile storage
        flash_write(CHECKPOINT_ADDRESS, &checkpoint, sizeof(checkpoint));
    }
    
    bool restore_checkpoint(void) {
        // Read from non-volatile storage
        flash_read(CHECKPOINT_ADDRESS, &checkpoint, sizeof(checkpoint));
        
        // Verify CRC
        uint32_t calculated_crc = calculate_crc(&checkpoint,
                                                sizeof(checkpoint) - sizeof(uint32_t));
        
        if (calculated_crc != checkpoint.crc) {
            printf("Checkpoint corrupted\n");
            return false;
        }
        
        // Restore state
        set_state_1(checkpoint.state_variable_1);
        set_state_2(checkpoint.state_variable_2);
        set_coefficients(checkpoint.coefficients);
        
        printf("State restored from checkpoint\n");
        return true;
    }

Best Practices
==============

1. **Use both hardware and software watchdogs**: Defense in depth
2. **Monitor all critical tasks**: Don't assume they're running
3. **Save diagnostic information**: Enables post-mortem analysis
4. **Test fault scenarios**: Simulate failures during development
5. **Implement graceful degradation**: Don't crash unnecessarily
6. **Set appropriate timeouts**: Too short = false positives, too long = slow detection
7. **Log everything**: Errors, warnings, state transitions
8. **Use checksums**: Validate critical data structures
9. **Implement safe defaults**: Known-good state on error
10. **Document recovery procedures**: For both automatic and manual recovery

Common Pitfalls
===============

1. **Watchdog starvation**: High-priority tasks prevent watchdog refresh
2. **Incomplete error handling**: Some error paths not handled
3. **Recursive faults**: Error handler itself crashes
4. **Lost diagnostic data**: Crash info not preserved across reset
5. **False positives**: Overly aggressive fault detection

See Also
========

- :doc:`../days/day05` - Semaphores and Mutexes (for coordination)
- :doc:`../overview/interrupts` - Fault interrupts
- :doc:`../overview/memory_management` - Stack overflow detection
- :doc:`state_machine_tasks` - Error state handling

Further Reading
===============

- "Embedded Software Development for Safety-Critical Systems" - Chris Hobbs
- IEC 61508 Functional Safety standard
- ARM Cortex-M Fault Handling documentation
- Watchdog timer application notes
