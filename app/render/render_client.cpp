#include "render_client.h"
#include <iostream>
#include "../core/models/client.h"
#include "../config.h"
#include <cmath>

void CairoClientRenderer::renderTitlebar(const ClientModel* client) {
    if (!client->getFrameId()) return;
    
    xcb_get_window_attributes_cookie_t attr_cookie = 
        xcb_get_window_attributes(context.connection, client->getFrameId());
    xcb_get_window_attributes_reply_t *attr_reply = 
        xcb_get_window_attributes_reply(context.connection, attr_cookie, NULL);
    
    if (!attr_reply) return;
    
    xcb_visualtype_t *visual = findVisual(attr_reply->visual);
    if (!visual) {
        free(attr_reply);
        return;
    }
    
    Size size = client->getSize();
    
    cairo_surface_t *surface = cairo_xcb_surface_create(
        context.connection, 
        client->getFrameId(), 
        visual,
        size.width, size.height
    );
    
    cairo_t *cr = cairo_create(surface);
    
    cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
    
    drawTitlebarBackground(cr, size.width, size.height);
    
    // drawTitlebarText(cr, client->getTitle(), size.width);
    
    cairo_surface_flush(surface);
    cairo_destroy(cr);
    cairo_surface_destroy(surface);
    free(attr_reply);
    
    xcb_flush(context.connection);
}

void CairoClientRenderer::drawTitlebarBackground(cairo_t* cr, int width, int height) {
    const int radius = OUTER_RADIUS;
    const int x = 0, y = 0;
    
    cairo_set_source_rgba(cr, 0, 0, 51.0/255, 0.8);
    
    cairo_new_path(cr);
    cairo_arc(cr, x + width - radius, y + radius, radius, -M_PI/2, 0);
    cairo_line_to(cr, x + width, y + height - radius);
    cairo_arc(cr, x + width - radius, y + height - radius, radius, 0, M_PI/2);
    cairo_line_to(cr, x + radius, y + height);
    cairo_arc(cr, x + radius, y + height - radius, radius, M_PI/2, M_PI);
    cairo_line_to(cr, x, y + radius);
    cairo_arc(cr, x + radius, y + radius, radius, M_PI, -M_PI/2);
    cairo_close_path(cr);
    cairo_fill(cr);
}

void CairoClientRenderer::drawTitlebarText(cairo_t* cr, const std::string& title, int width) {
    cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.95);
    cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, 11);
    
    cairo_text_extents_t extents;
    cairo_text_extents(cr, title.c_str(), &extents);
    
    double text_y = (TITLEBAR_HEIGHT + extents.height) / 2.0;
    cairo_move_to(cr, FRAME_PADDING, text_y);
    cairo_show_text(cr, title.c_str());
}

xcb_window_t CairoClientRenderer::createClientFrame(const ClientModel* client) {
    Rectangle bounds = client->getBounds();
    
    xcb_window_t frame = xcb_generate_id(context.connection);
    xcb_create_window(context.connection,
        context.screen->root_depth,
        frame, context.screen->root,
        bounds.position.x, bounds.position.y,
        bounds.size.width, bounds.size.height,
        0,
        XCB_WINDOW_CLASS_INPUT_OUTPUT,
        context.screen->root_visual,
        XCB_CW_BORDER_PIXEL | XCB_CW_EVENT_MASK, 
        (uint32_t[]){
            0x000000, 
            XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_BUTTON_PRESS |
            XCB_EVENT_MASK_BUTTON_RELEASE | XCB_EVENT_MASK_POINTER_MOTION,
        }
    );
    
    return frame;
}

xcb_visualtype_t* CairoClientRenderer::findVisual(xcb_visualid_t visual_id){
    xcb_visualtype_t *visual = nullptr;
    xcb_depth_iterator_t depth_iter = xcb_screen_allowed_depths_iterator(screen);
    
    for (; depth_iter.rem; xcb_depth_next(&depth_iter)) {
        xcb_visualtype_iterator_t visual_iter = xcb_depth_visuals_iterator(depth_iter.data);
        for (; visual_iter.rem; xcb_visualtype_next(&visual_iter)) {
            if (visual_iter.data->visual_id == visual_id) {
                visual = visual_iter.data;
                break;
            }
        }
        if (visual) break;
    }
    
    return visual;
}
