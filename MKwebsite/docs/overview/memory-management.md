# :material-memory: Memory Management

<div class="grid cards" markdown>
- :material-lightbulb-on: **Static Allocation** — allocate all buffers and stacks at compile time to eliminate heap fragmentation
- :material-chip: **Heap Scheme** — FreeRTOS offers five heap implementations; heap_4 is the best general-purpose choice
- :material-alert: **Stack Overflow** — undetected stack overflow corrupts adjacent memory silently; enable watermark checking
- :material-check-circle: **Use When** — use memory pools for fixed-size, frequently allocated objects to avoid fragmentation
</div>

---

## :material-scale-balance: Static vs Dynamic Allocation

### Static Allocation

All tasks, queues, semaphores, and buffers are declared as global or static variables. Memory is reserved at link time—no runtime allocation.

```c
/* Statically allocate a task */
static StaticTask_t xTaskBuffer;
static StackType_t  xStack[256];   /* 256 × 4 = 1 KB stack */

TaskHandle_t xHandle = xTaskCreateStatic(
    vMyTask,        /* function */
    "MyTask",       /* name */
    256,            /* stack depth in words */
    NULL,           /* parameters */
    3,              /* priority */
    xStack,
    &xTaskBuffer
);
```

**Advantages:** zero heap fragmentation, deterministic memory map, link-time detection of memory budget violations (linker error if RAM overflows).

### Dynamic Allocation

Tasks and RTOS objects are allocated from the FreeRTOS heap at runtime using `pvPortMalloc()` / `vPortFree()`.

```c
/* Dynamic allocation — simpler but uses heap */
xTaskCreate(vMyTask, "MyTask", 256, NULL, 3, NULL);
```

**Disadvantage:** heap fragmentation can cause allocation failures after extended runtime in systems that frequently create/delete tasks.

!!! tip "Best Practice for Safety-Critical Systems"
    Allocate all objects during `vTaskStartScheduler()` init, then never call `pvPortMalloc()` again at runtime. This gives static-like determinism with dynamic convenience.

---

## :material-layers: Heap Fragmentation

Fragmentation occurs when freed blocks are interspersed with allocated blocks, leaving no contiguous region large enough for a new allocation even when total free bytes are sufficient.

```
Initial:  [FREE: 10 KB total]
After A:  [A: 2 KB][FREE: 8 KB]
After B:  [A: 2 KB][B: 3 KB][FREE: 5 KB]
Free A:   [FREE: 2 KB][B: 3 KB][FREE: 5 KB]
Alloc 6K: FAILS — largest contiguous = 5 KB
```

Heap_4 and heap_5 coalesce adjacent free blocks to mitigate fragmentation, but cannot eliminate it for variable-size allocations.

---

## :material-compare: FreeRTOS Heap Schemes (heap_1 – heap_5)

| Scheme | Algorithm | `vPortFree()`? | Fragmentation | Best For |
|--------|-----------|---------------|---------------|----------|
| **heap_1** | Simple bump allocator | **No** | None (no free) | Safety-critical static-only systems |
| **heap_2** | Best-fit, no coalescing | Yes | High (no merge) | Legacy only (heap_4 is better) |
| **heap_3** | Wrapper for libc `malloc`/`free` | Yes | Library-defined | When libc heap is already present |
| **heap_4** | First-fit with coalescing | Yes | Low | **General purpose — recommended** |
| **heap_5** | heap_4 across multiple discontiguous memory regions | Yes | Low | Systems with internal SRAM + external PSRAM |

### Configuring heap_4

```c
/* FreeRTOSConfig.h */
#define configTOTAL_HEAP_SIZE   ( ( size_t ) ( 48 * 1024 ) )  /* 48 KB */

/* Query free heap at runtime */
size_t freeBytes = xPortGetFreeHeapSize();
size_t minEver   = xPortGetMinimumEverFreeHeapSize();  /* watermark */
```

### heap_5: Multiple Regions

