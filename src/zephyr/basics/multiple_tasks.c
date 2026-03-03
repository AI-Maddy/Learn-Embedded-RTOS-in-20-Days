/**
 * @file multiple_tasks.c
 * @brief Zephyr example with multiple threads at different priorities
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#define STACK_SIZE  1024

/* Thread functions */
void low_priority_thread(void *a, void *b, void *c)
{
    uint32_t count = 0;
    while (1) {
        printk("[LOW]    Thread %u\n", count++);
        k_msleep(1000);
    }
}

void medium_priority_thread(void *a, void *b, void *c)
{
    uint32_t count = 0;
    while (1) {
        printk("  [MEDIUM] Thread %u\n", count++);
        k_msleep(500);
    }
}

void high_priority_thread(void *a, void *b, void *c)
{
    uint32_t count = 0;
    while (1) {
        printk("    [HIGH]   Thread %u\n", count++);
        k_msleep(2000);
    }
}

/* Define threads with different priorities */
K_THREAD_DEFINE(low_tid, STACK_SIZE, low_priority_thread,
                NULL, NULL, NULL, 7, 0, 0);
K_THREAD_DEFINE(med_tid, STACK_SIZE, medium_priority_thread,
                NULL, NULL, NULL, 5, 0, 0);
K_THREAD_DEFINE(high_tid, STACK_SIZE, high_priority_thread,
                NULL, NULL, NULL, 3, 0, 0);

void main(void)
{
    printk("\nZephyr Multiple Threads Example\n");
    printk("Priority: 3=high, 5=medium, 7=low\n\n");
}
