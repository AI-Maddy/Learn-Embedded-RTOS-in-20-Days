=====================
Azure ThreadX Cheat Sheet
=====================

Quick Start
===========

Initialization
--------------

.. code-block:: c

    #include "tx_api.h"
    
    int main(void) {
        // Enter ThreadX kernel
        tx_kernel_enter();
        // Never returns
    }
    
    // Application definition function
    void tx_application_define(void *first_unused_memory) {
        // Create threads, semaphores, queues, etc.
        
        UCHAR *pointer = (UCHAR *)first_unused_memory;
        
        // Create thread
        tx_thread_create(&my_thread, "My Thread", 
                         my_thread_entry, 0,
                         pointer, 1024,
                         16, 16,
                         TX_NO_TIME_SLICE,
                         TX_AUTO_START);
        
        pointer += 1024;
    }

Thread API
==========

Create/Delete
-------------

.. code-block:: c

    TX_THREAD my_thread;
    UCHAR my_thread_stack[1024];
    
    UINT tx_thread_create(
        TX_THREAD *thread_ptr,
        CHAR *name_ptr,
        VOID (*entry_function)(ULONG),
        ULONG entry_input,
        VOID *stack_start,
        ULONG stack_size,
        UINT priority,              // 0 = highest, 31 = lowest
        UINT preempt_threshold,     // 0-31, disable preemption
        ULONG time_slice,           // TX_NO_TIME_SLICE
        UINT auto_start             // TX_AUTO_START or TX_DONT_START
    );
    
    // Delete thread
    UINT tx_thread_delete(TX_THREAD *thread_ptr);
    
    // Thread entry function
    void my_thread_entry(ULONG thread_input) {
        while (1) {
            // Thread body
            tx_thread_sleep(100);
        }
    }

Sleep/Delay
-----------

.. code-block:: c

    // Sleep (ticks)
    UINT tx_thread_sleep(ULONG timer_ticks);
    
    // Relinquish (yield to equal priority)
    VOID tx_thread_relinquish(VOID);

Priority/Control
----------------

.. code-block:: c

    // Change priority
    UINT tx_thread_priority_change(
        TX_THREAD *thread_ptr,
        UINT new_priority,
        UINT *old_priority
    );
    
    // Suspend/Resume
    UINT tx_thread_suspend(TX_THREAD *thread_ptr);
    UINT tx_thread_resume(TX_THREAD *thread_ptr);
    
    // Terminate thread
    UINT tx_thread_terminate(TX_THREAD *thread_ptr);
    
    // Reset thread (restart from entry point)
    UINT tx_thread_reset(TX_THREAD *thread_ptr);
    
    // Preemption threshold (prevent preemption by lower priorities)
    UINT tx_thread_preemption_change(
        TX_THREAD *thread_ptr,
        UINT new_threshold,
        UINT *old_threshold
    );
    
    // Time slice change
    UINT tx_thread_time_slice_change(
        TX_THREAD *thread_ptr,
        ULONG new_time_slice,
        ULONG *old_time_slice
    );

Thread Info
-----------

.. code-block:: c

    // Get current thread
    TX_THREAD *tx_thread_identify(VOID);
    
    // Get thread info
    UINT tx_thread_info_get(
        TX_THREAD *thread_ptr,
        CHAR **name,
        UINT *state,
        ULONG *run_count,
        UINT *priority,
        UINT *preemption_threshold,
        ULONG *time_slice,
        TX_THREAD **next_thread,
        TX_THREAD **suspended_thread
    );
    
    // States: TX_READY, TX_COMPLETED, TX_TERMINATED, TX_SUSPENDED, etc.

Semaphore API
=============

.. code-block:: c

    TX_SEMAPHORE my_semaphore;
    
    // Create
    UINT tx_semaphore_create(
        TX_SEMAPHORE *semaphore_ptr,
        CHAR *name_ptr,
        ULONG initial_count
    );
    
    // Delete
    UINT tx_semaphore_delete(TX_SEMAPHORE *semaphore_ptr);
    
    // Get (wait/decrement)
    UINT tx_semaphore_get(
        TX_SEMAPHORE *semaphore_ptr,
        ULONG wait_option
    );
    // wait_option: TX_NO_WAIT, TX_WAIT_FOREVER, or timeout value
    
    // Put (signal/increment)
    UINT tx_semaphore_put(TX_SEMAPHORE *semaphore_ptr);
    
    // Prioritize waiting threads (default is FIFO)
    UINT tx_semaphore_prioritize(TX_SEMAPHORE *semaphore_ptr);
    
    // Get info
    UINT tx_semaphore_info_get(
        TX_SEMAPHORE *semaphore_ptr,
        CHAR **name,
        ULONG *current_value,
        TX_THREAD **first_suspended,
        ULONG *suspended_count,
        TX_SEMAPHORE **next_semaphore
    );

