# :material-penguin: NuttX Cheatsheet

> **Dense quick-reference** for NuttX 12.x. POSIX threads, mutexes, semaphores, message queues, signals, NSH commands, and NuttX-specific extensions.

---

<div class="grid cards" markdown>

-   :material-cpu-64-bit: **Kernel**

    ---
    Preemptive priority-based. Full POSIX.1-2017 compliance. 256 priority levels. Separate process and thread models. Virtual filesystem (VFS), BSD sockets.

-   :material-memory: **Footprint**

    ---
    Min ~32 KB RAM, ~64 KB Flash for POSIX subsystem. Smaller configurations exist without full POSIX. Larger than bare-metal RTOSes but enables Linux app porting.

-   :material-github: **Source**

    ---
    [github.com/apache/nuttx](https://github.com/apache/nuttx) — Apache 2.0

-   :material-sticker-check: **Best For**

    ---
    Porting Linux/POSIX applications to constrained hardware. PX4 autopilot, Espressif ESP-IDF devices, and NASA small spacecraft.

</div>

---

## POSIX Threads (pthread)

| Function | Signature | Description |
|----------|-----------|-------------|
| `pthread_create` | `int pthread_create(pthread_t *thread, const pthread_attr_t *attr, void *(*start_routine)(void *), void *arg)` | Create and start thread |
| `pthread_join` | `int pthread_join(pthread_t thread, void **retval)` | Wait for thread completion |
| `pthread_exit` | `void pthread_exit(void *retval)` | Terminate calling thread |
| `pthread_cancel` | `int pthread_cancel(pthread_t thread)` | Request thread cancellation |
| `pthread_detach` | `int pthread_detach(pthread_t thread)` | Detach thread (auto-cleanup on exit) |
| `pthread_self` | `pthread_t pthread_self(void)` | Get current thread ID |
| `pthread_equal` | `int pthread_equal(pthread_t t1, pthread_t t2)` | Compare thread IDs |
| `pthread_attr_init` | `int pthread_attr_init(pthread_attr_t *attr)` | Initialize thread attributes |
| `pthread_attr_destroy` | `int pthread_attr_destroy(pthread_attr_t *attr)` | Destroy thread attributes |
| `pthread_attr_setstacksize` | `int pthread_attr_setstacksize(pthread_attr_t *attr, size_t stacksize)` | Set stack size |
| `pthread_attr_setschedparam` | `int pthread_attr_setschedparam(pthread_attr_t *attr, const struct sched_param *param)` | Set scheduling priority |
| `pthread_attr_setschedpolicy` | `int pthread_attr_setschedpolicy(pthread_attr_t *attr, int policy)` | Set `SCHED_FIFO`, `SCHED_RR`, or `SCHED_OTHER` |
| `pthread_setschedparam` | `int pthread_setschedparam(pthread_t thread, int policy, const struct sched_param *param)` | Change running thread priority |
| `pthread_getschedparam` | `int pthread_getschedparam(pthread_t thread, int *policy, struct sched_param *param)` | Get thread priority |

```c title="POSIX Thread Pattern — Real-Time Priority"
#include <pthread.h>
#include <sched.h>

void *sensor_thread(void *arg) {
    struct timespec ts;
    while (1) {
        read_sensor();
        // Sleep 100ms
        ts.tv_sec = 0;
        ts.tv_nsec = 100 * 1000000L;
        nanosleep(&ts, NULL);
    }
    return NULL;
}

int start_sensor_thread(void) {
    pthread_t tid;
    pthread_attr_t attr;
    struct sched_param param;

    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, 4096);
    param.sched_priority = 100;  // NuttX: 1–255
    pthread_attr_setschedparam(&attr, &param);
    pthread_attr_setschedpolicy(&attr, SCHED_FIFO);

    int ret = pthread_create(&tid, &attr, sensor_thread, NULL);
    pthread_attr_destroy(&attr);
    return ret;
}
```

---

## POSIX Mutex (pthread_mutex)

| Function | Signature | Description |
|----------|-----------|-------------|
| `pthread_mutex_init` | `int pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *attr)` | Initialize mutex |
| `pthread_mutex_destroy` | `int pthread_mutex_destroy(pthread_mutex_t *mutex)` | Destroy mutex |
| `pthread_mutex_lock` | `int pthread_mutex_lock(pthread_mutex_t *mutex)` | Lock (blocking) |
| `pthread_mutex_trylock` | `int pthread_mutex_trylock(pthread_mutex_t *mutex)` | Non-blocking lock attempt |
| `pthread_mutex_timedlock` | `int pthread_mutex_timedlock(pthread_mutex_t *mutex, const struct timespec *abstime)` | Lock with absolute timeout |
| `pthread_mutex_unlock` | `int pthread_mutex_unlock(pthread_mutex_t *mutex)` | Unlock |
| `pthread_mutexattr_settype` | `int pthread_mutexattr_settype(pthread_mutexattr_t *attr, int type)` | Set type: `PTHREAD_MUTEX_NORMAL`, `PTHREAD_MUTEX_RECURSIVE`, `PTHREAD_MUTEX_ERRORCHECK` |
| `pthread_mutexattr_setprotocol` | `int pthread_mutexattr_setprotocol(pthread_mutexattr_t *attr, int protocol)` | Set `PTHREAD_PRIO_INHERIT` for priority inheritance |

```c title="Mutex Pattern — Priority Inheritance"
pthread_mutex_t spi_mutex;
pthread_mutexattr_t attr;

pthread_mutexattr_init(&attr);
pthread_mutexattr_setprotocol(&attr, PTHREAD_PRIO_INHERIT);
pthread_mutex_init(&spi_mutex, &attr);
pthread_mutexattr_destroy(&attr);

// Usage:
pthread_mutex_lock(&spi_mutex);
spi_transfer();
pthread_mutex_unlock(&spi_mutex);
```

---

## POSIX Semaphore (sem)

| Function | Signature | Description |
|----------|-----------|-------------|
| `sem_init` | `int sem_init(sem_t *sem, int pshared, unsigned int value)` | Initialize semaphore (pshared=0 for threads) |
| `sem_destroy` | `int sem_destroy(sem_t *sem)` | Destroy semaphore |
| `sem_wait` | `int sem_wait(sem_t *sem)` | Decrement (block if 0) |
| `sem_trywait` | `int sem_trywait(sem_t *sem)` | Non-blocking decrement; returns `EAGAIN` if 0 |
| `sem_timedwait` | `int sem_timedwait(sem_t *sem, const struct timespec *abstime)` | Wait with absolute timeout |
| `sem_post` | `int sem_post(sem_t *sem)` | Increment (ISR-safe in NuttX) |
| `sem_getvalue` | `int sem_getvalue(sem_t *sem, int *sval)` | Read current count |
| `sem_open` | `sem_t *sem_open(const char *name, int oflag, ...)` | Open named semaphore |
| `sem_close` | `int sem_close(sem_t *sem)` | Close named semaphore |
| `sem_unlink` | `int sem_unlink(const char *name)` | Remove named semaphore |

```c title="Semaphore Pattern"
sem_t data_ready;
sem_init(&data_ready, 0, 0);  // unnamed, threads only, initial value 0

// ISR or producer:
sem_post(&data_ready);  // NuttX sem_post is ISR-safe

// Consumer:
struct timespec timeout;
clock_gettime(CLOCK_REALTIME, &timeout);
timeout.tv_sec += 1;  // 1 second from now
if (sem_timedwait(&data_ready, &timeout) == 0) {
    process_data();
} else {
    // errno == ETIMEDOUT
}
```

---

## POSIX Message Queue (mq)

| Function | Signature | Description |
|----------|-----------|-------------|
| `mq_open` | `mqd_t mq_open(const char *name, int oflag, ...)` | Open/create named message queue |
| `mq_close` | `int mq_close(mqd_t mqdes)` | Close message queue descriptor |
| `mq_unlink` | `int mq_unlink(const char *name)` | Remove named message queue |
| `mq_send` | `int mq_send(mqd_t mqdes, const char *msg_ptr, size_t msg_len, unsigned int msg_prio)` | Send message (blocks if full in blocking mode) |
| `mq_timedsend` | `int mq_timedsend(mqd_t mqdes, const char *msg_ptr, size_t msg_len, unsigned int msg_prio, const struct timespec *abstime)` | Send with timeout |
| `mq_receive` | `ssize_t mq_receive(mqd_t mqdes, char *msg_ptr, size_t msg_len, unsigned int *msg_prio)` | Receive highest-priority message |
| `mq_timedreceive` | `ssize_t mq_timedreceive(mqd_t mqdes, char *msg_ptr, size_t msg_len, unsigned int *msg_prio, const struct timespec *abstime)` | Receive with timeout |
| `mq_notify` | `int mq_notify(mqd_t mqdes, const struct sigevent *sevp)` | Register async notification on message arrival |
| `mq_getattr` | `int mq_getattr(mqd_t mqdes, struct mq_attr *mqstat)` | Get queue attributes |
| `mq_setattr` | `int mq_setattr(mqd_t mqdes, const struct mq_attr *mqstat, struct mq_attr *omqstat)` | Set queue attributes (e.g., O_NONBLOCK) |

```c title="Message Queue Pattern"
#include <mqueue.h>
#include <fcntl.h>

// Queue attributes
struct mq_attr attr = {
    .mq_flags   = 0,
    .mq_maxmsg  = 10,
    .mq_msgsize = 64,
    .mq_curmsgs = 0
};

// Create queue (producer)
mqd_t mqd = mq_open("/sensor_data", O_CREAT | O_WRONLY, 0644, &attr);
char msg[64] = "sensor:42.5";
mq_send(mqd, msg, strlen(msg) + 1, 0);

// Open for reading (consumer)
mqd_t rqd = mq_open("/sensor_data", O_RDONLY);
char buf[64];
unsigned prio;
ssize_t n = mq_receive(rqd, buf, sizeof(buf), &prio);
if (n > 0) handle_message(buf);

// Cleanup
mq_close(mqd);
mq_close(rqd);
mq_unlink("/sensor_data");
```

---

## Signals

| Function | Signature | Description |
|----------|-----------|-------------|
| `sigaction` | `int sigaction(int signum, const struct sigaction *act, struct sigaction *oldact)` | Set signal handler |
| `kill` | `int kill(pid_t pid, int sig)` | Send signal to process/task |
| `pthread_kill` | `int pthread_kill(pthread_t thread, int sig)` | Send signal to thread |
| `sigwaitinfo` | `int sigwaitinfo(const sigset_t *set, siginfo_t *info)` | Synchronously wait for signal |
| `sigtimedwait` | `int sigtimedwait(const sigset_t *set, siginfo_t *info, const struct timespec *timeout)` | Wait for signal with timeout |
| `sigprocmask` | `int sigprocmask(int how, const sigset_t *set, sigset_t *oldset)` | Block/unblock signals |
| `sigemptyset` | `int sigemptyset(sigset_t *set)` | Initialize empty signal set |
| `sigaddset` | `int sigaddset(sigset_t *set, int signo)` | Add signal to set |

```c title="Signals Pattern — Synchronous Wait"
#include <signal.h>

// Thread waits synchronously for SIGUSR1
void *wait_thread(void *arg) {
    sigset_t set;
    siginfo_t info;
    sigemptyset(&set);
    sigaddset(&set, SIGUSR1);

    // Block SIGUSR1 from async delivery (we want synchronous wait)
    pthread_sigmask(SIG_BLOCK, &set, NULL);

    // Wait for signal
    sigwaitinfo(&set, &info);
    printf("Got SIGUSR1 from pid %d\n", info.si_pid);
    return NULL;
}

// Sender:
pthread_kill(target_tid, SIGUSR1);
```

---

## NSH (NuttX Shell) Commands Reference

| Command | Description |
|---------|-------------|
| `help` | List all NSH commands |
| `ps` | Show all tasks/threads with PID, priority, state, stack |
| `free` | Show heap memory usage |
| `ls [path]` | List filesystem contents |
| `cat <file>` | Print file contents |
| `mount -t <fstype> <dev> <mountpoint>` | Mount filesystem |
| `umount <mountpoint>` | Unmount filesystem |
| `dd if=<src> of=<dst> bs=<n> count=<n>` | Copy data blocks |
| `hexdump <file>` | Hex dump file |
| `echo <text>` | Print text |
| `sleep <seconds>` | Sleep |
| `ifconfig` | Show/configure network interfaces |
| `ping <host>` | Ping network host |
| `nslookup <hostname>` | DNS lookup |
| `wget <url>` | HTTP download |
| `i2cget -b <bus> -a <addr> -r <reg>` | Read I2C register |
| `i2cset -b <bus> -a <addr> -r <reg> <val>` | Write I2C register |
| `spi <bus> <word>` | SPI transfer |
| `adc <channel>` | Read ADC channel |
| `gpio -p <pin> -s <value>` | Set GPIO |
| `perf reset / perf start / perf stop / perf print` | Performance counters |
| `irqinfo` | Show interrupt statistics |
| `dmesg` | Show kernel log |
| `reboot` | Reboot system |

---

## NuttX-Specific Extensions

| Function / Attribute | Description |
|----------------------|-------------|
| `up_enable_irq(irq)` | Enable IRQ number in interrupt controller |
| `up_disable_irq(irq)` | Disable IRQ number |
| `up_prioritize_irq(irq, priority)` | Set IRQ priority |
| `IRAM_ATTR` | Place function in IRAM (ESP32 specific) — `int IRAM_ATTR fast_isr(void)` |
| `DRAM_ATTR` | Place data in DRAM (not flash) |
| `CONFIG_SCHED_CPULOAD` | Enable CPU load measurement |
| `clock_systime_ticks()` | Get system tick count |
| `clock_systime_timespec()` | Get system time as timespec |
| `nxsched_get_tcb(pid)` | Get Task Control Block by PID |
| `task_create(name, prio, stack, main, argv)` | NuttX task_create (higher-level than pthread) |
| `task_delete(pid)` | Delete task by PID |
| `BOARD_LOOPSPERMSEC` | Calibration constant for delay loops |

```c title="NuttX Task Create (Alternative to pthread)"
#include <nuttx/sched.h>

// NuttX-specific task creation (supports argc/argv like main())
int pid = task_create("MySensor",           // name
                      100,                  // priority (1-255)
                      4096,                 // stack size
                      sensor_main,          // entry: int sensor_main(int argc, char *argv[])
                      NULL);                // argv (NULL = no args)

if (pid < 0) {
    fprintf(stderr, "task_create failed: %d\n", errno);
}
```

---

## NuttX Build System

```bash title="NuttX Build Workflow"
# Configure for target board
./tools/configure.sh esp32-devkitc:nsh  # board:config

# Interactive configuration
make menuconfig

# Build
make -j$(nproc)

# Flash (board-specific)
make flash ESPTOOL_PORT=/dev/ttyUSB0  # ESP32 example

# Connect to NSH
minicom -D /dev/ttyUSB0 -b 115200

# Common configs to enable:
# CONFIG_NSH_BUILTIN_APPS=y      -- apps accessible from NSH
# CONFIG_SYSTEM_NSH=y            -- enable NSH
# CONFIG_EXAMPLES_HELLO=y        -- hello world example
# CONFIG_PTHREAD_STACK_DEFAULT=2048
```

---

## Gotchas & Pitfalls

!!! danger "sem_post in ISR — Only for Named/Unnamed sem_t, Not POSIX MQ"
    NuttX `sem_post()` is safe to call from ISR context for `sem_t` semaphores. However, `mq_send()` is NOT ISR-safe — it may block if the queue is full. Use semaphores for ISR-to-task signaling.

!!! danger "SCHED_RR Without Priority Differentiation Wastes CPU"
    If all threads have the same priority and use `SCHED_RR`, time is wasted on context switches between threads that have no work. Use `SCHED_FIFO` for real-time threads and only apply `SCHED_RR` for background threads of equal importance.

!!! warning "mq_open — Named Queues Persist"
    Named message queues created with `mq_open(O_CREAT)` persist until explicitly unlinked with `mq_unlink()`. If your application crashes and restarts, the old queue still exists. Always call `mq_unlink()` at startup before `mq_open()` or handle `EEXIST` gracefully.

!!! warning "pthread Priority — NuttX Scale (1–255)"
    NuttX uses priority range 1–255 (255 = highest), opposite to some POSIX systems. Verify the priority scale on your NuttX configuration — `SCHED_PRIORITY_MAX` and `SCHED_PRIORITY_MIN` give the actual range.

!!! tip "Use task_create for Top-Level Tasks, pthread for Worker Threads"
    `task_create()` creates a NuttX task with its own address space (on MMU-enabled platforms) and command-line argument support. Use it for top-level application modules. Use `pthread_create()` for worker threads that share memory with the parent task.

!!! tip "NSH as Your Debug Console"
    NSH `ps` command shows all tasks with real-time stack usage, priority, and state. `free` shows heap fragmentation. These are invaluable for debugging memory and scheduling issues without a full debug probe.
