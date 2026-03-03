==================
eCos Cheat Sheet
==================

Quick Start
===========

eCos (Embedded Configurable Operating System) is a highly configurable, open-source RTOS.

Configuration
-------------

.. code-block:: bash

    # Create configuration
    ecosconfig new <target> <template>
    
    # Configure (GUI)
    configtool
    
    # Or text-based
    ecosconfig tree
    
    # Build library
    make

Build Application
-----------------

.. code-block:: bash

    # Compile with eCos library
    arm-eabi-gcc -o app main.c -I$ECOS_INSTALL/include -L$ECOS_INSTALL/lib -Ttarget.ld

Thread API (Cyg_Thread)
========================

Create/Start
------------

.. code-block:: c

    #include <cyg/kernel/kapi.h>
    
    cyg_thread thread_obj;
    cyg_handle_t thread_handle;
    cyg_uint8 thread_stack[4096];
    
    // Thread entry point
    void thread_entry(cyg_addrword_t data) {
        while (1) {
            // Thread body
            cyg_thread_delay(100);
        }
    }
    
    // Create thread
    cyg_thread_create(
        10,                     // Priority (0 = highest, 31 = lowest)
        thread_entry,           // Entry function
        (cyg_addrword_t)0,      // Entry data
        "MyThread",             // Name
        thread_stack,           // Stack base
        sizeof(thread_stack),   // Stack size
        &thread_handle,         // Handle
        &thread_obj             // Thread object
    );
    
    // Resume (start) thread
    cyg_thread_resume(thread_handle);
    
    // Delete thread
    cyg_thread_delete(thread_handle);

Delay/Yield
-----------

.. code-block:: c

    // Delay (ticks)
    cyg_thread_delay(100);
    
    // Yield to equal priority
    cyg_thread_yield();
    
    // Sleep until woken
    cyg_thread_suspend(cyg_thread_self());
    
    // Wake thread
    cyg_thread_resume(thread_handle);

Priority/Control
----------------

.. code-block:: c

    // Get/Set priority
    cyg_priority_t old = cyg_thread_set_priority(thread_handle, new_priority);
    cyg_priority_t prio = cyg_thread_get_priority(thread_handle);
    
    // Get current thread
    cyg_handle_t self = cyg_thread_self();
    
    // Kill thread
    cyg_thread_kill(thread_handle);
    
    // Release (allow killed thread to be deleted)
    cyg_thread_release(thread_handle);

Thread Info
-----------

.. code-block:: c

    #include <cyg/kernel/thread.hxx>
    #include <cyg/kernel/thread.inl>
    
    // Get info
    cyg_thread_info info;
    cyg_uint16 id;
    cyg_bool result = cyg_thread_get_next(&thread_handle, &id);
    cyg_thread_get_info(thread_handle, id, &info);
    
    // info contains: handle, id, state, name, priority, stack info

Semaphore API
=============

.. code-block:: c

    #include <cyg/kernel/kapi.h>
    
    cyg_sem_t semaphore;
    
    // Initialize
    cyg_semaphore_init(&semaphore, 0);  // Count = 0
    
    // Wait (decrement)
    cyg_bool_t cyg_semaphore_wait(&semaphore);
    
    // Timed wait (returns false on timeout)
    cyg_bool_t cyg_semaphore_timed_wait(
        &semaphore,
        cyg_current_time() + 100
    );
    
    // Try wait (non-blocking)
    cyg_bool_t got = cyg_semaphore_trywait(&semaphore);
    
    // Post (increment)
    cyg_semaphore_post(&semaphore);
    
    // Peek value
    cyg_count32 value = cyg_semaphore_peek(&semaphore);
    
    // Destroy
    cyg_semaphore_destroy(&semaphore);

Mutex API
=========

.. code-block:: c

    cyg_mutex_t mutex;
    
    // Initialize
    cyg_mutex_init(&mutex);
    
    // Lock
    cyg_bool_t cyg_mutex_lock(&mutex);
    
    // Try lock (non-blocking)
    cyg_bool_t got = cyg_mutex_trylock(&mutex);
    
    // Unlock
    cyg_mutex_unlock(&mutex);
    
    // Release (unblock all waiters)
    cyg_mutex_release(&mutex);
    
    // Destroy
    cyg_mutex_destroy(&mutex);

⚠️ **Supports priority inheritance and ceiling by default**

Condition Variable API
======================

