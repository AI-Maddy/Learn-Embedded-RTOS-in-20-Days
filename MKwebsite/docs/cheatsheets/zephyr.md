# :material-layers: Zephyr RTOS Cheatsheet

> **Dense quick-reference** for Zephyr 3.7.x. Threads, synchronization, message queues, work queues, devicetree macros, and west commands.

---

<div class="grid cards" markdown>

-   :material-cpu-64-bit: **Kernel**

    ---
    Preemptive + cooperative. Priority -16 (highest) to 14 (lowest) for preemptive; 15 for cooperative. Tickless by default.

-   :material-folder-cog: **Build System**

    ---
    CMake + west. Board-specific configs via Kconfig (`prj.conf`) and Devicetree (`.dts`/`.overlay`).

-   :material-github: **Source**

    ---
    [github.com/zephyrproject-rtos/zephyr](https://github.com/zephyrproject-rtos/zephyr) — Apache 2.0

-   :material-chip: **Supported Boards**

    ---
    600+ boards. nRF52/53/91, STM32, ESP32, MIMXRT, nRF7002, RP2040, and more.

</div>

---

## Thread API

| Function / Macro | Signature | Description |
|------------------|-----------|-------------|
| `K_THREAD_DEFINE` | `K_THREAD_DEFINE(name, stack_size, entry, p1, p2, p3, prio, options, delay)` | Statically define and auto-start thread at boot |
| `K_THREAD_STACK_DEFINE` | `K_THREAD_STACK_DEFINE(name, size)` | Statically define thread stack buffer |
| `k_thread_create` | `k_tid_t k_thread_create(struct k_thread *new_thread, k_thread_stack_t *stack, size_t stack_size, k_thread_entry_t entry, void *p1, void *p2, void *p3, int prio, uint32_t options, k_timeout_t delay)` | Create thread with pre-allocated stack |
| `k_thread_start` | `void k_thread_start(k_tid_t thread)` | Start thread created with `K_INIT_DELAY_FOREVER` |
| `k_thread_suspend` | `void k_thread_suspend(k_tid_t thread)` | Suspend thread indefinitely |
| `k_thread_resume` | `void k_thread_resume(k_tid_t thread)` | Resume suspended thread |
| `k_thread_abort` | `void k_thread_abort(k_tid_t thread)` | Abort and terminate thread |
| `k_thread_join` | `int k_thread_join(struct k_thread *thread, k_timeout_t timeout)` | Wait for thread termination |
| `k_sleep` | `int32_t k_sleep(k_timeout_t timeout)` | Sleep for duration |
| `k_msleep` | `int32_t k_msleep(int32_t ms)` | Sleep for milliseconds |
| `k_usleep` | `int32_t k_usleep(int32_t us)` | Sleep for microseconds |
| `k_yield` | `void k_yield(void)` | Yield to equal/higher priority thread |
| `k_thread_priority_set` | `void k_thread_priority_set(k_tid_t thread, int prio)` | Change thread priority at runtime |
| `k_current_get` | `k_tid_t k_current_get(void)` | Get current thread ID |

```c title="Thread Pattern — Static Definition"
#include <zephyr/kernel.h>

void my_thread_entry(void *p1, void *p2, void *p3) {
    ARG_UNUSED(p1); ARG_UNUSED(p2); ARG_UNUSED(p3);
    while (1) {
        do_work();
        k_msleep(100);
    }
}

// Statically define thread — auto-starts at boot, priority 5
K_THREAD_DEFINE(my_thread, 1024, my_thread_entry,
                NULL, NULL, NULL, 5, 0, 0);
```

```c title="Thread Pattern — Dynamic Creation"
K_THREAD_STACK_DEFINE(worker_stack, 2048);
struct k_thread worker_thread;

k_tid_t tid = k_thread_create(&worker_thread, worker_stack,
                               K_THREAD_STACK_SIZEOF(worker_stack),
                               my_thread_entry,
                               NULL, NULL, NULL,
                               5,           // priority
                               0,           // options
                               K_NO_WAIT);  // start immediately
k_thread_name_set(tid, "worker");
```

---

## Semaphore API

| Function / Macro | Signature | Description |
|------------------|-----------|-------------|
| `K_SEM_DEFINE` | `K_SEM_DEFINE(name, initial_count, limit)` | Statically define and initialize semaphore |
| `k_sem_init` | `int k_sem_init(struct k_sem *sem, unsigned int initial_count, unsigned int limit)` | Runtime initialize semaphore |
| `k_sem_take` | `int k_sem_take(struct k_sem *sem, k_timeout_t timeout)` | Acquire; returns 0 on success, -EAGAIN on timeout |
| `k_sem_give` | `void k_sem_give(struct k_sem *sem)` | Release semaphore (ISR-safe) |
| `k_sem_count_get` | `unsigned int k_sem_count_get(struct k_sem *sem)` | Get current count |
| `k_sem_reset` | `void k_sem_reset(struct k_sem *sem)` | Reset count to 0 |

```c title="Semaphore Pattern"
K_SEM_DEFINE(data_ready_sem, 0, 1);  // binary semaphore

// ISR or producer task
k_sem_give(&data_ready_sem);

// Consumer task
if (k_sem_take(&data_ready_sem, K_MSEC(500)) == 0) {
    process_data();
} else {
    LOG_WRN("Timeout waiting for data");
}
```

---

## Mutex API

| Function / Macro | Signature | Description |
|------------------|-----------|-------------|
| `K_MUTEX_DEFINE` | `K_MUTEX_DEFINE(name)` | Statically define and initialize mutex |
| `k_mutex_init` | `int k_mutex_init(struct k_mutex *mutex)` | Runtime initialize mutex |
| `k_mutex_lock` | `int k_mutex_lock(struct k_mutex *mutex, k_timeout_t timeout)` | Lock; implements priority inheritance |
| `k_mutex_unlock` | `int k_mutex_unlock(struct k_mutex *mutex)` | Unlock (must be called by locking thread) |

```c title="Mutex Pattern — Shared Resource"
K_MUTEX_DEFINE(spi_mutex);

void task_a_access_spi(void) {
    k_mutex_lock(&spi_mutex, K_FOREVER);
    spi_transceive(...);
    k_mutex_unlock(&spi_mutex);
}
```

---

## Message Queue API

| Function / Macro | Signature | Description |
|------------------|-----------|-------------|
| `K_MSGQ_DEFINE` | `K_MSGQ_DEFINE(name, msg_size, max_msgs, align)` | Statically define message queue |
| `k_msgq_init` | `void k_msgq_init(struct k_msgq *msgq, char *buffer, size_t msg_size, uint32_t max_msgs)` | Runtime initialize with provided buffer |
| `k_msgq_put` | `int k_msgq_put(struct k_msgq *msgq, const void *data, k_timeout_t timeout)` | Send message; blocks if full |
| `k_msgq_get` | `int k_msgq_get(struct k_msgq *msgq, void *data, k_timeout_t timeout)` | Receive message; blocks if empty |
| `k_msgq_peek` | `int k_msgq_peek(struct k_msgq *msgq, void *data)` | Read without removing |
| `k_msgq_purge` | `void k_msgq_purge(struct k_msgq *msgq)` | Flush all pending messages |
| `k_msgq_num_used_get` | `uint32_t k_msgq_num_used_get(struct k_msgq *msgq)` | Number of queued messages |
| `k_msgq_num_free_get` | `uint32_t k_msgq_num_free_get(struct k_msgq *msgq)` | Remaining capacity |

```c title="Message Queue Pattern"
typedef struct { uint16_t adc_val; uint32_t timestamp; } Sample_t;

K_MSGQ_DEFINE(sample_queue, sizeof(Sample_t), 16, 4);

// Producer
Sample_t s = { .adc_val = adc_read(), .timestamp = k_uptime_get_32() };
k_msgq_put(&sample_queue, &s, K_NO_WAIT);  // non-blocking; drop if full

// Consumer
Sample_t received;
k_msgq_get(&sample_queue, &received, K_FOREVER);
```

---

## Work Queue API

Work queues allow deferring processing from ISR context to a thread context. The system work queue (`k_sys_work_q`) is built-in.

| Function / Macro | Signature | Description |
|------------------|-----------|-------------|
| `k_work_init` | `void k_work_init(struct k_work *work, k_work_handler_t handler)` | Initialize work item |
| `k_work_submit` | `int k_work_submit(struct k_work *work)` | Submit to system work queue (ISR-safe) |
| `k_work_submit_to_queue` | `int k_work_submit_to_queue(struct k_work_q *work_q, struct k_work *work)` | Submit to specific work queue |
| `K_WORK_DEFINE` | `K_WORK_DEFINE(name, handler)` | Statically define work item |
| `k_work_init_delayable` | `void k_work_init_delayable(struct k_work_delayable *dwork, k_work_handler_t handler)` | Initialize delayable work |
| `k_work_schedule` | `int k_work_schedule(struct k_work_delayable *dwork, k_timeout_t delay)` | Schedule delayable work after delay |
| `k_work_reschedule` | `int k_work_reschedule(struct k_work_delayable *dwork, k_timeout_t delay)` | Reschedule (reset timer) |
| `k_work_cancel_delayable` | `int k_work_cancel_delayable(struct k_work_delayable *dwork)` | Cancel pending delayable work |
| `k_work_is_pending` | `bool k_work_is_pending(const struct k_work *work)` | Check if work is queued |

```c title="Work Queue Pattern — ISR Deferred Processing"
static struct k_work uart_work;

void uart_work_handler(struct k_work *work) {
    // Runs in system work queue thread context (not ISR)
    process_uart_buffer();
}

void uart_isr(const struct device *dev, void *user_data) {
    // Fast ISR — just queue the work
    k_work_submit(&uart_work);
}

// In init:
k_work_init(&uart_work, uart_work_handler);
uart_irq_callback_set(uart_dev, uart_isr, NULL);
```

---

## Devicetree Macros

| Macro | Description | Example |
|-------|-------------|---------|
| `DT_ALIAS(alias)` | Node reference by alias | `DT_ALIAS(led0)` |
| `DT_NODELABEL(label)` | Node reference by label | `DT_NODELABEL(uart0)` |
| `DT_PATH(...)` | Node reference by path | `DT_PATH(soc, uart_40002000)` |
| `GPIO_DT_SPEC_GET(node, prop)` | Get GPIO spec from DT | `GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios)` |
| `GPIO_DT_SPEC_GET_BY_IDX` | Get GPIO by index | `GPIO_DT_SPEC_GET_BY_IDX(node, prop, idx)` |
| `DT_PROP(node, prop)` | Read DT property | `DT_PROP(DT_ALIAS(uart0), current-speed)` |
| `DT_NODE_HAS_STATUS(node, status)` | Check if node is `okay` | `DT_NODE_HAS_STATUS(DT_ALIAS(led0), okay)` |
| `DEVICE_DT_GET(node)` | Get device pointer from DT | `DEVICE_DT_GET(DT_NODELABEL(uart0))` |
| `DT_INST_FOREACH_STATUS_OKAY` | Iterate over driver instances | Used in driver implementations |

```c title="Devicetree Pattern — GPIO LED"
#include <zephyr/drivers/gpio.h>

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);

int init_led(void) {
    if (!gpio_is_ready_dt(&led)) return -ENODEV;
    return gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
}

void toggle_led(void) {
    gpio_pin_toggle_dt(&led);
}
```

---

## west Build Commands Reference

| Command | Description |
|---------|-------------|
| `west init -m <url>` | Initialize workspace from manifest URL |
| `west update` | Fetch/update all modules in manifest |
| `west build -b <board> .` | Build for target board |
| `west build -b <board> . -- -DCONF_FILE=my.conf` | Build with custom config |
| `west build --pristine` | Clean build (delete build directory first) |
| `west flash` | Flash to connected board (auto-detects runner) |
| `west flash --runner jlink` | Flash via J-Link |
| `west flash --runner openocd` | Flash via OpenOCD |
| `west debug` | Start GDB debug session |
| `west attach` | Attach GDB to running target |
| `west espressif monitor` | Serial monitor for ESP32 targets |
| `west boards` | List all supported boards |
| `west boards -n stm32` | Filter boards by name |
| `west build -t menuconfig` | Interactive Kconfig menu |
| `west build -t guiconfig` | GUI Kconfig (requires tkinter) |
| `west build -t boards` | Show board info |
| `west sign -t imgtool` | Sign firmware (MCUboot) |
| `west zephyr-export` | Export CMake package for IDE |
| `west blobs fetch` | Download binary blobs (WiFi firmware, etc.) |

```bash title="Full Build + Flash Workflow"
# Setup (once)
pip install west
west init ~/zephyr-workspace -m https://github.com/zephyrproject-rtos/zephyr
cd ~/zephyr-workspace && west update
pip install -r zephyr/scripts/requirements.txt
west sdk install

# Per project
cd my_app
west build -b nrf52840dk/nrf52840 .
west flash
west build -t menuconfig   # tune options
```

---

## Kconfig (prj.conf) Key Options

| Option | Value | Description |
|--------|-------|-------------|
| `CONFIG_HEAP_MEM_POOL_SIZE` | `4096` | Kernel heap size in bytes |
| `CONFIG_MAIN_STACK_SIZE` | `2048` | main() thread stack size |
| `CONFIG_SYSTEM_WORKQUEUE_STACK_SIZE` | `2048` | System work queue stack |
| `CONFIG_LOG` | `y` | Enable logging subsystem |
| `CONFIG_LOG_DEFAULT_LEVEL` | `3` | 0=off,1=err,2=warn,3=inf,4=dbg |
| `CONFIG_UART_CONSOLE` | `y` | Console over UART |
| `CONFIG_USB_DEVICE_STACK` | `y` | Enable USB device |
| `CONFIG_BT` | `y` | Enable Bluetooth |
| `CONFIG_NET_TCP` | `y` | Enable TCP/IP |
| `CONFIG_CBPRINTF_FP_SUPPORT` | `y` | Float support in printk |
| `CONFIG_ASSERT` | `y` | Enable assertions (debug) |
| `CONFIG_STACK_SENTINEL` | `y` | Stack overflow sentinel |
| `CONFIG_THREAD_ANALYZER` | `y` | Thread stack usage reporting |
| `CONFIG_TIMING_FUNCTIONS` | `y` | High-res timing APIs |

---

## Gotchas & Pitfalls

!!! danger "ISR Context — Never Call Blocking APIs"
    Functions like `k_sem_take(K_FOREVER)`, `k_msgq_get(K_FOREVER)`, `k_msleep()` will panic if called from ISR context. From ISR, only use ISR-safe variants and `K_NO_WAIT` timeouts.

!!! danger "k_mutex_unlock — Must Be Called by Locking Thread"
    Only the thread that called `k_mutex_lock()` can call `k_mutex_unlock()`. Unlocking from a different thread is undefined behavior. Use a semaphore if you need cross-thread signaling without ownership.

!!! warning "K_THREAD_DEFINE Stack Size — Minimum is Board-Dependent"
    Stack sizes smaller than ~256 bytes will cause stack overflow on Cortex-M. Use `CONFIG_THREAD_ANALYZER=y` and `k_thread_runtime_stats_get()` to measure actual stack usage.

!!! warning "k_msgq_put with K_NO_WAIT — Silent Drop"
    If the queue is full and you use `K_NO_WAIT`, the message is silently dropped and `-ENOMSG` is returned. Always check the return value or use a sufficiently large queue depth.

!!! tip "Use Logging Instead of printk in Production"
    `printk()` is synchronous and blocks. Use the Zephyr logging subsystem (`LOG_INF`, `LOG_ERR`, etc.) with deferred logging (`CONFIG_LOG_MODE_DEFERRED`) for non-blocking output in production code.

!!! tip "Devicetree Overlays for Board Customization"
    Create a `boards/<board>.overlay` file in your app directory to override DTS properties without modifying board files. This is the correct way to remap pins, change UART settings, or add custom peripherals.