```c
/* Define two memory regions: SRAM and CCMRAM */
HeapRegion_t xHeapRegions[] = {
    { (uint8_t *)0x20000000, 0xC000 },  /* 48 KB SRAM  */
    { (uint8_t *)0x10000000, 0x4000 },  /* 16 KB CCMRAM */
    { NULL, 0 }                          /* terminator  */
};
vPortDefineHeapRegions(xHeapRegions);
```

---

## :material-memory: Stack Sizing

Every task has its own private stack. Undersizing a stack causes overflow; oversizing wastes precious RAM.

### Stack Sizing Formula Guide

```
Stack needed ≈ (local variables in deepest call chain)
             + (each function frame: return address + saved registers ≈ 64–128 bytes on ARM)
             + (nested interrupt frame if task can be interrupted: ~128–200 bytes)
             + (safety margin: 20–30%)
```

| Task Complexity | Typical Stack (ARM Cortex-M) |
|----------------|------------------------------|
| Simple blinky, no printf | 128–256 words (512 B – 1 KB) |
| Moderate logic, no printf | 256–512 words |
| Uses `printf` / `sprintf` | 512–1024 words (2–4 KB) |
| Floating-point operations | Add 64 words for FPU context |
| Recursive or deep call chains | Profile carefully with watermark |

### Stack Overflow Detection

FreeRTOS provides two overflow check methods, set via `configCHECK_FOR_STACK_OVERFLOW`:

**Method 1** (`= 1`): At each context switch, check if the top-of-stack pointer is below the stack boundary. Fast but can miss overflows that don't move the pointer past the end.

**Method 2** (`= 2`): Fill the last 20 bytes of the stack with a known pattern (0xA5) at task creation. Check at each switch that the pattern is intact. Catches larger overflows.

```c
/* FreeRTOSConfig.h */
#define configCHECK_FOR_STACK_OVERFLOW   2

/* Application must define the hook */
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName) {
    /* Log the task name, then fail safe */
    log_critical("STACK OVERFLOW in task: %s", pcTaskName);
    taskDISABLE_INTERRUPTS();
    for (;;);   /* or trigger watchdog / safe state */
}
```

### Runtime Stack Watermark

```c
/* Check high-water mark (minimum ever free stack words) */
UBaseType_t uxHighWaterMark = uxTaskGetStackHighWaterMark(xTaskHandle);
/* If uxHighWaterMark < 20 words, stack is dangerously close to overflow */
```

---

## :material-package-variant-closed: Memory Pools

A **memory pool** (or fixed-size block allocator) pre-allocates a set of equal-sized blocks. Allocation and deallocation are O(1) and fragmentation-free.

```c
/* Simple memory pool using a queue of pointers */
#define POOL_SIZE   8
#define BLOCK_SIZE  64

static uint8_t pool_storage[POOL_SIZE][BLOCK_SIZE];
static QueueHandle_t xPool;

void pool_init(void) {
    xPool = xQueueCreate(POOL_SIZE, sizeof(void *));
    for (int i = 0; i < POOL_SIZE; i++) {
        void *p = pool_storage[i];
        xQueueSend(xPool, &p, 0);
    }
}

void *pool_alloc(TickType_t timeout) {
    void *p = NULL;
    xQueueReceive(xPool, &p, timeout);
    return p;
}

void pool_free(void *p) {
    xQueueSend(xPool, &p, 0);
}
```

---

## :material-shield-lock: MPU (Memory Protection Unit) in RTOS

The ARM Cortex-M MPU partitions memory into regions with configurable access permissions. FreeRTOS MPU port creates **unprivileged tasks** that can only access their own stack and designated data regions—kernel memory is protected from runaway tasks.

```
MPU Region Layout (example):
┌─────────────────────────────────┐ 0x20010000
│  Kernel + privileged task data  │  PRIV R/W
├─────────────────────────────────┤ 0x2000C000
│  Task A stack                   │  Task A R/W only
├─────────────────────────────────┤ 0x20008000
│  Task B stack                   │  Task B R/W only
├─────────────────────────────────┤ 0x20004000
│  Shared read-only config        │  All tasks R only
└─────────────────────────────────┘ 0x20000000
```

A task that writes outside its MPU region triggers a **MemManage fault**, isolating the fault to that task rather than corrupting the entire system.

