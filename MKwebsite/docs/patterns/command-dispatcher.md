# :material-send: Command Dispatcher

<div class="grid cards" markdown>
- :material-lightbulb-on: **Command Pattern** — encapsulate a request as a struct with a function pointer; decouple sender from handler
- :material-chip: **Command Queue** — a FreeRTOS queue of command structs serialises requests from multiple sources to a single dispatcher loop
- :material-alert: **Stack Depth** — command handler function pointers execute in the dispatcher task's context; size its stack for the deepest handler
- :material-check-circle: **Use When** — use command dispatcher when multiple tasks or ISRs need to invoke actions on a shared resource or state machine
</div>

---

## :material-lightbulb-on: Pattern Overview

The **command dispatcher** decouples the *source* of a request from its *execution*. Senders construct a command struct (containing an opcode or function pointer plus parameters) and post it to a queue. A single dispatcher task dequeues and executes commands one at a time—serialising access to shared resources without explicit mutex locking.

```
┌────────────┐  xQueueSend()  ┌──────────────────────┐
│  Source A  │──────────────▶ │                      │
│ (Task/ISR) │                │   Command Queue      │
├────────────┤                │  [CMD_SET_SPEED     ]│  xQueueReceive()  ┌──────────────────┐
│  Source B  │──────────────▶ │  [CMD_ENABLE_OUTPUT ]│──────────────────▶│ Dispatcher Task  │
│ (Task/ISR) │                │  [CMD_READ_STATUS   ]│                   │ executes handler │
├────────────┤                │  [CMD_EMERGENCY_STOP]│                   └──────────────────┘
│  Source C  │──────────────▶ │                      │
│  (Timer)   │                └──────────────────────┘
└────────────┘
```

---

## :material-code-tags: Command Struct Design

### Opcode-Based Command (switch-case dispatch)

```c
/* ---- Command opcodes ------------------------------------------ */
typedef enum {
    CMD_NOP             = 0,
    CMD_SET_SPEED       = 1,
    CMD_ENABLE_OUTPUT   = 2,
    CMD_DISABLE_OUTPUT  = 3,
    CMD_SET_POSITION    = 4,
    CMD_READ_STATUS     = 5,
    CMD_EMERGENCY_STOP  = 0xFF,
} CommandId_t;

/* ---- Command payload ------------------------------------------ */
typedef union {
    struct { int32_t rpm; }             set_speed;
    struct { uint8_t channel; }         enable;
    struct { int32_t counts; }          set_position;
    struct { void *response_queue; }    read_status;
} CommandPayload_t;

/* ---- Command struct ------------------------------------------- */
typedef struct {
    CommandId_t      id;
    CommandPayload_t payload;
    TickType_t       issued_at;  /* for timeout detection */
} Command_t;
```

### Function-Pointer-Based Command (table-free dispatch)

```c
/* Command carries its own handler — no switch needed */
typedef void (*CommandHandler_t)(const void *params);

typedef struct {
    CommandHandler_t handler;       /* function to call */
    uint8_t          params[16];    /* inline parameter storage */
    TickType_t       issued_at;
} FPCommand_t;

/* Sender constructs a ready-to-execute command */
void send_set_speed(QueueHandle_t q, int32_t rpm) {
    FPCommand_t cmd;
    cmd.handler   = handle_set_speed;   /* pointer to handler */
    cmd.issued_at = xTaskGetTickCount();
    memcpy(cmd.params, &rpm, sizeof(rpm));
    xQueueSend(q, &cmd, pdMS_TO_TICKS(10));
}
```

---

## :material-play-circle: Dispatcher Loop

### Opcode Dispatcher

