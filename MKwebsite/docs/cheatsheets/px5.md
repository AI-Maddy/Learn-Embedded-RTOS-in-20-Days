# :material-lightning-bolt: PX5 RTOS Cheatsheet

> **Dense quick-reference** for PX5 2.x — the ultra-compact picokernel RTOS by William Lamie (original ThreadX author). Designed for severely resource-constrained devices where every byte counts.

---

<div class="grid cards" markdown>

-   :material-lightning-bolt: **Picokernel**

    ---
    Preemptive priority-based. 32 priority levels. Unique **preemption-threshold** scheduling (inherited from ThreadX). Designed for < 8 KB RAM systems.

-   :material-memory: **Footprint**

    ---
    Min ~1 KB RAM, ~3 KB Flash — smallest footprint of any full-featured RTOS. No dynamic memory allocation in kernel — zero fragmentation.

-   :material-account: **Creator**

    ---
    William Lamie, original author of ThreadX. PX5 is a clean-sheet redesign optimized for picokernel use cases. Commercial license via Eventure Embedded.

-   :material-compare: **vs ThreadX**

    ---
    API structure is nearly identical to ThreadX. Teams familiar with ThreadX can adopt PX5 in days. PX5 trades ThreadX's richer feature set for a smaller kernel.

</div>

---

## PX5 vs ThreadX API Similarity

PX5 was designed by the same author as ThreadX with near-identical API patterns. The main differences are the `px_` prefix instead of `tx_` and the absence of some advanced ThreadX features.

| Concept | ThreadX API | PX5 API | Notes |
|---------|-------------|---------|-------|
| Create thread | `tx_thread_create()` | `px_thread_create()` | Same parameters |
| Thread sleep | `tx_thread_sleep()` | `px_thread_sleep()` | Same behavior |
| Create semaphore | `tx_semaphore_create()` | `px_semaphore_create()` | Same parameters |
| Get semaphore | `tx_semaphore_get()` | `px_semaphore_get()` | Same semantics |
| Put semaphore | `tx_semaphore_put()` | `px_semaphore_put()` | Same semantics |
| Create mutex | `tx_mutex_create()` | `px_mutex_create()` | Same parameters |
| Create queue | `tx_queue_create()` | `px_queue_create()` | Same parameters |
| Event flags | `tx_event_flags_*` | `px_event_flags_*` | Same API |
| Byte pool | `tx_byte_pool_*` | `px_byte_pool_*` | Same API |
| Block pool | `tx_block_pool_*` | `px_block_pool_*` | Same API |
| App entry | `tx_application_define()` | `px_application_define()` | Same callback pattern |
| Kernel start | `tx_kernel_enter()` | `px_kernel_enter()` | Same entry |

!!! info "Migration: ThreadX → PX5"
    A global search-and-replace of `tx_` → `px_` and `TX_` → `PX_` covers the vast majority of migration work. Feature differences to check: SMP (not in PX5), FileX/NetX middleware (ThreadX-specific), and TraceX (use PX5 equivalent).

---

## Thread API

| Function | Signature | Description |
|----------|-----------|-------------|
| `px_thread_create` | `UINT px_thread_create(PX_THREAD *thread_ptr, CHAR *name_ptr, VOID (*entry_function)(ULONG), ULONG entry_input, VOID *stack_start, ULONG stack_size, UINT priority, UINT preempt_threshold, ULONG time_slice, UINT auto_start)` | Create and optionally start thread |
| `px_thread_delete` | `UINT px_thread_delete(PX_THREAD *thread_ptr)` | Delete suspended/completed thread |
| `px_thread_suspend` | `UINT px_thread_suspend(PX_THREAD *thread_ptr)` | Suspend thread |
| `px_thread_resume` | `UINT px_thread_resume(PX_THREAD *thread_ptr)` | Resume suspended thread |
| `px_thread_sleep` | `UINT px_thread_sleep(ULONG timer_ticks)` | Sleep current thread for N ticks |
| `px_thread_terminate` | `UINT px_thread_terminate(PX_THREAD *thread_ptr)` | Terminate (does not delete) |
| `px_thread_relinquish` | `VOID px_thread_relinquish(void)` | Yield to equal/higher priority |
| `px_thread_priority_change` | `UINT px_thread_priority_change(PX_THREAD *thread_ptr, UINT new_priority, UINT *old_priority)` | Change thread priority |
| `px_thread_preemption_change` | `UINT px_thread_preemption_change(PX_THREAD *thread_ptr, UINT new_threshold, UINT *old_threshold)` | Set preemption threshold |
| `px_thread_identify` | `PX_THREAD *px_thread_identify(void)` | Get current thread pointer |
| `px_thread_info_get` | `UINT px_thread_info_get(PX_THREAD *thread_ptr, CHAR **name, UINT *state, ULONG *run_count, UINT *priority, UINT *preemption_threshold, ULONG *time_slice)` | Query thread info |

