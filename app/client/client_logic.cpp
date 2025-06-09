#include "client_logic.h"
#include "../config.h"
#include <xcb/xcb.h>
#include <iostream>
#include <xcb/composite.h>
#include <xcb/xcb_renderutil.h>
#include <math.h>

extern xcb_connection_t* connection;

typedef struct {
    xcb_render_pictforminfo_t *rgba;
    xcb_render_pictforminfo_t *rgb;
    xcb_render_pictforminfo_t *a8;
} ambi_render_formats;


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

xcb_render_picture_t ClientLogic::generateMask(xcb_connection_t *connection,
                                              xcb_drawable_t drawable, 
                                              int width, int height) {
    ambi_render_formats formats = get_render_formats(connection);

    xcb_pixmap_t pixmap = xcb_generate_id(connection);
    xcb_create_pixmap(connection, 8, pixmap, drawable, width, height);

    xcb_render_picture_t picture = xcb_generate_id(connection);
    xcb_render_create_picture(connection, picture, pixmap, 
                             formats.a8->id, 0, NULL);

    cairo_surface_t *surface = cairo_image_surface_create(CAIRO_FORMAT_A8, width, height);
    cairo_t *cr = cairo_create(surface);

    cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
    cairo_paint(cr);

    cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
    cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 1.0);

    double radius = INNER_RADIUS;
    int x = 0, y = 0;

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

    cairo_surface_flush(surface);
    unsigned char *data = cairo_image_surface_get_data(surface);
    int stride = cairo_image_surface_get_stride(surface);
    
    xcb_gcontext_t gc = xcb_generate_id(connection);
    xcb_create_gc(connection, gc, pixmap, 0, NULL);
    
    xcb_put_image(connection, XCB_IMAGE_FORMAT_Z_PIXMAP, pixmap, gc,
                  width, height, 0, 0, 0, 8,
                  stride * height, data);
    
    xcb_free_gc(connection, gc);
    cairo_destroy(cr);
    cairo_surface_destroy(surface);

    return picture;
}

void ClientLogic::draw(ClientModel* client, xcb_render_picture_t buffer){    

    Rectangle shape = client->getBounds();
    xcb_render_composite(connection, XCB_RENDER_PICT_OP_OVER, client->getFramePicture(), XCB_RENDER_PICTURE_NONE, buffer, 0,0,0,0, shape.position.x, shape.position.y,shape.size.width, shape.size.height);

    xcb_render_composite(
            connection, 
            XCB_RENDER_PICT_OP_OVER, 
            client->getClientPicture(), 
            client->getMask(), 
            buffer, 0,0,0,0,
            shape.position.x + 10, shape.position.y + 10, shape.size.width - (2*10), shape.size.height - (2*10));

};