```c
#define COMMAND_QUEUE_LENGTH   20
static QueueHandle_t xCommandQueue;

/* ---- Command handlers ----------------------------------------- */
static void handle_set_speed(const CommandPayload_t *p) {
    motor_set_rpm(p->set_speed.rpm);
}

static void handle_enable(const CommandPayload_t *p) {
    output_enable(p->enable.channel);
}

static void handle_emergency_stop(const CommandPayload_t *p) {
    (void)p;
    motor_set_rpm(0);
    output_disable_all();
    enter_safe_state();
}

/* ---- Dispatcher task ------------------------------------------ */
static void vCommandDispatcher(void *pv) {
    Command_t cmd;

    for (;;) {
        /* Block until a command arrives */
        xQueueReceive(xCommandQueue, &cmd, portMAX_DELAY);

        /* Age check — discard stale commands */
        TickType_t age = xTaskGetTickCount() - cmd.issued_at;
        if (age > pdMS_TO_TICKS(500)) {
            log_warning("Stale command %d dropped (%lu ms old)", cmd.id, age);
            continue;
        }

        /* Dispatch */
        switch (cmd.id) {
            case CMD_SET_SPEED:     handle_set_speed(&cmd.payload);     break;
            case CMD_ENABLE_OUTPUT: handle_enable(&cmd.payload);        break;
            case CMD_DISABLE_OUTPUT:output_disable(cmd.payload.enable.channel); break;
            case CMD_EMERGENCY_STOP:handle_emergency_stop(&cmd.payload);break;
            default:
                log_error("Unknown command id: %d", cmd.id);
                break;
        }
    }
}

/* ---- Initialisation ------------------------------------------- */
void dispatcher_init(void) {
    xCommandQueue = xQueueCreate(COMMAND_QUEUE_LENGTH, sizeof(Command_t));
    configASSERT(xCommandQueue != NULL);

    /* Dispatcher runs at medium priority — above background, below sensors */
    xTaskCreate(vCommandDispatcher, "Dispatcher", 512, NULL, 3, NULL);
}
```

### Function-Pointer Dispatcher (zero switch overhead)

```c
static void vFPCommandDispatcher(void *pv) {
    FPCommand_t cmd;
    for (;;) {
        xQueueReceive(xFPCommandQueue, &cmd, portMAX_DELAY);
        if (cmd.handler != NULL) {
            cmd.handler(cmd.params);   /* execute handler directly */
        }
    }
}
```

---

## :material-priority-high: Priority Command Queue

For systems that need urgent commands (e.g., emergency stop) to bypass pending normal commands, use two queues and prioritise them:

```c
static QueueHandle_t xNormalQueue;
static QueueHandle_t xUrgentQueue;

static void vPriorityDispatcher(void *pv) {
    Command_t cmd;
    for (;;) {
        /* Always drain urgent queue first */
        if (xQueueReceive(xUrgentQueue, &cmd, 0) == pdTRUE) {
            dispatch_command(&cmd);
            continue;
        }
        /* Then process normal queue — block up to 10 ms */
        if (xQueueReceive(xNormalQueue, &cmd, pdMS_TO_TICKS(10)) == pdTRUE) {
            dispatch_command(&cmd);
        }
    }
}

/* Emergency stop goes to urgent queue */
void post_emergency_stop(void) {
    Command_t cmd = { .id = CMD_EMERGENCY_STOP, .issued_at = xTaskGetTickCount() };
    xQueueSendToFront(xUrgentQueue, &cmd, 0);   /* front for minimum latency */
}
```

---

## :material-state-machine: Integration with State Machine

The dispatcher is the natural entry point for a state machine: commands are events that drive transitions.

```c
typedef enum { STATE_IDLE, STATE_RUNNING, STATE_FAULT } MotorState_t;
static MotorState_t current_state = STATE_IDLE;

static void dispatch_command(const Command_t *cmd) {
    switch (current_state) {
        case STATE_IDLE:
            if (cmd->id == CMD_ENABLE_OUTPUT) {
                output_enable(cmd->payload.enable.channel);
                current_state = STATE_RUNNING;
            }
            break;

        case STATE_RUNNING:
            if (cmd->id == CMD_SET_SPEED) {
                motor_set_rpm(cmd->payload.set_speed.rpm);
            } else if (cmd->id == CMD_EMERGENCY_STOP) {
                handle_emergency_stop(NULL);
                current_state = STATE_FAULT;
            }
            break;

        case STATE_FAULT:
            /* Only allow reset command in fault state */
            if (cmd->id == CMD_NOP) {
                current_state = STATE_IDLE;
            }
            break;
    }
}
```

