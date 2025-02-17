#include <xcb/xcb.h>
#include <xcb/xproto.h>
#include <xcb/xcb_ewmh.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cmath>
#include <iostream>
#include <xcb/xcb_keysyms.h>
#include <X11/keysym.h>
#include <vector>
#include <cmath>

#include "./app/config.h"
#include "./app/workspace.h"
#include "./app/client.h"

// typedef struct {
//     uint32_t left;
//     uint32_t right;
//     uint32_t top;
//     uint32_t bottom;
// } wm_struts_t;
//
// enum ResizeEdge {
//     NONE,
//     LEFT,
//     RIGHT,
//     TOP,
//     BOTTOM,
//     BOTTOM_RIGHT,
//     BOTTOM_LEFT,
//     TOP_RIGHT,
//     TOP_LEFT
// };

// ResizeEdge resize_edge = NONE;

// bool is_resizing = false;
// int initial_root_x = 0, initial_root_y = 0;
// int initial_width = 0, initial_height = 0;
//
//
xcb_window_t             root;
xcb_screen_t            *screen;
xcb_connection_t        *connection;
xcb_key_symbols_t       *keysyms; 
root_config              config;

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

// bool is_dock_window(xcb_window_t w) {
//     xcb_atom_t net_wm_window_type = ewmh->_NET_WM_WINDOW_TYPE;
//     xcb_atom_t net_wm_window_type_dock = ewmh->_NET_WM_WINDOW_TYPE_DOCK;
//
//     xcb_get_property_cookie_t cookie = xcb_get_property(
//         connection, 0, w,
//         net_wm_window_type, 
//         XCB_ATOM_ATOM,     
//         0, 32
//     );
//     xcb_get_property_reply_t *reply = xcb_get_property_reply(connection, cookie, NULL);
//     if (!reply) return false;
//
//     bool dock = false;
//     if (xcb_get_property_value_length(reply) > 0) {
//         xcb_atom_t *atoms = (xcb_atom_t *) xcb_get_property_value(reply);
//         int count = xcb_get_property_value_length(reply) / sizeof(xcb_atom_t);
//         for(int i = 0; i < count; i++) {
//             if (atoms[i] == net_wm_window_type_dock) {
//                 dock = true;
//                 break;
//             }
//         }
//     }
//     free(reply);
//     return dock;
// }
//
// void read_dock_struts(xcb_window_t w) {
//     memset(&g_dock_struts, 0, sizeof(g_dock_struts));
//
//     xcb_atom_t net_wm_strut_partial = ewmh->_NET_WM_STRUT_PARTIAL;
//     xcb_get_property_cookie_t c_partial = xcb_get_property(
//         connection, 0, w,
//         net_wm_strut_partial,
//         XCB_ATOM_CARDINAL,
//         0, 12 
//     );
//     xcb_get_property_reply_t *r_partial = xcb_get_property_reply(connection, c_partial, NULL);
//
//     if (r_partial && xcb_get_property_value_length(r_partial) >= 12 * (int)sizeof(uint32_t)) {
//         uint32_t *struts = (uint32_t*) xcb_get_property_value(r_partial);
//         g_dock_struts.left   = struts[0];
//         g_dock_struts.right  = struts[1];
//         g_dock_struts.top    = struts[2];
//         g_dock_struts.bottom = struts[3];
//         free(r_partial);
//         return; 
//     }
//     free(r_partial);
//
//     // Caso nao tenha partial, tenta _NET_WM_STRUT
//     xcb_atom_t net_wm_strut = ewmh->_NET_WM_STRUT;
//     xcb_get_property_cookie_t c_strut = xcb_get_property(
//         connection, 0, w,
//         net_wm_strut,
//         XCB_ATOM_CARDINAL,
//         0, 4
//     );
//     xcb_get_property_reply_t *r_strut = xcb_get_property_reply(connection, c_strut, NULL);
//     if (r_strut && xcb_get_property_value_length(r_strut) >= 4 * (int)sizeof(uint32_t)) {
//         uint32_t *s = (uint32_t *) xcb_get_property_value(r_strut);
//         g_dock_struts.left   = s[0];
//         g_dock_struts.right  = s[1];
//         g_dock_struts.top    = s[2];
//         g_dock_struts.bottom = s[3];
//     }
//     free(r_strut);
// }

// cairo_t *create_cairo_context(xcb_window_t window) {
//     xcb_visualtype_t *visual = get_visualtype(screen);
//     if (!visual) {
//         fprintf(stderr, "Erro ao obter o visual da tela\n");
//         exit(EXIT_FAILURE);
//     }
//
//     xcb_get_geometry_reply_t *geometry = xcb_get_geometry_reply(
//         connection, xcb_get_geometry(connection, window), NULL);
//
//     if (!geometry) {
//         fprintf(stderr, "Erro ao obter geometria da janela\n");
//         exit(EXIT_FAILURE);
//     }
//
//     cairo_surface_t *surface = cairo_xcb_surface_create(
//         connection, window, visual, geometry->width, geometry->height);
//     cairo_t *cr = cairo_create(surface);
//
//     cairo_surface_destroy(surface);
//     free(geometry);
//
//     return cr;
// }

