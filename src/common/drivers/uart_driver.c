/**
 * @file uart_driver.c
 * @brief Buffered UART driver with interrupt support
 * 
 * This driver provides a portable UART interface with TX/RX ring buffers,
 * interrupt-driven operation, and non-blocking APIs.
 * 
 * Features:
 * - Interrupt-driven TX/RX
 * - Ring buffer for TX and RX
 * - Non-blocking read/write
 * - Printf-style formatted output
 * - Multiple UART instance support
 */

#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>
#include "uart_driver.h"

/* Ring buffer configuration */
#ifndef UART_RX_BUFFER_SIZE
#define UART_RX_BUFFER_SIZE  256
#endif

#ifndef UART_TX_BUFFER_SIZE
#define UART_TX_BUFFER_SIZE  512
#endif

/* Ring buffer structure */
typedef struct {
    uint8_t buffer[UART_RX_BUFFER_SIZE];
    volatile uint16_t head;
    volatile uint16_t tail;
    volatile uint16_t count;
} ring_buffer_t;

/* UART handle structure */
struct uart_handle {
    void *base;              /* UART peripheral base address */
    uint32_t baudrate;
    ring_buffer_t rx_buf;
    ring_buffer_t tx_buf;
    bool initialized;
};

/* Static UART handles (adjust based on your MCU) */
static uart_handle_t uart_handles[UART_MAX_INSTANCES];

/* Ring buffer helper functions */
static inline bool ring_buffer_empty(const ring_buffer_t *rb)
{
    return rb->count == 0;
}

static inline bool ring_buffer_full(const ring_buffer_t *rb)
{
    return rb->count >= UART_RX_BUFFER_SIZE;
}

static inline void ring_buffer_put(ring_buffer_t *rb, uint8_t data)
{
    if (!ring_buffer_full(rb)) {
        rb->buffer[rb->head] = data;
        rb->head = (rb->head + 1) % UART_RX_BUFFER_SIZE;
        rb->count++;
    }
}

static inline uint8_t ring_buffer_get(ring_buffer_t *rb)
{
    uint8_t data = 0;
    
    if (!ring_buffer_empty(rb)) {
        data = rb->buffer[rb->tail];
        rb->tail = (rb->tail + 1) % UART_RX_BUFFER_SIZE;
        rb->count--;
    }
    
    return data;
}

/**
 * @brief Initialize UART peripheral
 * @param handle Pointer to UART handle
 * @param base UART peripheral base address
 * @param config Pointer to configuration structure
 * @return 0 on success, negative on error
 */
int uart_init(uart_handle_t *handle, void *base, const uart_config_t *config)
{
    if (!handle || !base || !config) {
        return -1;
    }
    
    /* Initialize handle */
    memset(handle, 0, sizeof(uart_handle_t));
    handle->base = base;
    handle->baudrate = config->baudrate;
    
    /* Initialize ring buffers */
    handle->rx_buf.head = 0;
    handle->rx_buf.tail = 0;
    handle->rx_buf.count = 0;
    handle->tx_buf.head = 0;
    handle->tx_buf.tail = 0;
    handle->tx_buf.count = 0;
    
    /* Hardware-specific initialization would go here:
     * 1. Enable UART clock
     * 2. Configure GPIO pins (TX, RX)
     * 3. Set baudrate
     * 4. Configure frame format (data bits, parity, stop bits)
     * 5. Enable UART
     * 6. Enable RX interrupt
     * 
     * Example for STM32:
     * RCC->APB1ENR |= RCC_APB1ENR_USART2EN;
     * USART2->BRR = calculate_brr(baudrate);
     * USART2->CR1 = USART_CR1_TE | USART_CR1_RE | USART_CR1_RXNEIE;
     * USART2->CR1 |= USART_CR1_UE;
     * NVIC_EnableIRQ(USART2_IRQn);
     */
    
    handle->initialized = true;
    return 0;
}

/**
 * @brief Write data to UART (non-blocking)
 * @param handle UART handle
 * @param data Pointer to data buffer
 * @param len Number of bytes to write
 * @return Number of bytes actually written, negative on error
 */
int uart_write(uart_handle_t *handle, const void *data, size_t len)
{
    if (!handle || !handle->initialized || !data) {
        return -1;
    }
    
    const uint8_t *ptr = (const uint8_t *)data;
    size_t written = 0;
    
    /* Critical section - disable interrupts */
    __disable_irq();
    
    for (size_t i = 0; i < len; i++) {
        if (!ring_buffer_full(&handle->tx_buf)) {
            ring_buffer_put(&handle->tx_buf, ptr[i]);
            written++;
        } else {
            break;  /* Buffer full */
        }
    }
    
    /* Enable TX interrupt to start transmission */
    /* Hardware-specific: enable TXE interrupt
     * Example: USART2->CR1 |= USART_CR1_TXEIE;
     */
    
    __enable_irq();
    
    return written;
}

