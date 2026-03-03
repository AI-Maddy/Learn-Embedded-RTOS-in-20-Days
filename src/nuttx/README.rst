NuttX Source Modules
====================

Purpose
-------
This directory contains NuttX-focused implementations for Day 16 concepts and
networking workflows. NuttX (Nuttx Real-Time Operating System) is a mature,
POSIX-compliant RTOS emphasizing standards compliance and scalability from
8-bit to 64-bit systems.

NuttX provides a small Unix-like environment with full MMU support (when available),
VFS, networking stacks, USB, and extensive driver support. It's used in Apache projects,
drones (PX4), and IoT devices.

Structure
---------

**basics/**
  Core NuttX tasking and synchronization examples:
  
  - Task creation (task_create, pthread_create)
  - POSIX threads API
  - Message queues (mq_open, mq_send, mq_receive)
  - Semaphores (sem_init, sem_wait, sem_post)
  - Mutexes (pthread_mutex_init, pthread_mutex_lock)
  - Condition variables (pthread_cond_wait)
  - Signals and signal handlers
  - Named semaphores for IPC

**patterns/**
  Reusable architecture mappings:
  
  - Producer-consumer with POSIX message queues
  - State machines with event-driven tasks
  - File system operations
  - Device driver integration
  - Work queues for deferred processing

**networking/**
  Socket-oriented examples and telemetry flows:
  
  - TCP/UDP socket programming
  - HTTP client/server examples
  - MQTT integration
  - CoAP for IoT
  - Network interface configuration
  - SSL/TLS secure communication

Coding Guidelines
-----------------

**Task/Thread Creation**
  - Use task_create() for NuttX tasks
  - Use pthread_create() for POSIX threads
  - Both approaches are valid; choose based on needs
  - Set appropriate stack sizes
  - Configure priority (0-255, higher = higher priority)

**POSIX Compliance**
  - Prefer POSIX APIs for portability
  - Standard file I/O: open(), read(), write(), close()
  - Use standard errno for error handling
  - POSIX message queues for IPC
  - Standard socket API for networking

**Synchronization**
  - POSIX semaphores (sem_t) for signaling
  - pthread_mutex_t for resource protection
  - pthread_cond_t for condition variables
  - Message queues (mq_t) for data passing
  - Signals for asynchronous notification

**File System**
  - VFS supports multiple file systems (FAT, ROMFS, NFS, etc.)
  - Mount file systems: mount(device, mountpoint, fstype, ...)
  - Standard file operations
  - /dev for device files
  - /proc for process information

**Device Drivers**
  - Character drivers: /dev/ttyS0, /dev/i2c0
  - Block drivers for storage
  - Network drivers register with netdev
  - Use standard open/read/write/ioctl
  - Register drivers with register_driver()

**Memory Management**
  - malloc/free available (from libc)
  - Task stacks from heap or static
  - Kernel heap separate from user heap
  - Memory pools for fixed allocation
  - Check CONFIG_MM_REGIONS for heap config

Build Instructions
------------------

NuttX uses a configure-build approach:

.. code-block:: bash

   # Configure for specific board
   cd nuttx
   ./tools/configure.sh stm32f4discovery:nsh
   
   # Or use newer cmake-based build
   cmake -B build -DBOARD_CONFIG=stm32f4discovery:nsh
   cmake --build build
   
   # Flash
   make flash
   # Or: openocd -f interface/stlink.cfg -f target/stm32f4x.cfg \
   #      -c "program nuttx.bin 0x08000000 verify reset exit"

Configuration
-------------

NuttX uses Kconfig (menuconfig):

.. code-block:: bash

   make menuconfig
   # Or: cmake --build build -t menuconfig

Common configurations in ``.config``:

.. code-block:: ini

   CONFIG_ARCH="arm"
   CONFIG_ARCH_CHIP="stm32"
   CONFIG_START_YEAR=2026
   CONFIG_START_MONTH=3
   CONFIG_START_DAY=2
   
   # POSIX features
   CONFIG_DISABLE_POSIX_TIMERS=n
   CONFIG_DISABLE_PTHREAD=n
   CONFIG_DISABLE_MQUEUE=n
   
   # Networking
   CONFIG_NET=y
   CONFIG_NET_TCP=y
   CONFIG_NET_UDP=y
   CONFIG_NETDEV_PHY_IOCTL=y

Debugging
---------

NuttShell (NSH) for interactive debugging:

.. code-block:: bash

   # Enable NSH in menuconfig
   CONFIG_NSH_CONSOLE=y
   CONFIG_NSH_LIBRARY=y
   
   # Connect via serial terminal
   picocom -b 115200 /dev/ttyUSB0
   
   # NSH commands:
   nsh> help
   nsh> ps          # List tasks
   nsh> free        # Memory usage
   nsh> ls /dev     # List devices
   nsh> ifconfig    # Network status
   nsh> mount       # Mounted filesystems

Debug with GDB:

.. code-block:: bash

   arm-none-eabi-gdb nuttx
   (gdb) target extended-remote :3333
   (gdb) monitor reset halt
   (gdb) load
   (gdb) break main
   (gdb) continue

Key Examples
------------

- **hello_task.c**: Basic task with printf
- **multiple_tasks.c**: Multiple tasks with priorities
- **queue_example.c**: POSIX message queue usage
- **semaphore_example.c**: POSIX semaphore synchronization
- **producer_consumer.c**: Message queue producer-consumer
- **state_machine.c**: Event-driven FSM
- **tcp_server.c** (networking/): Simple TCP echo server
- **http_client.c** (networking/): HTTP GET example

Networking Examples
-------------------

**TCP Server**:

.. code-block:: c

   int sockfd = socket(AF_INET, SOCK_STREAM, 0);
   bind(sockfd, (struct sockaddr*)&addr, sizeof(addr));
   listen(sockfd, 5);
   int client = accept(sockfd, NULL, NULL);
   recv(client, buffer, size, 0);
   send(client, response, len, 0);

**UDP Client**:

.. code-block:: c

   int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
   sendto(sockfd, data, len, 0, (struct sockaddr*)&addr, sizeof(addr));
   recvfrom(sockfd, buffer, size, 0, NULL, NULL);

Advanced Features
-----------------

**Board Support**:
  Extensive board support in boards/ directory

**File Systems**:
  FAT, ROMFS, NFS, TMPFS, BINFS, PROCFS

**Networking**:
  IPv4/IPv6, TCP/UDP, ICMP, ARP, routing

**USB**:
  Device and host stacks

**Graphics**:
  NX graphics system

**Security**:
  SSL/TLS via mbedTLS

**Power Management**:
  Dynamic frequency scaling, sleep modes

Resources
---------

- NuttX Website: https://nuttx.apache.org/
- Documentation: https://nuttx.apache.org/docs/latest/
- Git Repository: https://github.com/apache/nuttx
- Mailing Lists: https://nuttx.apache.org/community/
- PX4 Project: https://px4.io/ (major NuttX user)
