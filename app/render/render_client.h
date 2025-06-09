#ifndef CAIRO_CLIENT_RENDERER_H
#define CAIRO_CLIENT_RENDERER_H

#include "../core/models/client.h"
#include <cairo/cairo.h>
#include <cairo/cairo-xcb.h>
#include <xcb/xcb.h>
#include <xcb/render.h>

struct RenderContext {
    xcb_connection_t* connection;
    xcb_window_t root;
    xcb_screen_t* screen;
    xcb_visualtype_t* visual;
};

class CairoClientRenderer {
private:
    RenderContext context;
    
public:
    CairoClientRenderer(const RenderContext& ctx) : context(ctx) {}

    RenderContext getContext() { return context; }
    
    void renderTitlebar(const ClientModel* client);
    
    xcb_window_t createClientFrame(const ClientModel* client);
    xcb_window_t createTitlebar(const ClientModel* client);
    
    // void configureClientWindow(const ClientModel* client);
    // void mapClientWindows(const ClientModel* client);
    // void destroyClientFrame(const ClientModel* client);
    
private:
    void drawTitlebarBackground(cairo_t* cr, int width, int height);
    void drawTitlebarText(cairo_t* cr, const std::string& title, int width);
    void drawClientBorder(cairo_t* cr, const ClientModel* client);
    
    xcb_visualtype_t* findVisual(xcb_visualid_t visual_id);
};

#endif
