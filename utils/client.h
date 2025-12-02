#pragma once
#include <utility>
#include <xcb/render.h>

void client_find_grid_pos(int32_t *x, int32_t *y); 
std::pair<xcb_pixmap_t, xcb_render_picture_t> create_picture_from_window(xcb_window_t client, xcb_render_pictvisual_t *pict_format);
