============================
PikeOS (PX5/P4) Cheat Sheet
============================

Quick Start
===========

PikeOS is a commercial separation kernel/hypervisor providing time and space partitioning for safety-critical systems.

Architecture Overview
---------------------

.. code-block:: text

    PikeOS Microkernel
    ├── Partitions (isolated address spaces)
    │   ├── POSIX Partitions (PSE51/PSE52/PSE53)
    │   ├── Native PikeOS API
    │   ├── Linux Personality
    │   └── AUTOSAR Personality
    └── Resource Management (CPU time, memory, I/O)

Configuration
-------------

**Project XML** defines partitions and resource allocation:

.. code-block:: xml

    <project>
      <partition name="Part1">
        <memory>4MB</memory>
        <priority>10</priority>
        <schedule>
          <window start="0ms" duration="10ms"/>
        </schedule>
      </partition>
    </project>

Native API (P4/VM API)
======================

Thread API
----------

.. code-block:: c

    #include <vm.h>
    
    // Thread structure
    vm_thread_t thread;
    
    // Thread function
    void thread_func(void *arg) {
        while (1) {
            // Thread body
            vm_delay(VM_TIME_MS(100));
        }
    }
    
    // Create thread
    vm_error_t vm_cre_tsk(
        vm_thread_t *thread,
        const vm_cre_tsk_t *pk_ctsk
    );
    
    // Thread attributes
    vm_cre_tsk_t attr;
    attr.exinf = NULL;
    attr.tskatr = VM_TA_HLNG | VM_TA_ACT;
    attr.task = thread_func;
    attr.itskpri = 10;
    attr.stksz = 4096;
    attr.stk = NULL;  // Auto-allocate
    
    vm_cre_tsk(&thread, &attr);
    
    // Start thread (if not auto-started)
    vm_sta_tsk(thread, 0);
    
    // Terminate thread
    vm_ter_tsk(thread);
    vm_ext_tsk();  // Exit self

Delay/Sleep
-----------

.. code-block:: c

    // Delay (microseconds)
    vm_delay(VM_TIME_US(100));
    vm_delay(VM_TIME_MS(100));
    vm_delay(VM_TIME_S(1));
    
    // Delay until absolute time
    vm_dly_tsk(ticks);
    
    // Sleep until woken
    vm_slp_tsk(timeout);
    
    // Wake thread
    vm_wup_tsk(thread);

Priority/Control
----------------

.. code-block:: c

    // Change priority
    vm_chg_pri(thread, new_priority);
    
    // Get priority
    vm_ref_tsk_t status;
    vm_ref_tsk(thread, &status);
    
    // Suspend/Resume
    vm_sus_tsk(thread);
    vm_rsm_tsk(thread);

Semaphore API
=============

.. code-block:: c

    vm_sem_t sem;
    
    // Create semaphore
    vm_cre_sem_t attr;
    attr.sematr = VM_TA_TFIFO;  // FIFO or TPRI (priority)
    attr.isemcnt = 0;
    attr.maxsem = 1;
    
    vm_cre_sem(&sem, &attr);
    
    // Wait (decrement)
    vm_wai_sem(sem);
    vm_twai_sem(sem, timeout);
    vm_pol_sem(sem);  // Non-blocking
    
    // Signal (increment)
    vm_sig_sem(sem);
    
    // Delete
    vm_del_sem(sem);

Mutex API
=========

.. code-block:: c

    vm_mtx_t mutex;
    
    // Create mutex
    vm_cre_mtx_t attr;
    attr.mtxatr = VM_TA_TFIFO | VM_TA_INHERIT;  // Priority inheritance
    attr.ceilpri = 0;  // Priority ceiling protocol
    
    vm_cre_mtx(&mutex, &attr);
    
    // Lock
    vm_loc_mtx(mutex);
    vm_tloc_mtx(mutex, timeout);
    
    // Unlock
    vm_unl_mtx(mutex);
    
    // Delete
    vm_del_mtx(mutex);