---

## :material-help-circle: Flashcards

???+ question "What advantage does a command dispatcher give over calling handler functions directly from multiple tasks?"
    A dispatcher **serialises** execution: only one command handler runs at a time in the dispatcher task's context. Handlers can safely access shared resources (motor driver, hardware peripherals) without per-resource mutexes. It also decouples senders from the execution context — senders construct and post commands without knowing when or how they execute.

???+ question "Why must you size the dispatcher task's stack for the deepest handler, not the dispatcher loop itself?"
    Command handlers execute as function calls *within* the dispatcher task's stack frame. If a handler calls deeply nested functions (e.g., a PID update that calls a math library), all those frames consume the dispatcher's stack. If the stack is sized only for the thin dispatcher loop, a complex handler will overflow it. Profile each handler's stack consumption and sum them for the deepest call chain.

???+ question "What is the difference between opcode-based and function-pointer-based command dispatch?"
    **Opcode-based**: the command carries an integer ID; the dispatcher uses a switch-case to look up the handler. Adding new commands requires modifying the dispatcher. **Function-pointer-based**: the command carries the handler address directly; the dispatcher blindly calls it. Adding new commands requires no dispatcher modification. The function-pointer approach follows the Open-Closed Principle but loses the ability to validate/log commands centrally.

???+ question "How do you implement command prioritisation without a priority-aware queue?"
    Use **two queues**: an urgent queue and a normal queue. The dispatcher checks the urgent queue with timeout=0 first; only if it is empty does it block on the normal queue. Emergency or safety commands are posted to the urgent queue via `xQueueSendToFront`; regular operational commands go to the normal queue.

---

## :material-clipboard-check: Self Test

=== "Question 1"
    Three tasks (UI task, network task, timer callback) all post commands to a single command queue of length 10. The dispatcher task runs at priority 3. The UI task runs at priority 4 and floods the queue with `CMD_SET_SPEED` at 500 Hz. The dispatcher processes commands at 200 Hz. What happens after ~50 ms and how do you fix it?

=== "Answer 1"
    In 50 ms the UI task sends 25 commands but the dispatcher processes only 10. The queue fills at ~20 ms (10/500 Hz). After that `xQueueSend` returns `errQUEUE_FULL` for every subsequent UI command — commands are dropped silently if the return value is not checked. Fix options:
    1. Reduce UI command rate (debounce input, coalesce repeated commands).
    2. Use `xQueueOverwrite` for setpoint-type commands (only latest matters).
    3. Increase queue length to buffer the burst.
    4. Lower UI task priority so the dispatcher has CPU to drain the queue faster.

=== "Question 2"
    You are refactoring a bare-metal system where five subsystems all call `motor_set_rpm()` directly. Intermittent corruption occurs because two subsystems call it concurrently. How does the command dispatcher pattern fix this without adding a mutex?

=== "Answer 2"
    With the command dispatcher, all five subsystems post `CMD_SET_SPEED` to a single queue instead of calling `motor_set_rpm()` directly. The dispatcher task is the **only** context that calls `motor_set_rpm()`. Since the dispatcher processes one command at a time and no other task calls the function, concurrent access is structurally impossible — serialisation is enforced by the queue, not by a mutex. This eliminates the race condition without any locking.

---

## :material-check-circle: Summary

!!! success "Key Takeaways"
    - The command dispatcher encapsulates requests as structs and executes them serially in a dedicated task, eliminating concurrent access to shared resources without per-resource mutexes.
    - Use **opcode + switch-case** for a small, well-known command set with central logging; use **function pointers** for extensible, plugin-style command registration.
    - Size the dispatcher task stack for the **deepest handler call chain**, not the dispatcher loop.
    - Implement a **priority command queue** (two queues: urgent + normal) when safety-critical commands must bypass pending normal commands.
    - Always check `xQueueSend` return values — a full queue means commands are being dropped.
    - The dispatcher naturally integrates with a **state machine**: commands are events; the dispatch function enforces which commands are valid in each state.
