#ifndef CLIENT_H
#define CLIENT_H

#include <xcb/xcb.h>
#include <xcb/render.h>
#include <xcb/damage.h>
#include <string>

// #define TITLEBAR_HEIGHT 20
// #define TITLEBAR_PADDING 20
// #define TITLEBAR_RADIUS 8
// #define CLOSE_BUTTON_SIZE 15

typedef struct entity {
    xcb_window_t id;
    xcb_pixmap_t pixmap;
    xcb_render_picture_t picture;
} entity;

typedef struct titlebar_info {
    xcb_window_t id;
    int width;
    std::string title;

    xcb_window_t close_button;
} titlebar;

typedef struct client {
    entity window;
    xcb_rectangle_t shape;
    
    xcb_window_t frame;
    xcb_damage_damage_t damage;
    titlebar_info titlebar; 

    int width;
    int height;
    int x;
    int y;
    int fixed;
    int resizing; 
    void draw(xcb_render_picture_t buffer);
} client;


struct resize_context {
    client* target_client;
    int direction;
    int start_x, start_y;
    int start_width, start_height;
    bool active;
};

extern resize_context resize_ctx;

extern client *current_resizing_client;
extern bool is_resizing, is_moving;
extern int start_x, start_y;

void remove_client(xcb_generic_event_t *event);
void add_client(xcb_generic_event_t *event);
client* find_client(xcb_window_t window);
void client_button_press(xcb_generic_event_t *event);
void client_button_release(xcb_generic_event_t *event);void client_motion_handle(xcb_generic_event_t *event);

void client_find_grid_pos(int32_t *x, int32_t *y);

// Novas funções para barra de título
std::string get_window_title(xcb_window_t window);
void draw_titlebar(client *c, xcb_render_pictforminfo_t* format);
void create_titlebar(client *c);
void handle_titlebar_click(xcb_button_press_event_t *event);

#endif