.. code-block:: c

    cyg_cond_t cond;
    cyg_mutex_t mutex;
    
    // Initialize
    cyg_mutex_init(&mutex);
    cyg_cond_init(&cond, &mutex);
    
    // Wait (releases mutex, reacquires on wake)
    cyg_mutex_lock(&mutex);
    while (!condition) {
        cyg_cond_wait(&cond);
    }
    // Process
    cyg_mutex_unlock(&mutex);
    
    // Timed wait
    cyg_bool_t woken = cyg_cond_timed_wait(
        &cond,
        cyg_current_time() + 100
    );
    
    // Signal one waiter
    cyg_cond_signal(&cond);
    
    // Signal all waiters (broadcast)
    cyg_cond_broadcast(&cond);
    
    // Destroy
    cyg_cond_destroy(&cond);

Message Box API
===============

Fixed-size message queue:

.. code-block:: c

    cyg_mbox mbox;
    cyg_handle_t mbox_handle;
    
    // Create
    cyg_mbox_create(&mbox_handle, &mbox);
    
    // Put message (pointer)
    cyg_bool_t cyg_mbox_put(mbox_handle, (void *)msg);
    
    // Timed put
    cyg_bool_t cyg_mbox_timed_put(
        mbox_handle,
        (void *)msg,
        cyg_current_time() + 100
    );
    
    // Try put (non-blocking)
    cyg_bool_t ok = cyg_mbox_tryput(mbox_handle, (void *)msg);
    
    // Get message (blocking)
    void *msg = cyg_mbox_get(mbox_handle);
    
    // Timed get
    void *msg = cyg_mbox_timed_get(mbox_handle, cyg_current_time() + 100);
    
    // Try get (non-blocking)
    void *msg = cyg_mbox_tryget(mbox_handle);
    
    // Peek (don't remove)
    void *msg = cyg_mbox_peek_item(mbox_handle, 0);  // Index
    
    // Get count
    cyg_count32 count = cyg_mbox_peek(mbox_handle);
    
    // Waiting count
    cyg_count32 waiting = cyg_mbox_waiting_to_get(mbox_handle);
    cyg_count32 waiting = cyg_mbox_waiting_to_put(mbox_handle);
    
    // Delete
    cyg_mbox_delete(mbox_handle);

Event Flags API
===============

.. code-block:: c

    cyg_flag_t flag;
    
    // Initialize
    cyg_flag_init(&flag);
    
    // Set bits
    cyg_flag_setbits(&flag, 0x03);
    
    // Clear bits  
    cyg_flag_maskbits(&flag, ~0x03);
    
    // Wait for bits (OR)
    cyg_flag_value_t result = cyg_flag_wait(
        &flag,
        0x03,
        CYG_FLAG_WAITMODE_OR | CYG_FLAG_WAITMODE_CLR
    );
    
    // Wait for bits (AND)
    result = cyg_flag_wait(
        &flag,
        0x03,
        CYG_FLAG_WAITMODE_AND | CYG_FLAG_WAITMODE_CLR
    );
    
    // Timed wait
    result = cyg_flag_timed_wait(
        &flag,
        0x03,
        CYG_FLAG_WAITMODE_OR,
        cyg_current_time() + 100
    );
    
    // Poll (non-blocking)
    result = cyg_flag_poll(&flag, 0x03, CYG_FLAG_WAITMODE_OR);
    
    // Peek value
    cyg_flag_value_t value = cyg_flag_peek(&flag);
    
    // Destroy
    cyg_flag_destroy(&flag);

Alarm API
=========

One-shot or periodic callbacks:

.. code-block:: c

    cyg_alarm alarm_obj;
    cyg_handle_t alarm_handle;
    cyg_handle_t counter_handle;
    
    // Alarm callback
    void alarm_handler(cyg_handle_t alarm, cyg_addrword_t data) {
        // Called from DSR context
    }
    
    // Get system clock counter
    counter_handle = cyg_real_time_clock();
    
    // Create alarm
    cyg_alarm_create(
        counter_handle,
        alarm_handler,
        (cyg_addrword_t)0,
        &alarm_handle,
        &alarm_obj
    );
    
    // Initialize alarm (one-shot)
    cyg_alarm_initialize(alarm_handle, cyg_current_time() + 100, 0);
    
    // Initialize alarm (periodic)
    cyg_alarm_initialize(
        alarm_handle,
        cyg_current_time() + 100,  // Trigger time
        100                         // Period (0 = one-shot)
    );
    
    // Enable/Disable
    cyg_alarm_enable(alarm_handle);
    cyg_alarm_disable(alarm_handle);
    
    // Delete
    cyg_alarm_delete(alarm_handle);

