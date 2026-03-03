/**
 * @file semaphore_example.c
 * @brief Zephyr semaphore synchronization example
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#define STACK_SIZE  1024

K_SEM_DEFINE(my_sem, 0, 1);  /* Binary semaphore */
K_MUTEX_DEFINE(my_mutex);

static uint32_t shared_counter = 0;

void signaling_thread(void *a, void *b, void *c)
{
    uint32_t count = 0;
    
    while (1) {
        k_msleep(1000);
        printk("[Signal] Giving semaphore %u\n", count++);
        k_sem_give(&my_sem);
    }
}

void waiting_thread(void *a, void *b, void *c)
{
    while (1) {
        printk("  [Wait] Waiting...\n");
        k_sem_take(&my_sem, K_FOREVER);
        printk("  [Wait] Got semaphore!\n");
    }
}

void resource_thread(void *a, void *b, void *c)
{
    uint32_t id = (uint32_t)a;
    
    while (1) {
        k_mutex_lock(&my_mutex, K_FOREVER);
        printk("    [Task %u] Counter=%u\n", id, shared_counter++);
        k_mutex_unlock(&my_mutex);
        k_msleep(500);
    }
}

K_THREAD_DEFINE(sig_tid, STACK_SIZE, signaling_thread,
                NULL, NULL, NULL, 5, 0, 0);
K_THREAD_DEFINE(wait_tid, STACK_SIZE, waiting_thread,
                NULL, NULL, NULL, 5, 0, 0);
K_THREAD_DEFINE(res1_tid, STACK_SIZE, resource_thread,
                (void *)1, NULL, NULL, 5, 0, 0);
K_THREAD_DEFINE(res2_tid, STACK_SIZE, resource_thread,
                (void *)2, NULL, NULL, 5, 0, 0);

void main(void)
{
    printk("\nZephyr Semaphore Example\n\n");
}
