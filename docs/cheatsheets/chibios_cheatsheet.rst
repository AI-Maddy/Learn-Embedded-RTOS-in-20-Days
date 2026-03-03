=======================
ChibiOS/RT Cheat Sheet
=======================

Quick Start
===========

Configuration
-------------

**chconf.h** - Key kernel settings:

.. code-block:: c

    #define CH_CFG_ST_FREQUENCY                 1000    // System tick (Hz)
    #define CH_CFG_ST_TIMEDELTA                 0       // Tickless idle
    #define CH_CFG_TIME_QUANTUM                 20      // Round-robin quantum
    #define CH_CFG_MEMCORE_SIZE                 0       // Core allocator size
    #define CH_CFG_NO_IDLE_THREAD               FALSE
    #define CH_CFG_OPTIMIZE_SPEED               TRUE
    #define CH_CFG_USE_REGISTRY                 TRUE
    #define CH_CFG_USE_WAITEXIT                 TRUE
    #define CH_CFG_USE_SEMAPHORES               TRUE
    #define CH_CFG_USE_MUTEXES                  TRUE
    #define CH_CFG_USE_CONDVARS                 TRUE
    #define CH_CFG_USE_EVENTS                   TRUE
    #define CH_CFG_USE_MESSAGES                 TRUE
    #define CH_CFG_USE_MAILBOXES                TRUE
    #define CH_CFG_USE_MEMCORE                  TRUE
    #define CH_CFG_USE_HEAP                     TRUE
    #define CH_CFG_USE_MEMPOOLS                 TRUE
    #define CH_CFG_USE_OBJ_FIFOS                TRUE

Thread API
==========

Create/Start
------------

.. code-block:: c

    #include "ch.h"
    
    // Thread function signature
    static THD_FUNCTION(MyThread, arg) {
        (void)arg;  // Unused parameter
        
        while (true) {
            // Thread body
            chThdSleepMilliseconds(100);
        }
    }
    
    // Static thread with working area
    static THD_WORKING_AREA(waMyThread, 128);  // 128 bytes stack
    
    thread_t *tp = chThdCreateStatic(
        waMyThread,
        sizeof(waMyThread),
        NORMALPRIO,         // Priority
        MyThread,           // Function
        NULL                // Argument
    );
    
    // Dynamic thread creation
    thread_t *tp = chThdCreateFromHeap(
        NULL,               // Heap (NULL = default)
        THD_WORKING_AREA_SIZE(128),
        "mythread",         // Name
        NORMALPRIO,
        MyThread,
        NULL
    );

Sleep/Delay
-----------

.. code-block:: c

    // Sleep functions
    chThdSleepMilliseconds(100);
    chThdSleepMicroseconds(500);
    chThdSleepSeconds(1);
    chThdSleep(TIME_MS2I(100));         // Ticks
    chThdSleepUntil(deadline);           // Absolute time
    
    // Yield
    chThdYield();
    
    // Time conversion macros
    TIME_MS2I(ms)       // Milliseconds to ticks
    TIME_US2I(us)       // Microseconds to ticks
    TIME_S2I(s)         // Seconds to ticks
    TIME_I2MS(ticks)    // Ticks to milliseconds

Priority/Control
----------------

.. code-block:: c

    // Set priority
    tprio_t oldprio = chThdSetPriority(NORMALPRIO + 1);
    
    // Get priority
    tprio_t prio = chThdGetPriorityX();
    
    // Suspend/Resume
    chThdSuspendS(&tr);     // System locked version
    chThdResumeS(&tr, MSG_OK);
    
    // Terminate thread
    chThdTerminate(tp);     // Request termination
    chThdExit(MSG_OK);      // Exit self
    chThdWait(tp);          // Wait for termination
    
    // Check termination request
    if (chThdShouldTerminateX()) {
        chThdExit(MSG_OK);
    }

Thread Info
-----------

.. code-block:: c

    // Get current thread
    thread_t *tp = chThdGetSelfX();
    
    // Get thread state
    tstate_t state = tp->state;
    // States: CH_STATE_READY, CH_STATE_CURRENT, CH_STATE_SLEEPING, etc.
    
    // Get thread name
    const char *name = chRegGetThreadNameX(tp);

