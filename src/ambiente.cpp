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

typedef struct {
    uint32_t left;
    uint32_t right;
    uint32_t top;
    uint32_t bottom;
} wm_struts_t;

static xcb_window_t g_dock_window = XCB_WINDOW_NONE;
static wm_struts_t  g_dock_struts = {0,0,0,0};

typedef struct client {
    struct client *next;
    xcb_window_t frame;
    xcb_window_t child;
} client;

static xcb_connection_t        *connection;
static xcb_screen_t            *screen;
static xcb_ewmh_connection_t   *ewmh;
static xcb_window_t             root;
static client                  *clients = NULL;

void add_client(xcb_window_t frame, xcb_window_t child);
void arrange_clients();
cairo_t * create_cairo_context(xcb_window_t window);
void draw_decorations(cairo_t *cr, int width, int height, const char *title);
void configure_client(client *c, uint32_t width, uint32_t * rect);
void remove_client(xcb_window_t window);
void handle_destroy_notify(xcb_generic_event_t *event);
void handle_map_request(xcb_generic_event_t *event);
void setup();
void event_loop();
xcb_visualtype_t *get_visualtype(xcb_screen_t *screen);

xcb_atom_t get_atom(const char *name) {
    xcb_intern_atom_cookie_t cookie = xcb_intern_atom(
        connection,
        0,
        strlen(name),
        name
    );
    xcb_intern_atom_reply_t *reply = xcb_intern_atom_reply(connection, cookie, NULL);
    if (!reply) {
        fprintf(stderr, "Falha ao obter átomo: %s\n", name);
        return XCB_ATOM_NONE;
    }
    xcb_atom_t atom = reply->atom;
    free(reply);
    return atom;
}

bool is_dock_window(xcb_window_t w) {
    xcb_atom_t net_wm_window_type = ewmh->_NET_WM_WINDOW_TYPE;
    xcb_atom_t net_wm_window_type_dock = ewmh->_NET_WM_WINDOW_TYPE_DOCK;

    xcb_get_property_cookie_t cookie = xcb_get_property(
        connection, 0, w,
        net_wm_window_type, 
        XCB_ATOM_ATOM,     
        0, 32
    );
    xcb_get_property_reply_t *reply = xcb_get_property_reply(connection, cookie, NULL);
    if (!reply) return false;

    bool dock = false;
    if (xcb_get_property_value_length(reply) > 0) {
        xcb_atom_t *atoms = (xcb_atom_t *) xcb_get_property_value(reply);
        int count = xcb_get_property_value_length(reply) / sizeof(xcb_atom_t);
        for(int i = 0; i < count; i++) {
            if (atoms[i] == net_wm_window_type_dock) {
                dock = true;
                break;
            }
        }
    }
    free(reply);
    return dock;
}

void read_dock_struts(xcb_window_t w) {
    memset(&g_dock_struts, 0, sizeof(g_dock_struts));

    xcb_atom_t net_wm_strut_partial = ewmh->_NET_WM_STRUT_PARTIAL;
    xcb_get_property_cookie_t c_partial = xcb_get_property(
        connection, 0, w,
        net_wm_strut_partial,
        XCB_ATOM_CARDINAL,
        0, 12 
    );
    xcb_get_property_reply_t *r_partial = xcb_get_property_reply(connection, c_partial, NULL);

    if (r_partial && xcb_get_property_value_length(r_partial) >= 12 * (int)sizeof(uint32_t)) {
        uint32_t *struts = (uint32_t*) xcb_get_property_value(r_partial);
        g_dock_struts.left   = struts[0];
        g_dock_struts.right  = struts[1];
        g_dock_struts.top    = struts[2];
        g_dock_struts.bottom = struts[3];
        free(r_partial);
        return; 
    }
    free(r_partial);

    // Caso nao tenha partial, tenta _NET_WM_STRUT
    xcb_atom_t net_wm_strut = ewmh->_NET_WM_STRUT;
    xcb_get_property_cookie_t c_strut = xcb_get_property(
        connection, 0, w,
        net_wm_strut,
        XCB_ATOM_CARDINAL,
        0, 4
    );
    xcb_get_property_reply_t *r_strut = xcb_get_property_reply(connection, c_strut, NULL);
    if (r_strut && xcb_get_property_value_length(r_strut) >= 4 * (int)sizeof(uint32_t)) {
        uint32_t *s = (uint32_t *) xcb_get_property_value(r_strut);
        g_dock_struts.left   = s[0];
        g_dock_struts.right  = s[1];
        g_dock_struts.top    = s[2];
        g_dock_struts.bottom = s[3];
    }
    free(r_strut);
}

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