```c
/* FreeRTOSConfig.h — enable MPU port */
#define configUSE_MPU_WRAPPERS_V1    0   /* use V2 wrappers */
#define configENABLE_MPU             1
```

---

## :material-help-circle: Flashcards

???+ question "What is the difference between heap_1 and heap_4 in FreeRTOS?"
    **heap_1** is a simple bump allocator that never frees memory. It is deterministic and fragmentation-free but unsuitable if objects are ever deleted. **heap_4** supports freeing and coalesces adjacent free blocks to reduce fragmentation. heap_4 is the recommended general-purpose choice; heap_1 is for safety-critical systems where memory is allocated once and never freed.

???+ question "What does `configCHECK_FOR_STACK_OVERFLOW 2` do?"
    Method 2 fills the last 20 bytes of each task's stack with a known canary pattern (0xA5A5A5A5) at creation time. At every context switch the kernel checks that the pattern is intact. If a task's stack has grown into that region, `vApplicationStackOverflowHook()` is called. This catches overflows that Method 1 would miss.

???+ question "Why should you prefer a memory pool over `pvPortMalloc` for frequently-allocated objects?"
    `pvPortMalloc` (heap_4) with variable-size allocations leads to **heap fragmentation** over time—free memory exists but in non-contiguous blocks too small for new allocations. A **memory pool** pre-allocates fixed-size blocks, so allocation and deallocation are O(1) and the total free space is always exactly the number of unallocated blocks × block size. No fragmentation can occur.

???+ question "What protection does the ARM MPU give in an RTOS application?"
    The MPU enforces per-region access permissions (read/write/execute/no access). In an RTOS MPU build, each unprivileged task can only access its own stack and permitted data regions. A task that overruns its stack or writes to kernel memory triggers a MemManage fault rather than silently corrupting another task's data.

---

## :material-clipboard-check: Self Test

=== "Question 1"
    Your system allocates and frees many 12-byte and 20-byte network packets at runtime. After 48 hours the system throws `pvPortMalloc` returning NULL, but `xPortGetFreeHeapSize()` shows 8 KB free. What is the cause and two possible solutions?

=== "Answer 1"
    The cause is **heap fragmentation**: 8 KB of free space is scattered in blocks smaller than the requested allocation. Two solutions:
    1. Use a **memory pool** with fixed block sizes (e.g., one 12-byte pool and one 20-byte pool) — no fragmentation possible.
    2. Switch to **heap_5** with `configTOTAL_HEAP_SIZE` tuned for peak utilization, and profile `xPortGetMinimumEverFreeHeapSize()` during testing to verify margin.

=== "Question 2"
    `uxTaskGetStackHighWaterMark()` returns 4 for a task with a 128-word stack. What does this mean and what should you do?

=== "Answer 2"
    The high-water mark of 4 means only 4 words (16 bytes) of stack space remained at the deepest point ever recorded. The task is close to stack overflow. You should **increase the stack size**—at minimum double it, then re-profile. Also enable `configCHECK_FOR_STACK_OVERFLOW 2` and check whether any code path calls a function (e.g., `printf`) that was not accounted for in the original size estimate.

---

## :material-check-circle: Summary

!!! success "Key Takeaways"
    - **Static allocation** (`xTaskCreateStatic`) eliminates heap fragmentation and makes memory use visible at link time—preferred for safety-critical systems.
    - **heap_4** is the recommended general-purpose FreeRTOS heap: supports `free()`, coalesces adjacent free blocks, and is thread-safe.
    - **heap_5** extends heap_4 across multiple discontiguous memory regions (e.g., SRAM + CCMRAM on STM32).
    - Enable **`configCHECK_FOR_STACK_OVERFLOW 2`** and always implement `vApplicationStackOverflowHook` to catch stack overflows before they corrupt other data.
    - Monitor `xPortGetMinimumEverFreeHeapSize()` and `uxTaskGetStackHighWaterMark()` during testing to right-size allocations.
    - Use **memory pools** for fixed-size, frequently-allocated objects to avoid fragmentation and achieve O(1) deterministic allocation.
    - The **MPU** confines runaway task memory accesses, turning silent corruption into a catchable MemManage fault.
