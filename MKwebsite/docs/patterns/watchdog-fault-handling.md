# :material-shield-alert: Watchdog & Fault Handling

<div class="grid cards" markdown>
- :material-lightbulb-on: **Hardware Watchdog** — an independent timer that resets the MCU if the firmware fails to "kick" it within the timeout window
- :material-chip: **Software Monitor** — a high-priority task that verifies all critical tasks are alive by checking their heartbeat counters
- :material-alert: **HardFault** — an unhandled Cortex-M exception; implement the handler to capture a crash dump before reset rather than spinning silently
- :material-check-circle: **Use When** — always enable the watchdog in production firmware; there is no reliable substitute for a hardware reset on runaway code
</div>

---

## :material-dog: Hardware Watchdog

### IWDG (Independent Watchdog) on STM32

The **IWDG** runs from a separate low-speed RC oscillator (LSI, ~32 kHz) and is independent of the main clock. Even if the main oscillator fails or the CPU hangs, the IWDG keeps running and resets the system.

```c
#include "stm32f4xx_hal.h"

static IWDG_HandleTypeDef hiwdg;

void watchdog_init(uint32_t timeout_ms) {
    /* LSI ≈ 32 kHz. Prescaler 64 → 500 Hz. Each count = 2 ms.
     * Reload = timeout_ms / 2 */
    hiwdg.Instance       = IWDG;
    hiwdg.Init.Prescaler = IWDG_PRESCALER_64;
    hiwdg.Init.Reload    = (timeout_ms / 2U) & 0x0FFF;  /* 12-bit max */
    HAL_IWDG_Init(&hiwdg);
}

/* Call this from the watchdog monitor task, not from each individual task */
void watchdog_kick(void) {
    HAL_IWDG_Refresh(&hiwdg);
}
```

### WWDG (Window Watchdog) on STM32

The **WWDG** adds a *window*: the kick must arrive within a specific time range. Kicking too early (before the window opens) also triggers a reset. Use WWDG when you need to detect both stuck tasks (too late) and runaway loops (too early).

```c
/* WWDG window: kick only valid between T_min and T_max */
/*   T_min (window upper bound) and T_max (counter reload) */
/* Reset if kick arrives before T_min or after T_max */
```

---

## :material-monitor-dashboard: Software Watchdog Pattern (Monitor Task)

A hardware watchdog alone cannot distinguish "system running" from "one critical task hung". A software monitor task checks each task's heartbeat and only kicks the hardware watchdog if all tasks are healthy.

```c
/* ---- Per-task health record ----------------------------------- */
#define MAX_MONITORED_TASKS  8

typedef struct {
    const char *name;
    uint32_t    last_heartbeat;   /* tick count of last check-in */
    uint32_t    timeout_ticks;    /* max allowed silence */
    bool        enabled;
} TaskHealth_t;

static TaskHealth_t health_table[MAX_MONITORED_TASKS];
static uint8_t      health_count = 0;

/* Register a task with the monitor */
uint8_t monitor_register(const char *name, uint32_t timeout_ms) {
    configASSERT(health_count < MAX_MONITORED_TASKS);
    uint8_t id = health_count++;
    health_table[id].name            = name;
    health_table[id].last_heartbeat  = xTaskGetTickCount();
    health_table[id].timeout_ticks   = pdMS_TO_TICKS(timeout_ms);
    health_table[id].enabled         = true;
    return id;
}

/* Each monitored task calls this periodically */
void monitor_heartbeat(uint8_t task_id) {
    health_table[task_id].last_heartbeat = xTaskGetTickCount();
}

/* ---- Monitor task --------------------------------------------- */
static void vWatchdogMonitor(void *pv) {
    const TickType_t xCheckInterval = pdMS_TO_TICKS(50);

    for (;;) {
        TickType_t now = xTaskGetTickCount();
        bool all_healthy = true;

        for (uint8_t i = 0; i < health_count; i++) {
            if (!health_table[i].enabled) continue;

            TickType_t age = now - health_table[i].last_heartbeat;
            if (age > health_table[i].timeout_ticks) {
                log_critical("Task '%s' missed heartbeat! age=%lu ms",
                             health_table[i].name,
                             age * portTICK_PERIOD_MS);
                all_healthy = false;
            }
        }

        if (all_healthy) {
            watchdog_kick();   /* kick hardware watchdog only if all OK */
        }
        /* If not healthy, hardware watchdog will expire and reset system */

        vTaskDelay(xCheckInterval);
    }
}
```

### Watchdog Architecture

