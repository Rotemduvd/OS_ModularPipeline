#include <stdio.h>
#include <pthread.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include "consumer_producer.h"

// Thread args
typedef struct {
    consumer_producer_t* queue;
    int num_items;
    const char** items;
} thread_args_t;

/* === Helper Threads === */
void* producer_thread(void* arg) {
    thread_args_t* args = (thread_args_t*)arg;
    for (int i = 0; i < args->num_items; i++) {
        const char* result = consumer_producer_put(args->queue, args->items[i]);
        assert(result == NULL); // should succeed
    }
    return NULL;
}

void* consumer_thread(void* arg) {
    thread_args_t* args = (thread_args_t*)arg;
    for (int i = 0; i < args->num_items; i++) {
        char* item = consumer_producer_get(args->queue);
        assert(item != NULL);
        free(item); // must free strdup'd item
    }
    return NULL;
}

/* === TESTS === */

// 1. Initialization edge cases
void test_init() {
    printf("Testing initialization...\n");
    consumer_producer_t q;

    // Valid initialization
    assert(consumer_producer_init(&q, 5) == NULL);
    consumer_producer_destroy(&q);

    // Invalid init cases
    assert(consumer_producer_init(NULL, 5) != NULL);
    assert(consumer_producer_init(&q, 0) != NULL);
    assert(consumer_producer_init(&q, -1) != NULL);
}

// 2. Basic put/get operations
void test_basic_operations() {
    printf("Testing basic put/get...\n");
    consumer_producer_t q;
    consumer_producer_init(&q, 3);

    // Put one, get one
    assert(consumer_producer_put(&q, "test1") == NULL);
    char* item = consumer_producer_get(&q);
    assert(strcmp(item, "test1") == 0);
    free(item);

    consumer_producer_destroy(&q);
}

// 3. Full queue blocking
void test_full_queue() {
    printf("Testing full queue blocking...\n");
    consumer_producer_t q;
    consumer_producer_init(&q, 2);

    assert(consumer_producer_put(&q, "item1") == NULL);
    assert(consumer_producer_put(&q, "item2") == NULL);

    pthread_t producer;
    const char* test_item = "item3";
    thread_args_t args = {&q, 1, &test_item};
    pthread_create(&producer, NULL, producer_thread, &args);

    sleep(1); // ensure producer blocks

    char* item = consumer_producer_get(&q);
    assert(item != NULL);
    free(item);

    pthread_join(producer, NULL);
    consumer_producer_destroy(&q);
}

// 4. Empty queue blocking
void test_empty_queue() {
    printf("Testing empty queue blocking...\n");
    consumer_producer_t q;
    consumer_producer_init(&q, 2);

    pthread_t consumer;
    thread_args_t args = {&q, 1, NULL};
    pthread_create(&consumer, NULL, consumer_thread, &args);

    sleep(1); // ensure consumer blocks
    assert(consumer_producer_put(&q, "test_item") == NULL);

    pthread_join(consumer, NULL);
    consumer_producer_destroy(&q);
}

// 5. Multiple producers and consumers
void test_multiple_producers_consumers() {
    printf("Testing multiple producers/consumers...\n");
    consumer_producer_t q;
    consumer_producer_init(&q, 5);

    #define NUM_THREADS 3
    #define ITEMS_PER_THREAD 50
    pthread_t producers[NUM_THREADS], consumers[NUM_THREADS];
    const char* items[ITEMS_PER_THREAD];

    for (int i = 0; i < ITEMS_PER_THREAD; i++) items[i] = "item";

    thread_args_t args = {&q, ITEMS_PER_THREAD, items};

    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_create(&producers[i], NULL, producer_thread, &args);
        pthread_create(&consumers[i], NULL, consumer_thread, &args);
    }

    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(producers[i], NULL);
        pthread_join(consumers[i], NULL);
    }

    consumer_producer_destroy(&q);
}

// 6. Finished state behavior
void test_finished_state() {
    printf("Testing finished state...\n");
    consumer_producer_t q;
    consumer_producer_init(&q, 2);

    consumer_producer_signal_finished(&q);

    // After finished: put should fail, get should return NULL
    assert(strcmp(consumer_producer_put(&q, "test"), "queue finished") == 0);
    assert(consumer_producer_get(&q) == NULL);

    consumer_producer_destroy(&q);
}

