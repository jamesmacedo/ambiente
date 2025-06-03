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

#include <cairo/cairo.h>
#include <cairo/cairo-xcb.h>

#include <xcb/damage.h>
#include <xcb/render.h>
#include <xcb/composite.h>
#include <xcb/xfixes.h>
#include <xcb/xcb_renderutil.h>

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

const xcb_query_extension_reply_t *damage_ext;
const xcb_query_extension_reply_t *xfixes_ext;
const xcb_query_extension_reply_t *composite_ext;
const xcb_query_extension_reply_t *render_ext;

xcb_render_pictformat_t root_format;

xcb_render_picture_t root_picture;
xcb_render_picture_t root_buffer;
xcb_render_picture_t root_tile;

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

xcb_pixmap_t generate_wallpaper_pixmap(std::string image) {
    const char *wallpaper_path = image.c_str();

    int screen_width = screen->width_in_pixels;
    int screen_height = screen->height_in_pixels;

    cairo_surface_t *img_surface = cairo_image_surface_create_from_png(wallpaper_path);
    if (cairo_surface_status(img_surface) != CAIRO_STATUS_SUCCESS) {
        fprintf(stderr, "Erro ao carregar wallpaper: %s\n", wallpaper_path);
        cairo_surface_destroy(img_surface);
        exit(1);
    }
    int img_width = cairo_image_surface_get_width(img_surface);
    int img_height = cairo_image_surface_get_height(img_surface);

    double scale_x = (double)screen_width / img_width;
    double scale_y = (double)screen_height / img_height;
    double scale = (scale_x > scale_y) ? scale_x : scale_y;
    int new_w = (int)(img_width * scale);
    int new_h = (int)(img_height * scale);
    int offset_x = (screen_width - new_w) / 2;
    int offset_y = (screen_height - new_h) / 2;

    xcb_pixmap_t pixmap = xcb_generate_id(connection);
    xcb_create_pixmap(connection, screen->root_depth, pixmap, root, screen_width, screen_height);

    xcb_visualtype_t *visual = get_visualtype(screen);
    cairo_surface_t *cairo_surface = cairo_xcb_surface_create(connection, pixmap, visual, screen_width, screen_height);
    cairo_t *cr = cairo_create(cairo_surface);

    cairo_set_source_rgb(cr, 0, 0, 0);
    cairo_paint(cr);

    cairo_save(cr);
    cairo_translate(cr, offset_x, offset_y);
    cairo_scale(cr, scale, scale);
    cairo_set_source_surface(cr, img_surface, 0, 0);
    cairo_paint(cr);
    cairo_restore(cr);

    cairo_destroy(cr);
    cairo_surface_destroy(cairo_surface);
    cairo_surface_destroy(img_surface);

    return pixmap;
}

void setup_root_background(){

    xcb_pixmap_t wallpaper_pix = generate_wallpaper_pixmap("/home/nemo/Documentos/wallpaper/forest.png");

    root_tile = xcb_generate_id(connection);

    xcb_render_create_picture(connection, root_tile, wallpaper_pix, root_format, 0, NULL);
    xcb_render_composite(connection, XCB_RENDER_PICT_OP_SRC, root_tile, XCB_RENDER_PICTURE_NONE, root_buffer, 0,0,0,0,0,0, screen->width_in_pixels, screen->height_in_pixels);
}

