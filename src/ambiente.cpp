#include "config.h"
#include <xcb/xcb.h>
#include <xcb/xproto.h>
#include <xcb/xcb_ewmh.h>
#include <cairo/cairo.h>
#include <cairo/cairo-xcb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cmath>

typedef struct client {
    struct client *next;
    xcb_window_t frame;
    xcb_window_t child;
} client;

static xcb_connection_t *connection;
static xcb_screen_t *screen;
static xcb_ewmh_connection_t *ewmh;
static xcb_window_t root;
static client *clients = NULL;

void add_client(xcb_window_t frame, xcb_window_t child);
void arrange_clients();
cairo_t * create_cairo_context(xcb_window_t window);
void draw_decorations(cairo_t *cr, int width, int height, const char *title);
void configure_client(client *c, uint32_t width, uint32_t * rect);


xcb_visualtype_t *get_visualtype(xcb_screen_t *screen) {
    xcb_depth_iterator_t depth_iter = xcb_screen_allowed_depths_iterator(screen);
    for (; depth_iter.rem; xcb_depth_next(&depth_iter)) {
        xcb_visualtype_iterator_t visual_iter = xcb_depth_visuals_iterator(depth_iter.data);
        for (; visual_iter.rem; xcb_visualtype_next(&visual_iter)) {
            if (screen->root_visual == visual_iter.data->visual_id) {
                return visual_iter.data;
            }
        }
    }
    return NULL;
}


void add_client(xcb_window_t frame, xcb_window_t child) {
    client *c = (client *)malloc(sizeof(client));
    if (!c) {
        fprintf(stderr, "Erro ao alocar memória para cliente\n");
        exit(EXIT_FAILURE);
    }
    c->frame = frame;
    c->child = child;
    c->next = clients;
    clients = c;
}

void configure_client(client *c, uint32_t width, uint32_t * rect) {

    xcb_configure_window(connection, c->frame, XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y | XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT,
                         rect);

    cairo_t *cr = create_cairo_context(c->frame);
    draw_decorations(cr, width, BAR_HEIGHT, "Client");
    cairo_destroy(cr);

}

void arrange_clients() {

    if(!clients) return;
    int window_count = 0;

    for (client *c = clients; c; c = c->next) {
        window_count++;
    }

    uint32_t master_width = screen->width_in_pixels;
    uint32_t master_height = screen->height_in_pixels;

    uint32_t y = BORDER_GAP;
    uint32_t x = BORDER_GAP;
    int i = 0;

    uint32_t client_width = (master_width - 2 * BORDER_GAP - (window_count - 1) * BORDER_GAP) / window_count;
    uint32_t client_height = (master_height - BORDER_GAP * 2); 

    for (client *c = clients; c; c = c->next, i++) {

        if(i > MAX_CLIENTS){
            // TODO: Implement the client queue
            break;
        }

        uint32_t *rect= new uint32_t[4]{x, y, client_width, client_height};
        configure_client(c, client_width, rect);

        x += client_width + BORDER_GAP;
    }

}

void remove_client(xcb_window_t window) {
    client **c = &clients;
    while (*c) {
        fprintf(stderr, "Janela %d foi destruídaaaaaaaaaa.\n", (*c)->child);
        fprintf(stderr, "Essa Janela %d .\n", window);
        if ((*c)->child == window) {
            client *tmp = *c;
            xcb_destroy_window(connection, (*c)->frame);
            *c = (*c)->next;
            free(tmp);
            return;
        }
        c = &(*c)->next;
    }
}

void handle_destroy_notify(xcb_generic_event_t *event) {
    xcb_destroy_notify_event_t *destroy_notify = (xcb_destroy_notify_event_t *)event;
    fprintf(stderr, "Janela %d foi destruída.\n", destroy_notify->window);
    remove_client(destroy_notify->window);
    arrange_clients();
}

cairo_t *create_cairo_context(xcb_window_t window) {
    xcb_visualtype_t *visual = get_visualtype(screen);
    if (!visual) {
        fprintf(stderr, "Erro ao obter o visual da tela\n");
        exit(EXIT_FAILURE);
    }

    xcb_get_geometry_reply_t *geometry = xcb_get_geometry_reply(
        connection, xcb_get_geometry(connection, window), NULL);

    if (!geometry) {
        fprintf(stderr, "Erro ao obter geometria da janela\n");
        exit(EXIT_FAILURE);
    }

    cairo_surface_t *surface = cairo_xcb_surface_create(
        connection, window, visual, geometry->width, geometry->height);

    cairo_t *cr = cairo_create(surface);

    cairo_surface_destroy(surface);
    free(geometry);

    return cr;
}

