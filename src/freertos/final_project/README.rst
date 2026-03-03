FreeRTOS Final Project
======================

Purpose
-------
This directory contains an integrated, real-world application demonstrating complete
FreeRTOS system design. This is the culmination of Day 20, combining all concepts
from previous days into a cohesive, production-quality embedded application.

The final project implements a multi-sensor data acquisition and processing system
with networking, storage, and user interface components.

Project Overview
----------------

**Application**: Environmental Monitoring System

**Features**:
  - Multiple sensor acquisition (temperature, humidity, pressure)
  - Data processing and filtering
  - Local data logging to flash storage
  - Network telemetry (HTTP/MQTT)
  - Shell interface for debugging
  - Error monitoring and recovery
  - Power management

**Architecture**:
  - 6-8 tasks with different priorities
  - Multiple queues for inter-task communication
  - Semaphores and mutexes for synchronization
  - Software timers for periodic operations
  - Interrupt-driven I/O

System Architecture
-------------------

**Tasks**:

1. **Sensor Task** (High Priority)
   - Reads sensors via I2C/SPI
   - 100ms periodic sampling
   - Publishes to processing queue

2. **Processing Task** (Medium Priority)
   - Filters and validates sensor data
   - Applies calibration
   - Sends to storage and network queues

3. **Storage Task** (Low Priority)
   - Writes data to flash file system
   - Manages log rotation
   - Handles write errors

4. **Network Task** (Medium Priority)
   - Sends telemetry to server
   - HTTP POST or MQTT publish
   - Handles connection failures

5. **Shell Task** (Lowest Priority)
   - Interactive command line
   - Status queries, configuration
   - Debug commands

6. **Monitor Task** (High Priority)
   - Watchdog management
   - Stack usage monitoring
   - Error detection and reporting

**Communication**:

.. code-block:: text

   Sensor Task ──[Queue]──> Processing Task ──[Queue]──> Storage Task
                                            └──[Queue]──> Network Task
                                            
   Shell Task  ──[Semaphore]──> All Tasks (for status/control)
   
   Monitor Task ─[Checks]─> All Tasks (for health)

Build Instructions
------------------

.. code-block:: bash

   cd src/freertos/final_project
   
   # Configure for your board
   make menuconfig  # Or edit config.h
   
   # Build
   make BOARD=stm32f4_discovery
   
   # Flash
   make flash
   
   # Connect to shell
   picocom -b 115200 /dev/ttyUSB0

Configuration
-------------

**config.h**:

.. code-block:: c

   // Task priorities
   #define SENSOR_TASK_PRIORITY      4
   #define PROCESSING_TASK_PRIORITY  3
   #define NETWORK_TASK_PRIORITY     3
   #define STORAGE_TASK_PRIORITY     2
   #define SHELL_TASK_PRIORITY       1
   #define MONITOR_TASK_PRIORITY     5
   
   // Queue sizes
   #define SENSOR_QUEUE_SIZE        10
   #define STORAGE_QUEUE_SIZE       50
   #define NETWORK_QUEUE_SIZE       20
   
   // Timing
   #define SENSOR_PERIOD_MS         100
   #define NETWORK_SEND_PERIOD_MS   5000
   
   // Features
   #define ENABLE_NETWORK           1
   #define ENABLE_STORAGE           1
   #define ENABLE_SHELL             1

**FreeRTOSConfig.h**:

.. code-block:: c

   #define configTOTAL_HEAP_SIZE            (40 * 1024)
   #define configMAX_PRIORITIES             6
   #define configUSE_MUTEXES                1
   #define configUSE_TIMERS                 1
   #define configUSE_COUNTING_SEMAPHORES    1
   #define configCHECK_FOR_STACK_OVERFLOW   2
   #define configUSE_MALLOC_FAILED_HOOK     1

Key Components
--------------

**main.c**:
  System initialization and task creation

**sensor_task.c**:
  Sensor acquisition logic

**processing_task.c**:
  Data processing algorithms

**storage_task.c**:
  Flash file system integration

**network_task.c**:
  HTTP/MQTT client implementation

**shell_task.c**:
  Command line interface

**monitor_task.c**:
  System health monitoring

**common.h**:
  Shared data structures and definitions

Shell Commands
--------------

Interactive commands available:

.. code-block:: text

   > help
   Available commands:
     status       - Show system status
     tasks        - List tasks and CPU usage
     sensors      - Show sensor readings
     queues       - Show queue utilization
     network      - Network statistics
     storage      - Storage info
     config       - Show/set configuration
     reset        - Reset the system
   
   > status
   System uptime: 00:05:23
   CPU usage: 45%
   Free heap: 28672 bytes
   Sensors: OK
   Network: Connected
   Storage: 234/1024 MB used

Error Handling
--------------

**Strategy**:
  - Each task has error recovery logic
  - Monitor task detects stuck/crashed tasks
  - Watchdog prevents system lockup
  - Errors logged to storage and console
  - Graceful degradation (continue with reduced functionality)

**Error Types**:
  - Sensor read failures (retry with backoff)
  - Network disconnection (reconnect with exponential backoff)
  - Storage full (rotate logs, discard oldest)
  - Memory allocation failure (reduce queue sizes, disable features)
  - Task deadlock (watchdog reset)

Testing
-------

**Unit Tests**:
  - Each task can be tested independently
  - Mock interfaces for hardware

**Integration Tests**:
  - Run system with simulated sensors
  - Verify data flow through all tasks
  - Test error scenarios (disconnect network, fill storage)

**Stress Tests**:
  - High sensor data rates
  - Network latency/packet loss
  - Storage write failures
  - Low memory conditions

Performance Metrics
-------------------

System achieves:

- Sensor sampling: 100ms ±5ms (jitter)
- Processing latency: <10ms
- Network telemetry: 5-second intervals
- Storage write: <50ms
- Shell responsiveness: <100ms
- CPU utilization: 40-60% average
- Memory usage: 30-35KB heap, 12KB total stack

Porting Guide
-------------

To port to your hardware:

1. **Update board_init.c**: Initialize your peripherals
2. **Modify sensor_task.c**: Read your actual sensors
3. **Adapt network_task.c**: Configure for your network stack
4. **Configure storage**: Choose file system (FatFS, LittleFS)
5. **Adjust priorities**: Based on your timing requirements
6. **Test thoroughly**: Especially error conditions

Enhancements
------------

Possible extensions:

- Add more sensors or actuators
- Implement OTA firmware updates
- Add encryption for network data
- Implement power saving modes
- Add graphical display
- Web server for configuration
- Cloud integration (AWS IoT, Azure IoT)
- Data visualization and analytics

Lessons Learned
---------------

This project demonstrates:

- Task priority assignment strategies
- Queue-based architecture scalability
- Error handling and recovery patterns
- Resource management in constrained systems
- Integration of multiple subsystems
- Real-world timing and performance constraints
- Debugging and monitoring techniques