/**
 * @brief Read data from UART (non-blocking)
 * @param handle UART handle
 * @param buffer Pointer to buffer to store received data
 * @param len Maximum number of bytes to read
 * @return Number of bytes actually read, negative on error
 */
int uart_read(uart_handle_t *handle, void *buffer, size_t len)
{
    if (!handle || !handle->initialized || !buffer) {
        return -1;
    }
    
    uint8_t *ptr = (uint8_t *)buffer;
    size_t read = 0;
    
    /* Critical section */
    __disable_irq();
    
    while (read < len && !ring_buffer_empty(&handle->rx_buf)) {
        ptr[read++] = ring_buffer_get(&handle->rx_buf);
    }
    
    __enable_irq();
    
    return read;
}

/**
 * @brief Get number of bytes available in RX buffer
 * @param handle UART handle
 * @return Number of bytes available
 */
int uart_available(uart_handle_t *handle)
{
    if (!handle || !handle->initialized) {
        return -1;
    }
    
    return handle->rx_buf.count;
}

/**
 * @brief Check if TX buffer is empty (transmission complete)
 * @param handle UART handle
 * @return true if TX buffer is empty
 */
bool uart_tx_done(uart_handle_t *handle)
{
    if (!handle || !handle->initialized) {
        return true;
    }
    
    return ring_buffer_empty(&handle->tx_buf);
}

/**
 * @brief Flush TX buffer (wait for transmission to complete)
 * @param handle UART handle
 * @param timeout_ms Timeout in milliseconds (0 = no timeout)
 * @return 0 on success, -1 on timeout
 */
int uart_flush(uart_handle_t *handle, uint32_t timeout_ms)
{
    if (!handle || !handle->initialized) {
        return -1;
    }
    
    uint32_t start = board_get_tick();  /* Get current tick */
    
    while (!uart_tx_done(handle)) {
        if (timeout_ms > 0) {
            uint32_t elapsed = board_get_tick() - start;
            if (elapsed > timeout_ms) {
                return -1;  /* Timeout */
            }
        }
    }
    
    return 0;
}

/**
 * @brief Formatted output to UART (like printf)
 * @param handle UART handle
 * @param format Format string
 * @param ... Variable arguments
 * @return Number of bytes written
 */
int uart_printf(uart_handle_t *handle, const char *format, ...)
{
    char buffer[256];  /* Adjust size as needed */
    va_list args;
    int len;
    
    va_start(args, format);
    len = vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    if (len > 0) {
        if (len > sizeof(buffer) - 1) {
            len = sizeof(buffer) - 1;
        }
        return uart_write(handle, buffer, len);
    }
    
    return 0;
}

/**
 * @brief UART RX interrupt handler
 * 
 * This function should be called from the UART RX interrupt service routine.
 * It reads the received byte and stores it in the ring buffer.
 * 
 * @param handle UART handle
 */
void uart_rx_irq_handler(uart_handle_t *handle)
{
    if (!handle || !handle->initialized) {
        return;
    }
    
    /* Hardware-specific: read data register
     * Example: uint8_t data = (uint8_t)(USART2->DR & 0xFF);
     */
    uint8_t data = 0;  /* Read from hardware register */
    
    /* Store in RX buffer */
    if (!ring_buffer_full(&handle->rx_buf)) {
        ring_buffer_put(&handle->rx_buf, data);
    }
    /* If buffer is full, data is lost - could set overflow flag */
}

/**
 * @brief UART TX interrupt handler
 * 
 * This function should be called from the UART TX interrupt service routine.
 * It sends the next byte from the TX ring buffer.
 * 
 * @param handle UART handle
 */
void uart_tx_irq_handler(uart_handle_t *handle)
{
    if (!handle || !handle->initialized) {
        return;
    }
    
    if (!ring_buffer_empty(&handle->tx_buf)) {
        uint8_t data = ring_buffer_get(&handle->tx_buf);
        
        /* Hardware-specific: write to data register
         * Example: USART2->DR = data;
         */
    } else {
        /* No more data to send, disable TX interrupt
         * Example: USART2->CR1 &= ~USART_CR1_TXEIE;
         */
    }
}

/**
 * @brief Example ISR integration (STM32)
 * 
 * In your startup code or interrupt vector table, you would have:
 * 
 * void USART2_IRQHandler(void)
 * {
 *     uart_handle_t *uart = &uart_handles[UART_2];
 *     
 *     // Check for RX interrupt
 *     if (USART2->SR & USART_SR_RXNE) {
 *         uart_rx_irq_handler(uart);
 *     }
 *     
 *     // Check for TX interrupt  
 *     if (USART2->SR & USART_SR_TXE) {
 *         uart_tx_irq_handler(uart);
 *     }
 * }
 */

/**
 * @brief Get UART handle by index
 * @param index UART instance index (0 to UART_MAX_INSTANCES-1)
 * @return Pointer to UART handle, NULL on error
 */
uart_handle_t *uart_get_handle(uint8_t index)
{
    if (index >= UART_MAX_INSTANCES) {
        return NULL;
    }
    
    return &uart_handles[index];
}
