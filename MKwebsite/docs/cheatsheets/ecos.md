# :material-cog: eCos Cheatsheet

> **Dense quick-reference** for eCos 3.0. Threads, semaphores, mutexes, interrupts, clocks/alarms, and ecosconfig commands. eCos is a legacy RTOS still found in deployed industrial and networking systems.

---

<div class="grid cards" markdown>

-   :material-history: **Legacy RTOS**

    ---
    eCos (embedded Configurable operating system) was created by Red Hat in 1997. Last major release: 3.0 (2009). Still deployed in networking equipment, industrial controllers, and legacy embedded systems.

-   :material-cog-sync: **Configurable**

    ---
    Extreme configurability via `ecosconfig` tool and CDL (Component Definition Language). Only include what you need — unused components contribute zero overhead.

-   :material-scale-balance: **License**

    ---
    Modified GPL with a special exception: application code linking against eCos APIs is NOT considered a derivative work and may remain proprietary.

-   :material-memory: **Footprint**

    ---
    Min ~10 KB RAM, ~30 KB Flash for a minimal configuration. Configurable down further by removing unused components.

</div>

---

## Thread API

| Function | Signature | Description |
|----------|-----------|-------------|
| `cyg_thread_create` | `void cyg_thread_create(cyg_addrword_t sched_info, cyg_thread_entry_t *entry, cyg_addrword_t entry_data, char *name, void *stack_base, cyg_ucount32 stack_size, cyg_handle_t *handle, cyg_thread *thread)` | Create thread (does not start it) |
| `cyg_thread_resume` | `void cyg_thread_resume(cyg_handle_t thread)` | Start or resume thread |
| `cyg_thread_suspend` | `void cyg_thread_suspend(cyg_handle_t thread)` | Suspend thread |
| `cyg_thread_exit` | `void cyg_thread_exit(void)` | Exit current thread |
| `cyg_thread_kill` | `void cyg_thread_kill(cyg_handle_t thread)` | Kill specified thread |
| `cyg_thread_delay` | `void cyg_thread_delay(cyg_tick_count_t delay)` | Delay for N clock ticks |
| `cyg_thread_self` | `cyg_handle_t cyg_thread_self(void)` | Get current thread handle |
| `cyg_thread_set_priority` | `void cyg_thread_set_priority(cyg_handle_t thread, cyg_priority_t priority)` | Change thread priority |
| `cyg_thread_get_priority` | `cyg_priority_t cyg_thread_get_priority(cyg_handle_t thread)` | Get thread priority |
| `cyg_thread_yield` | `void cyg_thread_yield(void)` | Yield to equal or higher priority |
| `cyg_thread_get_stack_base` | `void *cyg_thread_get_stack_base(cyg_handle_t thread)` | Get stack base address |
| `cyg_thread_get_stack_size` | `cyg_ucount32 cyg_thread_get_stack_size(cyg_handle_t thread)` | Get stack size |
| `cyg_thread_measure_stack_usage` | `cyg_ucount32 cyg_thread_measure_stack_usage(cyg_handle_t thread)` | Measure maximum stack used |

!!! note "eCos Priority Convention"
    eCos uses lower numbers for higher priority (0 = highest). Default range is 0–31. The scheduler type (MLQUEUE or BITMAP) is configured at build time.

```c title="Thread Pattern — Static Creation"
#include <cyg/kernel/kapi.h>

static cyg_thread  thread_obj;
static cyg_handle_t thread_handle;
static char         thread_stack[4096];

static void thread_entry(cyg_addrword_t data) {
    while (1) {
        do_work();
        cyg_thread_delay(100);  // 100 ticks
    }
}

// In cyg_user_start() or main():
cyg_thread_create(
    5,                      // priority (0=highest)
    thread_entry,           // entry function
    (cyg_addrword_t)0,      // entry data
    "Worker",               // name
    thread_stack,           // stack buffer
    sizeof(thread_stack),   // stack size
    &thread_handle,         // handle out
    &thread_obj             // thread object storage
);
cyg_thread_resume(thread_handle);   // start it
```

---

## Semaphore API

