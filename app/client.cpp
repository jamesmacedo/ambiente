#include <iostream>
#include <algorithm>

#include "math.h"

#include <xcb/xcb.h>
#include <xcb/damage.h>
#include <xcb/render.h>
#include <xcb/composite.h>
#include <xcb/xcb_renderutil.h>

#include <xcb/xcb_ewmh.h>
#include <string>

#include <cairo/cairo.h>
#include <cairo/cairo-xcb.h>

#include "client.h"
#include "./workspace.h"
#include "./config.h"

client *current_resizing_client = nullptr;
int start_x, start_y = 0;
int initial_width, initial_height = 0;
int initial_x, initial_y = 0;
bool is_resizing, is_moving = false;

xcb_visualtype_t* find_argb_visual(xcb_visualid_t *visual_id, uint8_t *depth) {
    xcb_depth_iterator_t depth_iter = xcb_screen_allowed_depths_iterator(screen);
    
    for (; depth_iter.rem; xcb_depth_next(&depth_iter)) {
        if (depth_iter.data->depth == 32) {
            xcb_visualtype_iterator_t visual_iter = xcb_depth_visuals_iterator(depth_iter.data);
            if (visual_iter.rem) {
                *visual_id = visual_iter.data->visual_id;
                *depth = depth_iter.data->depth;
                return visual_iter.data;
            }
        }
    }
    
    *visual_id = screen->root_visual;
    *depth = screen->root_depth;
    return nullptr;
}

void swap_client() {
    if (workspaces[current_workspace].clients.size() >= 2) {
        std::swap(workspaces[current_workspace].clients[current_workspace], workspaces[current_workspace].clients[1]);
    }
}

void client_find_grid_pos(int32_t *x, int32_t *y) {
    *x = floor(*x/CLIENT_POSITION_SPACING) * CLIENT_POSITION_SPACING;
    *y = floor(*y/CLIENT_POSITION_SPACING) * CLIENT_POSITION_SPACING;
}

client* find_client(xcb_window_t window) {
    for (auto it = workspaces[current_workspace].clients.begin(); it != workspaces[current_workspace].clients.end(); ++it) {
        if(it->frame == window || it->window.id == window || it->titlebar.id == window){
            return &(*it);
        }   
    } 
    return nullptr;
}

void client::draw(xcb_render_picture_t buffer){    
    xcb_render_composite(connection, XCB_RENDER_PICT_OP_OVER, this->window.picture, XCB_RENDER_PICTURE_NONE, buffer, 0,0,0,0,this->shape.x, this->shape.y,this->shape.width,this->shape.height);
};

std::string get_window_title(xcb_window_t window) {
    xcb_ewmh_get_utf8_strings_reply_t title_reply;
    
    if (xcb_ewmh_get_wm_name_reply(ewmh, 
            xcb_ewmh_get_wm_name(ewmh, window),
            &title_reply, NULL)) {
        std::string title(title_reply.strings, title_reply.strings_len);
        xcb_ewmh_get_utf8_strings_reply_wipe(&title_reply);
        return title;
    }
    
    xcb_get_property_cookie_t cookie = xcb_get_property(
        connection, 0, window,
        XCB_ATOM_WM_NAME, XCB_ATOM_STRING,
        0, 256
    );
    
    xcb_get_property_reply_t *reply = xcb_get_property_reply(connection, cookie, NULL);
    if (reply && xcb_get_property_value_length(reply) > 0) {
        char *title_str = (char*)xcb_get_property_value(reply);
        int title_len = xcb_get_property_value_length(reply);
        std::string title(title_str, title_len);
        free(reply);
        return title;
    }
    
    if (reply) free(reply);
    return "Sem Título";
}