Mutex API
=========

.. code-block:: c

    TX_MUTEX my_mutex;
    
    // Create
    UINT tx_mutex_create(
        TX_MUTEX *mutex_ptr,
        CHAR *name_ptr,
        UINT priority_inherit  // TX_INHERIT or TX_NO_INHERIT
    );
    
    // Delete
    UINT tx_mutex_delete(TX_MUTEX *mutex_ptr);
    
    // Get (lock)
    UINT tx_mutex_get(
        TX_MUTEX *mutex_ptr,
        ULONG wait_option
    );
    
    // Put (unlock)
    UINT tx_mutex_put(TX_MUTEX *mutex_ptr);
    
    // Prioritize waiting threads
    UINT tx_mutex_prioritize(TX_MUTEX *mutex_ptr);
    
    // Info
    UINT tx_mutex_info_get(
        TX_MUTEX *mutex_ptr,
        CHAR **name,
        ULONG *count,
        TX_THREAD **owner,
        TX_THREAD **first_suspended,
        ULONG *suspended_count,
        TX_MUTEX **next_mutex
    );

⚠️ **Supports priority inheritance, recursive locking allowed**

Queue API
=========

.. code-block:: c

    TX_QUEUE my_queue;
    ULONG my_queue_storage[100];  // Space for messages
    
    // Create
    UINT tx_queue_create(
        TX_QUEUE *queue_ptr,
        CHAR *name_ptr,
        UINT message_size,          // In ULONG words (1, 2, 4, 8, 16)
        VOID *queue_start,
        ULONG queue_size            // Total bytes
    );
    
    // Delete
    UINT tx_queue_delete(TX_QUEUE *queue_ptr);
    
    // Send (to back)
    UINT tx_queue_send(
        TX_QUEUE *queue_ptr,
        VOID *source_ptr,
        ULONG wait_option
    );
    
    // Send to front (urgent)
    UINT tx_queue_front_send(
        TX_QUEUE *queue_ptr,
        VOID *source_ptr,
        ULONG wait_option
    );
    
    // Receive
    UINT tx_queue_receive(
        TX_QUEUE *queue_ptr,
        VOID *destination_ptr,
        ULONG wait_option
    );
    
    // Flush (empty)
    UINT tx_queue_flush(TX_QUEUE *queue_ptr);
    
    // Prioritize waiting threads
    UINT tx_queue_prioritize(TX_QUEUE *queue_ptr);
    
    // Send notify callback
    UINT tx_queue_send_notify(
        TX_QUEUE *queue_ptr,
        VOID (*queue_send_notify)(TX_QUEUE *)
    );
    
    // Info
    UINT tx_queue_info_get(
        TX_QUEUE *queue_ptr,
        CHAR **name,
        ULONG *enqueued,
        ULONG *available_storage,
        TX_THREAD **first_suspended,
        ULONG *suspended_count,
        TX_QUEUE **next_queue
    );

Event Flags API
===============

.. code-block:: c

    TX_EVENT_FLAGS_GROUP my_event_flags;
    
    // Create
    UINT tx_event_flags_create(
        TX_EVENT_FLAGS_GROUP *group_ptr,
        CHAR *name_ptr
    );
    
    // Delete
    UINT tx_event_flags_delete(TX_EVENT_FLAGS_GROUP *group_ptr);
    
    // Set flags
    UINT tx_event_flags_set(
        TX_EVENT_FLAGS_GROUP *group_ptr,
        ULONG flags_to_set,
        UINT set_option         // TX_OR or TX_AND
    );
    
    // Get flags (wait)
    UINT tx_event_flags_get(
        TX_EVENT_FLAGS_GROUP *group_ptr,
        ULONG requested_flags,
        UINT get_option,        // TX_OR | TX_AND, TX_OR_CLEAR, TX_AND_CLEAR
        ULONG *actual_flags_ptr,
        ULONG wait_option
    );
    
    // Info
    UINT tx_event_flags_info_get(
        TX_EVENT_FLAGS_GROUP *group_ptr,
        CHAR **name,
        ULONG *current_flags,
        TX_THREAD **first_suspended,
        ULONG *suspended_count,
        TX_EVENT_FLAGS_GROUP **next_group
    );
    
    // Notify callback
    UINT tx_event_flags_set_notify(
        TX_EVENT_FLAGS_GROUP *group_ptr,
        VOID (*events_set_notify)(TX_EVENT_FLAGS_GROUP *)
    );

