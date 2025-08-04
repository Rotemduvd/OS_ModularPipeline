#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include "monitor.h"

monitor_t test_monitor;

void* signal_before_wait_thread(void* arg) {
    printf("[Signal-before] Sending signal BEFORE wait...\n");
    monitor_signal(&test_monitor);
    return NULL;
}

void* wait_thread(void* arg) {
    printf("[Wait] Starting wait...\n");
    int result = monitor_wait(&test_monitor);
    printf("[Wait] Wait completed with result: %d\n", result);
    return NULL;
}

void* delayed_signal_thread(void* arg) {
    sleep(1); // Short delay
    printf("[Delayed-Signal] Sending signal AFTER wait...\n");
    monitor_signal(&test_monitor);
    return NULL;
}

int test_signal_before_wait() {
    printf("\nTesting signal before wait...\n");
    monitor_reset(&test_monitor);
    
    pthread_t signal_thread, wait_thread_id;
    
    pthread_create(&signal_thread, NULL, signal_before_wait_thread, NULL);
    sleep(1); // Ensure signal is sent first
    pthread_create(&wait_thread_id, NULL, wait_thread, NULL);
    
    pthread_join(signal_thread, NULL);
    pthread_join(wait_thread_id, NULL);
    
    return 1;
}

int test_wait_then_signal() {
    printf("\nTesting wait then signal...\n");
    monitor_reset(&test_monitor);
    
    pthread_t wait_thread_id, signal_thread;
    
    pthread_create(&wait_thread_id, NULL, wait_thread, NULL);
    pthread_create(&signal_thread, NULL, delayed_signal_thread, NULL);
    
    pthread_join(wait_thread_id, NULL);
    pthread_join(signal_thread, NULL);
    
    return 1;
}

int test_reset() {
    printf("\nTesting reset functionality...\n");
    monitor_signal(&test_monitor);  // Set signaled=1
    monitor_reset(&test_monitor);   // Clear it

    // Spawn a delayed signal thread to wake us after reset
    pthread_t signal_thread;
    pthread_create(&signal_thread, NULL, delayed_signal_thread, NULL);

    int result = monitor_wait(&test_monitor); // Wait should now block until signaled again
    pthread_join(signal_thread, NULL);

    printf("Wait after reset returned: %d (expected 0)\n", result);
    return result == 0;
}

int test_multiple_waiters() {
    printf("\nTesting multiple waiters...\n");
    monitor_reset(&test_monitor);

    pthread_t waiter1, waiter2;

    // Create two waiter threads
    pthread_create(&waiter1, NULL, wait_thread, NULL);
    pthread_create(&waiter2, NULL, wait_thread, NULL);

    sleep(1); // Ensure both are waiting

    printf("[Main] Broadcasting signal to wake all waiters...\n");
    monitor_signal(&test_monitor); // Should wake both waiters

    pthread_join(waiter1, NULL);
    pthread_join(waiter2, NULL);

    return 1; // If both join successfully, test passed
}

int test_signal_without_waiters() {
    printf("\nTesting signal without waiters (stateful)...\n");
    monitor_reset(&test_monitor);

    // Signal without any waiters
    monitor_signal(&test_monitor);
    printf("[Main] Signal sent with no waiters.\n");

    // Now call wait - should immediately pass
    int result = monitor_wait(&test_monitor);
    printf("Wait after signal without waiters returned: %d (expected 0)\n", result);

    return result == 0;
}

int test_multiple_signals() {
    printf("\nTesting multiple signals...\n");
    monitor_reset(&test_monitor);

    // Send signal twice
    monitor_signal(&test_monitor);
    monitor_signal(&test_monitor); // Second signal should not break anything

    // Wait should pass immediately
    int result = monitor_wait(&test_monitor);
    printf("Wait after multiple signals returned: %d (expected 0)\n", result);

    return result == 0;
}

int test_reset_without_signal() {
    printf("\nTesting reset without prior signal...\n");
    monitor_reset(&test_monitor); // Reset when already clear
    printf("Reset performed on unsignaled monitor.\n");

    // Spawn delayed signal and wait
    pthread_t signal_thread;
    pthread_create(&signal_thread, NULL, delayed_signal_thread, NULL);

    int result = monitor_wait(&test_monitor);
    pthread_join(signal_thread, NULL);

    printf("Wait after reset-without-signal returned: %d (expected 0)\n", result);
    return result == 0;
}

int main() {
    printf("Starting monitor tests...\n");
    
    if (monitor_init(&test_monitor) != 0) {
        printf("Failed to initialize monitor\n");
        return 1;
    }

    int tests_passed = 0;
    int total_tests = 7;

    tests_passed += test_signal_before_wait();
    tests_passed += test_wait_then_signal();
    tests_passed += test_reset();
    tests_passed += test_multiple_waiters();
    tests_passed += test_signal_without_waiters();
    tests_passed += test_multiple_signals();
    tests_passed += test_reset_without_signal();

    printf("\nTest Results: %d/%d tests passed\n", tests_passed, total_tests);
    
    monitor_destroy(&test_monitor);
    return (tests_passed == total_tests) ? 0 : 1;
}