Semaphore API
=============

.. code-block:: c

    #include "ch.h"
    
    // Define semaphore
    semaphore_t sem;
    
    // Initialize
    chSemObjectInit(&sem, 0);           // Binary (n=0)
    chSemObjectInit(&sem, 5);           // Counting (n=5)
    
    // Wait (decrement)
    msg_t msg = chSemWait(&sem);
    msg_t msg = chSemWaitTimeout(&sem, TIME_MS2I(100));
    msg_t msg = chSemWaitTimeoutS(&sem, TIME_MS2I(100));  // S-class
    
    // Signal (increment)
    void chSemSignal(&sem);
    void chSemSignalI(&sem);            // I-class (from ISR)
    
    // Get counter
    cnt_t count = chSemGetCounterI(&sem);
    
    // Reset
    void chSemReset(&sem, cnt_t n);
    void chSemResetI(&sem, cnt_t n);

Mutex API
=========

.. code-block:: c

    // Define mutex
    mutex_t mtx;
    
    // Initialize
    chMtxObjectInit(&mtx);
    
    // Lock
    void chMtxLock(&mtx);
    bool chMtxTryLock(&mtx);
    msg_t chMtxLockTimeout(&mtx, sysinterval_t timeout);
    
    // Unlock
    void chMtxUnlock(&mtx);
    void chMtxUnlockAll(void);          // Unlock all mutexes owned by thread
    
    // Get next waiting thread
    thread_t *tp = chMtxGetNextMutexX();

⚠️ **Supports priority inheritance and priority ceiling**

Condition Variable API
======================

.. code-block:: c

    // Define
    condition_variable_t cond;
    
    // Initialize
    chCondObjectInit(&cond);
    
    // Wait (releases mutex, reacquires on wake)
    msg_t chCondWait(&cond);
    msg_t chCondWaitTimeout(&cond, sysinterval_t timeout);
    
    // Signal one waiter
    void chCondSignal(&cond);
    void chCondSignalI(&cond);
    
    // Signal all waiters (broadcast)
    void chCondBroadcast(&cond);
    void chCondBroadcastI(&cond);
    
    // Example usage
    chMtxLock(&mtx);
    while (!condition) {
        chCondWait(&cond);
    }
    // Process
    chMtxUnlock(&mtx);

Event API
=========

.. code-block:: c

    // Events are built into thread structure
    
    // Wait for events
    eventmask_t chEvtWaitAny(eventmask_t events);
    eventmask_t chEvtWaitAll(eventmask_t events);
    eventmask_t chEvtWaitAnyTimeout(eventmask_t events, sysinterval_t timeout);
    eventmask_t chEvtWaitAllTimeout(eventmask_t events, sysinterval_t timeout);
    
    // Wait with callback
    eventmask_t chEvtWaitOne(eventmask_t event);
    eventmask_t chEvtWaitOneTimeout(eventmask_t event, sysinterval_t timeout);
    
    // Signal events to thread
    void chEvtSignal(thread_t *tp, eventmask_t events);
    void chEvtSignalI(thread_t *tp, eventmask_t events);
    
    // Broadcast to all threads
    void chEvtBroadcast(event_source_t *esp);
    void chEvtBroadcastI(event_source_t *esp);
    
    // Clear events
    eventmask_t chEvtGetAndClearEvents(eventmask_t events);
    
    // Add flags
    eventmask_t chEvtAddEvents(eventmask_t events);

Event Sources
-------------

.. code-block:: c

    // Event source (for pub/sub pattern)
    event_source_t event_src;
    
    // Initialize
    chEvtObjectInit(&event_src);
    
    // Register listener
    event_listener_t listener;
    chEvtRegister(&event_src, &listener, EVENT_MASK(0));
    
    // Unregister
    chEvtUnregister(&event_src, &listener);
    
    // Broadcast event
    chEvtBroadcast(&event_src);

Mailbox API
===========

