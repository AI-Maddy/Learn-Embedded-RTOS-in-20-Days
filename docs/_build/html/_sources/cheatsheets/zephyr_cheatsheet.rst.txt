=====================
Zephyr RTOS Cheat Sheet
=====================

Quick Start
===========

Project Structure
-----------------

.. code-block:: text

    project/
    ├── CMakeLists.txt
    ├── prj.conf          # Configuration
    ├── src/
    │   └── main.c
    └── boards/           # Optional board-specific config

Build Commands
--------------

.. code-block:: bash

    # Setup environment
    source ~/zephyrproject/.venv/bin/activate
    export ZEPHYR_BASE=~/zephyrproject/zephyr
    
    # Build
    west build -b <board> -p
    
    # Flash
    west flash
    
    # Debug
    west debug

Configuration (prj.conf)
------------------------

.. code-block:: text

    CONFIG_HEAP_MEM_POOL_SIZE=16384
    CONFIG_MAIN_STACK_SIZE=2048
    CONFIG_SYSTEM_WORKQUEUE_STACK_SIZE=2048
    CONFIG_LOG=y
    CONFIG_LOG_DEFAULT_LEVEL=3
    CONFIG_THREAD_NAME=y
    CONFIG_THREAD_MONITOR=y

Thread API
==========

Create/Spawn
------------

.. code-block:: c

    #include <zephyr/kernel.h>
    
    // Static thread definition
    K_THREAD_DEFINE(
        my_thread_id,           // Name (creates struct k_thread)
        1024,                   // Stack size
        my_thread_entry,        // Entry function
        NULL, NULL, NULL,       // 3 parameters
        5,                      // Priority (lower = higher)
        0,                      // Options
        0                       // Delay before start (0=immediate)
    );
    
    // Dynamic thread creation
    struct k_thread my_thread_data;
    K_THREAD_STACK_DEFINE(my_stack_area, 1024);
    
    k_tid_t tid = k_thread_create(
        &my_thread_data,
        my_stack_area,
        K_THREAD_STACK_SIZEOF(my_stack_area),
        my_thread_entry,
        NULL, NULL, NULL,       // Parameters
        5,                      // Priority (-16 to 15)
        0,                      // Options
        K_NO_WAIT               // Delay
    );
    
    // Entry point
    void my_thread_entry(void *p1, void *p2, void *p3) {
        while (1) {
            // Work
            k_sleep(K_MSEC(100));
        }
    }

Sleep/Delay
-----------

.. code-block:: c

    // Sleep for duration
    k_sleep(K_MSEC(100));
    k_sleep(K_SECONDS(1));
    k_sleep(K_MINUTES(1));
    k_sleep(K_HOURS(1));
    k_sleep(K_USEC(500));
    k_sleep(K_NSEC(1000));
    k_sleep(K_TICKS(10));
    k_sleep(K_FOREVER);        // Sleep indefinitely
    
    // Busy wait (doesn't yield)
    k_busy_wait(100);          // Microseconds
    
    // Yield to equal/higher priority
    k_yield();

Priority/Suspend
----------------

.. code-block:: c

    // Change priority
    k_thread_priority_set(k_current_get(), 10);
    int old_prio = k_thread_priority_get(tid);
    
    // Suspend/Resume
    k_thread_suspend(tid);
    k_thread_resume(tid);
    
    // Abort (terminate)
    k_thread_abort(tid);
    
    // Join (wait for termination)
    k_thread_join(tid, K_FOREVER);

Thread Info
-----------

.. code-block:: c

    // Get current thread
    k_tid_t tid = k_current_get();
    
    // Get thread name
    const char *name = k_thread_name_get(tid);
    
    // Set thread name
    k_thread_name_set(tid, "MyThread");

Semaphore API
=============