| Function | Signature | Description |
|----------|-----------|-------------|
| `cyg_semaphore_init` | `void cyg_semaphore_init(cyg_sem_t *sem, cyg_count32 val)` | Initialize semaphore with value |
| `cyg_semaphore_wait` | `cyg_bool_t cyg_semaphore_wait(cyg_sem_t *sem)` | Decrement; blocks if 0 (returns true) |
| `cyg_semaphore_timed_wait` | `cyg_bool_t cyg_semaphore_timed_wait(cyg_sem_t *sem, cyg_tick_count_t abstime)` | Wait with absolute tick timeout |
| `cyg_semaphore_trywait` | `cyg_bool_t cyg_semaphore_trywait(cyg_sem_t *sem)` | Non-blocking attempt; false if 0 |
| `cyg_semaphore_post` | `void cyg_semaphore_post(cyg_sem_t *sem)` | Increment (ISR-safe) |
| `cyg_semaphore_peek` | `void cyg_semaphore_peek(cyg_sem_t *sem, cyg_count32 *val)` | Read current value without blocking |
| `cyg_semaphore_destroy` | `void cyg_semaphore_destroy(cyg_sem_t *sem)` | Destroy semaphore |

```c title="Semaphore Pattern — Binary Sync"
cyg_sem_t data_ready;
cyg_semaphore_init(&data_ready, 0);  // start blocked

// ISR or producer:
cyg_semaphore_post(&data_ready);

// Consumer:
cyg_semaphore_wait(&data_ready);
process_data();
```

---

## Mutex API

| Function | Signature | Description |
|----------|-----------|-------------|
| `cyg_mutex_init` | `void cyg_mutex_init(cyg_mutex_t *mutex)` | Initialize mutex |
| `cyg_mutex_lock` | `cyg_bool_t cyg_mutex_lock(cyg_mutex_t *mutex)` | Lock (blocking) |
| `cyg_mutex_trylock` | `cyg_bool_t cyg_mutex_trylock(cyg_mutex_t *mutex)` | Non-blocking attempt |
| `cyg_mutex_unlock` | `void cyg_mutex_unlock(cyg_mutex_t *mutex)` | Unlock |
| `cyg_mutex_destroy` | `void cyg_mutex_destroy(cyg_mutex_t *mutex)` | Destroy mutex |
| `cyg_mutex_set_ceiling` | `void cyg_mutex_set_ceiling(cyg_mutex_t *mutex, cyg_priority_t priority)` | Set priority ceiling |
| `cyg_mutex_set_protocol` | `void cyg_mutex_set_protocol(cyg_mutex_t *mutex, enum cyg_mutex_protocol protocol)` | Set NONE, INHERIT, or CEILING |

```c title="Mutex Pattern — Priority Inheritance"
cyg_mutex_t spi_mutex;
cyg_mutex_init(&spi_mutex);
cyg_mutex_set_protocol(&spi_mutex, CYG_MUTEX_INHERIT);  // priority inheritance

void access_spi(void) {
    cyg_mutex_lock(&spi_mutex);
    spi_transfer();
    cyg_mutex_unlock(&spi_mutex);
}
```

---

## Interrupt API

eCos uses a two-level ISR model: a fast ISR runs at interrupt level, and a Deferred Service Routine (DSR) runs at a lower level for longer processing.

| Function | Signature | Description |
|----------|-----------|-------------|
| `cyg_interrupt_create` | `void cyg_interrupt_create(cyg_vector_t vector, cyg_priority_t priority, cyg_addrword_t data, cyg_ISR_t *isr, cyg_DSR_t *dsr, cyg_handle_t *handle, cyg_interrupt *intr)` | Create interrupt object |
| `cyg_interrupt_attach` | `void cyg_interrupt_attach(cyg_handle_t interrupt)` | Attach (connect) interrupt to vector |
| `cyg_interrupt_detach` | `void cyg_interrupt_detach(cyg_handle_t interrupt)` | Detach interrupt from vector |
| `cyg_interrupt_mask` | `void cyg_interrupt_mask(cyg_vector_t vector)` | Mask (disable) interrupt vector |
| `cyg_interrupt_unmask` | `void cyg_interrupt_unmask(cyg_vector_t vector)` | Unmask (enable) interrupt vector |
| `cyg_interrupt_mask_intunsafe` | `void cyg_interrupt_mask_intunsafe(cyg_vector_t vector)` | Mask from ISR context (not interrupt-safe) |
| `cyg_interrupt_unmask_intunsafe` | `void cyg_interrupt_unmask_intunsafe(cyg_vector_t vector)` | Unmask from ISR context |
| `cyg_interrupt_acknowledge` | `void cyg_interrupt_acknowledge(cyg_vector_t vector)` | Acknowledge interrupt to hardware |
| `cyg_interrupt_configure` | `void cyg_interrupt_configure(cyg_vector_t vector, cyg_bool_t level, cyg_bool_t up)` | Configure level/edge, high/low |
| `cyg_interrupt_disable` | `void cyg_interrupt_disable(void)` | Disable all maskable interrupts |
| `cyg_interrupt_enable` | `void cyg_interrupt_enable(void)` | Enable all maskable interrupts |