FIFO queue for messages:

.. code-block:: c

    // Define
    mailbox_t mb;
    msg_t mb_buffer[8];     // 8 message slots
    
    // Initialize
    chMBObjectInit(&mb, mb_buffer, 8);
    
    // Post (send)
    msg_t chMBPostTimeout(&mb, msg_t msg, sysinterval_t timeout);
    msg_t chMBPostI(&mb, msg_t msg);
    msg_t chMBPostAheadTimeout(&mb, msg_t msg, sysinterval_t timeout);  // Urgent
    
    // Fetch (receive)
    msg_t result;
    msg_t chMBFetchTimeout(&mb, &result, sysinterval_t timeout);
    msg_t chMBFetchI(&mb, &result);
    
    // Peek
    msg_t chMBPeekI(&mb);
    
    // Status
    size_t chMBGetUsedCountI(&mb);
    size_t chMBGetFreeCountI(&mb);
    
    // Reset
    void chMBReset(&mb);
    void chMBResetI(&mb);

Object FIFO API
===============

For typed object queues:

.. code-block:: c

    #include "chobjfifos.h"
    
    // Define
    objects_fifo_t fifo;
    
    // Initialize
    void chFifoObjectInit(&fifo, size_t objsize, size_t objn, 
                          uint8_t *objbuf, void *msgbuf);
    
    // Post object
    msg_t chFifoSendObjectTimeout(&fifo, void *objp, sysinterval_t timeout);
    msg_t chFifoSendObjectI(&fifo, void *objp);
    
    // Receive object
    msg_t chFifoReceiveObjectTimeout(&fifo, void **objpp, sysinterval_t timeout);
    msg_t chFifoReceiveObjectI(&fifo, void **objpp);
    
    // Release object
    void chFifoReturnObject(&fifo, void *objp);
    void chFifoReturnObjectI(&fifo, void *objp);

Synchronous Messages
====================

Direct thread-to-thread messages:

.. code-block:: c

    // Server thread waits for message
    thread_t *tp = chMsgWait();
    msg_t msg = chMsgGet(tp);
    
    // Process request
    
    // Send response
    chMsgRelease(tp, MSG_OK);
    
    // Client sends message and waits for response
    msg_t response = chMsgSend(server_tp, (msg_t)data);

Virtual Timers
==============

.. code-block:: c

    // Define
    virtual_timer_t vt;
    
    // Initialize
    chVTObjectInit(&vt);
    
    // Callback
    void timer_callback(void *arg) {
        // Called from ISR context!
        chSysLockFromISR();
        // Do something
        chSysUnlockFromISR();
    }
    
    // Set timer (one-shot)
    chVTSet(&vt, TIME_MS2I(100), timer_callback, NULL);
    
    // Set timer (periodic in callback)
    void periodic_callback(void *arg) {
        chSysLockFromISR();
        // Do work
        chVTSetI(&vt, TIME_MS2I(100), periodic_callback, NULL);
        chSysUnlockFromISR();
    }
    
    // Reset (cancel)
    void chVTReset(&vt);
    void chVTResetI(&vt);
    
    // Check if armed
    bool chVTIsArmed(&vt);

Memory Management
=================

Core Allocator
--------------

.. code-block:: c

    // Allocate from core (no free!)
    void *chCoreAlloc(size_t size);
    void *chCoreAllocAligned(size_t size, unsigned align);

Heap
----

.. code-block:: c

    // Heap allocation
    void *chHeapAlloc(memory_heap_t *heapp, size_t size);
    void chHeapFree(void *p);
    
    // Default heap
    void *ptr = chHeapAlloc(NULL, 100);
    chHeapFree(ptr);
    
    // Heap status
    size_t chHeapStatus(memory_heap_t *heapp, size_t *sizep, size_t *largestp);

Memory Pools
------------