Counter API
===========

.. code-block:: c

    cyg_counter counter_obj;
    cyg_handle_t counter_handle;
    
    // Create counter
    cyg_counter_create(&counter_handle, &counter_obj);
    
    // Tick counter
    cyg_counter_tick(counter_handle);
    
    // Get current value
    cyg_tick_count_t ticks = cyg_counter_current_value(counter_handle);
    
    // Set value
    cyg_counter_set_value(counter_handle, 1000);
    
    // Delete
    cyg_counter_delete(counter_handle);

Clock API
=========

.. code-block:: c

    #include <cyg/kernel/kapi.h>
    
    // Get current time (ticks)
    cyg_tick_count_t now = cyg_current_time();
    
    // Get resolution (nanoseconds per tick)
    cyg_resolution_t res = cyg_clock_get_resolution(cyg_real_time_clock());
    
    // Convert time
    cyg_tick_count_t ticks = cyg_tick_count_t(milliseconds * res.dividend / res.divisor / 1000000);

Interrupt Handling
==================

.. code-block:: c

    #include <cyg/hal/hal_intr.h>
    
    cyg_interrupt interrupt_obj;
    cyg_handle_t interrupt_handle;
    
    // ISR (Interrupt Service Routine)
    cyg_uint32 my_isr(cyg_vector_t vector, cyg_addrword_t data) {
        // Minimal ISR code
        // Return CYG_ISR_HANDLED or CYG_ISR_CALL_DSR
        return CYG_ISR_CALL_DSR;
    }
    
    // DSR (Deferred Service Routine)
    void my_dsr(cyg_vector_t vector, cyg_ucount32 count, cyg_addrword_t data) {
        // More complex processing
        // Safe to call most kernel APIs
    }
    
    // Create interrupt
    cyg_interrupt_create(
        IRQ_VECTOR,
        0,                  // Priority
        (cyg_addrword_t)0,  // Data
        my_isr,
        my_dsr,
        &interrupt_handle,
        &interrupt_obj
    );
    
    // Attach to vector
    cyg_interrupt_attach(interrupt_handle);
    
    // Detach
    cyg_interrupt_detach(interrupt_handle);
    
    // Delete
    cyg_interrupt_delete(interrupt_handle);
    
    // Mask/Unmask
    cyg_interrupt_mask(IRQ_VECTOR);
    cyg_interrupt_unmask(IRQ_VECTOR);
    
    // Acknowledge
    cyg_interrupt_acknowledge(IRQ_VECTOR);

Scheduler Lock
==============

.. code-block:: c

    // Lock scheduler (prevent context switch)
    cyg_scheduler_lock();
    
    // Critical section (interrupts still enabled)
    
    // Unlock scheduler
    cyg_scheduler_unlock();

Critical Sections
=================

.. code-block:: c

    // Disable interrupts (spinlock on SMP)
    CYG_INTERRUPT_STATE state;
    
    cyg_interrupt_disable();
    // Critical section
    cyg_interrupt_enable();
    
    // With state save/restore
    HAL_DISABLE_INTERRUPTS(state);
    // Critical section
    HAL_RESTORE_INTERRUPTS(state);

Memory Allocation
=================

Fixed-size Pools
----------------

.. code-block:: c

    cyg_mempool_fix pool_obj;
    cyg_handle_t pool_handle;
    cyg_uint8 pool_memory[1000];
    
    // Create pool (10 blocks of 100 bytes)
    cyg_mempool_fix_create(
        pool_memory,
        1000,
        100,
        &pool_handle,
        &pool_obj
    );
    
    // Allocate
    void *ptr = cyg_mempool_fix_alloc(pool_handle);
    void *ptr = cyg_mempool_fix_timed_alloc(pool_handle, cyg_current_time() + 100);
    void *ptr = cyg_mempool_fix_try_alloc(pool_handle);
    
    // Free
    cyg_mempool_fix_free(pool_handle, ptr);
    
    // Get info
    cyg_mempool_info info;
    cyg_mempool_fix_get_info(pool_handle, &info);
    
    // Delete
    cyg_mempool_fix_delete(pool_handle);

