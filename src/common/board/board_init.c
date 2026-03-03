/**
 * @file board_init.c
 * @brief Board initialization and hardware setup
 * 
 * This file provides board-specific initialization code that sets up
 * clocks, peripherals, and basic hardware configuration before the
 * RTOS starts.
 * 
 * Target: STM32F4 Discovery (adaptable to other boards)
 */

#include <stdint.h>
#include <stdbool.h>
#include "board_init.h"

/* STM32F4 Register definitions (example - adapt for your MCU) */
#define RCC_BASE            0x40023800
#define RCC_CR              (*(volatile uint32_t*)(RCC_BASE + 0x00))
#define RCC_PLLCFGR         (*(volatile uint32_t*)(RCC_BASE + 0x04))
#define RCC_CFGR            (*(volatile uint32_t*)(RCC_BASE + 0x08))
#define RCC_AHB1ENR         (*(volatile uint32_t*)(RCC_BASE + 0x30))
#define RCC_APB2ENR         (*(volatile uint32_t*)(RCC_BASE + 0x44))

#define FLASH_BASE          0x40023C00
#define FLASH_ACR           (*(volatile uint32_t*)(FLASH_BASE + 0x00))

#define GPIOD_BASE          0x40020C00
#define GPIOD_MODER         (*(volatile uint32_t*)(GPIOD_BASE + 0x00))
#define GPIOD_ODR           (*(volatile uint32_t*)(GPIOD_BASE + 0x14))

/* PLL configuration for 168 MHz from 8 MHz HSE */
#define PLL_M   8
#define PLL_N   336
#define PLL_P   2
#define PLL_Q   7

#define CPU_FREQ_HZ     168000000UL
#define APB1_FREQ_HZ    42000000UL
#define APB2_FREQ_HZ    84000000UL

/* Private variables */
static bool board_initialized = false;

/**
 * @brief Very early initialization (before .data and .bss)
 * 
 * This function is called from the startup code before C runtime
 * initialization. Only minimal setup should be done here.
 */
void board_early_init(void)
{
    /* Enable FPU if available (Cortex-M4F) */
#if defined(__FPU_PRESENT) && (__FPU_PRESENT == 1U)
    SCB->CPACR |= ((3UL << 10*2) | (3UL << 11*2));
#endif

    /* Set vector table offset if needed */
    SCB->VTOR = 0x08000000;
}

/**
 * @brief Configure system clock to 168 MHz using PLL
 * @return 0 on success, negative on error
 */
static int clock_init(void)
{
    uint32_t timeout = 0;
    
    /* Enable HSE (High Speed External oscillator) */
    RCC_CR |= (1 << 16);  /* HSEON */
    
    /* Wait for HSE to be ready */
    timeout = 100000;
    while (!(RCC_CR & (1 << 17)) && timeout--) {
        /* Wait for HSERDY */
    }
    
    if (timeout == 0) {
        return -1;  /* HSE failed to start */
    }
    
    /* Configure Flash latency for 168 MHz */
    FLASH_ACR = (FLASH_ACR & ~0x0F) | 0x05;  /* 5 wait states */
    
    /* Configure PLL */
    RCC_PLLCFGR = (PLL_M << 0)   |  /* PLLM */
                  (PLL_N << 6)   |  /* PLLN */
                  (((PLL_P >> 1) - 1) << 16) |  /* PLLP */
                  (1 << 22)      |  /* PLL source = HSE */
                  (PLL_Q << 24);    /* PLLQ */
    
    /* Enable PLL */
    RCC_CR |= (1 << 24);  /* PLLON */
    
    /* Wait for PLL to lock */
    timeout = 100000;
    while (!(RCC_CR & (1 << 25)) && timeout--) {
        /* Wait for PLLRDY */
    }
    
    if (timeout == 0) {
        return -2;  /* PLL failed to lock */
    }
    
    /* Configure bus prescalers */
    RCC_CFGR = (RCC_CFGR & ~0x0000FF00) |
               (0 << 4)  |   /* AHB  prescaler = 1   */
               (5 << 10) |   /* APB1 prescaler = 4   */
               (4 << 13);    /* APB2 prescaler = 2   */
    
    /* Switch system clock to PLL */
    RCC_CFGR = (RCC_CFGR & ~0x3) | 0x2;
    
    /* Wait for PLL to be used as system clock */
    timeout = 100000;
    while (((RCC_CFGR >> 2) & 0x3) != 0x2 && timeout--) {
        /* Wait for SWS */
    }
    
    if (timeout == 0) {
        return -3;  /* Failed to switch to PLL */
    }
    
    return 0;  /* Success */
}

