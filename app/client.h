#ifndef CLIENT_H
#define CLIENT_H

#include <xcb/xcb.h>
#include <xcb/render.h>
#include <xcb/damage.h>

typedef struct entity {
    xcb_window_t id;
    xcb_pixmap_t pixmap;
    xcb_render_picture_t picture;
} entity;

typedef struct client {
    xcb_window_t frame;
    xcb_window_t toolbar;
    xcb_window_t child;
    xcb_rectangle_t shape;
    xcb_damage_damage_t damage;
    entity window; 
    entity frame_v; 
    entity mask;
    entity bar; 

    void draw(xcb_render_picture_t buffer);

} client;

extern client *active_client;             
extern client *current_resizing_client;
extern bool is_resizing, is_moving;
extern int start_x, start_y;


void remove_client(xcb_generic_event_t *event);
void add_client(xcb_generic_event_t *event);
client* find_client(xcb_window_t window);
void client_button_press(xcb_generic_event_t *event);
void client_button_release(xcb_generic_event_t *event);
void client_motion_handle(xcb_generic_event_t *event);

void client_find_grid_pos(int32_t *x, int32_t *y);

#endif
