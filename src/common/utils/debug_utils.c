/**
 * @file debug_utils.c
 * @brief Debug logging and diagnostics utilities
 * 
 * This module provides debug printf with log levels, assertions,
 * memory dumping, and runtime statistics collection.
 */

#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include "debug_utils.h"
#include "uart_driver.h"

/* Configuration */
#ifndef DEBUG_BUFFER_SIZE
#define DEBUG_BUFFER_SIZE  256
#endif

#ifndef DEBUG_ENABLED
#define DEBUG_ENABLED  1
#endif

/* Current debug level */
static debug_level_t current_level = DEBUG_LEVEL_INFO;

/* UART handle for debug output */
static uart_handle_t *debug_uart = NULL;

/* ANSI color codes */
#ifdef DEBUG_USE_COLOR
#define COLOR_RESET   "\033[0m"
#define COLOR_ERROR   "\033[1;31m"  /* Bold Red */
#define COLOR_WARN    "\033[1;33m"  /* Bold Yellow */
#define COLOR_INFO    "\033[1;32m"  /* Bold Green */
#define COLOR_DEBUG   "\033[1;36m"  /* Bold Cyan */
#define COLOR_TRACE   "\033[0;37m"  /* White */
#else
#define COLOR_RESET   ""
#define COLOR_ERROR   ""
#define COLOR_WARN    ""
#define COLOR_INFO    ""
#define COLOR_DEBUG   ""
#define COLOR_TRACE   ""
#endif

/* Level name strings */
static const char *level_names[] = {
    "NONE",
    "ERROR",
    "WARN",
    "INFO",
    "DEBUG",
    "TRACE"
};

/* Level colors */
static const char *level_colors[] = {
    COLOR_RESET,
    COLOR_ERROR,
    COLOR_WARN,
    COLOR_INFO,
    COLOR_DEBUG,
    COLOR_TRACE
};

/**
 * @brief Initialize debug utilities
 * @param uart UART handle for debug output
 * @param level Initial debug level
 */
void debug_init(uart_handle_t *uart, debug_level_t level)
{
    debug_uart = uart;
    current_level = level;
}

/**
 * @brief Set debug output level
 * @param level New debug level
 */
void debug_set_level(debug_level_t level)
{
    current_level = level;
}

/**
 * @brief Get current debug level
 * @return Current debug level
 */
debug_level_t debug_get_level(void)
{
    return current_level;
}

/**
 * @brief Output formatted debug message
 * @param level Message level
 * @param file Source file name
 * @param line Line number
 * @param func Function name
 * @param format Format string
 * @param ... Variable arguments
 */
void debug_printf_impl(debug_level_t level, const char *file, int line,
                       const char *func, const char *format, ...)
{
#if DEBUG_ENABLED
    if (!debug_uart || level > current_level) {
        return;
    }
    
    char buffer[DEBUG_BUFFER_SIZE];
    va_list args;
    int pos = 0;
    
    /* Get current timestamp (milliseconds) */
    uint32_t timestamp = board_get_tick();
    
    /* Format: [TIME] LEVEL [file:line:func] message */
    pos = snprintf(buffer, sizeof(buffer),
                   "%s[%6lu] %-5s [%s:%d:%s] ",
                   level_colors[level],
                   timestamp,
                   level_names[level],
                   file, line, func);
    
    /* Append user message */
    va_start(args, format);
    pos += vsnprintf(buffer + pos, sizeof(buffer) - pos, format, args);
    va_end(args);
    
    /* Add color reset */
    if (pos < sizeof(buffer) - sizeof(COLOR_RESET)) {
        strcpy(buffer + pos, COLOR_RESET);
        pos += strlen(COLOR_RESET);
    }
    
    /* Output to UART */
    uart_write(debug_uart, buffer, pos);
#endif
}

/**
 * @brief Dump memory contents in hexadecimal format
 * @param addr Starting address
 * @param len Number of bytes to dump
 * @param width Bytes per line (8 or 16)
 */
void debug_dump_memory(const void *addr, size_t len, uint8_t width)
{
#if DEBUG_ENABLED
    if (!debug_uart || !addr) {
        return;
    }
    
    const uint8_t *ptr = (const uint8_t *)addr;
    char line[128];
    
    debug_printf(DEBUG_LEVEL_INFO, "Memory dump @ 0x%08X (%u bytes):\n",
                 (uintptr_t)addr, len);
    
    for (size_t i = 0; i < len; i += width) {
        int pos = 0;
        
        /* Address */
        pos = snprintf(line, sizeof(line), "  %08X: ",
                      (uintptr_t)(ptr + i));
        
        /* Hex bytes */
        for (size_t j = 0; j < width && (i + j) < len; j++) {
            pos += snprintf(line + pos, sizeof(line) - pos,
                           "%02X ", ptr[i + j]);
        }
        
        /* Padding */
        for (size_t j = len - i; j < width && j < width; j++) {
            pos += snprintf(line + pos, sizeof(line) - pos, "   ");
        }
        
        pos += snprintf(line + pos, sizeof(line) - pos, " | ");
        
        /* ASCII repr */
        for (size_t j = 0; j < width && (i + j) < len; j++) {
            uint8_t c = ptr[i + j];
            line[pos++] = (c >= 32 && c < 127) ? c : '.';
        }
        
        line[pos++] = '\n';
        line[pos] = '\0';
        
        uart_write(debug_uart, line, pos);
    }
#endif
}

