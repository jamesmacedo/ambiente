#ifndef CONFIG_H
#define CONFIG_H

#include <xcb/xcb.h>
#include <cairo/cairo.h>
#include <xcb/xcb_keysyms.h>
#include <xcb/xcb_ewmh.h>
#include <xcb/damage.h>
#include "client.h" 

#include "./client/client_logic.h"

#define TITLEBAR_HEIGHT 20
#define FRAME_PADDING   10
#define OUTER_RADIUS    24
#define INNER_RADIUS    14




#define DEFAULT_FONT_SIZE 12
#define CLIENT_BORDER_RADIUS 8
#define FRAME_BORDER_WIDTH 5

#define WORKSPACE_GAP_SIZE 20
#define WORKSPACE_BAR_HEIGHT 30

#define CLIENT_MIN_WIDTH 200
#define CLIENT_MIN_HEIGHT 200

#define CLIENT_BORDER_SIZE 10

#define CLIENT_MIN_HEIGHT 200
#define CLIENT_MAX_HEIGHT 200

#define CLIENT_POSITION_SPACING 10

#define USE_GRID false 

typedef struct root_config {
    int width;
    int height;
    int x;
    int y;
} root_config;

extern xcb_window_t             root;
extern xcb_screen_t            *screen;
extern xcb_connection_t        *connection;
extern xcb_key_symbols_t       *keysyms; 
extern root_config              config; 
extern xcb_ewmh_connection_t *ewmh;

extern xcb_render_pictformat_t root_format;

extern xcb_render_picture_t root_picture;
extern xcb_render_picture_t root_buffer;
extern xcb_render_picture_t root_tile;

extern ClientLogic* client_logic;

cairo_t * create_cairo_context(xcb_window_t window);
xcb_visualtype_t *get_visualtype(xcb_screen_t *screen);

void draw_decorations(cairo_t *cr, int width, int height, const char *title);
void configure_client(client *c, uint32_t width, uint32_t * rect);

void order_client();
void draw(xcb_damage_damage_t damage);

xcb_atom_t get_atom(const char *name);
// static xcb_ewmh_connection_t   *ewmh;
// static uint16_t                 modifiers;
// static xcb_keycode_t            keycode;
#endif