```c title="Thread Pattern"
#include "px_api.h"

PX_THREAD  sensor_thread;
ULONG      sensor_stack[256];   // 256 × 4 = 1024 bytes

VOID sensor_thread_entry(ULONG input) {
    while (1) {
        read_sensor();
        px_thread_sleep(10);    // 10 ticks
    }
}

VOID px_application_define(VOID *first_unused_memory) {
    px_thread_create(&sensor_thread,
                     "Sensor",
                     sensor_thread_entry,
                     0,                       // entry input
                     sensor_stack,
                     sizeof(sensor_stack),
                     5,                       // priority (0=highest)
                     5,                       // preempt threshold
                     PX_NO_TIME_SLICE,
                     PX_AUTO_START);
}

int main(void) {
    px_kernel_enter();  // Never returns
}
```

---

## Semaphore API

| Function | Signature | Description |
|----------|-----------|-------------|
| `px_semaphore_create` | `UINT px_semaphore_create(PX_SEMAPHORE *semaphore_ptr, CHAR *name_ptr, ULONG initial_count)` | Create counting semaphore |
| `px_semaphore_get` | `UINT px_semaphore_get(PX_SEMAPHORE *semaphore_ptr, ULONG wait_option)` | Acquire; `PX_WAIT_FOREVER` to block |
| `px_semaphore_put` | `UINT px_semaphore_put(PX_SEMAPHORE *semaphore_ptr)` | Release (ISR-safe) |
| `px_semaphore_ceiling_put` | `UINT px_semaphore_ceiling_put(PX_SEMAPHORE *semaphore_ptr, ULONG ceiling)` | Put with max count ceiling |
| `px_semaphore_delete` | `UINT px_semaphore_delete(PX_SEMAPHORE *semaphore_ptr)` | Delete semaphore |
| `px_semaphore_info_get` | `UINT px_semaphore_info_get(PX_SEMAPHORE *semaphore_ptr, CHAR **name, ULONG *current_value, PX_THREAD **first_suspended, ULONG *suspended_count)` | Query semaphore |

```c title="Semaphore Pattern — Signal from ISR"
PX_SEMAPHORE uart_sem;
px_semaphore_create(&uart_sem, "UART", 0);   // binary-style, start at 0

// ISR:
px_semaphore_put(&uart_sem);   // ISR-safe

// Task:
px_semaphore_get(&uart_sem, PX_WAIT_FOREVER);
process_uart();
```

---

## Mutex API

| Function | Signature | Description |
|----------|-----------|-------------|
| `px_mutex_create` | `UINT px_mutex_create(PX_MUTEX *mutex_ptr, CHAR *name_ptr, UINT inherit)` | Create mutex; `PX_INHERIT` enables priority inheritance |
| `px_mutex_get` | `UINT px_mutex_get(PX_MUTEX *mutex_ptr, ULONG wait_option)` | Lock mutex (recursive) |
| `px_mutex_put` | `UINT px_mutex_put(PX_MUTEX *mutex_ptr)` | Unlock mutex |
| `px_mutex_delete` | `UINT px_mutex_delete(PX_MUTEX *mutex_ptr)` | Delete mutex |
| `px_mutex_info_get` | `UINT px_mutex_info_get(PX_MUTEX *mutex_ptr, CHAR **name, ULONG *count, PX_THREAD **owner)` | Query mutex state |

```c title="Mutex Pattern"
PX_MUTEX spi_mutex;
px_mutex_create(&spi_mutex, "SPI", PX_INHERIT);

void access_spi(void) {
    px_mutex_get(&spi_mutex, PX_WAIT_FOREVER);
    spi_transfer();
    px_mutex_put(&spi_mutex);
}
```

---

## Queue API

| Function | Signature | Description |
|----------|-----------|-------------|
| `px_queue_create` | `UINT px_queue_create(PX_QUEUE *queue_ptr, CHAR *name_ptr, UINT message_size, VOID *queue_start, ULONG queue_size)` | Create queue (message_size: 1/2/4/8/16 ULONGs) |
| `px_queue_send` | `UINT px_queue_send(PX_QUEUE *queue_ptr, VOID *source_ptr, ULONG wait_option)` | Send to back of queue |
| `px_queue_send_notify` | `UINT px_queue_send_notify(PX_QUEUE *queue_ptr, VOID (*notify)(PX_QUEUE *))` | Register send notification |
| `px_queue_receive` | `UINT px_queue_receive(PX_QUEUE *queue_ptr, VOID *destination_ptr, ULONG wait_option)` | Receive from front |
| `px_queue_front_send` | `UINT px_queue_front_send(PX_QUEUE *queue_ptr, VOID *source_ptr, ULONG wait_option)` | Send to front (priority) |
| `px_queue_flush` | `UINT px_queue_flush(PX_QUEUE *queue_ptr)` | Discard all pending messages |
| `px_queue_delete` | `UINT px_queue_delete(PX_QUEUE *queue_ptr)` | Delete queue |
| `px_queue_info_get` | `UINT px_queue_info_get(PX_QUEUE *queue_ptr, CHAR **name, ULONG *enqueued, ULONG *available_storage)` | Query queue state |