void draw_titlebar(client *c) {
    if (!c->titlebar.id) return;
    
    xcb_get_window_attributes_cookie_t attr_cookie = xcb_get_window_attributes(connection, c->titlebar.id);
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
    
    const int width = c->titlebar.width;
    const int height = c->shape.height;

    cairo_surface_t *surface = cairo_xcb_surface_create(
        connection, c->titlebar.id, visual,
        c->titlebar.width, height 
    );
    
    cairo_t *cr = cairo_create(surface);
    
    // cairo_save(cr);
    // cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
    // cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 0.0);
    // cairo_rectangle(cr, 0, 0, c->titlebar.width, 30);
    // cairo_fill(cr);
    // cairo_restore(cr);
    
    // Voltar ao modo normal de composição
    cairo_set_operator(cr, CAIRO_OPERATOR_OVER);

    const int radius = 20;

    const int x = 0;
    const int y = 0;
    
    cairo_set_source_rgba(cr, 0.0, 0.1, 0.1, 0.8); // Cinza escuro com 80% opacidade
    // cairo_rectangle(cr, 0, 0, width, height);
    cairo_move_to(cr, x, y + height);

    cairo_line_to(cr, x, y + radius);

    cairo_arc(cr, x + radius, y + radius, radius, M_PI, 3 * M_PI / 2);

    cairo_line_to(cr, x + width - radius, y);

    cairo_arc(cr, x + width - radius, y + radius, radius, 3 * M_PI / 2, 0);

    cairo_line_to(cr, x + width, y + height);

    cairo_line_to(cr, x, y + height);

    cairo_close_path(cr);

    cairo_fill(cr);
    
    // cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 1.0); // Branco com 95% opacidade
    // cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    // cairo_set_font_size(cr, 11);
    
    // cairo_text_extents_t extents;
    // cairo_text_extents(cr, c->titlebar.title.c_str(), &extents);
    
    // Centralizar o texto verticalmente
    // double text_y = (30 + extents.height) / 2.0;
    // cairo_move_to(cr, TITLEBAR_PADDING, text_y);
    // cairo_show_text(cr, c->titlebar.title.c_str());

    // cairo_set_source_rgba(cr, 0.0, 0.5, 0.0, 1.0); 
    // cairo_set_line_width(cr, 4.0);
    // cairo_move_to(cr, 0, height);
    // cairo_line_to(cr, width, height);
    // cairo_stroke(cr);

    // cairo_set_source_rgba(cr, 0.0, 0.5, 0.0, 1.0);
    // cairo_arc(cr, width - extents.width, height - (height/4)*2, height/4, 0, 2 * M_PI);
    // cairo_fill(cr);

    cairo_surface_flush(surface);
    
    cairo_destroy(cr);
    cairo_surface_destroy(surface);
    free(attr_reply);
    
    xcb_flush(connection);
}