Event Flag API
==============

.. code-block:: c

    vm_flg_t flag;
    
    // Create
    vm_cre_flg_t attr;
    attr.flgatr = VM_TA_TFIFO | VM_TA_WMUL;  // Multiple waiters
    attr.iflgptn = 0;
    
    vm_cre_flg(&flag, &attr);
    
    // Set flags
    vm_set_flg(flag, pattern);
    
    // Clear flags
    vm_clr_flg(flag, ~pattern);
    
    // Wait for flags
    vm_flgptn_t result;
    vm_wai_flg(flag, pattern, VM_TWF_ANDW, &result);  // AND wait
    vm_wai_flg(flag, pattern, VM_TWF_ORW, &result);   // OR wait
    
    // Timed wait
    vm_twai_flg(flag, pattern, VM_TWF_ORW, &result, timeout);
    
    // Delete
    vm_del_flg(flag);

Message Queue API
=================

.. code-block:: c

    vm_dtq_t queue;
    
    // Create data queue
    vm_cre_dtq_t attr;
    attr.dtqatr = VM_TA_TFIFO;
    attr.dtqcnt = 10;  // Max messages
    attr.dtq = NULL;   // Auto-allocate
    
    vm_cre_dtq(&queue, &attr);
    
    // Send
    vm_intptr_t data = 42;
    vm_snd_dtq(queue, data);
    vm_tsnd_dtq(queue, data, timeout);
    vm_psnd_dtq(queue, data);  // Non-blocking
    
    // Receive
    vm_intptr_t received;
    vm_rcv_dtq(queue, &received);
    vm_trcv_dtq(queue, &received, timeout);
    vm_prcv_dtq(queue, &received);  // Non-blocking
    
    // Delete
    vm_del_dtq(queue);

Mailbox API
===========

.. code-block:: c

    vm_mbx_t mailbox;
    
    // Message structure (must start with T_MSG)
    typedef struct {
        T_MSG msgque;
        int data;
    } MY_MSG;
    
    // Create
    vm_cre_mbx_t attr;
    attr.mbxatr = VM_TA_TFIFO | VM_TA_MFIFO;
    attr.maxmpri = 0;
    attr.mprihd = NULL;
    
    vm_cre_mbx(&mailbox, &attr);
    
    // Send
    MY_MSG *msg = malloc(sizeof(MY_MSG));
    msg->data = 42;
    vm_snd_mbx(mailbox, (T_MSG *)msg);
    
    // Receive
    T_MSG *recv_msg;
    vm_rcv_mbx(mailbox, &recv_msg);
    MY_MSG *my_msg = (MY_MSG *)recv_msg;
    
    // Process and free
    free(my_msg);
    
    // Delete
    vm_del_mbx(mailbox);

Memory Pool API
===============

Fixed-size Blocks
-----------------

.. code-block:: c

    vm_mpf_t pool;
    
    // Create
    vm_cre_mpf_t attr;
    attr.mpfatr = VM_TA_TFIFO;
    attr.blkcnt = 10;
    attr.blksz = 64;
    attr.mpf = NULL;  // Auto-allocate
    attr.mpfmb = NULL;
    
    vm_cre_mpf(&pool, &attr);
    
    // Allocate block
    void *block;
    vm_get_mpf(&pool, &block);
    vm_tget_mpf(&pool, &block, timeout);
    vm_pget_mpf(&pool, &block);  // Non-blocking
    
    // Release block
    vm_rel_mpf(pool, block);
    
    // Delete pool
    vm_del_mpf(pool);

Variable-size Allocator
-----------------------