.. code-block:: c

    // Define pool
    memory_pool_t mp;
    
    // Pool buffer (for N objects of size S)
    uint8_t pool_buffer[N * MEM_ALIGN_SIZE(S)];
    
    // Initialize
    chPoolObjectInit(&mp, S, NULL);
    chPoolLoadArray(&mp, pool_buffer, N);
    
    // Allocate object
    void *objp = chPoolAlloc(&mp);
    void *objp = chPoolAllocI(&mp);
    
    // Free object
    void chPoolFree(&mp, void *objp);
    void chPoolFreeI(&mp, void *objp);

Guards (Memory Pools for Objects)
----------------------------------

.. code-block:: c

    // Guarded pool (includes object counting)
    guarded_memory_pool_t gmp;
    
    // Initialize
    chGuardedPoolObjectInit(&gmp, size_t size);
    chGuardedPoolLoadArray(&gmp, void *p, size_t n);
    
    // Allocate with timeout
    void *chGuardedPoolAllocTimeout(&gmp, sysinterval_t timeout);
    
    // Free
    void chGuardedPoolFree(&gmp, void *objp);

Critical Sections
=================

.. code-block:: c

    // System lock (disable scheduler + interrupts)
    chSysLock();
    // Critical code
    chSysUnlock();
    
    // From ISR
    chSysLockFromISR();
    // Critical code
    chSysUnlockFromISR();
    
    // Suspend scheduler only (interrupts enabled)
    chSchLock();
    // No context switches
    chSchUnlock();

System Functions
================

.. code-block:: c

    // Get system time
    systime_t time = chVTGetSystemTimeX();
    
    // Check if time expired
    bool expired = chVTTimeElapsedSinceX(time) > TIME_MS2I(100);
    
    // Kernel info
    const ch_info_t *info = chSysGetInfo();
    
    // Halt system
    chSysHalt(const char *reason);

API Classes
===========

ChibiOS uses naming conventions for different contexts:

.. code-block:: text

    Normal API:     chXxxYyy()      Can block, only from thread context
    S-class API:    chXxxYyyS()     System locked, no blocking
    I-class API:    chXxxYyyI()     From ISR or system locked, no blocking
    X-class API:    chXxxYyyX()     Any context, usually just queries

Example Application
===================

.. code-block:: c

    #include "ch.h"
    #include "hal.h"
    
    static THD_WORKING_AREA(waProducer, 128);
    static THD_FUNCTION(Producer, arg) {
        (void)arg;
        mailbox_t *mbp = (mailbox_t *)arg;
        msg_t msg = 0;
        
        while (true) {
            chMBPostTimeout(mbp, msg, TIME_INFINITE);
            msg++;
            chThdSleepMilliseconds(1000);
        }
    }
    
    static THD_WORKING_AREA(waConsumer, 128);
    static THD_FUNCTION(Consumer, arg) {
        mailbox_t *mbp = (mailbox_t *)arg;
        msg_t msg;
        
        while (true) {
            if (chMBFetchTimeout(mbp, &msg, TIME_INFINITE) == MSG_OK) {
                // Process msg
            }
        }
    }
    
    int main(void) {
        halInit();
        chSysInit();
        
        // Create mailbox
        static mailbox_t mb;
        static msg_t mb_buffer[8];
        chMBObjectInit(&mb, mb_buffer, 8);
        
        // Create threads
        chThdCreateStatic(waProducer, sizeof(waProducer), 
                         NORMALPRIO, Producer, &mb);
        chThdCreateStatic(waConsumer, sizeof(waConsumer), 
                         NORMALPRIO, Consumer, &mb);
        
        while (true) {
            chThdSleepMilliseconds(1000);
        }
    }

Common Pitfalls
===============

.. code-block:: text

    ✗ Calling S-class functions without system lock
    ✗ Calling normal functions from ISR (use I-class)
    ✗ Not calling halInit() and chSysInit()
    ✗ Insufficient working area size
    ✗ Using blocking calls in S/I-class contexts
    ✗ Forgetting to enable features in chconf.h
    ✗ Not handling MSG_RESET, MSG_TIMEOUT return values

See Also
========

- :doc:`rtos_basics_cheatsheet` - Generic RTOS concepts
- :doc:`../days/day13` - ChibiOS deep dive
- Official ChibiOS documentation: chibios.org
