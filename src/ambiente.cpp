#include "config.h"
#include <xcb/xcb.h>
#include <xcb/xproto.h>
#include <xcb/xcb_ewmh.h>
#include <cairo/cairo.h>
#include <cairo/cairo-xcb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
        xcb_configure_window(connection, c->frame, XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y | XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT,
                             rect);
        x += client_width + BORDER_GAP;
    }

}

void remove_client(xcb_window_t window) {
    client **c = &clients;
    while (*c) {
        if ((*c)->child == window) {
            client *tmp = *c;
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

void handle_map_request(xcb_generic_event_t *event) {
    xcb_map_request_event_t *map_request = (xcb_map_request_event_t *)event;

    xcb_get_geometry_cookie_t geometry_cookie = xcb_get_geometry(connection, map_request->window);
    xcb_get_geometry_reply_t *geometry_reply = xcb_get_geometry_reply(connection, geometry_cookie, NULL);

    if(!geometry_reply) {
        fprintf(stderr, "Erro ao obter geometria da janela\n");
        exit(EXIT_FAILURE);
    }

    // xcb_window_t frame = decorate_window(map_request->window, geometry_reply->x, geometry_reply->y, geometry_reply->width, geometry_reply->height);

    add_client(map_request->window, map_request->window);

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
