Common Peripheral Drivers
=========================

Purpose
-------
This directory provides reusable peripheral driver abstractions that work across
multiple RTOS implementations. The drivers offer a consistent API while hiding
hardware-specific implementation details.

The driver layer sits between the board support package (BSP) and application code,
providing high-level peripheral access with buffering, error handling, and optional
RTOS integration.

Scope
-----

Drivers in this directory are designed to be:

- **Portable**: Work with different RTOS or bare-metal systems
- **Efficient**: Minimal overhead, interrupt-driven operation
- **Safe**: Robust error handling and resource management
- **Configurable**: Compile-time and runtime configuration options

File Organization
-----------------

**uart_driver.c/h**
  Universal Asynchronous Receiver/Transmitter:
  
  - Buffered TX/RX with ring buffers
  - Interrupt-driven operation
  - Non-blocking read/write APIs
  - Configurable baud rate and frame format
  - Optional DMA support

**spi_driver.c/h**
  Serial Peripheral Interface:
  
  - Master and slave mode support
  - Synchronous transfer operations
  - Chip select management
  - Configurable clock polarity and phase
  - Multi-slave support

**i2c_driver.c/h**
  Inter-Integrated Circuit:
  
  - Master mode implementation
  - 7-bit and 10-bit addressing
  - Register read/write helpers
  - Clock stretching support
  - Bus error detection and recovery

**gpio_driver.c/h**
  General Purpose Input/Output:
  
  - Pin configuration (input, output, alternate function)
  - Pull-up/pull-down resistor control
  - Read/write operations
  - Interrupt configuration
  - Pin grouping for efficient multi-pin operations

**timer_driver.c/h**
  Hardware timers and PWM:
  
  - One-shot and periodic timers
  - PWM generation
  - Input capture
  - Frequency and duty cycle control
  - Timer chaining

Coding Guidelines
-----------------

**API Design**
  - Consistent naming: ``<peripheral>_init()``, ``<peripheral>_read()``, etc.
  - Return error codes (int) or data count (ssize_t)
  - Use opaque handles or descriptor structs
  - Separate initialization from configuration

**Memory Management**
  - Statically allocated buffers where possible
  - User-provided buffers for flexibility
  - No dynamic allocation in interrupt context
  - Clear buffer ownership semantics

**Thread Safety**
  - Document thread-safety of each function
  - Use atomic operations for flags/counters
  - Provide locked/unlocked variants if needed
  - Critical sections should be minimal

**Error Handling**
  - Return negative errno-style codes for errors
  - Return byte count for successful transfers
  - Provide error-to-string conversion
  - Log errors for debugging (optional)

**Interrupt Handling**
  - Keep ISRs minimal and fast
  - Defer processing to task context when possible
  - Use interrupt-safe operations only
  - Document interrupt priority requirements

Build Instructions
------------------

Drivers are compiled as part of the common library:

.. code-block:: bash

   cd src/common/drivers
   make ARCH=arm CROSS_COMPILE=arm-none-eabi-
   
Configuration is done through ``drivers_config.h``:

.. code-block:: c

   #define UART_RX_BUFFER_SIZE  256
   #define UART_TX_BUFFER_SIZE  512
   #define MAX_SPI_DEVICES      3

Key Examples
------------

**uart_driver.c**
  Full-featured UART driver with:
  
  - Initialization and configuration
  - Interrupt-driven TX/RX with ring buffers
  - Non-blocking read/write functions
  - UART_printf() helper for formatted output
  - Flow control support (RTS/CTS)
  - Frame error detection

**Example Usage**:

.. code-block:: c

   uart_handle_t uart1;
   uart_config_t config = {
       .baudrate = 115200,
       .data_bits = 8,
       .parity = UART_PARITY_NONE,
       .stop_bits = 1
   };
   
   uart_init(&uart1, UART1_BASE, &config);
   uart_write(&uart1, "Hello\n", 6);
   
   char buf[32];
   int n = uart_read(&uart1, buf, sizeof(buf));

Integration with RTOS
---------------------

Drivers can optionally integrate with RTOS features:

**Semaphores for blocking**:
  - Define ``UART_USE_RTOS_SEMAPHORE`` to enable
  - Wait for transfer completion in task context

**Callbacks**:
  - Register callback for transfer completion
  - Called from ISR or deferred to task

**Power Management**:
  - Implement suspend/resume hooks
  - Support low-power modes

Porting Guide
-------------

To port drivers to a new MCU:

1. Implement platform-specific init/deinit
2. Configure peripheral registers in init
3. Implement ISR handlers
4. Map peripheral base addresses
5. Test with loopback and real peripherals

Testing
-------

Driver unit tests:

.. code-block:: bash

   cd tests/drivers
   make test_uart
   ./test_uart --loopback