// 7. Wait for finish
void test_wait_for_finish() {
    printf("Testing wait_for_finish...\n");
    consumer_producer_t q;
    consumer_producer_init(&q, 2);

    pthread_t producer;
    const char* items[] = {"a", "b"};
    thread_args_t args = {&q, 2, items};

    pthread_create(&producer, NULL, producer_thread, &args);
    pthread_join(producer, NULL);

    consumer_producer_signal_finished(&q);
    int res = consumer_producer_wait_finished(&q);
    assert(res == 0);

    consumer_producer_destroy(&q);
}

// 8. Error handling & null safety
void test_error_handling() {
    printf("Testing error handling and NULL safety...\n");
    consumer_producer_t q;
    consumer_producer_init(&q, 2);

    assert(strcmp(consumer_producer_put(NULL, "x"), "args are invalid") == 0);
    assert(strcmp(consumer_producer_put(&q, NULL), "args are invalid") == 0);
    assert(consumer_producer_get(NULL) == NULL);
    consumer_producer_signal_finished(NULL); // should not crash
    consumer_producer_destroy(&q);
}

// 9. Destroy with unconsumed items (memory safety)
void test_destroy_with_items() {
    printf("Testing destroy with unconsumed items...\n");
    consumer_producer_t q;
    consumer_producer_init(&q, 3);
    consumer_producer_put(&q, "x");
    consumer_producer_put(&q, "y");
    // Destroy without consuming - should free safely
    consumer_producer_destroy(&q);
}

// 10. Stress test (optional high load)
void test_stress() {
    printf("Stress testing with high load...\n");
    consumer_producer_t q;
    consumer_producer_init(&q, 50);

    #define BIG_THREADS 4
    #define BIG_ITEMS 1000
    pthread_t prod[BIG_THREADS], cons[BIG_THREADS];
    const char* items[BIG_ITEMS];
    for (int i = 0; i < BIG_ITEMS; i++) items[i] = "bulk";

    thread_args_t args = {&q, BIG_ITEMS, items};
    for (int i = 0; i < BIG_THREADS; i++) {
        pthread_create(&prod[i], NULL, producer_thread, &args);
        pthread_create(&cons[i], NULL, consumer_thread, &args);
    }

    for (int i = 0; i < BIG_THREADS; i++) {
        pthread_join(prod[i], NULL);
        pthread_join(cons[i], NULL);
    }

    consumer_producer_signal_finished(&q);
    consumer_producer_destroy(&q);
}


// 1. Capacity = 1 edge case (single-thread sanity)
void test_capacity_one_put_get() {
    printf("Testing capacity=1 put/get...\n");
    consumer_producer_t q;
    assert(consumer_producer_init(&q, 1) == NULL);

    assert(consumer_producer_put(&q, "A") == NULL);
    char* item = consumer_producer_get(&q);
    assert(strcmp(item, "A") == 0);
    free(item);

    consumer_producer_destroy(&q);
}

// 2. Two producers and two consumers (concurrency)
#define RUNS 1000
static void* simple_producer(void* arg) {
    consumer_producer_t* q = arg;
    char buf[16];
    for (int i = 0; i < RUNS; ++i) {
        snprintf(buf, sizeof buf, "%d", i);
        consumer_producer_put(q, buf);
    }
    return NULL;
}

static void* simple_consumer(void* arg) {
    consumer_producer_t* q = arg;
    int got = 0;
    while (got < RUNS) {
        char* s = consumer_producer_get(q);
        if (s) {
            ++got;
            free(s);
        }
    }
    return NULL;
}

void test_two_by_two_threads() {
    printf("Testing two producers and two consumers...\n");
    consumer_producer_t q;
    assert(consumer_producer_init(&q, 64) == NULL);

    pthread_t p1, p2, c1, c2;
    pthread_create(&p1, NULL, simple_producer, &q);
    pthread_create(&p2, NULL, simple_producer, &q);
    pthread_create(&c1, NULL, simple_consumer, &q);
    pthread_create(&c2, NULL, simple_consumer, &q);

    pthread_join(p1, NULL);
    pthread_join(p2, NULL);
    consumer_producer_signal_finished(&q); // let consumers exit
    pthread_join(c1, NULL);
    pthread_join(c2, NULL);

    consumer_producer_destroy(&q);
}

/* === MAIN === */
int main() {
    printf("Starting consumer-producer tests...\n\n");

    test_init();
    test_basic_operations();
    test_full_queue();
    test_empty_queue();
    test_multiple_producers_consumers();
    test_finished_state();
    test_wait_for_finish();
    test_error_handling();
    test_destroy_with_items();
    test_stress();
    test_capacity_one_put_get();
    test_two_by_two_threads();

    printf("\n🎉 All tests passed!\n");
    return 0;
}