```c title="Queue Pattern"
PX_QUEUE cmd_queue;
ULONG    queue_storage[32];

px_queue_create(&cmd_queue, "Cmds", PX_1_ULONG,
                queue_storage, sizeof(queue_storage));

// Send pointer as message:
Cmd_t *cmd = get_cmd_from_pool();
px_queue_send(&cmd_queue, &cmd, PX_WAIT_FOREVER);

// Receive:
Cmd_t *received;
px_queue_receive(&cmd_queue, &received, PX_WAIT_FOREVER);
handle_cmd(received);
return_cmd_to_pool(received);
```

---

## Footprint Comparison Table

PX5 vs other RTOSes on a typical ARM Cortex-M0+ (32-bit, no FPU):

| RTOS | Min Kernel RAM | Min Kernel Flash | Notes |
|------|:--------------:|:----------------:|-------|
| **PX5** | ~1 KB | ~3 KB | Picokernel; limited middleware |
| **embOS** | ~1 KB | ~4 KB | Similar footprint; more certification options |
| **ThreadX** | ~2 KB | ~6 KB | PX5's larger sibling; more features |
| **FreeRTOS** | ~4 KB | ~6 KB | heap4.c adds overhead |
| **ChibiOS/RT** | ~2 KB | ~8 KB | Clean HAL adds to Flash |
| **Zephyr** | ~8 KB | ~32 KB | Rich subsystems; much larger |
| **NuttX** | ~32 KB | ~64 KB | Full POSIX; largest footprint |
| **eCos** | ~10 KB | ~30 KB | Configurable but old codebase |

!!! tip "When to Choose PX5 Over FreeRTOS"
    Choose PX5 when: (1) RAM is under 4 KB, (2) your team knows ThreadX-style APIs, (3) you need preemption-threshold scheduling, or (4) you are migrating from ThreadX to a smaller target. FreeRTOS is a better choice for teams starting fresh with large community support needs.

---

## Key PX5 Constants

| Constant | Value | Description |
|----------|-------|-------------|
| `PX_WAIT_FOREVER` | `0xFFFFFFFF` | Block indefinitely |
| `PX_NO_WAIT` | `0` | Non-blocking (immediate return) |
| `PX_AUTO_START` | `1` | Auto-start thread on create |
| `PX_DONT_START` | `0` | Create thread in suspended state |
| `PX_NO_TIME_SLICE` | `0` | Disable time slicing |
| `PX_INHERIT` | `1` | Enable priority inheritance (mutex) |
| `PX_NO_INHERIT` | `0` | Disable priority inheritance |
| `PX_AND` | `2` | Event flags: wait for ALL bits |
| `PX_OR` | `0` | Event flags: wait for ANY bit |
| `PX_AND_CLEAR` | `3` | Event flags: wait ALL and clear |
| `PX_OR_CLEAR` | `1` | Event flags: wait ANY and clear |
| `PX_1_ULONG` | `1` | Queue message: 1 word (4 bytes) |
| `PX_2_ULONG` | `2` | Queue message: 2 words (8 bytes) |
| `PX_4_ULONG` | `4` | Queue message: 4 words (16 bytes) |

---

## Gotchas & Pitfalls

!!! danger "px_application_define — No Blocking Allowed"
    Like ThreadX, `px_application_define()` runs before the scheduler. Calling any blocking API (`PX_WAIT_FOREVER`) will hang. Create all objects here; start work in thread entry functions.

!!! danger "Queue Message Size — Only 1/2/4/8/16 ULONGs"
    PX5 queues have the same constraint as ThreadX: message size must be exactly 1, 2, 4, 8, or 16 ULONG words. Use a pointer (PX_1_ULONG) for variable-size payloads.

!!! warning "No SMP in PX5"
    PX5 is a single-core kernel only. If your hardware has multiple cores, use ThreadX (which supports SMP) or Zephyr. PX5's picokernel design does not include SMP support.

!!! warning "Commercial License Required"
    PX5 is not open source. A commercial license from Eventure Embedded is required for production use. Evaluation licenses may be available — contact the vendor.

!!! tip "Preemption Threshold — Use Instead of Mutexes for Simple Critical Sections"
    `px_thread_preemption_change()` provides mutual exclusion without blocking overhead. Set threshold to the priority of the thread you want to exclude, do critical work, then restore. More efficient than mutex for short critical sections.
