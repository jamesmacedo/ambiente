#include <iostream>
#include <algorithm>

#include "math.h"

#include <xcb/xcb.h>
#include <xcb/damage.h>
#include <xcb/render.h>
#include <xcb/composite.h>
#include <xcb/xcb_renderutil.h>

#include "client.h"
#include "./workspace.h"
#include "./config.h"

client *current_resizing_client = nullptr;
int start_x, start_y = 0;
int initial_width, initial_height = 0;
int initial_x, initial_y = 0;
bool is_resizing, is_moving = false;

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
        if(it->frame == window || it->window.id == window){
            return &(*it);
        }   
    } 
    return nullptr;
}

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

void client::draw(xcb_render_picture_t buffer){    
    xcb_render_composite(connection, XCB_RENDER_PICT_OP_OVER, this->window.picture, XCB_RENDER_PICTURE_NONE, buffer, 0,0,0,0,this->shape.x, this->shape.y,this->shape.width,this->shape.height);
};

void add_client(xcb_generic_event_t *event) {

    xcb_map_request_event_t *map_request_event = (xcb_map_request_event_t *)event;
    xcb_window_t client_id = map_request_event->window;

    xcb_damage_damage_t damage = xcb_generate_id(connection);
    xcb_damage_create(connection, damage, client_id, XCB_DAMAGE_REPORT_LEVEL_RAW_RECTANGLES);

    xcb_window_t frame = xcb_generate_id(connection);
    uint32_t mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
    uint32_t frame_vals[3] = {
        0xffffff,
        XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_BUTTON_PRESS |
        XCB_EVENT_MASK_BUTTON_RELEASE | XCB_EVENT_MASK_POINTER_MOTION
    };

    int width = 500; 
    int height = 500;

    int x = CLIENT_POSITION_SPACING;
    int y = CLIENT_POSITION_SPACING;

    xcb_create_window(connection,
      screen->root_depth,
      frame, root,
      x, y, width, height,
      0,
      XCB_WINDOW_CLASS_INPUT_OUTPUT,
      screen->root_visual,
      mask, frame_vals
    );
    
    xcb_reparent_window(connection, client_id, frame, CLIENT_BORDER_SIZE, CLIENT_BORDER_SIZE);
    
    uint32_t config_values[] = { 
        CLIENT_BORDER_SIZE, 
        CLIENT_BORDER_SIZE,
        (uint32_t)(width - 2 * CLIENT_BORDER_SIZE), // width (accounting for borders)
        (uint32_t)(height - 2 * CLIENT_BORDER_SIZE), // height (accounting for borders)
        0
    };

    std::cout << "frame id: " << frame << std::endl; 
                    
    xcb_configure_window(
        connection,
        client_id,
        XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y | XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT | XCB_CONFIG_WINDOW_STACK_MODE,
        config_values
    );

    xcb_map_window(connection, frame);
    xcb_map_window(connection, client_id);

    xcb_rectangle_t shape = {
       .x = (int16_t) x,
       .y = (int16_t) y,
       .width = (uint16_t) width,
       .height = (uint16_t) height 
    };

    xcb_get_window_attributes_cookie_t attr_cookie = xcb_get_window_attributes(connection, client_id);
    xcb_get_window_attributes_reply_t *attr_reply = xcb_get_window_attributes_reply(connection, attr_cookie, NULL);
    xcb_render_pictvisual_t  *pict_format = xcb_render_util_find_visual_format(xcb_render_util_query_formats(connection), attr_reply->visual);

    std::pair<xcb_pixmap_t, xcb_render_picture_t> cli = create_picture_from_window(frame, pict_format);

    struct client c = {
        .window = {client_id, cli.first, cli.second},
        .shape = shape, 
        .frame = frame,
        .damage = damage,
        .width = width,
        .height = height,
        .x = x,
        .y = y,
        .fixed = 0,
        .resizing = 0
    };

    workspaces[current_workspace].clients.insert(workspaces[current_workspace].clients.begin(), c); 
    
    free(attr_reply);
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