Variable-size Pools
-------------------

.. code-block:: c

    cyg_mempool_var pool_obj;
    cyg_handle_t pool_handle;
    cyg_uint8 pool_memory[2000];
    
    // Create pool
    cyg_mempool_var_create(
        pool_memory,
        2000,
        &pool_handle,
        &pool_obj
    );
    
    // Allocate
    void *ptr = cyg_mempool_var_alloc(pool_handle, 100);
    void *ptr = cyg_mempool_var_timed_alloc(pool_handle, 100, cyg_current_time() + 100);
    void *ptr = cyg_mempool_var_try_alloc(pool_handle, 100);
    
    // Free
    cyg_mempool_var_free(pool_handle, ptr);
    
    // Get info
    cyg_mempool_info info;
    cyg_mempool_var_get_info(pool_handle, &info);
    
    // Delete
    cyg_mempool_var_delete(pool_handle);

Malloc/Free
-----------

.. code-block:: c

    #include <stdlib.h>
    
    // Allocate
    void *ptr = malloc(100);
    
    // Free
    free(ptr);
    
    // Calloc
    void *ptr = calloc(10, sizeof(int));
    
    // Realloc
    ptr = realloc(ptr, 200);

C++ API
=======

eCos provides C++ wrappers:

.. code-block:: cpp

    #include <cyg/kernel/thread.hxx>
    #include <cyg/kernel/mutex.hxx>
    #include <cyg/kernel/sema.hxx>
    
    // Thread
    class MyThread : public Cyg_Thread {
    public:
        MyThread() : Cyg_Thread(10, thread_entry, (CYG_ADDRWORD)this,
                                "MyThread", stack, sizeof(stack)) {}
        
        static void thread_entry(CYG_ADDRWORD data) {
            MyThread *self = (MyThread *)data;
            self->run();
        }
        
        void run() {
            while (1) {
                Cyg_Thread::delay(100);
            }
        }
        
    private:
        cyg_uint8 stack[4096];
    };
    
    // Mutex
    Cyg_Mutex mutex;
    mutex.lock();
    // Critical section
    mutex.unlock();
    
    // Semaphore
    Cyg_Counting_Semaphore sem(0);
    sem.wait();
    sem.post();

Example Application
===================

.. code-block:: c

    #include <cyg/kernel/kapi.h>
    #include <stdio.h>
    
    cyg_thread producer_thread, consumer_thread;
    cyg_handle_t producer_handle, consumer_handle;
    cyg_uint8 producer_stack[4096], consumer_stack[4096];
    
    cyg_mbox mbox;
    cyg_handle_t mbox_handle;
    
    void producer_entry(cyg_addrword_t data) {
        int count = 0;
        
        while (1) {
            cyg_mbox_put(mbox_handle, (void *)count);
            count++;
            cyg_thread_delay(100);
        }
    }
    
    void consumer_entry(cyg_addrword_t data) {
        void *msg;
        
        while (1) {
            msg = cyg_mbox_get(mbox_handle);
            printf("Received: %d\n", (int)msg);
        }
    }
    
    void cyg_user_start(void) {
        // Create mailbox
        cyg_mbox_create(&mbox_handle, &mbox);
        
        // Create threads
        cyg_thread_create(10, producer_entry, 0, "Producer",
                         producer_stack, sizeof(producer_stack),
                         &producer_handle, &producer_thread);
        
        cyg_thread_create(10, consumer_entry, 0, "Consumer",
                         consumer_stack, sizeof(consumer_stack),
                         &consumer_handle, &consumer_thread);
        
        // Resume threads
        cyg_thread_resume(producer_handle);
        cyg_thread_resume(consumer_handle);
    }

Common Pitfalls
===============

.. code-block:: text

    ✗ Not calling cyg_thread_resume() after cyg_thread_create()
    ✗ Forgetting to configure eCos with required features
    ✗ Priority 0 is highest (inverse of some RTOSes)
    ✗ Deleting threads without calling kill/release
    ✗ Not checking return values (cyg_bool_t)
    ✗ Insufficient stack size
    ✗ Using blocking calls from ISR (use try/DSR instead)
    ✗ Absolute time confusion (use cyg_current_time() + delta)

See Also
========

- :doc:`rtos_basics_cheatsheet` - Generic RTOS concepts
- :doc:`../days/day18` - eCos deep dive
- Official eCos documentation: ecos.sourceware.org
