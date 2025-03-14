#ifndef UTILS_H
#define UTILS_H 

#include <xcb/xcb.h>
#include <xcb/render.h>

typedef struct {
    xcb_render_pictforminfo_t *rgba;
    xcb_render_pictforminfo_t *rgb;
    xcb_render_pictforminfo_t *a8;
} ambi_render_formats;

xcb_render_picture_t ambi_generate_mask(xcb_connection_t *connection, xcb_drawable_t drawable, int width, int height); 
ambi_render_formats get_render_formats(xcb_connection_t *connection);

#endif
