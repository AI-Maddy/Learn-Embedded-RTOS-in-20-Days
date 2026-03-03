==================
NuttX Cheat Sheet
==================

Quick Start
===========

NuttX uses POSIX APIs extensively, making it familiar to Linux developers.

Configuration
-------------

.. code-block:: bash

    # Configure for board
    cd nuttx
    ./tools/configure.sh <board>:<config>
    
    # Menuconfig
    make menuconfig
    
    # Build
    make
    
    # Flash
    make flash

Task/Thread API
===============

POSIX Threads (pthreads)
-------------------------

.. code-block:: c

    #include <pthread.h>
    #include <sched.h>
    
    pthread_t thread;
    
    // Thread function
    void *thread_func(void *arg) {
        while (1) {
            // Thread body
            usleep(100000);  // 100ms
        }
        return NULL;
    }
    
    // Create thread
    pthread_attr_t attr;
    struct sched_param param;
    
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, 2048);
    param.sched_priority = 100;
    pthread_attr_setschedparam(&attr, &param);
    
    pthread_create(&thread, &attr, thread_func, NULL);
    
    // Join (wait for termination)
    pthread_join(thread, NULL);
    
    // Detach
    pthread_detach(thread);
    
    // Exit
    pthread_exit(NULL);
    
    // Cancel
    pthread_cancel(thread);

NuttX Task API
--------------

Alternative to pthreads:

.. code-block:: c

    #include <nuttx/sched.h>
    
    // Task entry
    int task_main(int argc, char *argv[]) {
        while (1) {
            // Task body
            usleep(100000);
        }
        return 0;
    }
    
    // Create task
    int task_create(
        const char *name,
        int priority,
        int stack_size,
        main_t entry,
        char * const argv[]
    );
    
    // Example
    char *argv[] = {NULL};
    int pid = task_create("mytask", 100, 2048, task_main, argv);
    
    // Delete task
    int task_delete(pid_t pid);
    
    // Restart task
    int task_restart(pid_t pid);

Sleep/Delay
-----------

.. code-block:: c

    #include <unistd.h>
    #include <time.h>
    
    // Sleep (microseconds)
    usleep(100000);  // 100ms
    
    // Sleep (seconds)
    sleep(1);
    
    // Nanosleep
    struct timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = 100000000;  // 100ms
    nanosleep(&ts, NULL);
    
    // Yield
    sched_yield();

Priority/Scheduling
-------------------

.. code-block:: c

    #include <sched.h>
    
    // Get priority
    struct sched_param param;
    pthread_getschedparam(pthread_self(), NULL, &param);
    int priority = param.sched_priority;
    
    // Set priority
    param.sched_priority = 150;
    pthread_setschedparam(pthread_self(), SCHED_FIFO, &param);
    
    // Get scheduling policy
    int policy = sched_getscheduler(0);  // 0 = current process
    
    // Set scheduling policy
    sched_setscheduler(0, SCHED_RR, &param);
    
    // Policies: SCHED_FIFO, SCHED_RR, SCHED_OTHER

Semaphore API
=============

POSIX Semaphores
----------------

.. code-block:: c

    #include <semaphore.h>
    
    sem_t sem;
    
    // Initialize (unnamed)
    sem_init(&sem, 0, 0);  // pshared=0, value=0
    
    // Wait (decrement)
    sem_wait(&sem);
    
    // Timed wait
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += 1;  // 1 second timeout
    sem_timedwait(&sem, &ts);
    
    // Try wait (non-blocking)
    if (sem_trywait(&sem) == 0) {
        // Got semaphore
    }
    
    // Post (increment)
    sem_post(&sem);
    
    // Get value
    int value;
    sem_getvalue(&sem, &value);
    
    // Destroy
    sem_destroy(&sem);

Named Semaphores
----------------

.. code-block:: c

    // Open/Create named semaphore
    sem_t *sem = sem_open("/mysem", O_CREAT, 0644, 0);
    
    // Use (same as unnamed)
    sem_wait(sem);
    sem_post(sem);
    
    // Close
    sem_close(sem);
    
    // Unlink (delete)
    sem_unlink("/mysem");

Mutex API
=========