std::pair<xcb_pixmap_t, xcb_render_picture_t>  ClientLogic::createPictureFromWindow(xcb_window_t client, xcb_render_pictvisual_t *pict_format){

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

void ClientLogic::handleMapRequest(xcb_generic_event_t* event) {
    xcb_map_request_event_t* map_request = (xcb_map_request_event_t*)event;
    
    uint16_t width = renderer.getContext().screen->width_in_pixels - (2 * 20);
    uint16_t height = renderer.getContext().screen->height_in_pixels - (2 * 20);

    Rectangle initial_bounds(20, 20, width, height);

    xcb_map_window(connection, map_request->window);
    xcb_flush(connection);

    const xcb_render_query_pict_formats_cookie_t format_cookies = xcb_render_query_pict_formats(connection);
    xcb_render_query_pict_formats_reply_t *format_reply = xcb_render_query_pict_formats_reply(connection, format_cookies, NULL);
    xcb_render_pictvisual_t *pict_forminfo = xcb_render_util_find_visual_format(format_reply, renderer.getContext().screen->root_visual);

    std::pair<xcb_pixmap_t, xcb_render_picture_t> client_tuple = createPictureFromWindow(map_request->window, pict_forminfo);


    
    ClientModel* client = workspace.addClient(
            {
                .id = map_request->window,
                .pixmap = client_tuple.first,
                .picture = client_tuple.second,
            },
            initial_bounds);
    
    xcb_render_picture_t mask = generateMask(connection, renderer.getContext().root, width - (2*10), height - (2*10)); 

    client->setMask(mask);

    xcb_damage_damage_t damage = xcb_generate_id(connection);
    xcb_damage_create(connection, damage, map_request->window, XCB_DAMAGE_REPORT_LEVEL_RAW_RECTANGLES);

    client->setDamageId(damage);

    xcb_window_t frame = renderer.createClientFrame(client);

    // xcb_reparent_window(connection, map_request->window, frame, FRAME_PADDING, FRAME_PADDING);
    xcb_configure_window(
        connection,
        map_request->window,
        XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y | 
        XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT | 
        XCB_CONFIG_WINDOW_STACK_MODE,
        (uint32_t[]){
            (uint32_t)FRAME_PADDING, 
            (uint32_t)FRAME_PADDING,
            (uint32_t)initial_bounds.size.width - (2*10),
            (uint32_t)(initial_bounds.size.height - (2*10)),
            0
        }
    );

    xcb_map_window(connection, frame);
    xcb_map_window(connection, map_request->window);
    xcb_flush(connection);

    std::pair<xcb_pixmap_t, xcb_render_picture_t> frame_tuple = createPictureFromWindow(frame, pict_forminfo);
    client->setFrame((Entity){
        .id = frame,
        .pixmap = frame_tuple.first,
        .picture = frame_tuple.second
    });
    
    // renderer.configureClientWindow(client);
    // renderer.mapClientWindows(client);
    
    renderer.renderTitlebar(client);
    
    std::cout << "Client mapped: " << map_request->window << std::endl;
}

void ClientLogic::handleButtonPress(xcb_generic_event_t* event) {
    xcb_button_press_event_t* button_event = (xcb_button_press_event_t*)event;
    
    ClientModel* client = workspace.findClient(button_event->event);
    if (!client) return;
    
    Point click_pos(button_event->event_x, button_event->event_y);
    
    if (button_event->detail == 1) {
        if (isInResizeZone(client, click_pos)) {
            // startResizing(client, Point(button_event->root_x, button_event->root_y));
        } else {
            startMoving(client, click_pos);
        }
    }
    
    // focusClient(client);
}

void ClientLogic::handleButtonRelease(xcb_generic_event_t* event) {
    xcb_button_press_event_t* button_event = (xcb_button_press_event_t*)event;
    
    ClientModel* client = workspace.findClient(button_event->event);
    if (!client) return;
    
    if(interaction_state.mode == InteractionMode::MOVING) {
        interaction_state.mode = InteractionMode::NONE;        
    }  
    // focusClient(client);
}

void ClientLogic::handleMotionNotify(xcb_generic_event_t* event) {
    xcb_motion_notify_event_t* motion = (xcb_motion_notify_event_t*)event;
    Point current_pos(motion->root_x, motion->root_y);
    
    switch (interaction_state.mode) {
        case InteractionMode::MOVING:
            updateMoving(current_pos);
            break;
        case InteractionMode::RESIZING:
            // updateResizing(current_pos);
            break;
        default:
            break;
    }
}

void ClientLogic::startMoving(ClientModel* client, const Point& start_pos) {
    interaction_state.mode = InteractionMode::MOVING;
    interaction_state.target_client = client;
    interaction_state.start_position = start_pos;
    interaction_state.initial_position = client->getPosition();
    
    std::cout << "Started moving client" << std::endl;
}

void ClientLogic::updateMoving(const Point& current_pos) {
    if (!interaction_state.target_client) return;
    
    Point delta(
        current_pos.x - interaction_state.start_position.x,
        current_pos.y - interaction_state.start_position.y
    );
    
    Point new_position(
        interaction_state.initial_position.x + delta.x,
        interaction_state.initial_position.y + delta.y
    );
    
    // if (use_grid_snapping) {
    //     workspace.snapToGrid(new_position, grid_spacing);
    // }
    
    workspace.moveClient(interaction_state.target_client, new_position);
    // renderer.configureClientWindow(interaction_state.target_client);
}

bool ClientLogic::isInResizeZone(const ClientModel* client, const Point& position) {
    Rectangle resize_zone = client->getResizeZone();
    return resize_zone.contains(position);
}