.. code-block:: c

    #include <zephyr/kernel.h>
    
    // Define
    struct k_sem my_sem;
    
    // Initialize
    k_sem_init(&my_sem, 0, 1);  // initial=0, limit=1 (binary)
    k_sem_init(&my_sem, 0, 10); // Counting semaphore
    
    // Static definition
    K_SEM_DEFINE(my_sem, 0, 1);
    
    // Take (wait)
    int ret = k_sem_take(&my_sem, K_FOREVER);
    // Returns 0 on success, -EAGAIN on timeout
    
    // Give (signal)
    k_sem_give(&my_sem);
    
    // Get count
    unsigned int count = k_sem_count_get(&my_sem);
    
    // Reset
    k_sem_reset(&my_sem);

Mutex API
=========

.. code-block:: c

    // Define
    struct k_mutex my_mutex;
    
    // Initialize
    k_mutex_init(&my_mutex);
    
    // Static definition
    K_MUTEX_DEFINE(my_mutex);
    
    // Lock
    k_mutex_lock(&my_mutex, K_FOREVER);
    
    // Unlock
    k_mutex_unlock(&my_mutex);

⚠️ **Supports priority inheritance by default**

Message Queue API
=================

.. code-block:: c

    // Define message structure
    struct data_item {
        uint32_t field1;
        uint32_t field2;
    };
    
    // Define queue
    struct k_msgq my_msgq;
    char __aligned(4) my_msgq_buffer[10 * sizeof(struct data_item)];
    
    // Initialize
    k_msgq_init(&my_msgq, my_msgq_buffer, sizeof(struct data_item), 10);
    
    // Static definition
    K_MSGQ_DEFINE(my_msgq, sizeof(struct data_item), 10, 4);
    
    // Send (copy data)
    struct data_item tx_data = {1, 2};
    int ret = k_msgq_put(&my_msgq, &tx_data, K_FOREVER);
    
    // Receive
    struct data_item rx_data;
    ret = k_msgq_get(&my_msgq, &rx_data, K_FOREVER);
    
    // Peek (don't remove)
    ret = k_msgq_peek(&my_msgq, &rx_data);
    
    // Purge (empty)
    k_msgq_purge(&my_msgq);
    
    // Number of used/free entries
    uint32_t num_used = k_msgq_num_used_get(&my_msgq);
    uint32_t num_free = k_msgq_num_free_get(&my_msgq);

FIFO/LIFO API
=============

For simple pointer queues:

.. code-block:: c

    // Define
    struct k_fifo my_fifo;
    struct k_lifo my_lifo;
    
    // Initialize
    k_fifo_init(&my_fifo);
    k_lifo_init(&my_lifo);
    
    // Static
    K_FIFO_DEFINE(my_fifo);
    K_LIFO_DEFINE(my_lifo);
    
    // Data items must have void* as first field
    struct data_item {
        void *fifo_reserved;   // For kernel use
        uint32_t data;
    };
    
    // Put (add item)
    struct data_item *item = k_malloc(sizeof(struct data_item));
    item->data = 42;
    k_fifo_put(&my_fifo, item);
    
    // Get (retrieve item)
    struct data_item *rx = k_fifo_get(&my_fifo, K_FOREVER);
    
    // LIFO works the same way
    k_lifo_put(&my_lifo, item);
    rx = k_lifo_get(&my_lifo, K_FOREVER);

Event API
=========

.. code-block:: c

    #include <zephyr/kernel.h>
    
    // Define
    struct k_event my_event;
    
    // Initialize
    k_event_init(&my_event);
    
    // Static
    K_EVENT_DEFINE(my_event);
    
    // Post (set bits)
    k_event_post(&my_event, 0x001);
    
    // Wait for events
    uint32_t events = k_event_wait(
        &my_event,
        0x001,              // Events to wait for
        false,              // Reset on read? (true/false)
        K_FOREVER           // Timeout
    );
    
    // Wait for ALL events
    events = k_event_wait_all(
        &my_event,
        0x003,              // Wait for bits 0 and 1
        false,
        K_FOREVER
    );
    
    // Clear events
    k_event_clear(&my_event, 0x001);
    
    // Set events
    k_event_set(&my_event, 0x003);

Poll API
========

Wait on multiple objects:

.. code-block:: c

    #include <zephyr/kernel.h>
    
    struct k_sem sem1, sem2;
    struct k_poll_event events[2];
    
    // Setup poll events
    k_poll_event_init(&events[0], K_POLL_TYPE_SEM_AVAILABLE,
                      K_POLL_MODE_NOTIFY_ONLY, &sem1);
    k_poll_event_init(&events[1], K_POLL_TYPE_SEM_AVAILABLE,
                      K_POLL_MODE_NOTIFY_ONLY, &sem2);
    
    // Wait on any event
    int ret = k_poll(events, 2, K_FOREVER);
    
    // Check which event occurred
    if (events[0].state == K_POLL_STATE_SEM_AVAILABLE) {
        k_sem_take(&sem1, K_NO_WAIT);
    }

Timer API
=========

.. code-block:: c

    // Define
    struct k_timer my_timer;
    
    // Timer callback
    void timer_expiry_fn(struct k_timer *timer_id) {
        // Called from ISR context!
    }
    
    void timer_stop_fn(struct k_timer *timer_id) {
        // Called when timer stopped
    }
    
    // Initialize
    k_timer_init(&my_timer, timer_expiry_fn, timer_stop_fn);
    
    // Static definition
    K_TIMER_DEFINE(my_timer, timer_expiry_fn, timer_stop_fn);
    
    // Start (one-shot)
    k_timer_start(&my_timer, K_MSEC(100), K_NO_WAIT);
    
    // Start (periodic)
    k_timer_start(&my_timer, K_MSEC(100), K_MSEC(100));
    
    // Stop
    k_timer_stop(&my_timer);
    
    // Status
    uint32_t status = k_timer_status_get(&my_timer);
    uint32_t remaining = k_timer_remaining_get(&my_timer);

Work Queue API
==============

Deferred work (like tasklets/bottom-halves):

.. code-block:: c

    #include <zephyr/kernel.h>
    
    // Define work item
    struct k_work my_work;
    
    // Work handler
    void work_handler(struct k_work *work) {
        // Process work
    }
    
    // Initialize
    k_work_init(&my_work, work_handler);
    
    // Submit to system work queue
    k_work_submit(&my_work);
    
    // Delayed work
    struct k_work_delayable my_delayed_work;
    
    k_work_init_delayable(&my_delayed_work, work_handler);
    
    // Schedule delayed work
    k_work_schedule(&my_delayed_work, K_MSEC(1000));
    
    // Custom work queue
    struct k_work_q my_work_q;
    K_THREAD_STACK_DEFINE(work_q_stack, 1024);
    
    k_work_queue_start(&my_work_q, work_q_stack,
                       K_THREAD_STACK_SIZEOF(work_q_stack),
                       5, NULL);
    
    k_work_submit_to_queue(&my_work_q, &my_work);

Memory Management
=================

.. code-block:: c

    #include <zephyr/kernel.h>
    
    // Heap allocation
    void *ptr = k_malloc(100);
    k_free(ptr);
    
    // Calloc (zero-initialized)
    void *ptr = k_calloc(10, sizeof(uint32_t));
    
    // Memory pools (fixed-size blocks)
    K_MEM_POOL_DEFINE(my_pool, 16, 64, 4, 4);  // min, max, blocks, align
    
    struct k_mem_block block;
    int ret = k_mem_pool_alloc(&my_pool, &block, 32, K_NO_WAIT);
    k_mem_pool_free(&block);
    
    // Memory slabs (uniform blocks, faster)
    struct k_mem_slab my_slab;
    char __aligned(4) slab_buffer[10 * 64];
    
    k_mem_slab_init(&my_slab, slab_buffer, 64, 10);
    
    void *ptr;
    k_mem_slab_alloc(&my_slab, &ptr, K_FOREVER);
    k_mem_slab_free(&my_slab, &ptr);

Interrupt Handling
==================

.. code-block:: c

    // Register ISR (direct)
    IRQ_CONNECT(IRQ_NUM, IRQ_PRIORITY, isr_handler, NULL, 0);
    irq_enable(IRQ_NUM);
    
    // ISR handler
    void isr_handler(const void *arg) {
        // ISR code
        // Can call _isr versions of some APIs
    }
    
    // Disable/Enable interrupts
    unsigned int key = irq_lock();
    // Critical section
    irq_unlock(key);

