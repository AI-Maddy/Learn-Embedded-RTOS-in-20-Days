/**
 * @file hello_task.c
 * @brief Minimal Zephyr "Hello World" example
 * 
 * Demonstrates Zephyr thread creation with K_THREAD_DEFINE
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#define THREAD_STACK_SIZE  1024
#define THREAD_PRIORITY    5

/**
 * @brief Hello thread function
 */
void hello_thread(void *arg1, void *arg2, void *arg3)
{
    uint32_t counter = 0;
    
    while (1) {
        printk("[%u] Hello from Zephyr!\n", counter++);
        k_msleep(1000);  /* Sleep for 1 second */
    }
}

/* Define thread statically */
K_THREAD_DEFINE(hello_tid, THREAD_STACK_SIZE,
                hello_thread, NULL, NULL, NULL,
                THREAD_PRIORITY, 0, 0);

void main(void)
{
    printk("\nZephyr Hello Thread Example\n");
    printk("============================\n\n");
    printk("Main thread: Thread started\n");
    
    /* Main thread can do other work or sleep */
    while (1) {
        k_msleep(5000);
    }
}
