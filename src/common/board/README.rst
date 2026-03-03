Board Support Package (BSP)
============================

Purpose
-------
This directory contains board-specific initialization code and hardware configuration
that is required before an RTOS can start. The BSP provides a consistent interface for
hardware setup across different boards and platforms.

The board support package handles:

- System clock configuration
- Memory controller initialization
- Pin multiplexing and GPIO setup
- Early hardware initialization
- Board-specific peripheral configuration

Scope
-----

The BSP layer is the lowest level of hardware abstraction, sitting directly above
the microcontroller hardware. It provides the foundation upon which RTOS and
application code can run reliably.

File Organization
-----------------

**board_init.c/h**
  Main board initialization module:
  
  - ``board_early_init()``: Very early init (before .data/.bss)
  - ``board_init()``: Full board initialization
  - ``board_get_cpu_freq()``: Query configured CPU frequency
  - Clock tree configuration
  - Power management setup

**board_config.h**
  Board-specific configuration constants:
  
  - Memory addresses and sizes
  - Peripheral base addresses
  - Default pin assignments
  - Clock frequencies
  - Board revision handling

**startup.c** (optional)
  Startup code and vector table:
  
  - Reset handler
  - Exception handlers
  - Interrupt vector table
  - Stack pointer initialization

Coding Guidelines
-----------------

**Initialization Sequence**
  1. Critical hardware setup (clocks, memory)
  2. Pin configuration for essential peripherals
  3. Console UART initialization (for debugging)
  4. Secondary peripherals
  5. Board self-test (optional)

**Portability**
  - Use HAL or CMSIS abstractions when available
  - Isolate register-level access to specific functions
  - Document hardware dependencies clearly
  - Support multiple board variants through conditional compilation

**Safety**
  - Verify PLL lock before switching clocks
  - Set up watchdog early if required
  - Configure memory protection unit (MPU) if available
  - Validate critical configuration before use

Build Instructions
------------------

Board support is compiled as part of the common library:

.. code-block:: bash

   cd src/common/board
   export BOARD=stm32f4_discovery
   make ARCH=arm CROSS_COMPILE=arm-none-eabi-

Supported Boards
----------------

- STM32F4 Discovery
- STM32 Nucleo-64 series
- NXP FRDM-K64F
- ESP32 DevKitC
- QEMU ARM virt machine (for testing)

Key Examples
------------

**board_init.c**
  Complete board initialization including:
  
  - System clock configuration (PLL setup)
  - GPIO initialization for onboard LEDs
  - UART console setup
  - Systick timer configuration
  - Error handling and diagnostics

Configuration
-------------

Board-specific settings in ``board_config.h``:

.. code-block:: c

   #define BOARD_CPU_FREQ_HZ    168000000UL
   #define BOARD_UART_CONSOLE   USART2
   #define BOARD_LED_PIN        GPIO_PIN_5
   #define BOARD_UART_BAUDRATE  115200

Debugging
---------

Enable board initialization debugging:

.. code-block:: c

   #define BOARD_DEBUG 1

This will output initialization progress over the console UART.