Atomic Operations
=================

.. code-block:: c

    #include <zephyr/sys/atomic.h>
    
    atomic_t my_atomic;
    
    atomic_set(&my_atomic, 0);
    atomic_inc(&my_atomic);
    atomic_dec(&my_atomic);
    atomic_add(&my_atomic, 5);
    atomic_sub(&my_atomic, 3);
    
    bool success = atomic_cas(&my_atomic, 5, 10);  // Compare-and-swap
    
    // Bit operations
    atomic_set_bit(&my_atomic, 3);
    atomic_clear_bit(&my_atomic, 3);
    atomic_test_bit(&my_atomic, 3);

Logging
=======

.. code-block:: c

    #include <zephyr/logging/log.h>
    
    LOG_MODULE_REGISTER(my_module, LOG_LEVEL_DBG);
    
    LOG_ERR("Error: %d", error_code);
    LOG_WRN("Warning message");
    LOG_INF("Info: %s", info_string);
    LOG_DBG("Debug value: %u", value);
    
    LOG_HEXDUMP_DBG(data, len, "Data dump:");

Device Tree
===========

Access devices defined in device tree:

.. code-block:: c

    #include <zephyr/device.h>
    #include <zephyr/drivers/gpio.h>
    
    // Get device from device tree
    const struct device *gpio = DEVICE_DT_GET(DT_NODELABEL(gpio0));
    
    if (!device_is_ready(gpio)) {
        // Device not ready
        return;
    }
    
    // Use device APIs
    gpio_pin_configure(gpio, PIN, GPIO_OUTPUT_ACTIVE);

Time Conversion Macros
======================

.. code-block:: c

    K_NO_WAIT           // Don't wait
    K_FOREVER           // Wait forever
    K_MSEC(ms)          // Milliseconds to timeout
    K_SECONDS(s)        // Seconds to timeout
    K_MINUTES(m)        // Minutes to timeout
    K_HOURS(h)          // Hours to timeout
    K_USEC(us)          // Microseconds
    K_NSEC(ns)          // Nanoseconds
    K_TICKS(t)          // Raw ticks
    
    // Uptime
    int64_t uptime_ms = k_uptime_get();
    uint32_t cycles = k_cycle_get_32();

Example Application
===================

.. code-block:: c

    #include <zephyr/kernel.h>
    #include <zephyr/logging/log.h>
    
    LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);
    
    K_MSGQ_DEFINE(my_msgq, sizeof(uint32_t), 10, 4);
    
    void producer_thread(void *p1, void *p2, void *p3) {
        uint32_t count = 0;
        
        while (1) {
            if (k_msgq_put(&my_msgq, &count, K_NO_WAIT) == 0) {
                LOG_INF("Produced: %u", count);
            }
            count++;
            k_sleep(K_MSEC(1000));
        }
    }
    
    void consumer_thread(void *p1, void *p2, void *p3) {
        uint32_t value;
        
        while (1) {
            if (k_msgq_get(&my_msgq, &value, K_FOREVER) == 0) {
                LOG_INF("Consumed: %u", value);
            }
        }
    }
    
    K_THREAD_DEFINE(producer, 1024, producer_thread, NULL, NULL, NULL, 5, 0, 0);
    K_THREAD_DEFINE(consumer, 1024, consumer_thread, NULL, NULL, NULL, 5, 0, 0);
    
    int main(void) {
        LOG_INF("Zephyr example started");
        return 0;
    }

Common Pitfalls
===============

.. code-block:: text

    ✗ Forgetting to check device_is_ready() for devices
    ✗ Using k_sleep() in ISR context (use k_work instead)
    ✗ Not aligning message queue buffers
    ✗ Priority confusion (lower number = higher priority)
    ✗ Using blocking calls with K_NO_WAIT expecting success
    ✗ Insufficient stack size (check CONFIG_*_STACK_SIZE)
    ✗ Not enabling features in prj.conf

See Also
========

- :doc:`rtos_basics_cheatsheet` - Generic RTOS concepts
- :doc:`../days/day12` - Zephyr deep dive
- Official Zephyr documentation: docs.zephyrproject.org