Block Memory Pool API
=====================

Fixed-size block allocator:

.. code-block:: c

    TX_BLOCK_POOL my_pool;
    UCHAR my_pool_storage[1000];
    
    // Create
    UINT tx_block_pool_create(
        TX_BLOCK_POOL *pool_ptr,
        CHAR *name_ptr,
        ULONG block_size,
        VOID *pool_start,
        ULONG pool_size
    );
    
    // Delete
    UINT tx_block_pool_delete(TX_BLOCK_POOL *pool_ptr);
    
    // Allocate block
    VOID *block_ptr;
    UINT tx_block_allocate(
        TX_BLOCK_POOL *pool_ptr,
        VOID **block_ptr,
        ULONG wait_option
    );
    
    // Release block
    UINT tx_block_release(VOID *block_ptr);
    
    // Prioritize
    UINT tx_block_pool_prioritize(TX_BLOCK_POOL *pool_ptr);
    
    // Info
    UINT tx_block_pool_info_get(
        TX_BLOCK_POOL *pool_ptr,
        CHAR **name,
        ULONG *available_blocks,
        ULONG *total_blocks,
        TX_THREAD **first_suspended,
        ULONG *suspended_count,
        TX_BLOCK_POOL **next_pool
    );

Byte Memory Pool API
====================

Variable-size allocator (like heap):

.. code-block:: c

    TX_BYTE_POOL my_byte_pool;
    UCHAR my_byte_pool_storage[2000];
    
    // Create
    UINT tx_byte_pool_create(
        TX_BYTE_POOL *pool_ptr,
        CHAR *name_ptr,
        VOID *pool_start,
        ULONG pool_size
    );
    
    // Delete
    UINT tx_byte_pool_delete(TX_BYTE_POOL *pool_ptr);
    
    // Allocate
    VOID *memory_ptr;
    UINT tx_byte_allocate(
        TX_BYTE_POOL *pool_ptr,
        VOID **memory_ptr,
        ULONG memory_size,
        ULONG wait_option
    );
    
    // Release
    UINT tx_byte_release(VOID *memory_ptr);
    
    // Prioritize
    UINT tx_byte_pool_prioritize(TX_BYTE_POOL *pool_ptr);
    
    // Info
    UINT tx_byte_pool_info_get(
        TX_BYTE_POOL *pool_ptr,
        CHAR **name,
        ULONG *available_bytes,
        ULONG *fragments,
        TX_THREAD **first_suspended,
        ULONG *suspended_count,
        TX_BYTE_POOL **next_pool
    );

Timer API
=========

.. code-block:: c

    TX_TIMER my_timer;
    
    // Timer callback
    void timer_expiration_function(ULONG id) {
        // Called from timer thread context
    }
    
    // Create
    UINT tx_timer_create(
        TX_TIMER *timer_ptr,
        CHAR *name_ptr,
        VOID (*expiration_function)(ULONG),
        ULONG expiration_input,
        ULONG initial_ticks,
        ULONG reschedule_ticks,     // 0 = one-shot
        UINT auto_activate          // TX_AUTO_ACTIVATE or TX_NO_ACTIVATE
    );
    
    // Delete
    UINT tx_timer_delete(TX_TIMER *timer_ptr);
    
    // Activate/Deactivate
    UINT tx_timer_activate(TX_TIMER *timer_ptr);
    UINT tx_timer_deactivate(TX_TIMER *timer_ptr);
    
    // Change
    UINT tx_timer_change(
        TX_TIMER *timer_ptr,
        ULONG initial_ticks,
        ULONG reschedule_ticks
    );
    
    // Info
    UINT tx_timer_info_get(
        TX_TIMER *timer_ptr,
        CHAR **name,
        UINT *active,
        ULONG *remaining_ticks,
        ULONG *reschedule_ticks,
        TX_TIMER **next_timer
    );

Interrupt Control
=================

.. code-block:: c

    // Disable interrupts
    UINT posture = tx_interrupt_control(TX_INT_DISABLE);
    
    // Restore interrupts
    tx_interrupt_control(posture);

Time Services
=============

.. code-block:: c

    // Get system time (ticks since startup)
    ULONG tx_time_get(VOID);
    
    // Set system time
    VOID tx_time_set(ULONG new_time);
    
    // Convert ms to ticks (assuming 1000 ticks/sec)
    #define MS_TO_TICKS(ms) (ms)
    #define TICKS_TO_MS(ticks) (ticks)

Return Values
=============

