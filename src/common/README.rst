Common Source Modules
=====================

Purpose
-------
This directory contains shared code that is reused across multiple RTOS implementations.
The goal is to provide a portable hardware abstraction layer and utility functions that
work consistently regardless of which RTOS is used in the final application.

The common modules help reduce code duplication, improve maintainability, and provide
a unified development experience when working with different RTOS platforms.

Folder Organization
-------------------

**board/**
  Board-specific initialization and configuration code:
  
  - Hardware initialization sequences
  - Clock configuration
  - Pin multiplexing setup
  - Board-specific peripherals
  - Memory region definitions

**drivers/**
  Reusable peripheral driver abstractions:
  
  - UART, SPI, I2C drivers
  - GPIO abstraction layer
  - Timer and PWM drivers
  - ADC/DAC interfaces
  - Hardware abstraction layer (HAL) wrappers

**utils/**
  General-purpose utility functions:
  
  - Debug logging and printf utilities
  - Assertion macros
  - Time conversion helpers
  - Ring buffers and containers
  - CRC and checksum functions
  - String manipulation helpers

Coding Guidelines
-----------------

**Portability**
  - Keep APIs portable and avoid RTOS-specific assumptions
  - Use standard C types (stdint.h, stdbool.h)
  - Avoid compiler-specific extensions unless necessary
  - Document any platform-specific behavior

**Memory Management**
  - Prefer explicit ownership and bounded memory behavior
  - No dynamic allocation in driver code unless documented
  - Use static buffers with configurable sizes
  - Clear initialization and cleanup patterns

**Error Handling**
  - Return explicit error codes (errno-style or custom enums)
  - Document all possible error conditions
  - Provide error code to string conversion
  - Use assertions for programming errors

**Thread Safety**
  - Document thread-safety guarantees
  - Common code should be reentrant where possible
  - Use clearly named _locked() variants when needed
  - Avoid global mutable state

Build Instructions
------------------

Common modules are typically compiled as a static library:

.. code-block:: bash

   # Using Makefile
   cd src/common
   make ARCH=arm CROSS_COMPILE=arm-none-eabi-
   
   # Using CMake
   mkdir build && cd build
   cmake -DCMAKE_TOOLCHAIN_FILE=../toolchain.cmake ..
   make

The resulting ``libcommon.a`` can be linked with any RTOS-specific code.

Key Examples
------------

- ``board/board_init.c``: Complete board initialization template
- ``drivers/uart_driver.c``: Buffered UART driver with interrupt support
- ``utils/debug_utils.c``: Debug printf and logging macros

Testing
-------

Unit tests for common modules can be run on the host system:

.. code-block:: bash

   cd tests/common
   make test
   ./run_tests

Integration
-----------

To integrate common modules with an RTOS project:

1. Include the common directory in your build system
2. Link against libcommon.a or compile sources directly
3. Implement any required callbacks (e.g., malloc hooks)
4. Configure platform-specific parameters in config.h
