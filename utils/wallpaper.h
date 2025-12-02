#pragma once
#include <string>
#include <xcb/xcb.h>

xcb_visualtype_t *get_visualtype(xcb_screen_t *screen);
std::string random_image();
xcb_pixmap_t generate_wallpaper_pixmap(xcb_screen_t* screen, xcb_connection_t* connection, xcb_window_t root);
