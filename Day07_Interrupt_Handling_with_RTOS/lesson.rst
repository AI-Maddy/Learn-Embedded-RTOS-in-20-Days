Day 07 Lesson - Interrupt Handling with RTOS
===============================================

Introduction
------------

Interrupts are critical for real-time responsiveness. Integrating interrupts with an RTOS requires understanding ISR-safe APIs, priority management, and deferred processing patterns.

ISR Constraints
---------------

**Rules for ISR code:**

1. **Must be fast** (< 10-50µs typical)
2. **Cannot call blocking APIs**
3. **Must use ISR-safe functions** (FromISR variants)
4. **Minimize stack usage**
5. **Disable/enable interrupts carefully**

ISR-to-Task Communication
--------------------------

**Pattern:** Keep ISR minimal, defer processing to task.

.. code-block:: c

   SemaphoreHandle_t xDataReadySemaphore;
   volatile uint32_t g_adc_value;
   
   void ADC_IRQHandler(void)
   {
       BaseType_t xHigherPriorityTaskWoken = pdFALSE;
       
       // Read hardware register (fast)
       g_adc_value = ADC->DR;
       
       // Clear interrupt flag
       ADC->ISR |= ADC_ISR_EOC;
       
       // Signal task (ISR-safe)
       xSemaphoreGiveFromISR(xDataReadySemaphore, &xHigherPriorityTaskWoken);
       
       // Yield if higher priority task now ready
       portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
   }
   
   void vADCProcessingTask(void *pvParameters)
   {
       uint32_t adc_value;
       
       for(;;)
       {
           // Wait for ISR signal
           xSemaphoreTake(xDataReadySemaphore, portMAX_DELAY);
           
           // Copy volatile data
           taskENTER_CRITICAL();
           adc_value = g_adc_value;
           taskEXIT_CRITICAL();
           
           // Process (can take time)
           process_adc_data(adc_value);
       }
   }

CMSIS-RTOS Integration
-----------------------

.. code-block:: c

   void EXTI0_IRQHandler(void)
   {
       if(EXTI->PR & EXTI_PR_PR0)
       {
           EXTI->PR = EXTI_PR_PR0;  // Clear pending
           
           // Signal via queue
           uint32_t event_type = EVENT_BUTTON_PRESS;
           xQueueSendFromISR(xEventQueue, &event_type, NULL);
       }
   }

Interrupt Priority Configuration
---------------------------------

**Critical:** ISRs using RTOS APIs must have priority **lower** than ``configMAX_SYSCALL_INTERRUPT_PRIORITY``.

.. code-block:: c

   // FreeRTOSConfig.h
   #define configMAX_SYSCALL_INTERRUPT_PRIORITY  0x20  // Priority 1 in CMSIS
   
   // Cortex-M NVIC priorities (lower number = higher priority)
   // 0x00 (0): Highest - can preempt RTOS
   // 0x10...0x1F: Cannot use RTOS APIs
   // 0x20 (1): configMAX_SYSCALL_INTERRUPT_PRIORITY
   // 0x30...0xFF: Can safely use FromISR APIs
   
   void init_interrupts(void)
   {
       // High priority (cannot call RTOS)
       NVIC_SetPriority(HardFault_IRQn, 0);
       
       // Medium priority (can call RTOS FromISR APIs)
       NVIC_SetPriority(UART1_IRQn, 5);
       NVIC_SetPriority(TIM2_IRQn, 6);
       
       // SysTick must be lowest among RTOS-aware interrupts
       NVIC_SetPriority(SysTick_IRQn, 15);
   }

Direct-to-Task Notifications
-----------------------------

**Lightweight alternative to semaphores** for simple signaling.

.. code-block:: c

   TaskHandle_t xHighPriorityTask;
   
   void UART_IRQHandler(void)
   {
       BaseType_t xHigherPriorityTaskWoken = pdFALSE;
       uint32_t notification_value = UART->DR;
       
       // Directly notify task
       xTaskNotifyFromISR(xHighPriorityTask, 
                          notification_value,
                          eSetValueWithOverwrite,
                          &xHigherPriorityTaskWoken);
       
       portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
   }
   
   void vHighPriorityTask(void *pvParameters)
   {
       uint32_t notification_value;
       
       for(;;)
       {
           ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
           // Process interrupt
       }
   }

**Benefits:**
- Faster than semaphore (~50% less overhead)
- Less RAM (no semaphore object)
- Built-in 32-bit value transfer

Software Timers from ISR
-------------------------

.. code-block:: c

   TimerHandle_t xRetryTimer;
   
   void COMM_ERROR_IRQHandler(void)
   {
       BaseType_t xHigherPriorityTaskWoken = pdFALSE;
       
       // Start retry timer from ISR
       xTimerStartFromISR(xRetryTimer, &xHigherPriorityTaskWoken);
       
       portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
   }

Best Practices
--------------

1. **ISR does minimal work**: Read register, signal task, exit
2. **Use task notifications** for simple ISR-to-task signaling
3. **Configure priorities correctly**: RTOS-aware ISRs must be lower priority
4. **Measure ISR latency**: Ensure deadlines met
5. **Disable interrupts briefly**: Only for atomic operations

Summary
-------

- Keep ISRs short and fast
- Use FromISR API variants
- Defer processing to tasks
- Configure interrupt priorities properly
- Use task notifications for efficiency
