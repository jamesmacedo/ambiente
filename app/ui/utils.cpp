#include <iostream>
#include <math.h>
#include <xcb/render.h>
#include <xcb/xcb_renderutil.h>

#include <cairo/cairo.h>
#include <cairo/cairo-xcb.h>

#include "./utils.h"
#include "../config.h"


ambi_render_formats get_render_formats(xcb_connection_t *connection) {
    ambi_render_formats  formats = {0};
    
    xcb_render_query_pict_formats_cookie_t cookie = 
        xcb_render_query_pict_formats(connection);
    xcb_render_query_pict_formats_reply_t *reply = 
        xcb_render_query_pict_formats_reply(connection, cookie, NULL);
    
    if (reply) {
        xcb_render_pictforminfo_iterator_t it = 
            xcb_render_query_pict_formats_formats_iterator(reply);
        
        for (; it.rem; xcb_render_pictforminfo_next(&it)) {
            if (it.data->depth == 32 &&
                it.data->direct.alpha_mask &&
                it.data->direct.red_mask &&
                it.data->direct.green_mask &&
                it.data->direct.blue_mask) {
                formats.rgba = it.data;
            }
            else if (it.data->depth == 24 &&
                    !it.data->direct.alpha_mask &&
                    it.data->direct.red_mask &&
                    it.data->direct.green_mask &&
                    it.data->direct.blue_mask) {
                formats.rgb = it.data;
            }
            else if (it.data->depth == 8 &&
                    it.data->direct.alpha_mask == 0xff &&
                    !it.data->direct.red_mask &&
                    !it.data->direct.green_mask &&
                    !it.data->direct.blue_mask) {
                formats.a8 = it.data;
            }
        }
        
        free(reply);
    }
    
    return formats;
}

xcb_render_picture_t ambi_generate_mask(xcb_connection_t *connection,xcb_drawable_t drawable, int width, int height){

    int x, y = 0;

    xcb_pixmap_t pixmap = xcb_generate_id(connection);
    xcb_create_pixmap(connection,
                     32,
                     pixmap,
                     drawable,
                     width, height);

    xcb_render_picture_t picture = xcb_generate_id(connection);
    xcb_render_create_picture(connection,
                             picture,
                             pixmap,
                             formats.rgba->id,
                             0, NULL);

    cairo_surface_t *surface = 
        cairo_xcb_surface_create_with_xrender_format(connection,
                                                   screen,
                                                   pixmap,
                                                   formats.rgba,
                                                   width, height);

    cairo_t *cr = cairo_create(surface);

    cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
    cairo_paint(cr);

    cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
    cairo_set_source_rgba(cr, 43/255.0, 45/255.0, 45/255.0, 1); //cor 

    double radius = 10.0;

    cairo_new_path(cr);

    cairo_arc(cr, x + width - radius, y + radius, radius, -M_PI/2, 0);
    cairo_line_to(cr, x+width, y+height - radius);

    cairo_arc(cr, x + width - radius, y + height - radius, radius, 0, M_PI/2);
    cairo_line_to(cr, x+radius, y + height);

    cairo_arc(cr, x + radius, y + height - radius, radius, M_PI/2, M_PI);
    cairo_line_to(cr, x, y + radius);

    cairo_arc(cr, x + radius, y + radius, radius, M_PI, -M_PI/2);

    cairo_close_path(cr);
    cairo_fill(cr);

    /* Finaliza os desenhos */
    cairo_destroy(cr);
    cairo_surface_destroy(surface);
    // xcb_free_pixmap(connection, pixmap);

    return picture;
}
