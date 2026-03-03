====================
embOS Cheat Sheet
====================

Quick Start
===========

Initialization
--------------

.. code-block:: c

    #include "RTOS.h"
    
    int main(void) {
        OS_Init();          // Initialize RTOS
        OS_InitHW();        // Initialize hardware
        
        // Create tasks
        
        OS_Start();         // Start multitasking (never returns)
        return 0;
    }

Configuration
-------------

**RTOSInit_*.c** or **RTOS.h** settings:

.. code-block:: c

    #define OS_FSYS                100000000  // CPU frequency
    #define OS_PCLK_TIMER          100000000  // Timer frequency
    #define OS_TICK_FREQ           1000       // Tick rate (Hz)

Task API
========

Create/Terminate
----------------

.. code-block:: c

    OS_TASK TCB_MyTask;
    OS_STACKPTR int Stack_MyTask[128];
    
    // Task function
    static void MyTask(void) {
        while (1) {
            // Task body
            OS_DELAY(100);
        }
    }
    
    // Create task (static)
    void OS_CreateTask(
        OS_TASK *pTask,
        const char *pName,
        OS_U8 Priority,         // 1 = highest user priority
        void (*pRoutine)(void),
        void OS_STACKPTR *pStack,
        unsigned StackSize,
        unsigned TimeSlice      // 0 = no time slicing
    );
    
    // Example
    OS_CreateTask(&TCB_MyTask, "MyTask", 100, MyTask,
                  Stack_MyTask, sizeof(Stack_MyTask), 0);
    
    // Create task (extended with context)
    void OS_CreateTaskEx(
        OS_TASK *pTask,
        const char *pName,
        OS_U8 Priority,
        void (*pRoutine)(void *),
        void OS_STACKPTR *pStack,
        unsigned StackSize,
        unsigned TimeSlice,
        void *pContext          // Parameter passed to task
    );
    
    // Terminate task
    void OS_Terminate(OS_TASK *pTask);  // NULL = self

Delay/Yield
-----------

.. code-block:: c

    // Delay (milliseconds)
    void OS_DELAY(OS_TIME ms);
    void OS_Delayms(OS_TIME ms);
    
    // Delay (ticks)
    void OS_Delay(OS_TIME ticks);
    
    // Delay until absolute time
    void OS_DelayUntil(OS_TIME time);
    
    // Yield to equal/higher priority
    void OS_Yield(void);

Priority/Suspend
----------------

.. code-block:: c

    // Get/Set priority
    OS_U8 OS_GetPriority(OS_TASK *pTask);
    void OS_SetPriority(OS_TASK *pTask, OS_U8 Priority);
    
    // Suspend/Resume
    void OS_Suspend(OS_TASK *pTask);        // NULL = self
    void OS_Resume(OS_TASK *pTask);
    void OS_ResumeAllSuspendedTasks(void);
    
    // Check if task suspended
    OS_BOOL OS_IsSuspended(OS_TASK *pTask);

Task Info
---------

.. code-block:: c

    // Get current task
    OS_TASK *OS_GetpCurrentTask(void);
    
    // Get task name
    const char *OS_GetTaskName(OS_TASK *pTask);
    
    // Get stack info
    unsigned OS_GetStackUsed(OS_TASK *pTask);
    unsigned OS_GetStackSize(OS_TASK *pTask);
    unsigned OS_GetStackSpace(OS_TASK *pTask);
    
    // Check stack
    void OS_CheckStack(OS_TASK *pTask);

Semaphore API
=============

Binary Semaphore
----------------

.. code-block:: c

    OS_CSEMA Semaphore;
    
    // Initialize
    void OS_CreateCSema(OS_CSEMA *pCSema, OS_UINT InitValue);
    
    // Wait (decrement, max wait if 0)
    void OS_WaitCSema(OS_CSEMA *pCSema);
    int OS_WaitCSemaTimed(OS_CSEMA *pCSema, OS_TIME Timeout);
    
    // Signal (increment, max 1)
    void OS_SignalCSema(OS_CSEMA *pCSema);
    void OS_SignalCSemaMax(OS_CSEMA *pCSema, OS_UINT MaxValue);
    
    // Get count
    OS_UINT OS_GetCSemaValue(OS_CSEMA *pCSema);
    
    // Set count
    void OS_SetCSemaValue(OS_CSEMA *pCSema, OS_UINT Value);
    
    // Delete
    void OS_DeleteCSema(OS_CSEMA *pCSema);