```
┌──────────────────────────────────────────────────────────┐
│                     Application Tasks                    │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐               │
│  │ Task A   │  │ Task B   │  │ Task C   │               │
│  │heartbeat │  │heartbeat │  │heartbeat │               │
│  └────┬─────┘  └────┬─────┘  └────┬─────┘               │
│       │             │             │                      │
│       └─────────────┼─────────────┘                      │
│                     │ monitor_heartbeat(id)               │
│              ┌──────▼──────────────────────┐             │
│              │   Watchdog Monitor Task     │             │
│              │   (highest priority)        │             │
│              │   checks all heartbeats     │             │
│              └──────────────┬──────────────┘             │
│                             │ watchdog_kick() if all OK  │
└─────────────────────────────┼──────────────────────────┘
                              │
                    ┌─────────▼──────────┐
                    │  Hardware Watchdog │
                    │  (IWDG / WWDG)     │
                    │  resets MCU on     │
                    │  timeout           │
                    └────────────────────┘
```

---

## :material-alert-circle: Cortex-M Fault Handlers

ARM Cortex-M provides three configurable fault exceptions:

| Fault | SCB Register | Typical Cause |
|-------|-------------|---------------|
| **HardFault** | Always enabled | Escalated fault, undefined instruction, bad vector |
| **MemManage** | `SCB->SHCSR` bit 16 | MPU access violation, execute from non-exec region |
| **BusFault** | `SCB->SHCSR` bit 17 | Invalid memory access, AMBA bus error |
| **UsageFault** | `SCB->SHCSR` bit 18 | Undefined instruction, unaligned access, divide by zero |

### HardFault Handler with Crash Dump

```c
/* Cortex-M stack frame pushed automatically on exception entry */
typedef struct {
    uint32_t r0, r1, r2, r3;
    uint32_t r12;
    uint32_t lr;    /* Link Register at fault */
    uint32_t pc;    /* Program Counter at fault */
    uint32_t xpsr;  /* Program Status Register */
} ExceptionFrame_t;

/* Crash dump stored in non-initialised RAM (survives reset) */
typedef struct {
    uint32_t         magic;        /* 0xDEADBEEF if valid */
    ExceptionFrame_t frame;
    uint32_t         cfsr;         /* Configurable Fault Status Register */
    uint32_t         hfsr;         /* HardFault Status Register */
    uint32_t         mmfar;        /* MemManage Fault Address */
    uint32_t         bfar;         /* Bus Fault Address */
    uint32_t         active_task;  /* FreeRTOS task at fault time */
} CrashDump_t;

/* Place in non-initialised section to survive reset */
__attribute__((section(".noinit")))
static CrashDump_t crash_dump;

/* HardFault trampoline — extract stack frame pointer from SP */
__attribute__((naked))
void HardFault_Handler(void) {
    __asm volatile (
        "tst    lr, #4          \n"  /* check EXC_RETURN bit 2 */
        "ite    eq              \n"
        "mrseq  r0, msp         \n"  /* main stack pointer */
        "mrsne  r0, psp         \n"  /* process stack pointer */
        "b      HardFault_C_Handler \n"
        ::: "r0"
    );
}

void HardFault_C_Handler(ExceptionFrame_t *frame) {
    /* Capture fault registers */
    crash_dump.magic    = 0xDEADBEEFU;
    crash_dump.frame    = *frame;
    crash_dump.cfsr     = SCB->CFSR;
    crash_dump.hfsr     = SCB->HFSR;
    crash_dump.mmfar    = SCB->MMFAR;
    crash_dump.bfar     = SCB->BFAR;

    /* Try to capture active FreeRTOS task name */
    TaskHandle_t t = xTaskGetCurrentTaskHandle();
    if (t) {
        crash_dump.active_task = (uint32_t)(uintptr_t)pcTaskGetName(t);
    }

    /* Write crash dump to non-volatile storage if possible */
    flash_write_crash_dump(&crash_dump);

    /* Force system reset */
    NVIC_SystemReset();
}
```

### Reading the Crash Dump at Next Boot

```c
void boot_check_crash_dump(void) {
    if (crash_dump.magic == 0xDEADBEEFU) {
        log_error("=== Previous crash detected ===");
        log_error("PC=0x%08lX LR=0x%08lX", crash_dump.frame.pc, crash_dump.frame.lr);
        log_error("CFSR=0x%08lX HFSR=0x%08lX", crash_dump.cfsr, crash_dump.hfsr);
        if (crash_dump.cfsr & SCB_CFSR_IBUSERR_Msk)
            log_error("Instruction bus error at 0x%08lX", crash_dump.bfar);
        if (crash_dump.cfsr & SCB_CFSR_MMARVALID_Msk)
            log_error("MemManage fault at address 0x%08lX", crash_dump.mmfar);

        crash_dump.magic = 0;  /* clear so we don't log again next boot */
    }
}
```

---

## :material-shield-lock: Safe State Pattern

When a fault is detected, the system must enter a **safe state** appropriate to the application:

```c
void enter_safe_state(void) {
    /* 1. Disable all actuators immediately */
    motor_disable_all();
    valve_close_all();
    output_disable_all();

    /* 2. Turn on fault indicator */
    led_fault_on();

    /* 3. Log the event */
    log_critical("System entered safe state at tick %lu", xTaskGetTickCount());

    /* 4. Options:
     *    a) Wait for external reset (maintenance decision)
     *    b) Attempt controlled restart after delay
     *    c) Reset immediately via watchdog */

    /* Option a — wait for manual intervention */
    taskDISABLE_INTERRUPTS();
    for (;;);   /* watchdog will eventually reset if not kicked */
}
```

---

## :material-help-circle: Flashcards

???+ question "What is the difference between IWDG and WWDG on STM32?"
    The **IWDG** (Independent Watchdog) runs from the LSI oscillator and is fully independent of the main CPU clock. It resets the system if not refreshed within a timeout — it only detects "too late" kicks (hung/stuck code). The **WWDG** (Window Watchdog) adds a *window*: the refresh must arrive within a specific time window. A refresh that is too early (before the window opens) also triggers a reset — it detects both stuck code and runaway tight loops.

???+ question "Why should the watchdog monitor task run at the highest application priority?"
    The monitor task must be able to detect when lower-priority tasks have stopped heartbeating. If the monitor runs at low priority, a runaway high-priority task could starve it, preventing it from kicking the hardware watchdog and causing a false reset. At the highest priority, only an ISR can starve the monitor — and ISRs should be too short to cause a problem.

???+ question "What information should a HardFault handler capture before resetting?"
    At minimum: **PC** (program counter at fault — identifies faulting instruction), **LR** (link register — identifies calling function), **CFSR** (Configurable Fault Status Register — identifies fault type), **HFSR** (HardFault Status), **MMFAR/BFAR** (fault addresses for memory/bus faults), and the **active FreeRTOS task name**. Store these in non-initialised RAM (`.noinit` section) so they survive the reset and can be logged at next boot.

???+ question "What is the `.noinit` section and why is it used for crash dumps?"
    The `.noinit` section contains variables that the C startup code (`__startup`) explicitly does NOT zero-initialise. Normally, all global/static variables are zeroed at boot. By placing the crash dump struct in `.noinit`, its contents persist across a software reset (NVIC_SystemReset), allowing the next boot sequence to read and log the fault information before it is overwritten.

---

## :material-clipboard-check: Self Test

=== "Question 1"
    The IWDG timeout is set to 2 seconds. The watchdog monitor task runs every 50 ms, calls `watchdog_kick()` only if all tasks are healthy, and has a 200 ms heartbeat timeout for each task. Task B hangs. Trace the timeline from hang to reset.

=== "Answer 1"
    - **t=0**: Task B hangs (stops calling `monitor_heartbeat()`).
    - **t=200 ms**: Monitor task checks at t=200 ms; Task B's age = 200 ms = timeout → health check **fails**. `watchdog_kick()` is NOT called.
    - **t=200 ms to t=2000 ms**: Monitor continues checking every 50 ms; Task B still hung; watchdog never kicked.
    - **t=2000 ms**: IWDG timer expires → **system reset**.
    - **t=2000 ms+**: Boot completes; crash dump (if captured) is logged. System attempts restart.

=== "Question 2"
    The CFSR register reads `0x00000400` after a HardFault. Which bit field is set? What type of fault occurred and what does it mean?

=== "Answer 2"
    `0x00000400` = bit 10 set. In `SCB->CFSR`, bits [15:8] are the BusFault Status Register (BFSR). Bit 10 is **BFSR.IMPRECISERR** — an imprecise data bus error. This means a write operation caused a bus fault but the faulting address is not captured in BFAR (because the fault was detected after the instruction pipeline moved on). Caused by: writing to an invalid or unmapped memory address, such as an uninitialised or null pointer write.

---

## :material-check-circle: Summary

!!! success "Key Takeaways"
    - **Always enable the hardware watchdog** (IWDG) in production firmware — it is the last line of defence against unrecoverable hangs.
    - Use a **software monitor task** (highest priority) to check per-task heartbeats; only kick the hardware watchdog if all tasks are healthy.
    - Implement `HardFault_Handler` to **capture PC, LR, CFSR, HFSR, and task name** into `.noinit` RAM before resetting — this makes post-mortem debugging possible.
    - Use `SCB->CFSR` to decode the exact fault type (instruction error, data bus error, MPU violation, usage fault).
    - The **safe state** must disable all actuators before logging or resetting — never leave actuators in an active state during fault recovery.
    - Store crash dumps in **.noinit** RAM so they survive the reset and can be transmitted over a debug interface or logged to flash at next boot.
    - The WWDG adds early-kick detection for runaway loops; use IWDG for general liveness monitoring and WWDG for timing-critical watchdog windows.
