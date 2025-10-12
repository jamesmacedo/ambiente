#include "math.h"
#include "config/config.h"

#include <xcb/xcb.h>
#include <xcb/damage.h>
#include <xcb/render.h>
#include <xcb/composite.h>
#include <xcb/xcb_renderutil.h>
#include <iostream>

void client_find_grid_pos(int32_t *x, int32_t *y) {
    *x = floor(*x/CLIENT_POSITION_SPACING) * CLIENT_POSITION_SPACING;
    *y = floor(*y/CLIENT_POSITION_SPACING) * CLIENT_POSITION_SPACING;
}

// client* find_client(xcb_window_t window) {
//     for (auto it = workspaces[current_workspace].clients.begin(); it != workspaces[current_workspace].clients.end(); ++it) {
//         if(it->frame == window || it->window.id == window){
//             return &(*it);
//         }   
//     } 
//     return nullptr;
// }

std::pair<xcb_pixmap_t, xcb_render_picture_t> create_picture_from_window(xcb_window_t client, xcb_render_pictvisual_t *pict_format){

    xcb_pixmap_t pix = xcb_generate_id(connection); 
    xcb_void_cookie_t cookie = xcb_composite_name_window_pixmap_checked(connection, client, pix);
    xcb_generic_error_t *er = xcb_request_check(connection, cookie);
    if (er) {
         std::cerr << "Erro ao criar window pixmap:"
              << " código = " << (int)er->error_code
              << " sequência = " << er->sequence
              << " recurso = " << er->resource_id
              << " tipo menor = " << (int)er->minor_code
              << " tipo maior = " << (int)er->major_code
              << std::endl;
        free(er);
        exit(1);
    }

    uint32_t v[] = {XCB_SUBWINDOW_MODE_INCLUDE_INFERIORS};
    xcb_render_picture_t pic = xcb_generate_id(connection);
    xcb_render_create_picture(connection, pic, pix, pict_format->format, XCB_RENDER_CP_SUBWINDOW_MODE, &v);

    return std::make_pair(pix, pic);

}
