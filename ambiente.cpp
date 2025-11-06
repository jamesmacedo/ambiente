#include <xcb/xcb.h>
#include <xcb/xproto.h>
#include <xcb/xcb_ewmh.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <xcb/xcb_keysyms.h>
#include <X11/keysym.h>

#include <cairo/cairo.h>
#include <cairo/cairo-xcb.h>

#include <xcb/damage.h>
#include <xcb/render.h>
#include <xcb/composite.h>
#include <xcb/xfixes.h>
#include <xcb/xcb_renderutil.h>

#include "core/workspace/workspace_manager.h"
#include "core/barra/barra.h"
#include "core/workspace/client.h"
#include "core/input/keymanager.h"
#include "config/keybindings.h"
#include "config/config.h"

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

xcb_render_picture_t root_picture = 0;
xcb_render_picture_t root_buffer = 0;
// xcb_render_picture_t root_tile = 0;

WorkspaceManager wom;
// Barra barra;

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
        XCB_EVENT_MASK_ENTER_WINDOW | XCB_EVENT_MASK_LEAVE_WINDOW | XCB_EVENT_MASK_FOCUS_CHANGE |
        XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE
    };
 
    xcb_change_window_attributes(connection, root, XCB_CW_EVENT_MASK, masks);
    xcb_composite_redirect_subwindows(connection, root, XCB_COMPOSITE_REDIRECT_MANUAL);

    const xcb_render_query_pict_formats_cookie_t format_cookies = xcb_render_query_pict_formats(connection);
    xcb_render_query_pict_formats_reply_t *format_reply = xcb_render_query_pict_formats_reply(connection, format_cookies, NULL);
    xcb_render_pictvisual_t *pict_forminfo = xcb_render_util_find_visual_format(format_reply, screen->root_visual);

    root_format = pict_forminfo->format;
    
    wom = WorkspaceManager(screen->width_in_pixels, screen->height_in_pixels);
    wom.current()->draw(connection);

    // barra = Barra();
    xcb_flush(connection);
}

void event_loop() {

    KeyManager km = KeyManager(connection, root); 
    register_keybindings(km);

    km.grab_all();

    xcb_generic_event_t *event;
    while ((event = xcb_wait_for_event(connection))) {

        uint8_t response_type = event->response_type & ~0x80;

        if(response_type == (XCB_DAMAGE_NOTIFY + damage_ext->first_event)){
                xcb_damage_notify_event_t *dn = (xcb_damage_notify_event_t *) event;

                std::cout << "Damage recebido: " << dn->drawable << std::endl;

                // if(dn->drawable == barra.get_id()){
                //     std::cout << "Damage barra: " << dn->drawable << std::endl;
                //     barra.draw();
                // }

                wom.current()->damaged(dn);
        }

        switch (response_type) {
            case XCB_FOCUS_IN:{
                xcb_focus_in_event_t *e = (xcb_focus_in_event_t *)event;
                wom.current()->focused(e);
                break;
            }
            case XCB_MAP_REQUEST:{
                xcb_map_request_event_t *e = (xcb_map_request_event_t *)event;
                wom.current()->add_client(e);
                break;
            }
            case XCB_DESTROY_NOTIFY:{
                xcb_destroy_notify_event_t *e = (xcb_destroy_notify_event_t  *)event;
                wom.current()->remove_client(e);
                break;
            }
            case XCB_BUTTON_PRESS: {
                // client_button_press(event);
                break;
            }
            case XCB_MOTION_NOTIFY: {
                // client_motion_handle(event);
                break;
            }
            case XCB_BUTTON_RELEASE: {
                // client_button_release(event);
                break;
            }
            case XCB_CONFIGURE_NOTIFY:{
                xcb_configure_notify_event_t *e = (xcb_configure_notify_event_t  *)event;
                wom.current()->configure_client(e);
                break;
            }
            case XCB_KEY_PRESS: {
                xcb_key_press_event_t *e = (xcb_key_press_event_t *)event;
                km.handle_event(e);
                break;
            }
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