.. code-block:: c

    #include <pthread.h>
    
    pthread_mutex_t mutex;
    
    // Initialize
    pthread_mutex_init(&mutex, NULL);
    
    // Initialize with attributes
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&mutex, &attr);
    
    // Lock
    pthread_mutex_lock(&mutex);
    
    // Try lock (non-blocking)
    if (pthread_mutex_trylock(&mutex) == 0) {
        // Got lock
    }
    
    // Timed lock
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += 1;
    pthread_mutex_timedlock(&mutex, &ts);
    
    // Unlock
    pthread_mutex_unlock(&mutex);
    
    // Destroy
    pthread_mutex_destroy(&mutex);

⚠️ **Priority inheritance supported with PTHREAD_PRIO_INHERIT**

Condition Variable API
======================

.. code-block:: c

    pthread_cond_t cond;
    pthread_mutex_t mutex;
    
    // Initialize
    pthread_cond_init(&cond, NULL);
    pthread_mutex_init(&mutex, NULL);
    
    // Wait (releases mutex, reacquires on wake)
    pthread_mutex_lock(&mutex);
    while (!condition) {
        pthread_cond_wait(&cond, &mutex);
    }
    // Process
    pthread_mutex_unlock(&mutex);
    
    // Timed wait
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += 1;
    pthread_cond_timedwait(&cond, &mutex, &ts);
    
    // Signal one waiter
    pthread_cond_signal(&cond);
    
    // Signal all waiters (broadcast)
    pthread_cond_broadcast(&cond);
    
    // Destroy
    pthread_cond_destroy(&cond);

Message Queue API
=================

POSIX Message Queues
--------------------

.. code-block:: c

    #include <mqueue.h>
    
    mqd_t mq;
    struct mq_attr attr;
    
    // Set attributes
    attr.mq_flags = 0;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = 64;
    attr.mq_curmsgs = 0;
    
    // Open/Create
    mq = mq_open("/myqueue", O_CREAT | O_RDWR, 0644, &attr);
    
    // Send
    char msg[64] = "Hello";
    mq_send(mq, msg, strlen(msg) + 1, 0);  // priority = 0
    
    // Timed send
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += 1;
    mq_timedsend(mq, msg, strlen(msg) + 1, 0, &ts);
    
    // Receive
    char buffer[64];
    unsigned int prio;
    ssize_t bytes = mq_receive(mq, buffer, sizeof(buffer), &prio);
    
    // Timed receive
    bytes = mq_timedreceive(mq, buffer, sizeof(buffer), &prio, &ts);
    
    // Get attributes
    mq_getattr(mq, &attr);
    
    // Close
    mq_close(mq);
    
    // Unlink (delete)
    mq_unlink("/myqueue");

Work Queue API
==============

NuttX-specific deferred work:

.. code-block:: c

    #include <nuttx/wqueue.h>
    
    struct work_s work;
    
    // Work callback
    void work_callback(void *arg) {
        // Process work
    }
    
    // Queue work
    work_queue(HPWORK, &work, work_callback, NULL, 0);
    work_queue(LPWORK, &work, work_callback, NULL, 0);
    
    // Queue delayed work (ticks)
    work_queue(HPWORK, &work, work_callback, NULL, MSEC2TICK(1000));
    
    // Cancel work
    work_cancel(HPWORK, &work);

Timer API
=========

POSIX Timers
------------

.. code-block:: c

    #include <time.h>
    #include <signal.h>
    
    timer_t timerid;
    struct sigevent sev;
    struct itimerspec its;
    
    // Setup signal event
    sev.sigev_notify = SIGEV_THREAD;
    sev.sigev_notify_function = timer_callback;
    sev.sigev_notify_attributes = NULL;
    sev.sigev_value.sival_ptr = &timerid;
    
    // Create timer
    timer_create(CLOCK_REALTIME, &sev, &timerid);
    
    // Set timer (one-shot or periodic)
    its.it_value.tv_sec = 1;
    its.it_value.tv_nsec = 0;
    its.it_interval.tv_sec = 1;      // Periodic (0 = one-shot)
    its.it_interval.tv_nsec = 0;
    timer_settime(timerid, 0, &its, NULL);
    
    // Get timer
    timer_gettime(timerid, &its);
    
    // Delete timer
    timer_delete(timerid);
    
    // Timer callback
    void timer_callback(union sigval val) {
        // Called from timer thread
    }

Signal API
==========