.. code-block:: c

    TX_SUCCESS              // 0x00 - Successful
    TX_DELETED              // 0x01 - Object was deleted
    TX_NO_MEMORY            // 0x10 - Pool empty
    TX_POOL_ERROR           // 0x02 - Invalid pool pointer
    TX_PTR_ERROR            // 0x03 - Invalid pointer
    TX_WAIT_ERROR           // 0x04 - Wait option invalid from ISR
    TX_SIZE_ERROR           // 0x05 - Invalid size
    TX_GROUP_ERROR          // 0x06 - Invalid event group
    TX_NO_EVENTS            // 0x07 - No events
    TX_OPTION_ERROR         // 0x08 - Invalid option
    TX_QUEUE_ERROR          // 0x09 - Invalid queue pointer
    TX_QUEUE_EMPTY          // 0x0A - Queue is empty
    TX_QUEUE_FULL           // 0x0B - Queue is full
    TX_SEMAPHORE_ERROR      // 0x0C - Invalid semaphore
    TX_NO_INSTANCE          // 0x0D - No available instance
    TX_THREAD_ERROR         // 0x0E - Invalid thread pointer
    TX_PRIORITY_ERROR       // 0x0F - Invalid priority
    TX_START_ERROR          // 0x10 - Invalid auto-start
    TX_DELETE_ERROR         // 0x11 - Thread not terminated
    TX_RESUME_ERROR         // 0x12 - Thread not suspended
    TX_CALLER_ERROR         // 0x13 - Invalid caller
    TX_SUSPEND_ERROR        // 0x14 - Invalid suspend
    TX_TIMER_ERROR          // 0x15 - Invalid timer
    TX_TICK_ERROR           // 0x16 - Invalid tick value
    TX_ACTIVATE_ERROR       // 0x17 - Timer already active
    TX_THRESH_ERROR         // 0x18 - Invalid threshold
    TX_SUSPEND_LIFTED       // 0x19 - Delayed suspend was lifted
    TX_WAIT_ABORTED         // 0x1A - Wait was aborted
    TX_MUTEX_ERROR          // 0x1C - Invalid mutex pointer
    TX_NOT_AVAILABLE        // 0x1D - Service not available
    TX_NOT_OWNED            // 0x1E - Not owned by caller
    TX_INHERIT_ERROR        // 0x1F - Invalid inherit

Example Application
===================

.. code-block:: c

    #include "tx_api.h"
    
    #define STACK_SIZE 1024
    
    TX_THREAD producer_thread;
    TX_THREAD consumer_thread;
    TX_QUEUE message_queue;
    
    UCHAR producer_stack[STACK_SIZE];
    UCHAR consumer_stack[STACK_SIZE];
    ULONG queue_storage[100];
    
    void producer_entry(ULONG input) {
        ULONG message = 0;
        
        while (1) {
            tx_queue_send(&message_queue, &message, TX_WAIT_FOREVER);
            message++;
            tx_thread_sleep(100);
        }
    }
    
    void consumer_entry(ULONG input) {
        ULONG message;
        
        while (1) {
            tx_queue_receive(&message_queue, &message, TX_WAIT_FOREVER);
            // Process message
        }
    }
    
    void tx_application_define(void *first_unused_memory) {
        // Create queue
        tx_queue_create(&message_queue, "Message Queue",
                       TX_1_ULONG, queue_storage, sizeof(queue_storage));
        
        // Create threads
        tx_thread_create(&producer_thread, "Producer",
                        producer_entry, 0,
                        producer_stack, STACK_SIZE,
                        16, 16, TX_NO_TIME_SLICE, TX_AUTO_START);
        
        tx_thread_create(&consumer_thread, "Consumer",
                        consumer_entry, 0,
                        consumer_stack, STACK_SIZE,
                        16, 16, TX_NO_TIME_SLICE, TX_AUTO_START);
    }
    
    int main(void) {
        // Hardware initialization
        
        // Enter ThreadX
        tx_kernel_enter();
        
        return 0;  // Never reached
    }

Common Pitfalls
===============

.. code-block:: text

    ✗ Using blocking calls from ISR (use TX_NO_WAIT)
    ✗ Not checking return values (always check!)
    ✗ Queue message size mismatch
    ✗ Insufficient stack size
    ✗ Priority 0 is highest (inverse of some RTOSes)
    ✗ Forgetting to create objects in tx_application_define()
    ✗ Mutex count > 1 means recursive locks
    ✗ Using tx_thread_sleep(0) expecting yield (use relinquish)

See Also
========

- :doc:`rtos_basics_cheatsheet` - Generic RTOS concepts
- :doc:`../days/day14` - ThreadX deep dive
- Official Azure RTOS documentation: docs.microsoft.com/azure-rtos