// void draw_decorations(cairo_t *cr, int width, int height, const char *title) {
//     cairo_rectangle(cr, 0, 0, width, BAR_HEIGHT);
//     cairo_set_source_rgb(cr, 0.0, 1.0, 1.0); 
//     cairo_fill(cr);
//
//     cairo_move_to(cr, 0, BORDER_RADIUS);
//     cairo_arc(cr, BORDER_RADIUS, BORDER_RADIUS, BORDER_RADIUS, M_PI, 1.5 * M_PI);
//     cairo_line_to(cr, width - BORDER_RADIUS, 0);
//     cairo_arc(cr, width - BORDER_RADIUS, BORDER_RADIUS, BORDER_RADIUS, 1.5 * M_PI, 2 * M_PI);
//     cairo_line_to(cr, width, height);
//     cairo_line_to(cr, 0, height);
//     cairo_line_to(cr, 0, BORDER_RADIUS);
//
//     cairo_set_source_rgb(cr, 1, 1, 1);
//     cairo_fill(cr);
// }

void draw() {
    // uint32_t usable_x = 0 + g_dock_struts.left;
    // uint32_t usable_y = 0 + g_dock_struts.top + BORDER_GAP;
    // uint32_t usable_width  = screen->width_in_pixels  - (g_dock_struts.left + g_dock_struts.right);
    // uint32_t usable_height = screen->height_in_pixels - (g_dock_struts.top + g_dock_struts.bottom) - BORDER_GAP;

    // uint32_t aw  = screen->width_in_pixels - (2*BORDER_GAP) - BORDER_WIDTH;
    // uint32_t ah = screen->height_in_pixels - (2*BORDER_GAP) - BORDER_WIDTH; 
    //     
    // uint32_t child_width = std::round(static_cast<double>(aw) / workspaces[current_workspace].clients.size());
    // uint32_t child_width = std::round(static_cast<double>(aw));
    // uint32_t child_height = ah;
    //
    // uint32_t x = BORDER_GAP;
    // uint32_t y = BORDER_GAP;
    
    int index = 0;
    // for (client c : workspaces[current_workspace].clients) {
    //     if(index == 3)
    //         break; 
    //
    //     uint32_t *rect_frame = new uint32_t[4]{(uint32_t)c.x, (uint32_t)c.y, (uint32_t)c.width, (uint32_t)c.height};
    //     xcb_configure_window(
    //         connection,
    //         c.frame,
    //         XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y | XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT,
    //         rect_frame
    //     );
    //
    //     uint32_t *rect_child = new uint32_t[4]{(uint32_t)c.x, (uint32_t)c.y, (uint32_t)c.width -  CLIENT_BORDER_SIZE*2, (uint32_t)c.height -  CLIENT_BORDER_SIZE*2 };
    //     xcb_configure_window(
    //         connection,
    //         c.child,
    //         XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y | XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT,
    //         rect_frame
    //     );
    //     index++;
    // }

    for (client &c : workspaces[current_workspace].clients) {
        xcb_map_window(connection, c.frame);
        // xcb_map_window(connection, c.child); 
    }

    xcb_flush(connection);
}

// void handle_map_request(xcb_generic_event_t *event) {
//     xcb_map_request_event_t *map_request = (xcb_map_request_event_t *)event;
//
//     // if (is_dock_window(map_request->window)) {
//     //     fprintf(stderr, "Janela %u DOCK.\n", map_request->window);
//     //     g_dock_window = map_request->window;
//     //     read_dock_struts(g_dock_window);
//     //
//     //     xcb_map_window(connection, g_dock_window);
//     //
//     //     uint32_t *rect= new uint32_t[4]{BORDER_GAP, BORDER_GAP, (uint32_t)(screen->width_in_pixels - (BORDER_GAP*2)) };
//     //     xcb_configure_window(
//     //         connection,
//     //         g_dock_window,
//     //         XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y | XCB_CONFIG_WINDOW_WIDTH,
//     //         rect
//     //     );
//     //
//     //     xcb_flush(connection);
//     //
//     //     arrange_clients();
//     //     return;
//     // }
//     
//     add_client(frame, map_request->window);
//
//     xcb_map_window(connection, map_request->window);
//
//     arrange_clients();
//     xcb_flush(connection);
// }

// void grab_key_with_modifiers(xcb_connection_t *connection, xcb_window_t root) {
//
//     const uint32_t num_lock_mask = XCB_MOD_MASK_2;
//     const uint32_t caps_lock_mask = XCB_MOD_MASK_LOCK;
//
//     uint32_t mod_combinations[] = {
//         modifiers,
//         modifiers | num_lock_mask,
//         modifiers | caps_lock_mask,
//         modifiers | num_lock_mask | caps_lock_mask
//     };
//
//     for (uint16_t mod : mod_combinations) {
//         xcb_grab_key(connection, 1, root, mod, keycode,
//                      XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);
//     }
//     xcb_flush(connection);
// }