.. code-block:: c

    #include <signal.h>
    
    // Signal handler
    void signal_handler(int signo) {
        // Handle signal
    }
    
    // Register handler
    signal(SIGUSR1, signal_handler);
    
    // Send signal
    kill(pid, SIGUSR1);
    pthread_kill(thread, SIGUSR1);
    
    // Wait for signal
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGUSR1);
    int sig;
    sigwait(&set, &sig);

File I/O
========

Standard POSIX file operations:

.. code-block:: c

    #include <fcntl.h>
    #include <unistd.h>
    
    // Open
    int fd = open("/dev/ttyS0", O_RDWR);
    
    // Read/Write
    char buffer[64];
    ssize_t bytes = read(fd, buffer, sizeof(buffer));
    bytes = write(fd, "Hello", 5);
    
    // Ioctl
    ioctl(fd, SOME_COMMAND, &arg);
    
    // Close
    close(fd);

Memory Management
=================

.. code-block:: c

    #include <stdlib.h>
    
    // Allocate
    void *ptr = malloc(100);
    
    // Calloc (zero-initialized)
    void *ptr = calloc(10, sizeof(int));
    
    // Realloc
    ptr = realloc(ptr, 200);
    
    // Free
    free(ptr);
    
    // Aligned allocation
    void *ptr;
    posix_memalign(&ptr, 32, 100);  // 32-byte aligned

Shared Memory
-------------

.. code-block:: c

    #include <sys/mman.h>
    
    // Create shared memory
    int fd = shm_open("/myshm", O_CREAT | O_RDWR, 0644);
    ftruncate(fd, 1024);
    
    // Map
    void *ptr = mmap(NULL, 1024, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    
    // Use shared memory
    
    // Unmap
    munmap(ptr, 1024);
    
    // Close and unlink
    close(fd);
    shm_unlink("/myshm");

Interrupts
==========

.. code-block:: c

    #include <nuttx/irq.h>
    
    // Disable interrupts
    irqstate_t flags = enter_critical_section();
    
    // Critical section
    
    // Restore interrupts
    leave_critical_section(flags);

Board-Specific APIs
===================

GPIO (example)
--------------

.. code-block:: c

    #include <nuttx/ioexpander/gpio.h>
    
    int fd = open("/dev/gpio0", O_RDWR);
    
    // Read
    uint8_t value;
    read(fd, &value, sizeof(value));
    
    // Write
    value = 1;
    write(fd, &value, sizeof(value));
    
    close(fd);

System Calls
============

.. code-block:: c

    #include <sys/boardctl.h>
    
    // Reboot
    boardctl(BOARDIOC_RESET, 0);
    
    // Power off
    boardctl(BOARDIOC_POWEROFF, 0);

Example Application
===================

.. code-block:: c

    #include <pthread.h>
    #include <semaphore.h>
    #include <stdio.h>
    #include <unistd.h>
    
    sem_t sem;
    
    void *producer_thread(void *arg) {
        int count = 0;
        
        while (1) {
            printf("Produced: %d\n", count);
            sem_post(&sem);
            count++;
            sleep(1);
        }
        return NULL;
    }
    
    void *consumer_thread(void *arg) {
        while (1) {
            sem_wait(&sem);
            printf("Consumed\n");
        }
        return NULL;
    }
    
    int main(int argc, char *argv[]) {
        pthread_t prod, cons;
        
        sem_init(&sem, 0, 0);
        
        pthread_create(&prod, NULL, producer_thread, NULL);
        pthread_create(&cons, NULL, consumer_thread, NULL);
        
        pthread_join(prod, NULL);
        pthread_join(cons, NULL);
        
        return 0;
    }

Common Pitfalls
===============

.. code-block:: text

    ✗ Forgetting to check return values for errors
    ✗ Not initializing pthread attributes
    ✗ Using absolute timeouts (need CLOCK_REALTIME)
    ✗ Priority range confusion (higher number = higher priority)
    ✗ Not enabling POSIX features in menuconfig
    ✗ Stack size too small for pthread
    ✗ Forgetting to close/unlink named objects

See Also
========

- :doc:`rtos_basics_cheatsheet` - Generic RTOS concepts
- :doc:`../days/day16` - NuttX deep dive
- Official NuttX documentation: nuttx.apache.org
- POSIX reference: pubs.opengroup.org
