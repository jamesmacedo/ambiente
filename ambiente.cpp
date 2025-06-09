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

#include <xcb/xcb_ewmh.h>
#include <xcb/damage.h>
#include <xcb/render.h>
#include <xcb/composite.h>
#include <xcb/xfixes.h>
#include <xcb/xcb_renderutil.h>

#include "./app/render/render_client.h"
#include "./app/core/models/client.h"
#include "./app/client/client_logic.h"

#include "./app/config.h"
#include "./app/workspace.h"
#include "./app/client.h"

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

xcb_ewmh_connection_t *ewmh;

ClientLogic* client_logic = nullptr;

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

    xcb_pixmap_t wallpaper_pix = generate_wallpaper_pixmap("/home/nemo/Documentos/wallpaper/by1dc24mnfn71.png");

    root_tile = xcb_generate_id(connection);

    xcb_render_create_picture(connection, root_tile, wallpaper_pix, root_format, 0, NULL);
    xcb_render_composite(connection, XCB_RENDER_PICT_OP_SRC, root_tile, XCB_RENDER_PICTURE_NONE, root_buffer, 0,0,0,0,0,0, screen->width_in_pixels, screen->height_in_pixels);
}

void draw(xcb_damage_damage_t damage) {    

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

    ClientModel* client = client_logic->getWorkspace().findClient(damage);
    if(client){
        std::cout << "drawing client" << std::endl;
        client_logic->draw(client, root_buffer);
    }

    xcb_render_composite(connection, XCB_RENDER_PICT_OP_SRC, root_buffer, XCB_RENDER_PICTURE_NONE, root_picture, 0,0,0,0,0,0, screen->width_in_pixels, screen->height_in_pixels);

    xcb_flush(connection);
}

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
    
    ewmh = (xcb_ewmh_connection_t*)malloc(sizeof(xcb_ewmh_connection_t));
    xcb_intern_atom_cookie_t *ewmh_cookies = xcb_ewmh_init_atoms(connection, ewmh);
    if (!xcb_ewmh_init_atoms_replies(ewmh, ewmh_cookies, NULL)) {
        fprintf(stderr, "Erro ao inicializar EWMH\n");
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
    xcb_composite_redirect_subwindows(connection, root, XCB_COMPOSITE_REDIRECT_MANUAL);

    const xcb_render_query_pict_formats_cookie_t format_cookies = xcb_render_query_pict_formats(connection);
    xcb_render_query_pict_formats_reply_t *format_reply = xcb_render_query_pict_formats_reply(connection, format_cookies, NULL);
    xcb_render_pictvisual_t *pict_forminfo = xcb_render_util_find_visual_format(format_reply, screen->root_visual);

    root_format = pict_forminfo->format;
    
    root_picture = xcb_generate_id(connection);
    uint32_t values[] = { XCB_SUBWINDOW_MODE_INCLUDE_INFERIORS };
    xcb_render_create_picture(connection, root_picture, root, root_format, XCB_RENDER_CP_SUBWINDOW_MODE, values);

    xcb_get_window_attributes_cookie_t attr_cookie = xcb_get_window_attributes(connection, screen->root);
    xcb_get_window_attributes_reply_t *attr_reply = xcb_get_window_attributes_reply(connection, attr_cookie, NULL);

    if (!attr_reply) return;

    xcb_visualtype_t *visual = nullptr;
    xcb_depth_iterator_t depth_iter = xcb_screen_allowed_depths_iterator(screen);

    for (; depth_iter.rem; xcb_depth_next(&depth_iter)) {
        xcb_visualtype_iterator_t visual_iter = xcb_depth_visuals_iterator(depth_iter.data);
        for (; visual_iter.rem; xcb_visualtype_next(&visual_iter)) {
            if (visual_iter.data->visual_id == attr_reply->visual) {
                visual = visual_iter.data;
                break;
            }
        }
        if (visual) break;
    }

    if (!visual) {
        free(attr_reply);
        return;
    }

    RenderContext render_ctx = {connection, root, screen, visual};
    Rectangle screen_bounds(0, 0, screen->width_in_pixels, screen->height_in_pixels);

    client_logic = new ClientLogic(render_ctx, screen_bounds);

    draw(0);
    xcb_flush(connection);
}

void event_loop() {
    xcb_generic_event_t *event;

    while ((event = xcb_wait_for_event(connection))) {
        uint8_t response_type = event->response_type & ~0x80;

        if(response_type == (XCB_DAMAGE_NOTIFY + damage_ext->first_event)){
                xcb_damage_notify_event_t *dn = (xcb_damage_notify_event_t *) event;

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
                client_logic->handleMapRequest(event);
                break;
            }
            case XCB_BUTTON_PRESS: {
                // xcb_button_press_event_t *be = (xcb_button_press_event_t *)event;
                break;
            }
            case XCB_MOTION_NOTIFY: {
                // client_logic->handleMotionNotify(event);
                break;
            }
            case XCB_BUTTON_RELEASE: {
                client_button_release(event);
                client_logic->handleButtonRelease(event);
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

    xcb_ewmh_connection_wipe(ewmh);
    free(ewmh);

    xcb_disconnect(connection);

    return 0;
}