void draw_decorations(cairo_t *cr, int width, int height, const char *title) {
    // cairo_set_source_rgb(cr, 0, 1, 1);
    cairo_rectangle(cr, 0, 0, width, BAR_HEIGHT);
    cairo_fill(cr);

    cairo_move_to(cr, 0, BORDER_RADIUS);
    cairo_arc(cr, BORDER_RADIUS, BORDER_RADIUS, BORDER_RADIUS, M_PI, 1.5 * M_PI);
    cairo_line_to(cr, width - BORDER_RADIUS, 0);
    cairo_arc(cr, width - BORDER_RADIUS, BORDER_RADIUS, BORDER_RADIUS, 1.5 * M_PI, 2 * M_PI);
    cairo_line_to(cr, width, height);
    cairo_line_to(cr, 0, height);
    cairo_line_to(cr, 0, BORDER_RADIUS);
    cairo_set_source_rgb(cr, 1, 1, 1);

    cairo_fill(cr);

    // cairo_set_source_rgb(cr, 0, 0, 0);
    // cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    // cairo_set_font_size(cr, FONT_SIZE);
    // cairo_move_to(cr, 10, (BAR_HEIGHT/2) - (FONT_SIZE/2));
    // cairo_show_text(cr, title);
    //
    // cairo_set_source_rgb(cr, 1, 0, 0);
    // cairo_arc(cr, width - 20, BAR_HEIGHT / 2, 10, 0, 2 * 3.14159);
    // cairo_fill(cr);

    // cairo_stroke(cr);
}

xcb_window_t decorate_window(xcb_window_t client_window, uint16_t x, uint16_t y, uint16_t width, uint16_t height) {
    xcb_window_t frame = xcb_generate_id(connection);

    uint32_t values[] = {screen->black_pixel, XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_BUTTON_PRESS};

    fprintf(stderr, "Frame x: %d, y: %d \n", x,y);

    xcb_create_window(
        connection,
        XCB_COPY_FROM_PARENT,                
        frame,                               
        root,                                
        x + 30, y + 30,                            
        width, height,                            
        0,                                   
        XCB_WINDOW_CLASS_INPUT_OUTPUT,       
        screen->root_visual,                 
        XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK, values);

    xcb_map_window(connection, frame);
    xcb_reparent_window(connection, client_window, frame, 0, BAR_HEIGHT);

    // char *title = get_window_name(client_window);
    xcb_flush(connection);
    return frame;
}

void handle_map_request(xcb_generic_event_t *event) {
    xcb_map_request_event_t *map_request = (xcb_map_request_event_t *)event;

    xcb_get_geometry_cookie_t geometry_cookie = xcb_get_geometry(connection, map_request->window);
    xcb_get_geometry_reply_t *geometry_reply = xcb_get_geometry_reply(connection, geometry_cookie, NULL);

    if(!geometry_reply) {
        fprintf(stderr, "Erro ao obter geometria da janela\n");
        exit(EXIT_FAILURE);
    }

    fprintf(stderr, "X:%d Y:$d.\n", geometry_reply->x);

    xcb_window_t frame = decorate_window(map_request->window, geometry_reply->x, geometry_reply->y, geometry_reply->width, geometry_reply->height);

    add_client(frame, map_request->window);

    xcb_map_window(connection, map_request->window);

    arrange_clients();
    xcb_flush(connection);
}

void setup() {
    connection = xcb_connect(NULL, NULL);
    if (xcb_connection_has_error(connection)) {
        fprintf(stderr, "Erro ao conectar ao servidor X\n");
        exit(EXIT_FAILURE);
    }

    ewmh = (xcb_ewmh_connection_t *)malloc(sizeof(xcb_ewmh_connection_t));
    if (!ewmh) {
        fprintf(stderr, "Erro ao alocar memória para EWMH\n");
        exit(EXIT_FAILURE);
    }

    xcb_intern_atom_cookie_t *ewmh_cookies = xcb_ewmh_init_atoms(connection, ewmh);
    if (!xcb_ewmh_init_atoms_replies(ewmh, ewmh_cookies, NULL)) {
        fprintf(stderr, "Erro ao inicializar os átomos EWMH\n");
        exit(EXIT_FAILURE);
    }

    screen = xcb_setup_roots_iterator(xcb_get_setup(connection)).data;
    root = screen->root;

    uint32_t values[] = {XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT | XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY | XCB_EVENT_MASK_BUTTON_PRESS};
    xcb_change_window_attributes(connection, root, XCB_CW_EVENT_MASK, values);
    xcb_flush(connection);
}

void event_loop() {
    xcb_generic_event_t *event;
    while ((event = xcb_wait_for_event(connection))) {
        uint8_t response_type = event->response_type & ~0x80;
        switch (response_type) {
        case XCB_MAP_REQUEST:
            handle_map_request(event);
            break;
        case XCB_DESTROY_NOTIFY:
            handle_destroy_notify(event);
            break;
        default:
            break;
        }
        free(event);
    }
}

int main() {
    setup();
    event_loop();

    xcb_ewmh_connection_wipe(ewmh);
    free(ewmh);
    xcb_disconnect(connection);

    return 0;
}
