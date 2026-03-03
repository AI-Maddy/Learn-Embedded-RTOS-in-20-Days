Common Utilities
================

Purpose
-------
This directory contains general-purpose utility functions and data structures
that are commonly needed in embedded RTOS applications. These utilities are
designed to be lightweight, efficient, and portable across different platforms.

The utilities provide essential functionality without depending on specific
RTOS features, making them suitable for use in any part of the system.

Scope
-----

Utilities include:

- Debug logging and diagnostics
- Assertion and error handling
- Time and delay functions
- Data structure implementations (ring buffers, linked lists)
- String manipulation and formatting
- CRC and checksum calculations
- Bit manipulation helpers
- Memory pool management

File Organization
-----------------

**debug_utils.c/h**
  Debug and logging facilities:
  
  - Debug printf with variable log levels
  - Assertion macros with file/line info
  - Memory dump utilities
  - Runtime statistics collection
  - Error code to string conversion
  - Compile-time debug control

**ring_buffer.c/h**
  Circular buffer implementation:
  
  - Thread-safe operations (with locking)
  - Byte-oriented or block-oriented
  - Overwrite or blocking modes
  - Peek without consuming
  - Buffer state queries (full, empty, count)

**time_utils.c/h**
  Time conversion and delay:
  
  - Millisecond to tick conversion
  - Timeout management
  - Uptime tracking
  - Microsecond delays (busy-wait)
  - Time difference calculations

**string_utils.c/h**
  String manipulation:
  
  - Safe string copy/concat (strlcpy, strlcat)
  - String to integer conversion
  - Hex dump formatting
  - Path manipulation
  - Wildcard matching

**crc_utils.c/h**
  Checksums and CRC:
  
  - CRC-16, CRC-32 implementations
  - Table-based and bitwise algorithms
  - Incremental CRC calculation
  - Common polynomial support

**list.c/h**
  Intrusive linked list:
  
  - Doubly-linked list macros
  - Insert, remove, iterate operations
  - Sorted list support
  - Compatible with kernel-style lists

**mem_pool.c/h**
  Fixed-size memory pools:
  
  - Pre-allocated block pools
  - Constant-time allocation/free
  - Pool statistics
  - No fragmentation

Coding Guidelines
-----------------

**Simplicity**
  - Keep utilities simple and focused
  - One utility, one purpose
  - Avoid feature creep
  - Document limitations clearly

**Efficiency**
  - Optimize for embedded constraints
  - Minimize memory usage
  - Use lookup tables for speed
  - Avoid floating-point if possible

**Safety**
  - Validate input parameters
  - Check buffer boundaries
  - Handle error cases gracefully
  - Use static analysis tools

**Portability**
  - Use standard C (C99 or C11)
  - Avoid platform-specific assumptions
  - Document any non-portable code
  - Test on multiple architectures

Build Instructions
------------------

Utilities are compiled as part of the common library:

.. code-block:: bash

   cd src/common/utils
   make ARCH=arm CROSS_COMPILE=arm-none-eabi-

Debug output can be configured:

.. code-block:: c

   // In debug_config.h
   #define DEBUG_LEVEL  DEBUG_LEVEL_INFO
   #define DEBUG_UART   USART2
   #define DEBUG_COLOR  1  // ANSI color codes

Key Examples
------------

**debug_utils.c**
  Comprehensive debug utilities:
  
  - ``DEBUG_PRINTF()``: Formatted debug output
  - ``DEBUG_ASSERT()``: Assertion with file/line
  - ``debug_dump_memory()``: Hexadecimal memory dump
  - ``debug_print_stats()``: Runtime statistics
  - Log level filtering (ERROR, WARN, INFO, DEBUG, TRACE)

**Example Usage**:

.. code-block:: c

   #include "debug_utils.h"
   
   void task_handler(void) {
       DEBUG_INFO("Task started\n");
       
       int result = process_data();
       DEBUG_ASSERT(result >= 0);
       
       if (result < 0) {
           DEBUG_ERROR("Process failed: %d\n", result);
           return;
       }
       
       DEBUG_DEBUG("Processed %d bytes\n", result);
   }

**ring_buffer.c**:
  Production-ready ring buffer:
  
  - Initialization and reset
  - Write and read operations
  - Atomic operations for lock-free use
  - Efficient modulo using power-of-2 sizes

**Example Usage**:

.. code-block:: c

   ring_buffer_t rb;
   uint8_t buffer[256];
   
   ring_buffer_init(&rb, buffer, sizeof(buffer));
   ring_buffer_write(&rb, data, len);
   
   int n = ring_buffer_read(&rb, output, 32);
   printf("Read %d bytes\n", n);

Configuration
-------------

Compile-time configuration in ``utils_config.h``:

.. code-block:: c

   // Debug configuration
   #define DEBUG_ENABLED         1
   #define DEBUG_BUFFER_SIZE     256
   #define DEBUG_MAX_LINE_LENGTH 128
   
   // Ring buffer defaults
   #define RING_BUFFER_POWER_OF_2  1
   
   // Memory pool
   #define MEM_POOL_GUARD_BYTES    4

Testing
-------

Unit tests for utilities:

.. code-block:: bash

   cd tests/utils
   make all
   ./test_ring_buffer
   ./test_debug_utils

Benchmarking
------------

Performance benchmarks:

.. code-block:: bash

   cd benchmarks/utils
   make bench_crc
   ./bench_crc
   # Reports CRC performance in MB/s
