/**
 * @file producer_consumer.c
 * @brief Zephyr producer-consumer pattern with message queues
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/random/random.h>

#define MSGQ_SIZE      20
#define STACK_SIZE     2048
#define NUM_PRODUCERS  2
#define NUM_CONSUMERS  2

typedef struct {
    uint32_t producer_id;
    uint32_t sequence;
    uint32_t data;
} data_item_t;

K_MSGQ_DEFINE(work_queue, sizeof(data_item_t), MSGQ_SIZE, 4);
K_MUTEX_DEFINE(stats_mutex);

static struct {
    uint32_t produced;
    uint32_t consumed;
} stats;

void producer_thread(void *p1, void *p2, void *p3)
{
    uint32_t id = (uint32_t)p1;
    uint32_t seq = 0;
    data_item_t item;
    
    while (1) {
        item.producer_id = id;
        item.sequence = seq++;
        item.data = sys_rand32_get();
        
        if (k_msgq_put(&work_queue, &item, K_MSEC(100)) == 0) {
            k_mutex_lock(&stats_mutex, K_FOREVER);
            stats.produced++;
            if (seq % 10 == 0) {
                printk("[Prod %u] Sent #%u\n", id, seq);
            }
            k_mutex_unlock(&stats_mutex);
        }
        
        k_msleep(100 + (sys_rand32_get() % 200));
    }
}

void consumer_thread(void *p1, void *p2, void *p3)
{
    uint32_t id = (uint32_t)p1;
    data_item_t item;
    uint32_t count = 0;
    
    while (1) {
        if (k_msgq_get(&work_queue, &item, K_MSEC(500)) == 0) {
            count++;
            k_mutex_lock(&stats_mutex, K_FOREVER);
            stats.consumed++;
            if (count % 10 == 0) {
                printk("  [Cons %u] Processed #%u from Prod %u\n",
                       id, count, item.producer_id);
            }
            k_mutex_unlock(&stats_mutex);
            
            k_msleep(50 + (sys_rand32_get() % 150));
        }
    }
}

K_THREAD_DEFINE(prod1, STACK_SIZE, producer_thread,
                (void *)1, NULL, NULL, 5, 0, 0);
K_THREAD_DEFINE(prod2, STACK_SIZE, producer_thread,
                (void *)2, NULL, NULL, 5, 0, 0);
K_THREAD_DEFINE(cons1, STACK_SIZE, consumer_thread,
                (void *)1, NULL, NULL, 5, 0, 0);
K_THREAD_DEFINE(cons2, STACK_SIZE, consumer_thread,
                (void *)2, NULL, NULL, 5, 0, 0);

void main(void)
{
    printk("\nZephyr Producer-Consumer Pattern\n");
    printk("Producers: %d, Consumers: %d\n\n", NUM_PRODUCERS, NUM_CONSUMERS);
    
    while (1) {
        k_msleep(5000);
        k_mutex_lock(&stats_mutex, K_FOREVER);
        printk("\n=== Stats: Produced=%u, Consumed=%u ===\n\n",
               stats.produced, stats.consumed);
        k_mutex_unlock(&stats_mutex);
    }
}
