Day 12 Lesson - Zephyr RTOS
===========================

Introduction
------------

Zephyr is a modern, Linux Foundation-backed RTOS designed for IoT and connected devices with extensive hardware support.

Architecture
------------

Key differences from FreeRTOS:
- Device tree for hardware description
- Kconfig for configuration
- Native networking stack (TCP/IP, Bluetooth, Thread)
- Rich device driver model
- POSIX API support

Thread Creation
---------------

.. code-block:: c

   #include <zephyr/kernel.h>
   
   #define STACK_SIZE 1024
   #define THREAD_PRIORITY 5
   
   K_THREAD_STACK_DEFINE(my_stack, STACK_SIZE);
   struct k_thread my_thread_data;
   k_tid_t my_tid;
   
   void my_entry_point(void *p1, void *p2, void *p3)
   {
       while (1) {
           printk("Thread running\n");
           k_sleep(K_MSEC(1000));
       }
   }
   
   my_tid = k_thread_create(&my_thread_data, my_stack,
                            K_THREAD_STACK_SIZEOF(my_stack),
                            my_entry_point,
                            NULL, NULL, NULL,
                            THREAD_PRIORITY, 0, K_NO_WAIT);

Synchronization Primitives
---------------------------

- Semaphores: k_sem_*
- Mutexes: k_mutex_*
- Message queues: k_msgq_*
- Mailboxes: k_mbox_*
- Pipes: k_pipe_*
- Work queues: k_work_*

Device Drivers
--------------

Zephyr's device model provides consistent API across hardware platforms.

.. code-block:: c

   const struct device *uart_dev = device_get_binding("UART_0");
   uart_irq_callback_set(uart_dev, uart_cb);
   uart_irq_rx_enable(uart_dev);

Key Advantages
--------------

1. Comprehensive hardware support
2. Built-in networking/Bluetooth
3. CMake-based build (west tool)
4. Security features (TLS, secure boot)
5. Active development community

When to Use Zephyr
------------------

- IoT/connected devices
- Bluetooth/WiFi requirements
- Need for Linux-like development experience
- Rapid prototyping with diverse hardware