Counting Semaphore
------------------

.. code-block:: c

    OS_RSEMA Semaphore;
    
    // Initialize
    void OS_CreateRSema(OS_RSEMA *pRSema);
    
    // Wait (decrement)
    void OS_WaitRSema(OS_RSEMA *pRSema);
    int OS_WaitRSemaTimed(OS_RSEMA *pRSema, OS_TIME Timeout);
    
    // Signal (increment)
    void OS_SignalRSema(OS_RSEMA *pRSema);
    
    // Get/Set count
    OS_UINT OS_GetRSemaValue(OS_RSEMA *pRSema);
    void OS_SetRSemaValue(OS_RSEMA *pRSema, OS_UINT Value);
    
    // Delete
    void OS_DeleteRSema(OS_RSEMA *pRSema);

Mutex API (Resource Semaphore)
===============================

.. code-block:: c

    OS_RSEMA Mutex;
    
    // Initialize (reuse RSEMA)
    OS_CreateRSema(&Mutex);
    
    // Use (lock)
    OS_Use(&Mutex);
    int OS_UseTimed(&Mutex, OS_TIME Timeout);
    
    // Unuse (unlock)
    OS_Unuse(&Mutex);
    
    // Get usage count
    OS_UINT OS_Request(&Mutex);  // Non-blocking

⚠️ **Supports priority inversion avoidance, allows nesting**

Mailbox API
===========

.. code-block:: c

    OS_MAILBOX Mailbox;
    char MailboxBuffer[16];
    
    // Create
    void OS_CreateMB(
        OS_MAILBOX *pMB,
        OS_U16 sizeofMsg,
        OS_UINT maxnofMsg,
        void *pMsg
    );
    
    // Example
    OS_CreateMB(&Mailbox, 1, sizeof(MailboxBuffer), &MailboxBuffer);
    
    // Put message (blocking)
    void OS_PutMail(OS_MAILBOX *pMB, const void *pMail);
    char OS_PutMailTimed(OS_MAILBOX *pMB, const void *pMail, OS_TIME Timeout);
    
    // Put message (non-blocking)
    char OS_PutMailCond(OS_MAILBOX *pMB, const void *pMail);
    
    // Put message (front)
    void OS_PutMailFront(OS_MAILBOX *pMB, const void *pMail);
    char OS_PutMailFrontTimed(OS_MAILBOX *pMB, const void *pMail, OS_TIME Timeout);
    
    // Get message
    void OS_GetMail(OS_MAILBOX *pMB, void *pDest);
    char OS_GetMailTimed(OS_MAILBOX *pMB, void *pDest, OS_TIME Timeout);
    char OS_GetMailCond(OS_MAILBOX *pMB, void *pDest);
    
    // Wait for message (don't retrieve)
    void OS_WaitMail(OS_MAILBOX *pMB);
    
    // Peek message
    void OS_PeekMail(OS_MAILBOX *pMB, void *pDest);
    
    // Clear mailbox
    void OS_ClearMB(OS_MAILBOX *pMB);
    
    // Get message count
    OS_BOOL OS_GetMessageCnt(OS_MAILBOX *pMB);
    
    // Delete
    void OS_DeleteMB(OS_MAILBOX *pMB);

Queue API
=========

Message queue (pointer-based):

.. code-block:: c

    OS_Q Queue;
    void *QueueBuffer[10];
    
    // Create
    void OS_Q_Create(
        OS_Q *pQ,
        void **ppData,
        OS_UINT Size
    );
    
    // Example
    OS_Q_Create(&Queue, QueueBuffer, 10);
    
    // Put (pointer)
    int OS_Q_Put(OS_Q *pQ, const void *pData);
    int OS_Q_PutTimed(OS_Q *pQ, const void *pData, OS_TIME Timeout);
    
    // Put (front)
    int OS_Q_PutFront(OS_Q *pQ, const void *pData);
    
    // Get (pointer)
    void *OS_Q_Get(OS_Q *pQ);
    void *OS_Q_GetTimed(OS_Q *pQ, OS_TIME Timeout);
    void *OS_Q_GetBlocked(OS_Q *pQ);
    
    // Peek
    void *OS_Q_Peek(OS_Q *pQ);
    
    // Clear
    void OS_Q_Clear(OS_Q *pQ);
    
    // Get message count
    int OS_Q_GetMessageCnt(OS_Q *pQ);
    
    // Check if pending
    int OS_Q_IsInUse(OS_Q *pQ);
    
    // Delete
    void OS_Q_Delete(OS_Q *pQ);