void draw(xcb_damage_damage_t damage) {    
    // for (client &c : workspaces[current_workspace].clients) {
    //     xcb_map_window(connection, c.frame);
    // }

    if(!root_buffer){
    
        std::cout << "creating background" << std::endl;
        root_buffer = xcb_generate_id(connection);

        xcb_pixmap_t root_tmp = xcb_generate_id(connection);

        xcb_create_pixmap(connection, screen->root_depth, root_tmp, root, screen->width_in_pixels, screen->height_in_pixels);
        xcb_render_create_picture(connection, root_buffer, root, root_format, 0, NULL);

        xcb_free_pixmap(connection, root_tmp);
        setup_root_background();
    }

    if(root_tile) {
        xcb_render_composite(connection, XCB_RENDER_PICT_OP_SRC, root_tile, XCB_RENDER_PICTURE_NONE, root_buffer, 
                            0, 0, 0, 0, 0, 0, screen->width_in_pixels, screen->height_in_pixels);
    }

    for (client c : workspaces[current_workspace].clients) {
        std::cout << "drawing" << std::endl;
        if(c.damage == damage){
            c.draw(root_buffer);
        }
    }

    xcb_render_composite(connection, XCB_RENDER_PICT_OP_SRC, root_buffer, XCB_RENDER_PICTURE_NONE, root_picture, 0,0,0,0,0,0, screen->width_in_pixels, screen->height_in_pixels);

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

     // EXTENTION VALIDATION

    xcb_prefetch_extension_data(connection, &xcb_damage_id);
    xcb_prefetch_extension_data(connection, &xcb_xfixes_id);
    xcb_prefetch_extension_data(connection, &xcb_composite_id);
    xcb_prefetch_extension_data(connection, &xcb_render_id);

    damage_ext = xcb_get_extension_data(connection, &xcb_damage_id);
    if(damage_ext && damage_ext->present){
        xcb_damage_query_version_cookie_t cookie = xcb_damage_query_version(connection, XCB_DAMAGE_MAJOR_VERSION, XCB_DAMAGE_MINOR_VERSION);
        xcb_damage_query_version_reply_t *reply = xcb_damage_query_version_reply(connection, cookie, NULL);
        if(reply){
            if(reply->major_version >= 1)
                printf("HAS DAMAGE\n"); 
            free(reply);
        }
    }

    xfixes_ext = xcb_get_extension_data(connection, &xcb_xfixes_id);
    if(xfixes_ext && xfixes_ext->present){
        xcb_xfixes_query_version_cookie_t cookie = xcb_xfixes_query_version(connection, XCB_XFIXES_MAJOR_VERSION, XCB_XFIXES_MINOR_VERSION);
        xcb_xfixes_query_version_reply_t *reply = xcb_xfixes_query_version_reply(connection, cookie, NULL);
        if(reply){
            if(reply->major_version >= 1)
                printf("HAS XFIXES\n"); 
            free(reply);
        }
    }

    composite_ext = xcb_get_extension_data(connection, &xcb_composite_id);
    if(composite_ext && composite_ext->present){
        xcb_composite_query_version_cookie_t cookie = xcb_composite_query_version(connection, XCB_COMPOSITE_MAJOR_VERSION, XCB_COMPOSITE_MINOR_VERSION);
        xcb_composite_query_version_reply_t *reply = xcb_composite_query_version_reply(connection, cookie, NULL);
        if(reply){
            if(reply->major_version >= 1)
                printf("HAS COMPOSITE\n"); 
            free(reply);
        }
    }

    render_ext = xcb_get_extension_data(connection, &xcb_render_id);
    if(render_ext && render_ext->present){
        xcb_render_query_version_cookie_t cookie = xcb_render_query_version(connection, XCB_RENDER_MAJOR_VERSION, XCB_RENDER_MINOR_VERSION);
        xcb_render_query_version_reply_t *reply = xcb_render_query_version_reply(connection, cookie, NULL);
        if(reply){
            if(reply->major_version >= 1)
                printf("HAS RENDER\n"); 
            free(reply);
        }
    }

    // END OF THE EXTENTION VALIDATION

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
    xcb_composite_redirect_subwindows(connection, root, XCB_COMPOSITE_REDIRECT_MANUAL);

    const xcb_render_query_pict_formats_cookie_t format_cookies = xcb_render_query_pict_formats(connection);
    xcb_render_query_pict_formats_reply_t *format_reply = xcb_render_query_pict_formats_reply(connection, format_cookies, NULL);
    xcb_render_pictvisual_t *pict_forminfo = xcb_render_util_find_visual_format(format_reply, screen->root_visual);

    root_format = pict_forminfo->format;
    
    root_picture = xcb_generate_id(connection);
    uint32_t values[] = { XCB_SUBWINDOW_MODE_INCLUDE_INFERIORS };
    xcb_render_create_picture(connection, root_picture, root, root_format, XCB_RENDER_CP_SUBWINDOW_MODE, values);

    draw(0);
    xcb_flush(connection);
}

void event_loop() {
    xcb_generic_event_t *event;
    while ((event = xcb_wait_for_event(connection))) {
        uint8_t response_type = event->response_type & ~0x80;

        if(response_type == (XCB_DAMAGE_NOTIFY + damage_ext->first_event)){
                xcb_damage_notify_event_t *dn = (xcb_damage_notify_event_t *) event;

                printf("Recebi DamageNotify: level=%u, área=(%d, %d, %u x %u)\n",
                   dn->level,
                   dn->area.x, dn->area.y,
                   dn->area.width, dn->area.height);

                draw(dn->damage);

                xcb_damage_subtract(
                    connection,
                    dn->damage,
                    XCB_XFIXES_REGION_NONE,
                    XCB_XFIXES_REGION_NONE
                );
                xcb_flush(connection);
        }

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
            case XCB_CONFIGURE_NOTIFY: {
                auto configure = reinterpret_cast<xcb_configure_notify_event_t*>(event);
                client* client = find_client(configure->window);
                if (client != nullptr) {
                    xcb_get_geometry_reply_t *geometry = xcb_get_geometry_reply(
                        connection, xcb_get_geometry(connection, configure->window), NULL);

                    // uint32_t config_values[] = { (uint32_t )geometry->x + CLIENT_BORDER_SIZE, (uint32_t)geometry->y + CLIENT_BORDER_SIZE, (uint32_t)geometry->width, (uint32_t)geometry->height, XCB_STACK_MODE_ABOVE};
                    //
                    // std::cout << "testando " << client->window.id << std::endl;
                    // 
                    // xcb_configure_window(
                    //     connection,
                    //     client->window.id,
                    //     XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y | XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT | XCB_CONFIG_WINDOW_STACK_MODE,
                    //     config_values
                    // );
                    xcb_flush(connection);
                }
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