//
// void start_move(client *c, int root_x, int root_y) {
//     is_resizing = false;
//     current_resizing_client = c;
//     initial_root_x = root_x;
//     initial_root_y = root_y;
//     initial_width = c->width;
//     initial_height = c->height;
//     resize_edge = NONE;
// }

// void move_client(int root_x, int root_y) {
//
//     std::cout << "Moving janela " << current_resizing_client->frame << std::endl;
//
//     int new_x = root_x - (initial_width/2);
//     int new_y = root_y - (initial_height/2);
//
//     uint32_t values[] = {current_resizing_client->x, current_resizing_client->y};
//     xcb_configure_window(connection, current_resizing_client->frame,
//                          XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y, 
//                          values);
//     xcb_flush(connection);
// }

// void stop_resize() {
//     is_resizing = false;
//     current_resizing_client = nullptr;
//     resize_edge = NONE;
// }

// void handle_motion_notify(xcb_generic_event_t *event) {
//     xcb_motion_notify_event_t *motion_event = (xcb_motion_notify_event_t *)event;
//     if (is_resizing && current_resizing_client) {
//         resize_client(motion_event->root_x, motion_event->root_y);
//     }
// }

// void handle_button_release(xcb_generic_event_t *event) {
//     stop_resize();
// }

void setup() {
    connection = xcb_connect(NULL, NULL);
    if (xcb_connection_has_error(connection)) {
        fprintf(stderr, "Erro trying to connect to X Server\n");
        exit(EXIT_FAILURE);
    }

    keysyms = xcb_key_symbols_alloc(connection);

    screen = xcb_setup_roots_iterator(xcb_get_setup(connection)).data;
    root   = screen->root;

    xcb_get_geometry_reply_t *geometry = xcb_get_geometry_reply(
        connection, xcb_get_geometry(connection, root), NULL);

    config = {geometry->width, geometry->height, 0,0, };

    uint32_t masks[] = {
        XCB_EVENT_MASK_KEY_PRESS  |
        XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT |
        XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY | 
        XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE
    };

    workspaces = {{}};
    
    xcb_change_window_attributes(connection, root, XCB_CW_EVENT_MASK, masks);
    xcb_flush(connection);
}

void event_loop() {
    xcb_generic_event_t *event;
    while ((event = xcb_wait_for_event(connection))) {
        uint8_t response_type = event->response_type & ~0x80;
        switch (response_type) {
            case XCB_MAP_REQUEST:{
                add_client(event);
                break;
            }
            case XCB_DESTROY_NOTIFY:
                // remove_client(event);
                break;
            case XCB_BUTTON_PRESS: {
                client_button_press(event);
                break;
            }
            case XCB_MOTION_NOTIFY: {
                client_motion_handle(event);
                break;
            }
            case XCB_BUTTON_RELEASE: {
                client_button_release(event);
                break;
            }
            // case XCB_KEY_PRESS: {
            //     // xcb_key_press_event_t *key_event = (xcb_key_press_event_t *)event;
            //     //
            //     // std::cout << "teste" << std::endl;
            //     //
            //     // if (key_event->detail == keycode && (key_event->state & modifiers) == modifiers) {
            //     //     system("kitty &");
            //     // }
            //
            //     xcb_key_press_event_t *key_event = (xcb_key_press_event_t *)event;
            //
            //
            //     std::cout << "Tecla pressionada: " << key_event->detail << std::endl;
            //     std::cout << "Event state: " << key_event->state << std::endl;
            //
            //     // Verificar se a tecla Control está pressionada
            //     bool ctrl_pressed = key_event->state & XCB_MOD_MASK_CONTROL;
            //
            //     std::cout << "Is pressed: " << ctrl_pressed << std::endl;
            //
            //     // Obter o keysym da tecla pressionada
            //     xcb_keysym_t keysym = xcb_key_symbols_get_keysym(keysyms, key_event->detail, 0);
            //
            //     if (ctrl_pressed && keysym == XK_r) {
            //         // swap_client();
            //         // arrange_clients();
            //         std::cout << "Ctrl + r foi pressionado!" << std::endl;
            //     }
            //
            //     if ((ctrl_pressed && keysym == XK_Left) || (ctrl_pressed && keysym == XK_Right)) { 
            //
            //         // std::cout << "Ctrl + Left ou Ctrl + Right foi pressionado, mudando area de trabalho!" << std::endl;
            //         
            //         for (client &c : workspaces[current_workspace].clients) {
            //             xcb_unmap_window(connection, c.child);
            //         }
            //
            //         if(keysym == XK_Left) {
            //             if(current_workspace > 0)
            //                 current_workspace--;
            //         } else if(keysym == XK_Right) {
            //             if(current_workspace < workspaces.size()) {
            //                 workspaces.push_back({{}});
            //                 current_workspace++;
            //             }
            //         }
            //         std::cout << "Workspace selecionado: " << current_workspace << std::endl;
            //         arrange_clients();
            //     }
            //
            //     break;
            // }
            default:
                break;
        }
        draw();
        free(event);
    }
}

int main() {
    setup();
    event_loop();

    // xcb_ewmh_connection_wipe(ewmh);
    // free(ewmh);
    xcb_disconnect(connection);

    return 0;

}