Event API
=========

.. code-block:: c

    OS_EVENT Event;
    
    // Create
    void OS_EVENT_Create(OS_EVENT *pEvent);
    void OS_EVENT_CreateEx(OS_EVENT *pEvent, OS_UINT Mask);
    
    // Set events
    void OS_EVENT_Set(OS_EVENT *pEvent, OS_UINT Mask);
    void OS_EVENT_SetMask(OS_EVENT *pEvent, OS_UINT Mask);
    
    // Reset (clear) events
    void OS_EVENT_Reset(OS_EVENT *pEvent, OS_UINT Mask);
    void OS_EVENT_ResetMask(OS_EVENT *pEvent, OS_UINT Mask);
    
    // Wait for events
    OS_UINT OS_EVENT_Wait(
        OS_EVENT *pEvent,
        OS_UINT Mask,
        OS_UINT WaitMode,       // OS_EVENT_MODE_OR, OS_EVENT_MODE_AND, etc.
        OS_TIME Timeout
    );
    
    // Wait modes
    #define OS_EVENT_MODE_OR        (0)
    #define OS_EVENT_MODE_AND       (1 << 0)
    #define OS_EVENT_MODE_OR_CLEAR  (1 << 1)
    #define OS_EVENT_MODE_AND_CLEAR (OS_EVENT_MODE_AND | OS_EVENT_MODE_OR_CLEAR)
    
    // Get event mask
    OS_UINT OS_EVENT_Get(OS_EVENT *pEvent);
    
    // Delete
    void OS_EVENT_Delete(OS_EVENT *pEvent);

Timer API
=========

Software Timers
---------------

.. code-block:: c

    OS_TIMER Timer;
    
    // Timer callback
    void TimerCallback(void) {
        // Called from timer context
    }
    
    // Create timer
    void OS_CreateTimer(
        OS_TIMER *pTimer,
        OS_TIMERROUTINE *Callback,
        OS_TIME Timeout
    );
    
    // Create timer (extended)
    void OS_CreateTimerEx(
        OS_TIMER *pTimer,
        OS_TIMERROUTINE_EX *Callback,
        OS_TIME Timeout,
        void *pData
    );
    
    // Start timer
    void OS_StartTimer(OS_TIMER *pTimer);
    
    // Stop timer
    void OS_StopTimer(OS_TIMER *pTimer);
    
    // Restart timer (change timeout)
    void OS_RetriggerTimer(OS_TIMER *pTimer);
    void OS_SetTimerPeriod(OS_TIMER *pTimer, OS_TIME Period);
    
    // Get remaining time
    OS_TIME OS_GetTimerValue(OS_TIMER *pTimer);
    
    // Check if active
    OS_BOOL OS_GetTimerStatus(OS_TIMER *pTimer);
    
    // Delete
    void OS_DeleteTimer(OS_TIMER *pTimer);

Memory Management
=================

Fixed Block Pools
-----------------

.. code-block:: c

    OS_MEMF MemPool;
    char MemPoolBuffer[10 * 32];  // 10 blocks of 32 bytes
    
    // Create
    void OS_MEMF_Create(
        OS_MEMF *pMEMF,
        void *pPool,
        OS_UINT NumBlocks,
        OS_UINT BlockSize
    );
    
    // Example
    OS_MEMF_Create(&MemPool, MemPoolBuffer, 10, 32);
    
    // Allocate block
    void *OS_MEMF_Alloc(OS_MEMF *pMEMF, int Purpose);
    void *OS_MEMF_AllocTimed(OS_MEMF *pMEMF, OS_TIME Timeout, int Purpose);
    void *OS_MEMF_Request(OS_MEMF *pMEMF, int Purpose);  // Non-blocking
    
    // Free block
    void OS_MEMF_Release(OS_MEMF *pMEMF, void *pMemBlock);
    
    // Get info
    int OS_MEMF_GetNumFreeBlocks(OS_MEMF *pMEMF);
    int OS_MEMF_GetMaxUsed(OS_MEMF *pMEMF);
    OS_BOOL OS_MEMF_IsInPool(OS_MEMF *pMEMF, void *pMemBlock);
    
    // Delete
    void OS_MEMF_Delete(OS_MEMF *pMEMF);

