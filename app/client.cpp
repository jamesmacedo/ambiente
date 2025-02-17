#include <iostream>
#include <xcb/xcb.h>
#include <algorithm>
#include "client.h"
#include "math.h"
#include "./workspace.h"
#include "./config.h"

client *current_resizing_client = nullptr;
int start_x, start_y = 0;
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

void add_client(xcb_generic_event_t *event) {

    xcb_map_request_event_t *map_request_event = (xcb_map_request_event_t *)event;
    xcb_window_t client = map_request_event->window;

    std::cout << "Mapeando: " << client << std::endl;

    xcb_window_t frame = xcb_generate_id(connection);
    std::cout << "Frame ID: " << frame << std::endl;
    uint32_t mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
    uint32_t frame_vals[2] = {
        screen->white_pixel,
        XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_BUTTON_PRESS |
        XCB_EVENT_MASK_BUTTON_RELEASE | XCB_EVENT_MASK_POINTER_MOTION
    };

    // xcb_get_geometry_reply_t *geometry = xcb_get_geometry_reply(
    //     connection, xcb_get_geometry(connection, client), NULL);

    // int width = geometry->width;
    // int height = geometry->height;
    int width = 500; 
    int height = 500;

    int x = 50;
    int y = 50;
    
    std::cout << "Tamanho: " << width << "x" << height << std::endl;

    xcb_create_window(connection,
      screen->root_depth,
      frame, root,
      x, y, width, height,
      CLIENT_BORDER_SIZE ,
      XCB_WINDOW_CLASS_INPUT_OUTPUT,
      screen->root_visual,
      mask, frame_vals
    );

    // xcb_reparent_window(connection, client, frame, x, y);

    struct client c = {frame, client, width, height, x, y, false};
    workspaces[current_workspace].clients.insert(workspaces[current_workspace].clients.begin(), c); 
}

void client_button_press(xcb_generic_event_t *event) {
    xcb_button_press_event_t *be = (xcb_button_press_event_t *)event;
    for (client &c : workspaces[current_workspace].clients) {
        if (be->event == c.frame) {
            if(be->detail == 1 && (be->event_x > c.width - CLIENT_BORDER_SIZE  && be->event_y < c.width) && (be->event_y > c.height - CLIENT_BORDER_SIZE && be->event_y < c.height)) {
                std::cout << "Resizing: " << c.frame << std::endl;
                is_resizing = true;
                start_x = be->root_x;
                start_y = be->root_y;

                current_resizing_client = &c;

            }
            if(be->detail == 3){
                std::cout << "Moving: " << c.frame << std::endl;
                start_x = be->event_x;
                start_y = be->event_y;
                is_moving = true;
                current_resizing_client = &c;
            }
            break;
        }
    }
}

void client_motion_handle(xcb_generic_event_t *event) {
    xcb_motion_notify_event_t *motion = (xcb_motion_notify_event_t *)event;

    if(is_moving){
        if (!is_moving || !current_resizing_client) return;

        xcb_get_geometry_reply_t *geometry = xcb_get_geometry_reply(
            connection, xcb_get_geometry(connection, motion->event), NULL);

        if (!geometry) return; // Proteção contra ponteiro nulo

        std::cout << "X: " << geometry->x << " Y: " << geometry->y << std::endl;

        int32_t new_x = (int32_t)geometry->x + (int32_t)motion->event_x - (int32_t)start_x;
        int32_t new_y = (int32_t)geometry->y + (int32_t)motion->event_y - (int32_t)start_y;

        uint32_t janela_largura = geometry->width;
        uint32_t janela_altura = geometry->height;

        if (new_x < config.x) {
            new_x = config.x;
        } else if (new_x + janela_largura > config.x + config.width) {
            new_x = config.x + config.width - janela_largura;
        }

        if (new_y < config.y || new_x < 0) {
            new_y = config.y;
        } else if (new_y + janela_altura > config.y + config.height) {
            new_y = config.y + config.height - janela_altura;
        }
    
        if(USE_GRID)
            client_find_grid_pos(&new_x, &new_y);

        int32_t values[] = {new_x, new_y};
        xcb_configure_window(connection, current_resizing_client->frame,
                             XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y, values);
    }

    if (is_resizing) {

        if (!is_resizing || !current_resizing_client) return;

        int32_t new_width = (motion->root_x - current_resizing_client->x);
        int32_t new_height = (motion->root_y - current_resizing_client->y);

        if(USE_GRID)
            client_find_grid_pos(&new_width, &new_height);

        if (new_width < CLIENT_MIN_WIDTH) new_width = CLIENT_MIN_WIDTH;
        if (new_height < CLIENT_MIN_HEIGHT) new_height = CLIENT_MIN_HEIGHT;

        int32_t values[] = {new_width, new_height};
        xcb_configure_window(connection, current_resizing_client->frame,
                             XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT,
                             values);
        // xcb_configure_window(connection, current_resizing_client->child,
        //                      XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT,
        //                      values);
        xcb_flush(connection);
    }
        
}

void client_button_release(xcb_generic_event_t *event) {

    xcb_button_release_event_t *br = (xcb_button_release_event_t  *)event;

    xcb_get_geometry_reply_t *geometry = xcb_get_geometry_reply(
        connection, xcb_get_geometry(connection, br->event), NULL);

    if(is_resizing){
        is_resizing = false;
        current_resizing_client->width = geometry->width;
        current_resizing_client->height = geometry->height;
        current_resizing_client = nullptr;
    }

    if(is_moving){
        is_moving = false;
        current_resizing_client->x = geometry->x;
        current_resizing_client->y = geometry->y;
        current_resizing_client = nullptr;
    }
}

void remove_client(xcb_generic_event_t *event) {
    xcb_destroy_notify_event_t *destroy_notify = (xcb_destroy_notify_event_t *)event;

    xcb_window_t window = destroy_notify->window;

    for (auto it = workspaces[current_workspace].clients.begin(); it != workspaces[current_workspace].clients.end(); ++it) {
        if (it->child == window) {
            // std::cout << "Destruindo frame " << it->frame << " do cliente " << window << std::endl;
            xcb_destroy_window(connection, it->frame);
            workspaces[current_workspace].clients.erase(it);
            xcb_flush(connection);
            return;
        }
    }
}