```c title="Interrupt Pattern — ISR + DSR"
#include <cyg/kernel/kapi.h>
#include <cyg/hal/hal_intr.h>

static cyg_interrupt intr_obj;
static cyg_handle_t  intr_handle;
static cyg_sem_t     intr_sem;

// Fast ISR — minimal work, acknowledge hardware
static cyg_uint32 my_isr(cyg_vector_t vector, cyg_addrword_t data) {
    cyg_interrupt_mask(vector);        // prevent re-entry
    cyg_interrupt_acknowledge(vector); // tell hardware we handled it
    return CYG_ISR_CALL_DSR;           // schedule DSR
}

// DSR — deferred, runs at lower priority than ISR
static void my_dsr(cyg_vector_t vector, cyg_ucount32 count, cyg_addrword_t data) {
    cyg_semaphore_post(&intr_sem);     // unblock waiting thread
    cyg_interrupt_unmask(vector);      // re-enable for next interrupt
}

// Thread that processes interrupt data
static void intr_thread_entry(cyg_addrword_t data) {
    while (1) {
        cyg_semaphore_wait(&intr_sem);
        process_interrupt_data();
    }
}

// In cyg_user_start():
cyg_semaphore_init(&intr_sem, 0);
cyg_interrupt_create(CYGNUM_HAL_INTERRUPT_RTC, 99,
                     (cyg_addrword_t)NULL,
                     my_isr, my_dsr,
                     &intr_handle, &intr_obj);
cyg_interrupt_attach(intr_handle);
cyg_interrupt_unmask(CYGNUM_HAL_INTERRUPT_RTC);
```

---

## Clock API

eCos provides a hardware-independent clock abstraction with alarms (one-shot or repeating callbacks).

| Function | Signature | Description |
|----------|-----------|-------------|
| `cyg_clock_to_counter` | `cyg_handle_t cyg_clock_to_counter(cyg_handle_t clock)` | Get counter handle from clock handle |
| `cyg_counter_current_value` | `cyg_tick_count_t cyg_counter_current_value(cyg_handle_t counter)` | Read current tick count |
| `cyg_alarm_create` | `void cyg_alarm_create(cyg_handle_t counter, cyg_alarm_t *alarmfn, cyg_addrword_t data, cyg_handle_t *handle, cyg_alarm *alarm)` | Create alarm object |
| `cyg_alarm_initialize` | `void cyg_alarm_initialize(cyg_handle_t alarm, cyg_tick_count_t trigger, cyg_tick_count_t interval)` | Set trigger and optional interval |
| `cyg_alarm_enable` | `void cyg_alarm_enable(cyg_handle_t alarm)` | Enable alarm |
| `cyg_alarm_disable` | `void cyg_alarm_disable(cyg_handle_t alarm)` | Disable alarm |
| `cyg_alarm_delete` | `void cyg_alarm_delete(cyg_handle_t alarm)` | Delete alarm |
| `cyg_current_time` | `cyg_tick_count_t cyg_current_time(void)` | Get current real-time clock tick |
| `cyg_thread_delay` | `void cyg_thread_delay(cyg_tick_count_t delay)` | Delay N ticks from now |

```c title="Clock / Alarm Pattern — Periodic Callback"
#include <cyg/kernel/kapi.h>

static cyg_alarm  alarm_obj;
static cyg_handle_t alarm_handle;

// Alarm callback — called from DSR context (not ISR)
static void alarm_callback(cyg_handle_t alarm, cyg_addrword_t data) {
    // Periodic processing
    update_watchdog();
    send_heartbeat();
}

// In cyg_user_start():
cyg_handle_t clock = cyg_real_time_clock();    // system real-time clock
cyg_handle_t counter;
cyg_clock_to_counter(clock, &counter);

cyg_alarm_create(counter,
                 alarm_callback,
                 (cyg_addrword_t)NULL,
                 &alarm_handle,
                 &alarm_obj);

cyg_tick_count_t now = cyg_current_time();
cyg_alarm_initialize(alarm_handle,
                     now + 100,   // first trigger: 100 ticks from now
                     100);        // repeat every 100 ticks (periodic)
cyg_alarm_enable(alarm_handle);
```

---

## ecosconfig Commands Reference

`ecosconfig` is the eCos build system tool for configuring and generating a custom eCos library.

