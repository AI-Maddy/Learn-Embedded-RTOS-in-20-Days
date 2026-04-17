# :material-microsoft: ThreadX (Azure RTOS) Cheatsheet

> **Dense quick-reference** for ThreadX 6.4.x (Azure RTOS). All major APIs: threads, semaphores, mutexes, queues, event flags, memory management, and preemption threshold.

---

<div class="grid cards" markdown>

-   :material-cpu-64-bit: **Kernel**

    ---
    Preemptive priority-based. 32 priority levels (0=highest, 31=lowest). Unique **preemption-threshold** scheduling algorithm. FIFO within same priority.

-   :material-memory: **Footprint**

    ---
    Min ~2 KB RAM, ~6 KB Flash. Extremely consistent — no fragmentation by design. Byte pool and block pool memory models.

-   :material-github: **Source**

    ---
    [github.com/eclipse-threadx/threadx](https://github.com/eclipse-threadx/threadx) — Apache 2.0

-   :material-shield-check: **Certifications**

    ---
    IEC 61508 SIL 3, ISO 26262 ASIL D, DO-178C Level A, IEC 62304 Class C, IEC 60730 Class C.

</div>

---

## Thread API

| Function | Signature | Description |
|----------|-----------|-------------|
| `tx_thread_create` | `UINT tx_thread_create(TX_THREAD *thread_ptr, CHAR *name_ptr, VOID (*entry_function)(ULONG), ULONG entry_input, VOID *stack_start, ULONG stack_size, UINT priority, UINT preempt_threshold, ULONG time_slice, UINT auto_start)` | Create and optionally start thread |
| `tx_thread_delete` | `UINT tx_thread_delete(TX_THREAD *thread_ptr)` | Delete a suspended/completed thread |
| `tx_thread_suspend` | `UINT tx_thread_suspend(TX_THREAD *thread_ptr)` | Suspend thread |
| `tx_thread_resume` | `UINT tx_thread_resume(TX_THREAD *thread_ptr)` | Resume suspended thread |
| `tx_thread_sleep` | `UINT tx_thread_sleep(ULONG timer_ticks)` | Sleep current thread for N ticks |
| `tx_thread_terminate` | `UINT tx_thread_terminate(TX_THREAD *thread_ptr)` | Terminate (does not delete) |
| `tx_thread_relinquish` | `VOID tx_thread_relinquish(void)` | Yield to equal/higher priority thread |
| `tx_thread_priority_change` | `UINT tx_thread_priority_change(TX_THREAD *thread_ptr, UINT new_priority, UINT *old_priority)` | Change priority at runtime |
| `tx_thread_info_get` | `UINT tx_thread_info_get(TX_THREAD *thread_ptr, CHAR **name, UINT *state, ULONG *run_count, UINT *priority, UINT *preemption_threshold, ULONG *time_slice, TX_THREAD **next_thread, TX_THREAD **next_suspended_thread)` | Query thread state/statistics |
| `tx_thread_identify` | `TX_THREAD *tx_thread_identify(void)` | Get current thread pointer |

```c title="Thread Pattern — Basic Creation"
#include "tx_api.h"

TX_THREAD sensor_thread;
ULONG     sensor_stack[512];  // 512 ULONG words = 2048 bytes

VOID sensor_thread_entry(ULONG input) {
    while (1) {
        read_sensor();
        tx_thread_sleep(10);  // sleep 10 ticks
    }
}

// In tx_application_define():
tx_thread_create(&sensor_thread,
                 "Sensor",
                 sensor_thread_entry,
                 0x1234,                  // entry_input parameter
                 sensor_stack,
                 sizeof(sensor_stack),
                 5,                       // priority (0=highest)
                 5,                       // preempt_threshold (= priority → no threshold)
                 TX_NO_TIME_SLICE,        // time slice disabled
                 TX_AUTO_START);          // start immediately
```

---

## Semaphore API

| Function | Signature | Description |
|----------|-----------|-------------|
| `tx_semaphore_create` | `UINT tx_semaphore_create(TX_SEMAPHORE *semaphore_ptr, CHAR *name_ptr, ULONG initial_count)` | Create counting semaphore |
| `tx_semaphore_get` | `UINT tx_semaphore_get(TX_SEMAPHORE *semaphore_ptr, ULONG wait_option)` | Acquire; `TX_WAIT_FOREVER` to block |
| `tx_semaphore_put` | `UINT tx_semaphore_put(TX_SEMAPHORE *semaphore_ptr)` | Release (ISR-safe) |
| `tx_semaphore_put_notify` | `UINT tx_semaphore_put_notify(TX_SEMAPHORE *semaphore_ptr, VOID (*semaphore_put_notify)(TX_SEMAPHORE *))` | Register callback on put |
| `tx_semaphore_ceiling_put` | `UINT tx_semaphore_ceiling_put(TX_SEMAPHORE *semaphore_ptr, ULONG ceiling)` | Put with maximum count ceiling |
| `tx_semaphore_delete` | `UINT tx_semaphore_delete(TX_SEMAPHORE *semaphore_ptr)` | Delete semaphore |
| `tx_semaphore_info_get` | `UINT tx_semaphore_info_get(TX_SEMAPHORE *semaphore_ptr, CHAR **name, ULONG *current_value, TX_THREAD **first_suspended, ULONG *suspended_count, TX_SEMAPHORE **next_semaphore)` | Query semaphore state |

```c title="Semaphore Pattern — Binary (ISR Sync)"
TX_SEMAPHORE data_ready;

// In init:
tx_semaphore_create(&data_ready, "data_rdy", 0);  // start at 0

// In ISR:
tx_semaphore_put(&data_ready);  // ISR-safe

// In task:
if (tx_semaphore_get(&data_ready, TX_WAIT_FOREVER) == TX_SUCCESS) {
    process_data();
}
```

---

## Mutex API

| Function | Signature | Description |
|----------|-----------|-------------|
| `tx_mutex_create` | `UINT tx_mutex_create(TX_MUTEX *mutex_ptr, CHAR *name_ptr, UINT inherit)` | Create mutex; `TX_INHERIT` enables priority inheritance |
| `tx_mutex_get` | `UINT tx_mutex_get(TX_MUTEX *mutex_ptr, ULONG wait_option)` | Lock mutex (recursive — same thread can lock again) |
| `tx_mutex_put` | `UINT tx_mutex_put(TX_MUTEX *mutex_ptr)` | Unlock mutex |
| `tx_mutex_delete` | `UINT tx_mutex_delete(TX_MUTEX *mutex_ptr)` | Delete mutex |
| `tx_mutex_info_get` | `UINT tx_mutex_info_get(TX_MUTEX *mutex_ptr, CHAR **name, ULONG *count, TX_THREAD **owner, TX_THREAD **first_suspended, ULONG *suspended_count, TX_MUTEX **next_mutex)` | Query mutex state |

```c title="Mutex Pattern"
TX_MUTEX spi_mutex;
tx_mutex_create(&spi_mutex, "SPI", TX_INHERIT);  // priority inheritance

void access_spi(void) {
    tx_mutex_get(&spi_mutex, TX_WAIT_FOREVER);
    // critical section
    spi_transfer();
    tx_mutex_put(&spi_mutex);
}
```

---

## Queue API

| Function | Signature | Description |
|----------|-----------|-------------|
| `tx_queue_create` | `UINT tx_queue_create(TX_QUEUE *queue_ptr, CHAR *name_ptr, UINT message_size, VOID *queue_start, ULONG queue_size)` | Create queue; `message_size` in 32-bit words (1, 2, 4, 8, or 16) |
| `tx_queue_send` | `UINT tx_queue_send(TX_QUEUE *queue_ptr, VOID *source_ptr, ULONG wait_option)` | Send to queue back; blocks if full |
| `tx_queue_send_notify` | `UINT tx_queue_send_notify(TX_QUEUE *queue_ptr, VOID (*queue_send_notify)(TX_QUEUE *))` | Callback when message sent |
| `tx_queue_receive` | `UINT tx_queue_receive(TX_QUEUE *queue_ptr, VOID *destination_ptr, ULONG wait_option)` | Receive from front; blocks if empty |
| `tx_queue_front_send` | `UINT tx_queue_front_send(TX_QUEUE *queue_ptr, VOID *source_ptr, ULONG wait_option)` | Send to front (priority message) |
| `tx_queue_flush` | `UINT tx_queue_flush(TX_QUEUE *queue_ptr)` | Discard all messages |
| `tx_queue_delete` | `UINT tx_queue_delete(TX_QUEUE *queue_ptr)` | Delete queue |
| `tx_queue_info_get` | `UINT tx_queue_info_get(TX_QUEUE *queue_ptr, CHAR **name, ULONG *enqueued, ULONG *available_storage, TX_THREAD **first_suspended, ULONG *suspended_count, TX_QUEUE **next_queue)` | Query queue state |

!!! note "Queue Message Size"
    ThreadX queue messages are a fixed multiple of 32-bit words. Supported sizes: 1, 2, 4, 8, or 16 words. To pass a pointer, use `message_size = 1`. For larger structs, send a pointer to the struct rather than copying.

```c title="Queue Pattern — Pass-by-Pointer"
TX_QUEUE cmd_queue;
ULONG    queue_storage[32];  // 32 ULONG slots = 32 single-word messages

typedef struct { uint8_t cmd; uint8_t *data; uint16_t len; } Cmd_t;

// Single-word queue stores pointer to command struct
tx_queue_create(&cmd_queue, "CmdQ", TX_1_ULONG,
                queue_storage, sizeof(queue_storage));

// Producer — allocate from byte pool, send pointer
Cmd_t *cmd_ptr;
tx_byte_allocate(&cmd_pool, (VOID **)&cmd_ptr, sizeof(Cmd_t), TX_NO_WAIT);
cmd_ptr->cmd = CMD_READ;
tx_queue_send(&cmd_queue, &cmd_ptr, TX_WAIT_FOREVER);

// Consumer
Cmd_t *received;
tx_queue_receive(&cmd_queue, &received, TX_WAIT_FOREVER);
handle_command(received);
tx_byte_release(received);  // return to pool
```

---

## Event Flags API

| Function | Signature | Description |
|----------|-----------|-------------|
| `tx_event_flags_create` | `UINT tx_event_flags_create(TX_EVENT_FLAGS_GROUP *group_ptr, CHAR *name_ptr)` | Create 32-bit event flags group |
| `tx_event_flags_set` | `UINT tx_event_flags_set(TX_EVENT_FLAGS_GROUP *group_ptr, ULONG flags_to_set, UINT set_option)` | Set or clear bits; `TX_OR` to set, `TX_AND` to clear |
| `tx_event_flags_get` | `UINT tx_event_flags_get(TX_EVENT_FLAGS_GROUP *group_ptr, ULONG requested_flags, UINT get_option, ULONG *actual_flags_ptr, ULONG wait_option)` | Wait for flags (`TX_AND`/`TX_OR`, optionally `_CLEAR`) |
| `tx_event_flags_delete` | `UINT tx_event_flags_delete(TX_EVENT_FLAGS_GROUP *group_ptr)` | Delete event flags group |
| `tx_event_flags_info_get` | `UINT tx_event_flags_info_get(TX_EVENT_FLAGS_GROUP *group_ptr, CHAR **name, ULONG *current_flags, TX_THREAD **first_suspended, ULONG *suspended_count, TX_EVENT_FLAGS_GROUP **next_group)` | Query current flag state |

```c title="Event Flags Pattern"
#define EVT_SENSOR_DONE   0x01
#define EVT_COMM_DONE     0x02
#define EVT_BUTTON_PRESS  0x04

TX_EVENT_FLAGS_GROUP system_events;
tx_event_flags_create(&system_events, "SysEvt");

// Set from task or ISR:
tx_event_flags_set(&system_events, EVT_SENSOR_DONE, TX_OR);

// Wait for ALL of sensor + comm (clear on success):
ULONG actual;
tx_event_flags_get(&system_events,
                   EVT_SENSOR_DONE | EVT_COMM_DONE,
                   TX_AND_CLEAR,  // both must be set; clear after
                   &actual,
                   TX_WAIT_FOREVER);
```

---

## Memory Management

### Byte Pool (Variable-Size Allocation)

| Function | Signature | Description |
|----------|-----------|-------------|
| `tx_byte_pool_create` | `UINT tx_byte_pool_create(TX_BYTE_POOL *pool_ptr, CHAR *name_ptr, VOID *pool_start, ULONG pool_size)` | Create variable-size memory pool |
| `tx_byte_allocate` | `UINT tx_byte_allocate(TX_BYTE_POOL *pool_ptr, VOID **memory_ptr, ULONG memory_size, ULONG wait_option)` | Allocate variable-size block |
| `tx_byte_release` | `UINT tx_byte_release(VOID *memory_ptr)` | Return block to pool |
| `tx_byte_pool_delete` | `UINT tx_byte_pool_delete(TX_BYTE_POOL *pool_ptr)` | Delete byte pool |

### Block Pool (Fixed-Size Allocation)

| Function | Signature | Description |
|----------|-----------|-------------|
| `tx_block_pool_create` | `UINT tx_block_pool_create(TX_BLOCK_POOL *pool_ptr, CHAR *name_ptr, ULONG block_size, VOID *pool_start, ULONG pool_size)` | Create fixed-size block pool (no fragmentation) |
| `tx_block_allocate` | `UINT tx_block_allocate(TX_BLOCK_POOL *pool_ptr, VOID **block_ptr, ULONG wait_option)` | Allocate one fixed-size block |
| `tx_block_release` | `UINT tx_block_release(VOID *block_ptr)` | Return block to pool |
| `tx_block_pool_delete` | `UINT tx_block_pool_delete(TX_BLOCK_POOL *pool_ptr)` | Delete block pool |

```c title="Memory Pattern — Block Pool for Zero Fragmentation"
#define NUM_MSGS  16
#define MSG_SIZE  64

TX_BLOCK_POOL msg_pool;
UCHAR         msg_pool_mem[NUM_MSGS * (MSG_SIZE + sizeof(void*))];

tx_block_pool_create(&msg_pool, "MsgPool", MSG_SIZE,
                     msg_pool_mem, sizeof(msg_pool_mem));

// Allocate:
VOID *msg_buf;
tx_block_allocate(&msg_pool, &msg_buf, TX_NO_WAIT);

// Use msg_buf ...

// Release:
tx_block_release(msg_buf);
```

---

## Preemption Threshold API

ThreadX's unique **preemption-threshold** feature lets a thread prevent preemption by threads below a specified priority while still allowing preemption by higher-priority threads.

| Function | Signature | Description |
|----------|-----------|-------------|
| `tx_thread_preemption_change` | `UINT tx_thread_preemption_change(TX_THREAD *thread_ptr, UINT new_threshold, UINT *old_threshold)` | Set preemption threshold dynamically |

```c title="Preemption Threshold Pattern"
// Thread at priority 5 can be preempted by priorities 0-4 normally.
// Set threshold to 3: can only be preempted by priorities 0-2.
// This is like a mutex but without the overhead of a mutex operation.

UINT old_threshold;
tx_thread_preemption_change(&my_thread, 3, &old_threshold);

// Critical section — only priority 0-2 can preempt
do_critical_work();

// Restore original preemption threshold
tx_thread_preemption_change(&my_thread, old_threshold, &old_threshold);
```

!!! info "Why Preemption Threshold?"
    Traditional mutexes cause priority inversion risk (even with inheritance). Preemption threshold provides mutual exclusion between specific priority levels without any blocking, priority inheritance overhead, or deadlock risk. It is deterministic and O(1).

---

## Initialization Pattern

```c title="tx_application_define — Entry Point"
#include "tx_api.h"

// ThreadX calls this function from tx_kernel_enter()
// All resources must be created here (or after kernel starts)
VOID tx_application_define(VOID *first_unused_memory) {
    // Divide first_unused_memory as needed for stacks/pools
    UCHAR *pointer = (UCHAR *)first_unused_memory;

    // Create byte pool from remaining memory
    tx_byte_pool_create(&heap_pool, "Heap", pointer, 32768);

    // Allocate stack for threads
    VOID *stack_ptr;
    tx_byte_allocate(&heap_pool, &stack_ptr, 2048, TX_NO_WAIT);

    // Create thread
    tx_thread_create(&main_thread, "Main", main_thread_entry,
                     0, stack_ptr, 2048, 5, 5,
                     TX_NO_TIME_SLICE, TX_AUTO_START);
}

int main(void) {
    tx_kernel_enter();  // Never returns
}
```

---

## Gotchas & Pitfalls

!!! danger "tx_application_define — Do NOT Call Blocking APIs Here"
    `tx_application_define` runs before the scheduler starts. Calling `tx_semaphore_get(TX_WAIT_FOREVER)` will hang because no threads are running to signal it. Create objects here; don't wait on them.

!!! danger "Queue Message Size — Only 1/2/4/8/16 Words"
    ThreadX queues only support message sizes that are exact multiples of 32-bit words: 1, 2, 4, 8, or 16 ULONGs. Attempting other sizes causes undefined behavior. Send a pointer for variable-size data.

!!! warning "tx_thread_delete — Thread Must Be Suspended or Completed"
    You cannot delete a running or ready thread. The thread must be in `TX_SUSPENDED` or `TX_COMPLETED` state. Use `tx_thread_terminate()` first, then `tx_thread_delete()`.

!!! warning "Byte Pool vs Block Pool — Choose Carefully"
    Byte pools (`tx_byte_allocate`) can fragment over time. Block pools (`tx_block_allocate`) never fragment but require fixed-size blocks. For real-time systems, prefer block pools to guarantee allocation times.

!!! tip "TX_WAIT_FOREVER vs TX_NO_WAIT"
    `TX_WAIT_FOREVER = 0xFFFFFFFF`. `TX_NO_WAIT = 0`. `TX_TIMER_TICKS_PER_SECOND * n` for n-second timeout. Avoid magic numbers — use these constants.

!!! tip "Enable TraceX for Debugging"
    Define `TX_ENABLE_EVENT_TRACE` in `tx_user.h` and link TraceX buffer. The Azure RTOS Studio extension for VS Code displays trace events graphically with task switch timelines.