/**
 * @brief Assert function implementation
 * @param expr Expression that was evaluated
 * @param file Source file name
 * @param line Line number
 * @param func Function name
 */
void debug_assert_impl(const char *expr, const char *file, int line,
                       const char *func)
{
    debug_printf(DEBUG_LEVEL_ERROR,
                 "ASSERTION FAILED: %s\n  at %s:%d in %s()\n",
                 expr, file, line, func);
    
    /* Halt execution */
    while (1) {
        __asm__ volatile ("bkpt #0");  /* Breakpoint for debugger */
    }
}

/**
 * @brief Print task/thread statistics
 * @param task_list Array of task info structures
 * @param task_count Number of tasks
 */
void debug_print_task_stats(const task_info_t *task_list, size_t task_count)
{
#if DEBUG_ENABLED
    if (!debug_uart || !task_list) {
        return;
    }
    
    debug_printf(DEBUG_LEVEL_INFO, "\n=== Task Statistics ===\n");
    debug_printf(DEBUG_LEVEL_INFO,
                 "%-20s %4s %8s %8s %5s\n",
                 "Name", "Prio", "Stack", "Free", "CPU%%");
    debug_printf(DEBUG_LEVEL_INFO,
                 "-----------------------------------------------------------\n");
    
    for (size_t i = 0; i < task_count; i++) {
        const task_info_t *task = &task_list[i];
        debug_printf(DEBUG_LEVEL_INFO,
                     "%-20s %4d %8u %8u %5u\n",
                     task->name,
                     task->priority,
                     task->stack_size,
                     task->stack_free,
                     task->cpu_percent);
    }
    
    debug_printf(DEBUG_LEVEL_INFO,
                 "=======================\n\n");
#endif
}

/**
 * @brief Print system resource usage
 * @param heap_used Heap bytes used
 * @param heap_total Total heap size
 * @param cpu_usage CPU usage percentage
 */
void debug_print_system_info(uint32_t heap_used, uint32_t heap_total,
                             uint8_t cpu_usage)
{
#if DEBUG_ENABLED
    if (!debug_uart) {
        return;
    }
    
    debug_printf(DEBUG_LEVEL_INFO, "\n=== System Information ===\n");
    debug_printf(DEBUG_LEVEL_INFO, "  Uptime:      %lu ms\n", board_get_tick());
    debug_printf(DEBUG_LEVEL_INFO, "  CPU Usage:   %u%%\n", cpu_usage);
    debug_printf(DEBUG_LEVEL_INFO, "  Heap Used:   %lu / %lu bytes (%u%%)\n",
                 heap_used, heap_total,
                 (uint32_t)(((uint64_t)heap_used * 100) / heap_total));
    debug_printf(DEBUG_LEVEL_INFO, "  Heap Free:   %lu bytes\n",
                 heap_total - heap_used);
    debug_printf(DEBUG_LEVEL_INFO, "===========================\n\n");
#endif
}

/**
 * @brief Convert error code to string
 * @param error Error code
 * @return Error string
 */
const char *debug_error_to_string(int error)
{
    switch (error) {
        case 0:    return "Success";
        case -1:   return "General error";
        case -2:   return "Invalid parameter";
        case -3:   return "Out of memory";
        case -4:   return "Timeout";
        case -5:   return "Busy";
        case -6:   return "Not supported";
        case -7:   return "Hardware error";
        case -8:   return "Not initialized";
        case -9:   return "Already initialized";
        case -10:  return "Queue full";
        case -11:  return "Queue empty";
        default:   return "Unknown error";
    }
}

/**
 * @brief Print formatted error message
 * @param function Function name where error occurred
 * @param error Error code
 */
void debug_print_error(const char *function, int error)
{
    debug_printf(DEBUG_LEVEL_ERROR,
                 "%s() failed: %s (code %d)\n",
                 function, debug_error_to_string(error), error);
}

/**
 * @brief Hexdump-style memory output
 * @param label Description label
 * @param data Pointer to data
 * @param len Data length
 */
void debug_hexdump(const char *label, const void *data, size_t len)
{
#if DEBUG_ENABLED
    if (!debug_uart) {
        return;
    }
    
    debug_printf(DEBUG_LEVEL_DEBUG, "%s (%u bytes):\n", label, len);
    debug_dump_memory(data, len, 16);
#endif
}
