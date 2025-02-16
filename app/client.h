#ifndef CLIENT_H
#define CLIENT_H

#include <xcb/xcb.h>

typedef struct client {
    xcb_window_t frame;
    xcb_window_t child;
    int width;
    int height;
    int x;
    int y;
    int fixed;
    int resizing; 
} client;

extern client *current_resizing_client;
extern bool is_resizing, is_moving;
extern int start_x, start_y;

void remove_client(xcb_generic_event_t *event);
void add_client(xcb_generic_event_t *event);
void client_button_press(xcb_generic_event_t *event);
void client_button_release(xcb_generic_event_t *event);
void client_motion_handle(xcb_generic_event_t *event);

#endif