Heap Management
---------------

.. code-block:: c

    // Allocate from heap
    void *OS_malloc(OS_UINT Size);
    
    // Free
    void OS_free(void *pMemBlock);
    
    // Realloc
    void *OS_realloc(void *pMemBlock, OS_UINT NewSize);

Critical Sections
=================

.. code-block:: c

    // Disable task switching
    void OS_EnterRegion(void);
    void OS_LeaveRegion(void);
    
    // Disable interrupts
    void OS_IncDI(void);
    void OS_DecRI(void);
    
    // Disable interrupts (with state)
    OS_U32 OS_DI(void);
    void OS_RestoreI(OS_U32 State);

System Functions
================

.. code-block:: c

    // Get system time (milliseconds)
    OS_TIME OS_GetTime(void);
    OS_TIME OS_GetTime32(void);
    
    // Get/Set time (ticks)
    OS_I32 OS_GetTime_Cycles(void);
    
    // Convert time
    OS_TIME OS_ConvertCycles2us(OS_I32 Cycles);
    OS_TIME OS_ConvertCycles2ms(OS_I32 Cycles);
    
    // Idle task hook
    void OS_OnIdle(void);  // User-defined function

Performance Measurement
=======================

.. code-block:: c

    // Start measurement
    OS_U32 t0 = OS_GetTime_Cycles();
    
    // Code to measure
    
    // Get elapsed time
    OS_U32 elapsed = OS_GetTime_Cycles() - t0;
    OS_TIME us = OS_ConvertCycles2us(elapsed);

Interrupt Handling
==================

.. code-block:: c

    // Enter interrupt
    void OS_EnterInterrupt(void);
    #define OS_INT_Enter() OS_EnterInterrupt()
    
    // Leave interrupt
    void OS_LeaveInterrupt(void);
    #define OS_INT_Leave() OS_LeaveInterrupt()
    
    // Leave interrupt with task switch
    void OS_LeaveInterruptNoSwitch(void);
    
    // ISR template
    void MyISR(void) {
        OS_EnterInterrupt();
        
        // ISR code
        
        OS_LeaveInterrupt();
    }

Return Values
=============

.. code-block:: c

    OS_OK                   // 0 = Success
    OS_ERR_TIMEOUT          // Timeout occurred
    OS_ERR_OVERRUN          // Queue/mailbox full
    
    // Boolean
    OS_TRUE                 // 1
    OS_FALSE                // 0

Example Application
===================

.. code-block:: c

    #include "RTOS.h"
    
    OS_STACKPTR int Stack_Producer[128];
    OS_STACKPTR int Stack_Consumer[128];
    OS_TASK TCB_Producer, TCB_Consumer;
    
    OS_Q Queue;
    void *QueueBuffer[10];
    
    static void ProducerTask(void) {
        int count = 0;
        
        while (1) {
            OS_Q_Put(&Queue, (void *)count);
            count++;
            OS_DELAY(1000);
        }
    }
    
    static void ConsumerTask(void) {
        void *msg;
        
        while (1) {
            msg = OS_Q_GetBlocked(&Queue);
            // Process message
        }
    }
    
    int main(void) {
        OS_Init();
        OS_InitHW();
        
        OS_Q_Create(&Queue, QueueBuffer, 10);
        
        OS_CreateTask(&TCB_Producer, "Producer", 100, ProducerTask,
                      Stack_Producer, sizeof(Stack_Producer), 0);
        
        OS_CreateTask(&TCB_Consumer, "Consumer", 100, ConsumerTask,
                      Stack_Consumer, sizeof(Stack_Consumer), 0);
        
        OS_Start();
        return 0;
    }

Common Pitfalls
===============

.. code-block:: text

    ✗ Not calling OS_Init() and OS_InitHW()
    ✗ Forgetting OS_EnterInterrupt()/OS_LeaveInterrupt() in ISRs
    ✗ Using blocking calls from ISR
    ✗ Insufficient stack size (check with OS_GetStackUsed())
    ✗ Priority 1 is highest user priority (0 is system)
    ✗ Not matching mailbox message size
    ✗ Using OS_DELAY(0) expecting yield (use OS_Yield())

See Also
========

- :doc:`rtos_basics_cheatsheet` - Generic RTOS concepts
- :doc:`../days/day15` - embOS deep dive
- Official embOS documentation: segger.com/embos