.. code-block:: c

    vm_mpl_t vpool;
    
    // Create
    vm_cre_mpl_t attr;
    attr.mplatr = VM_TA_TFIFO;
    attr.mplsz = 4096;
    attr.mpl = NULL;
    attr.mplmb = NULL;
    
    vm_cre_mpl(&vpool, &attr);
    
    // Allocate
    void *mem;
    vm_get_mpl(&vpool, 100, &mem);
    vm_tget_mpl(&vpool, 100, &mem, timeout);
    
    // Release
    vm_rel_mpl(vpool, mem);
    
    // Delete
    vm_del_mpl(vpool);

Cyclic Handler (Periodic Timer)
================================

.. code-block:: c

    vm_cyc_t cyclic;
    
    // Handler function
    void cyclic_handler(vm_exinf_t exinf) {
        // Called periodically from ISR context
    }
    
    // Create
    vm_cre_cyc_t attr;
    attr.cycatr = VM_TA_HLNG | VM_TA_STA;
    attr.exinf = NULL;
    attr.cychdr = cyclic_handler;
    attr.cyctim = VM_TIME_MS(100);  // Period
    attr.cycphs = 0;  // Phase offset
    
    vm_cre_cyc(&cyclic, &attr);
    
    // Start
    vm_sta_cyc(cyclic);
    
    // Stop
    vm_stp_cyc(cyclic);
    
    // Delete
    vm_del_cyc(cyclic);

Alarm Handler
=============

.. code-block:: c

    vm_alm_t alarm;
    
    // Handler
    void alarm_handler(vm_exinf_t exinf) {
        // One-shot callback
    }
    
    // Create
    vm_cre_alm_t attr;
    attr.almatr = VM_TA_HLNG;
    attr.exinf = NULL;
    attr.almhdr = alarm_handler;
    
    vm_cre_alm(&alarm, &attr);
    
    // Start (one-shot)
    vm_sta_alm(alarm, VM_TIME_MS(1000));
    
    // Stop
    vm_stp_alm(alarm);
    
    // Delete
    vm_del_alm(alarm);

Interrupt Handling
==================

.. code-block:: c

    // Define interrupt handler
    void my_isr(vm_exinf_t exinf) {
        // ISR code
        
        // Can call limited APIs from ISR
        vm_sig_sem(sem);
        vm_set_flg(flag, 0x01);
        vm_snd_dtq(queue, data);
    }
    
    // Register ISR
    vm_def_inh_t attr;
    attr.inhatr = VM_TA_HLNG;
    attr.inthdr = my_isr;
    
    vm_def_inh(IRQ_NUMBER, &attr);

Time Services
=============

.. code-block:: c

    // Get system time (microseconds)
    vm_time_t time = vm_get_tim();
    
    // Convert time
    VM_TIME_US(100)     // 100 microseconds
    VM_TIME_MS(100)     // 100 milliseconds
    VM_TIME_S(1)        // 1 second
    
    // Timeouts
    VM_TMO_POL          // 0 - No wait
    VM_TMO_FEVR         // -1 - Wait forever

POSIX Personality
=================

PikeOS supports POSIX PSE51-53 profiles:

.. code-block:: c

    #include <pthread.h>
    #include <semaphore.h>
    #include <mqueue.h>
    
    // Standard POSIX APIs work
    pthread_t thread;
    pthread_create(&thread, NULL, thread_func, NULL);
    
    sem_t sem;
    sem_init(&sem, 0, 0);
    sem_wait(&sem);
    sem_post(&sem);
    
    mqd_t mq;
    mq = mq_open("/myqueue", O_CREAT | O_RDWR, 0644, NULL);

Partition Communication
=======================

Queuing Ports (Inter-Partition)
--------------------------------

.. code-block:: c

    #include <p4.h>
    
    // Open port
    p4_qport_t port;
    p4_e_t err = p4_qport_open(&port, "PortName", P4_QPORT_SEND, NULL);
    
    // Send message
    char msg[64] = "Hello";
    err = p4_qport_send(port, msg, sizeof(msg), P4_TIMEOUT_INFINITE);
    
    // Receive message
    char buffer[64];
    p4_size_t size;
    err = p4_qport_receive(port, buffer, sizeof(buffer), &size, P4_TIMEOUT_INFINITE);
    
    // Close port
    p4_qport_close(port);