void remove_client(xcb_window_t window) {
    client **c = &clients;
    while (*c) {
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

    // Se for o dock que saiu, zera
    if (destroy_notify->window == g_dock_window) {
        g_dock_window = XCB_WINDOW_NONE;
        memset(&g_dock_struts, 0, sizeof(g_dock_struts));
    }

    remove_client(destroy_notify->window);
    arrange_clients();
}

void configure_client(client *c, uint32_t width, uint32_t *rect) {
    xcb_configure_window(
        connection,
        c->frame,
        XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y | XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT,
        rect
    );

    // cairo_t *cr = create_cairo_context(c->frame);
    // draw_decorations(cr, width, BAR_HEIGHT, "Client");
    // cairo_destroy(cr);
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
    cairo_rectangle(cr, 0, 0, width, BAR_HEIGHT);
    cairo_set_source_rgb(cr, 0.0, 1.0, 1.0); 
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
}

void arrange_clients() {
    uint32_t usable_x = 0 + g_dock_struts.left;
    uint32_t usable_y = 0 + g_dock_struts.top + BORDER_GAP;
    uint32_t usable_width  = screen->width_in_pixels  - (g_dock_struts.left + g_dock_struts.right);
    uint32_t usable_height = screen->height_in_pixels - (g_dock_struts.top + g_dock_struts.bottom) - BORDER_GAP;

    if(!clients) return;

    int window_count = 0;
    for (client *c = clients; c; c = c->next) {
        window_count++;
    }

    uint32_t x = usable_x + BORDER_GAP;
    uint32_t y = usable_y + BORDER_GAP;

    uint32_t client_width  = (usable_width  - 2 * BORDER_GAP - (window_count - 1) * BORDER_GAP) / window_count;
    uint32_t client_height = (usable_height - 2 * BORDER_GAP);

    int i = 0;
    for (client *c = clients; c; c = c->next, i++) {
        if(i > MAX_CLIENTS) {
            // TODO: Implement the client queue
            break;
        }

        uint32_t *rect= new uint32_t[4]{x, y, client_width, client_height};
        configure_client(c, client_width, rect);

        x += client_width + BORDER_GAP;
    }
    xcb_flush(connection);
}

void handle_map_request(xcb_generic_event_t *event) {
    xcb_map_request_event_t *map_request = (xcb_map_request_event_t *)event;

    if (is_dock_window(map_request->window)) {
        fprintf(stderr, "Janela %u DOCK.\n", map_request->window);
        g_dock_window = map_request->window;
        read_dock_struts(g_dock_window);

        xcb_map_window(connection, g_dock_window);

        uint32_t *rect= new uint32_t[4]{BORDER_GAP, BORDER_GAP, (uint32_t)(screen->width_in_pixels - (BORDER_GAP*2)) };
        xcb_configure_window(
            connection,
            g_dock_window,
            XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y | XCB_CONFIG_WINDOW_WIDTH,
            rect
        );

        xcb_flush(connection);

        arrange_clients();
        return;
    }

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
    root   = screen->root;

    uint32_t masks[] = {
        XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT |
        XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY  |
        XCB_EVENT_MASK_BUTTON_PRESS
    };

    xcb_change_window_attributes(connection, root, XCB_CW_EVENT_MASK, masks);
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
