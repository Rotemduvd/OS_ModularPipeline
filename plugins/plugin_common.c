#include "plugin_common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static plugin_context_t pg; // single plugin context

// generic consumer thread
void* plugin_consumer_thread(void* arg) {
    plugin_context_t* c = (plugin_context_t*)arg;
    while (1) {
        char* item = consumer_producer_get(c->queue); // get next item
        if (!item) break; // finished signal

        if (strcmp(item, "<END>") == 0) { // check end signal
            free(item);
            if (c->next_place_work) c->next_place_work("<END>");
            consumer_producer_signal_finished(c->queue);
            c->finished = 1;
            break;
        }

        const char* processed = c->process_function(item); // process item
        free(item); // free original string

        if (c->next_place_work && processed) {
            c->next_place_work(processed); // send to next plugin
        }
    }
    return NULL;
}

// log error
void log_error(plugin_context_t* c, const char* msg) {
    if (!c || !msg) return;
    printf("[ERROR][%s] - %s\n", c->name, msg);
}

// log info
void log_info(plugin_context_t* c, const char* msg) {
    if (!c || !msg) return;
    printf("[INFO][%s] - %s\n", c->name, msg);
}

// get plugin name
const char* plugin_get_name(void) {
    return pg.name;
}

// init common plugin
const char* common_plugin_init(const char* (*proc)(const char*), const char* name, int queue_size) {
    if (!proc || !name || queue_size <= 0) return "invalid args";

    pg.name = name;
    pg.process_function = proc;
    pg.initialized = 0;
    pg.finished = 0;

    pg.queue = (consumer_producer_t*)malloc(sizeof(consumer_producer_t));
    if (!pg.queue) return "malloc failed";

    const char* err = consumer_producer_init(pg.queue, queue_size);
    if (err) return err;

    if (pthread_create(&pg.consumer_thread, NULL, plugin_consumer_thread, &pg) != 0) {
        return "thread create failed";
    }

    pg.initialized = 1;
    return NULL;
}

// plugin init wrapper
const char* plugin_init(int queue_size) {
    return common_plugin_init(NULL, NULL, queue_size); // plugins replace this with real init
}

// finalize plugin
const char* plugin_fini(void) {
    if (!pg.initialized) return "not initialized";
    pthread_join(pg.consumer_thread, NULL); // wait for thread
    consumer_producer_destroy(pg.queue); // destroy queue
    free(pg.queue); // free struct
    pg.initialized = 0;
    return NULL;
}

// place work
const char* plugin_place_work(const char* str) {
    if (!pg.initialized) return "not initialized";
    return consumer_producer_put(pg.queue, str);
}

// attach next plugin
void plugin_attach(const char* (*next)(const char*)) {
    pg.next_place_work = next;
}

// wait for finish
const char* plugin_wait_finished(void) {
    if (!pg.initialized) return "not initialized";
    consumer_producer_wait_finished(pg.queue);
    return NULL;
}