Sampling Ports (Periodic Data)
-------------------------------

.. code-block:: c

    // Open sampling port
    p4_sport_t sport;
    p4_sport_open(&sport, "SamplingPort", P4_SPORT_WRITE, NULL);
    
    // Write sample
    uint32_t data = 42;
    p4_sport_write(sport, &data, sizeof(data));
    
    // Read sample
    uint32_t received;
    p4_bool_t valid;
    p4_sport_read(sport, &received, sizeof(received), &valid);
    
    // Close
    p4_sport_close(sport);

System Services
===============

.. code-block:: c

    // Get CPU time
    vm_time_t time = vm_get_tim();
    
    // Partition mode switch
    vm_set_ctx(ctx_id);
    
    // Virtual interrupt
    vm_sig_int(int_number);

Return Values
=============

.. code-block:: c

    VM_E_OK             // Success
    VM_E_SYS            // System error
    VM_E_NOMEM          // No memory
    VM_E_NOSPT          // Not supported
    VM_E_INOSPT         // Invalid operation
    VM_E_RSFN           // Reserved function
    VM_E_RSATR          // Reserved attribute
    VM_E_PAR            // Parameter error
    VM_E_ID             // Invalid ID
    VM_E_CTX            // Context error
    VM_E_MACV           // Memory access violation
    VM_E_OACV           // Object access violation
    VM_E_ILUSE          // Illegal service call
    VM_E_NOMEM          // Insufficient memory
    VM_E_NOID           // No ID available
    VM_E_OBJ            // Object error
    VM_E_NOEXS          // Object does not exist
    VM_E_QOVR           // Queue overflow
    VM_E_RLWAI          // Wait released
    VM_E_TMOUT          // Timeout
    VM_E_DLT            // Object deleted
    VM_E_CLS            // Waiting cancelled

Example Application
===================

.. code-block:: c

    #include <vm.h>
    
    vm_thread_t producer, consumer;
    vm_dtq_t queue;
    
    void producer_task(void *arg) {
        vm_intptr_t count = 0;
        
        while (1) {
            vm_snd_dtq(queue, count);
            count++;
            vm_delay(VM_TIME_MS(1000));
        }
    }
    
    void consumer_task(void *arg) {
        vm_intptr_t data;
        
        while (1) {
            vm_rcv_dtq(queue, &data);
            // Process data
        }
    }
    
    void main(void) {
        // Create queue
        vm_cre_dtq_t qattr;
        qattr.dtqatr = VM_TA_TFIFO;
        qattr.dtqcnt = 10;
        qattr.dtq = NULL;
        vm_cre_dtq(&queue, &qattr);
        
        // Create threads
        vm_cre_tsk_t tattr;
        tattr.exinf = NULL;
        tattr.tskatr = VM_TA_HLNG | VM_TA_ACT;
        tattr.itskpri = 10;
        tattr.stksz = 4096;
        tattr.stk = NULL;
        
        tattr.task = producer_task;
        vm_cre_tsk(&producer, &tattr);
        
        tattr.task = consumer_task;
        vm_cre_tsk(&consumer, &tattr);
        
        // Main loop
        while (1) {
            vm_delay(VM_TIME_S(1));
        }
    }

Common Pitfalls
===============

.. code-block:: text

    ✗ Not configuring partition XML correctly
    ✗ Exceeding partition memory/CPU time budget
    ✗ Using wrong API from ISR context
    ✗ Not checking return values
    ✗ Time window configuration errors
    ✗ Incorrect inter-partition communication setup
    ✗ Mixing native and POSIX APIs incorrectly

See Also
========

- :doc:`rtos_basics_cheatsheet` - Generic RTOS concepts
- :doc:`../days/day17` - PikeOS/PX5 deep dive
- Official PikeOS documentation: sysgo.com