| Command | Description |
|---------|-------------|
| `ecosconfig new <target> <template>` | Create new eCos configuration for target/template |
| `ecosconfig list` | List available targets, templates, and packages |
| `ecosconfig packages` | List all available packages |
| `ecosconfig add <package>` | Add a package to configuration |
| `ecosconfig remove <package>` | Remove a package from configuration |
| `ecosconfig check` | Verify configuration for conflicts |
| `ecosconfig resolve` | Automatically resolve conflicts |
| `ecosconfig merge <file>` | Merge saved configuration changes |
| `ecosconfig export <file>` | Export current config to file |
| `ecosconfig import <file>` | Import configuration from file |
| `ecosconfig diff` | Show differences from default config |
| `ecosconfig tree` | Generate build tree (Makefile, headers) |
| `make` | Build eCos library (after `ecosconfig tree`) |
| `make clean` | Clean build products |
| `make tests` | Build test programs |

```bash title="ecosconfig Workflow"
# 1. Set ECOS_REPOSITORY environment variable
export ECOS_REPOSITORY=/opt/ecos/packages

# 2. Create new config for ARM Cortex-M3 with default template
mkdir ecos_build && cd ecos_build
ecosconfig new arm_cortexm3 default

# 3. Inspect and edit configuration
ecosconfig check                    # verify no conflicts
configtool ecos.ecc                 # optional GUI configurator

# 4. Manual CDL edits in ecos.ecc (text-based):
# cdl_option CYGNUM_KERNEL_SCHED_PRIORITIES {
#     user_value 8            # reduce from 32 to 8 priority levels
# };

# 5. Generate build tree and compile
ecosconfig tree
make -j4

# 6. Link application with generated libextras.a and libtarget.a
arm-none-eabi-gcc -o app.elf app.o \
    -L. -Ttarget.ld \
    -Wl,--start-group -lextras -ltarget -Wl,--end-group
```

---

## CDL (Component Definition Language) — Key Options

| CDL Option | Description |
|------------|-------------|
| `CYGIMP_KERNEL_SCHED_MLQUEUE` | Use multi-level queue scheduler (default, supports round-robin) |
| `CYGIMP_KERNEL_SCHED_BITMAP` | Use bitmap scheduler (faster, no round-robin) |
| `CYGNUM_KERNEL_SCHED_PRIORITIES` | Number of thread priority levels |
| `CYGPKG_KERNEL_EXCEPTIONS` | Include exception handling |
| `CYGPKG_KERNEL_INTERRUPTS` | Include interrupt support |
| `CYGPKG_KERNEL_MUTEXES` | Include mutex support |
| `CYGPKG_KERNEL_SEMAPHORES` | Include semaphore support |
| `CYGPKG_KERNEL_CLOCKS` | Include clock and alarm support |
| `CYGPKG_KERNEL_THREADS_DESTRUCTORS` | Include thread destructor support |
| `CYGDBG_KERNEL_DEBUG_GDB_THREAD_SUPPORT` | GDB thread awareness |
| `CYGSEM_KERNEL_SCHED_TIMESLICE` | Enable round-robin time slicing |
| `CYGNUM_KERNEL_SCHED_TIMESLICE_TICKS` | Time slice duration in ticks |

---

## Gotchas & Pitfalls

!!! danger "cyg_thread_create Does NOT Start the Thread"
    Unlike most RTOSes, `cyg_thread_create()` creates the thread in suspended state. You MUST call `cyg_thread_resume()` to actually start it. Forgetting this means your thread never runs — a common source of confusion.

!!! danger "ISR Return Value — CYG_ISR_CALL_DSR"
    ISRs must return either `CYG_ISR_HANDLED` (processing complete, no DSR needed) or `CYG_ISR_CALL_DSR` (schedule DSR for deferred processing). Returning the wrong value will either miss processing or schedule unnecessary DSR calls.

!!! warning "cyg_interrupt_acknowledge — Must Be Called"
    On most hardware, you must call `cyg_interrupt_acknowledge(vector)` inside the ISR to clear the interrupt at the interrupt controller level. Forgetting this causes infinite interrupt re-entry.

!!! warning "eCos is No Longer Actively Maintained"
    The last eCos release was in 2009. There is no active upstream development. For new designs, consider FreeRTOS, Zephyr, or NuttX. eCos is appropriate for maintaining existing eCos-based products, not new development.

!!! tip "ecosconfig tree Must Be Re-Run After Config Changes"
    After editing `ecos.ecc` or using configtool, always run `ecosconfig tree` before `make`. The tree command regenerates all Makefiles and CDL headers. Skipping this step means your changes may not take effect.

!!! tip "Modified GPL Exception — Safe for Commercial Use"
    eCos's Modified GPL allows you to keep your application code proprietary as long as you publish any modifications to the eCos kernel itself. This is more permissive than standard GPL for application code.
