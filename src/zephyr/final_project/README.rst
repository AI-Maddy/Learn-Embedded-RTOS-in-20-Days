Zephyr Final Project
====================

Purpose
-------
Integrated Day 20 project demonstrating complete Zephyr application with multiple
subsystems, device tree integration, and modern embedded practices.

Project: IoT Environmental Monitor
----------------------------------

**Features**:
  - Multi-sensor data acquisition (I2C/SPI)
  - Data processing pipeline with work queues
  - Logging subsystem integration
  - Network telemetry (HTTP/CoAP/MQTT)
  - Shell interface for debugging
  - Storage (flash file system)
  - Power management
  - OTA update support (optional)

**Architecture**:
  - 7 threads with different priorities
  - Message queues for inter-thread comm
  - Work queues for deferred processing
  - Device tree for hardware config
  - Kconfig for feature selection
  - Logging framework

System Components
-----------------

**Threads**:

1. Sensor Thread (prio 5)
2. Processing Thread (prio 4)
3. Network Thread (prio 3)
4. Storage Thread (prio 2)
5. Shell Thread (prio 1)
6. Monitor Thread (prio 6)
7. PM Thread (prio 1)

**Subsystems**:
  - Device tree: sensors, peripherals
  - Logging: multi-backend (UART, flash)
  - Networking: IPv4/IPv6 ready
  - Storage: LittleFS on flash
  - Shell: Custom commands
  - PM: Low-power modes

Build Instructions
------------------

.. code-block:: bash

   cd src/zephyr/final_project
   west build -b nucleo_f429zi
   west flash
   west espressif monitor

Configuration
-------------

prj.conf:

.. code-block:: ini

   CONFIG_NET=y
   CONFIG_LOG=y
   CONFIG_SHELL=y
   CONFIG_FILE_SYSTEM=y
   CONFIG_FILE_SYSTEM_LITTLEFS=y

Device Tree Overlay
-------------------

boards/nucleo_f429zi.overlay:

.. code-block:: dts

   &i2c1 {
       bme280: bme280@76 {
           compatible = "bosch,bme280";
           reg = <0x76>;
       };
   };

Shell Commands
--------------

.. code-block:: text

   uart:~$ status
   uart:~$ sensors
   uart:~$ network info
   uart:~$ log stats

Testing
-------

Run in QEMU:

.. code-block:: bash

   west build -b qemu_cortex_m3 -t run

Performance
-----------

- Sensor sampling: 100ms ±5ms
- Network telemetry: 5s intervals
- CPU utilization: 30-50%
- Memory: 40KB heap, 15KB stacks

Next Steps
----------

- Add cloud integration (AWS IoT, Azure)
- Implement OTA updates
- Add security (TLS, secure boot)
- Power optimization
