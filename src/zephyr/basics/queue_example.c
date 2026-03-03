/**
 * @file queue_example.c
 * @brief Zephyr message queue example
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#define MSGQ_MAX_MSGS  10
#define STACK_SIZE     1024

K_MSGQ_DEFINE(data_msgq, sizeof(uint32_t), MSGQ_MAX_MSGS, 4);

void producer_thread(void *a, void *b, void *c)
{
    uint32_t data = 0;
    
    while (1) {
        if (k_msgq_put(&data_msgq, &data, K_MSEC(100)) == 0) {
            printk("[Producer] Sent: %u\n", data);
            data++;
        } else {
            printk("[Producer] Queue full!\n");
        }
        k_msleep(500);
    }
}

void consumer_thread(void *a, void *b, void *c)
{
    uint32_t data;
    
    while (1) {
        if (k_msgq_get(&data_msgq, &data, K_MSEC(1000)) == 0) {
            printk("  [Consumer] Received: %u\n", data);
        }
    }
}

K_THREAD_DEFINE(prod_tid, STACK_SIZE, producer_thread,
                NULL, NULL, NULL, 5, 0, 0);
K_THREAD_DEFINE(cons_tid, STACK_SIZE, consumer_thread,
                NULL, NULL, NULL, 5, 0, 0);

void main(void)
{
    printk("\nZephyr Message Queue Example\n\n");
}
