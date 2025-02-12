#ifndef CLIENT_H
#define CLIENT_H

#include <xcb/xcb.h>

typedef struct client {
    xcb_window_t frame;
    xcb_window_t child;
    int fixed;
    int resizing; 
} client;

extern client *current_resizing_client;

void remove_client(xcb_generic_event_t *event);
void add_client(xcb_window_t client);

#endif
