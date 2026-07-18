#include "ember_event_bus.h"
#include <string.h>

#define MAX_SUBSCRIBERS 16

typedef struct {
    EmberEventType      type;
    EmberEventCallback  callback;
} Subscriber;

static Subscriber subscribers[MAX_SUBSCRIBERS];
static int        sub_count = 0;

void ember_bus_init(void) {
    sub_count = 0;
}

void ember_bus_subscribe(EmberEventType type, EmberEventCallback cb) {
    if (sub_count < MAX_SUBSCRIBERS) {
        subscribers[sub_count].type     = type;
        subscribers[sub_count].callback = cb;
        sub_count++;
    }
}

void ember_bus_publish(const EmberEvent *ev) {
    for (int i = 0; i < sub_count; i++) {
        if (subscribers[i].type == ev->type) {
            subscribers[i].callback(ev);
        }
    }
}