/**
 * @brief Initialize GPIO for onboard LED (PD12 on STM32F4 Discovery)
 */
static void gpio_init(void)
{
    /* Enable GPIOD clock */
    RCC_AHB1ENR |= (1 << 3);
    
    /* Configure PD12-PD15 as output (LEDs) */
    GPIOD_MODER &= ~(0xFF << 24);
    GPIOD_MODER |= (0x55 << 24);  /* Output mode */
    
    /* Turn off all LEDs initially */
    GPIOD_ODR &= ~(0xF << 12);
}

/**
 * @brief Initialize SysTick timer for 1ms interrupts
 * @param cpu_freq CPU frequency in Hz
 */
static void systick_init(uint32_t cpu_freq)
{
    /* Configure SysTick for 1ms interrupts */
    uint32_t reload_value = (cpu_freq / 1000) - 1;
    
    SysTick->LOAD = reload_value;
    SysTick->VAL = 0;
    SysTick->CTRL = (1 << 2) |  /* Use processor clock */
                    (1 << 1) |  /* Enable interrupt */
                    (1 << 0);   /* Enable counter */
}

/**
 * @brief Full board initialization
 * 
 * This function should be called early in main() before starting
 * the RTOS. It configures clocks, GPIO, and essential peripherals.
 * 
 * @return 0 on success, negative on error
 */
int board_init(void)
{
    int ret;
    
    if (board_initialized) {
        return 0;  /* Already initialized */
    }
    
    /* Initialize system clock */
    ret = clock_init();
    if (ret < 0) {
        return ret;
    }
    
    /* Initialize GPIO */
    gpio_init();
    
    /* Initialize SysTick */
    systick_init(CPU_FREQ_HZ);
    
    /* Additional peripherals can be initialized here:
     * - UART for console
     * - Timers
     * - DMA
     * - ADC/DAC
     * - Communication interfaces (SPI, I2C, CAN)
     */
    
    board_initialized = true;
    return 0;
}

/**
 * @brief Get configured CPU frequency
 * @return CPU frequency in Hz
 */
uint32_t board_get_cpu_freq(void)
{
    return CPU_FREQ_HZ;
}

/**
 * @brief Get APB1 peripheral frequency
 * @return APB1 frequency in Hz
 */
uint32_t board_get_apb1_freq(void)
{
    return APB1_FREQ_HZ;
}

/**
 * @brief Get APB2 peripheral frequency
 * @return APB2 frequency in Hz
 */
uint32_t board_get_apb2_freq(void)
{
    return APB2_FREQ_HZ;
}

/**
 * @brief Set LED state
 * @param led LED number (0-3)
 * @param state true = on, false = off
 */
void board_set_led(uint8_t led, bool state)
{
    if (led > 3) return;
    
    if (state) {
        GPIOD_ODR |= (1 << (12 + led));
    } else {
        GPIOD_ODR &= ~(1 << (12 + led));
    }
}

/**
 * @brief Toggle LED state
 * @param led LED number (0-3)
 */
void board_toggle_led(uint8_t led)
{
    if (led > 3) return;
    GPIOD_ODR ^= (1 << (12 + led));
}

/**
 * @brief Board-specific delay in microseconds
 * @param us Delay in microseconds
 * 
 * Note: This is a busy-wait delay and should not be used in
 * an RTOS context. Use RTOS delay functions instead.
 */
void board_delay_us(uint32_t us)
{
    /* Approximate delay based on CPU frequency */
    /* Adjust the divisor based on your actual timing */
    uint32_t cycles = (CPU_FREQ_HZ / 1000000) * us / 4;
    
    while (cycles--) {
        __asm__ volatile ("nop");
    }
}

/**
 * @brief Board-specific delay in milliseconds
 * @param ms Delay in milliseconds
 * 
 * Note: For delays >1ms in RTOS context, use RTOS delay functions.
 */
void board_delay_ms(uint32_t ms)
{
    board_delay_us(ms * 1000);
}