std::pair<xcb_pixmap_t, xcb_render_picture_t>  create_picture_from_window(xcb_window_t client, xcb_render_pictvisual_t *pict_format){

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

void add_client(xcb_generic_event_t *event) {
    xcb_map_request_event_t *map_request_event = (xcb_map_request_event_t *)event;
    xcb_window_t client_id = map_request_event->window;

    xcb_damage_damage_t damage = xcb_generate_id(connection);
    xcb_damage_create(connection, damage, client_id, XCB_DAMAGE_REPORT_LEVEL_RAW_RECTANGLES);

    int width = screen->width_in_pixels / 2; 
    int height = screen->height_in_pixels - 30 - CLIENT_POSITION_SPACING * 2;
    int total_height = height + 30;

    int x = CLIENT_POSITION_SPACING;
    int y = CLIENT_POSITION_SPACING;

    
    // xcb_visualid_t argb_visual_id;
    // uint8_t argb_depth;
    // xcb_visualtype_t *argb_visual = find_argb_visual(&argb_visual_id, &argb_depth);
    // 
    // xcb_colormap_t colormap = xcb_generate_id(connection);
    // xcb_create_colormap(connection, XCB_COLORMAP_ALLOC_NONE, colormap, root, argb_visual_id);
    
    // Creating titlebar
    xcb_window_t titlebar = xcb_generate_id(connection);
    xcb_create_window(connection,
        screen->root_depth,
        titlebar, root,
        x, y,
        width, total_height,
        0,
        XCB_WINDOW_CLASS_INPUT_OUTPUT,
        screen->root_visual,
        XCB_CW_BORDER_PIXEL | XCB_CW_EVENT_MASK, 
        (uint32_t[]){
            0x000000, 
            XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_BUTTON_PRESS |
            XCB_EVENT_MASK_BUTTON_RELEASE | XCB_EVENT_MASK_POINTER_MOTION,
        }
    );
    
    // Configuring client
    xcb_reparent_window(connection, client_id, titlebar, 0, 30);
    xcb_configure_window(
        connection,
        client_id,
        XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y | 
        XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT | 
        XCB_CONFIG_WINDOW_STACK_MODE,
        (uint32_t[]){
            CLIENT_BORDER_SIZE, 
            30,
            (uint32_t)width - 2 * CLIENT_BORDER_SIZE,
            (uint32_t)(height - CLIENT_BORDER_SIZE),
            0
        }
    );

    xcb_map_window(connection, titlebar);
    xcb_map_window(connection, client_id);

    xcb_rectangle_t shape = {
       .x = (int16_t) x,
       .y = (int16_t) y,
       .width = (uint16_t) width,
       .height = (uint16_t) total_height
    };

    xcb_get_window_attributes_cookie_t attr_cookie = xcb_get_window_attributes(connection, titlebar);
    xcb_get_window_attributes_reply_t *attr_reply = xcb_get_window_attributes_reply(connection, attr_cookie, NULL);
    xcb_render_pictvisual_t *pict_format = xcb_render_util_find_visual_format(
        xcb_render_util_query_formats(connection), attr_reply->visual);

    std::pair<xcb_pixmap_t, xcb_render_picture_t> cli = create_picture_from_window(titlebar, pict_format);

    struct client c = {
        .window = {
            client_id, 
            cli.first, 
            cli.second
        },
        .shape = shape, 
        .frame = titlebar,
        .damage = damage,
        .titlebar = {
            .id = titlebar, 
            .width = width,
            .title = get_window_title(client_id)
        },
        .width = width,
        .height = total_height,
        .x = x,
        .y = y,
        .fixed = 0,
        .resizing = 0
    };

    workspaces[current_workspace].clients.insert(workspaces[current_workspace].clients.begin(), c);
    
    draw_titlebar(&workspaces[current_workspace].clients[0]);
    
    free(attr_reply);
    xcb_flush(connection);
}

void create_titlebar(client *c) {
    c->titlebar.title = get_window_title(c->window.id);
    
    draw_titlebar(c);
    
    xcb_flush(connection);
}

void client_button_press(xcb_generic_event_t *event) {
    xcb_button_press_event_t *be = (xcb_button_press_event_t *)event;
    
    client* clicked_client = find_client(be->event);
    if (!clicked_client) return;

    if (be->detail == 1) {
        int resize_zone_size = 20;
        if (be->event_x > clicked_client->width - resize_zone_size && 
            be->event_y > clicked_client->height - resize_zone_size) {
            
            std::cout << "Starting resize for client: " << clicked_client->frame << std::endl;
            is_resizing = true;
            current_resizing_client = clicked_client;
            
            start_x = be->root_x;
            start_y = be->root_y;
            initial_width = clicked_client->width;
            initial_height = clicked_client->height;
        } else {
            std::cout << "Starting move for client: " << clicked_client->frame << std::endl;
            is_moving = true;
            current_resizing_client = clicked_client;
            
            start_x = be->event_x;
            start_y = be->event_y;
            initial_x = clicked_client->x;
            initial_y = clicked_client->y;
        }
    }
    
    if (be->detail == 3) {
        std::cout << "Starting move (right-click) for client: " << clicked_client->frame << std::endl;
        is_moving = true;
        current_resizing_client = clicked_client;
        
        start_x = be->event_x;
        start_y = be->event_y;
        initial_x = clicked_client->x;
        initial_y = clicked_client->y;
    }
    
    xcb_flush(connection);
}

void client_motion_handle(xcb_generic_event_t *event) {
    xcb_motion_notify_event_t *motion = (xcb_motion_notify_event_t *)event;

    if (is_moving && current_resizing_client) {
        int32_t new_x = initial_x + (motion->root_x - (initial_x + start_x));
        int32_t new_y = initial_y + (motion->root_y - (initial_y + start_y));

        if (new_x < config.x) {
            new_x = config.x;
        } else if (new_x + current_resizing_client->width > config.x + config.width) {
            new_x = config.x + config.width - current_resizing_client->width;
        }

        if (new_y < config.y) {
            new_y = config.y;
        } else if (new_y + current_resizing_client->height > config.y + config.height) {
            new_y = config.y + config.height - current_resizing_client->height;
        }
    
        if (USE_GRID) {
            client_find_grid_pos(&new_x, &new_y);
        }

        current_resizing_client->x = new_x;
        current_resizing_client->y = new_y;
        current_resizing_client->shape.x = new_x;
        current_resizing_client->shape.y = new_y;

        uint32_t values[] = {(uint32_t)new_x, (uint32_t)new_y};
        xcb_configure_window(connection, current_resizing_client->frame,
                             XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y, values);
        xcb_flush(connection);
    }

    if (is_resizing && current_resizing_client) {
        int32_t new_width = initial_width + (motion->root_x - start_x);
        int32_t new_height = initial_height + (motion->root_y - start_y);

        if (USE_GRID) {
            client_find_grid_pos(&new_width, &new_height);
        }

        if (new_width < CLIENT_MIN_WIDTH) new_width = CLIENT_MIN_WIDTH;
        if (new_height < CLIENT_MIN_HEIGHT) new_height = CLIENT_MIN_HEIGHT;

        current_resizing_client->width = new_width;
        current_resizing_client->height = new_height;
        current_resizing_client->shape.width = new_width;
        current_resizing_client->shape.height = new_height;

        uint32_t frame_values[] = {(uint32_t)new_width, (uint32_t)new_height};
        xcb_configure_window(connection, current_resizing_client->frame,
                             XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT,
                             frame_values);

        uint32_t client_values[] = {
            (uint32_t)(new_width - 2 * CLIENT_BORDER_SIZE), 
            (uint32_t)(new_height - 2 * CLIENT_BORDER_SIZE)
        };
        xcb_configure_window(connection, current_resizing_client->window.id,
                             XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT,
                             client_values);
        
        xcb_flush(connection);
    }
}

void client_button_release(xcb_generic_event_t *event) {
    if (is_resizing) {
        std::cout << "Finished resizing client" << std::endl;
        is_resizing = false;
    }
    
    if (is_moving) {
        std::cout << "Finished moving client" << std::endl;
        is_moving = false;
    }
    
    current_resizing_client = nullptr;
    xcb_flush(connection);
}

void remove_client(xcb_generic_event_t *event) {
    xcb_destroy_notify_event_t *destroy_notify = (xcb_destroy_notify_event_t *)event;
    xcb_window_t window = destroy_notify->window;

    for (auto it = workspaces[current_workspace].clients.begin(); it != workspaces[current_workspace].clients.end(); ++it) {
        if (it->window.id == window || it->frame == window) {
            std::cout << "Removing client " << window << " with frame " << it->frame << std::endl;
            
            if (it->frame != 0) {
                xcb_destroy_window(connection, it->frame);
            }
            
            workspaces[current_workspace].clients.erase(it);
            xcb_flush(connection);
            return;
        }
    }
}
