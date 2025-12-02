#include <algorithm>
#include <iostream>

#include "math.h"

#include <xcb/composite.h>
#include <xcb/damage.h>
#include <xcb/render.h>
#include <xcb/xcb.h>
#include <xcb/xcb_renderutil.h>

#include "config/config.h"
#include "core/workspace/client.h"
#include "core/workspace/workspace.h"

Client::Client() {}
Client::Client(entity window, xcb_damage_damage_t damage)
    : window(window), damage(damage) {}
Client::~Client() {}

void Client::init(entity window, xcb_damage_damage_t damage){
    this->window = window;
    this->damage = damage;
}

void Client::draw(xcb_render_picture_t buffer) {
  xcb_render_composite(connection, XCB_RENDER_PICT_OP_OVER,
                       this->window.picture, XCB_RENDER_PICTURE_NONE, buffer, 0,
                       0, 0, 0, this->shape.x, this->shape.y, this->shape.width,
                       this->shape.height);
};

void Client::update_visuals(xcb_pixmap_t pixmap, xcb_render_picture_t picture) {
  this->window.pixmap = pixmap;
  this->window.picture = picture;
}

void Client::update_topology(int x, int y, int width, int height) {
  std::cout << "X: " << x << "Y: " << y << std::endl;

  xcb_rectangle_t shape = {
        (int16_t)x, (int16_t)y,
        (uint16_t)width, (uint16_t)height,
  };

  this->shape = shape;

  uint32_t config_values[] = {(uint32_t)this->shape.x, (uint32_t)this->shape.y,
                              (uint32_t)(this->shape.width),
                              (uint32_t)(this->shape.height), 0};

  xcb_configure_window(connection, this->window.id,
                       XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y |
                           XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT |
                           XCB_CONFIG_WINDOW_STACK_MODE,
                       config_values);
}

// void client_button_press(xcb_generic_event_t *event) {
//     xcb_button_press_event_t *be = (xcb_button_press_event_t *)event;
//
//     client* clicked_client = find_client(be->event);
//     if (!clicked_client) return;
//
//     if (be->detail == 1) {
//         int resize_zone_size = 20;
//         if (be->event_x > clicked_client->width - resize_zone_size &&
//             be->event_y > clicked_client->height - resize_zone_size) {
//
//             std::cout << "Starting resize for client: " <<
//             clicked_client->frame << std::endl; is_resizing = true;
//             current_resizing_client = clicked_client;
//
//             start_x = be->root_x;
//             start_y = be->root_y;
//             initial_width = clicked_client->width;
//             initial_height = clicked_client->height;
//         } else {
//             std::cout << "Starting move for client: " <<
//             clicked_client->frame << std::endl; is_moving = true;
//             current_resizing_client = clicked_client;
//
//             start_x = be->event_x;
//             start_y = be->event_y;
//             initial_x = clicked_client->x;
//             initial_y = clicked_client->y;
//         }
//     }
//
//     if (be->detail == 3) {
//         std::cout << "Starting move (right-click) for client: " <<
//         clicked_client->frame << std::endl; is_moving = true;
//         current_resizing_client = clicked_client;
//
//         start_x = be->event_x;
//         start_y = be->event_y;
//         initial_x = clicked_client->x;
//         initial_y = clicked_client->y;
//     }
//
//     xcb_flush(connection);
// }

// void client_motion_handle(xcb_generic_event_t *event) {
//   xcb_motion_notify_event_t *motion = (xcb_motion_notify_event_t *)event;
//
//   if (is_moving && current_resizing_client) {
//     int32_t new_x = initial_x + (motion->root_x - (initial_x + start_x));
//     int32_t new_y = initial_y + (motion->root_y - (initial_y + start_y));
//
//     if (new_x < config.x) {
//       new_x = config.x;
//     } else if (new_x + current_resizing_client->width >
//                config.x + config.width) {
//       new_x = config.x + config.width - current_resizing_client->width;
//     }
//
//     if (new_y < config.y) {
//       new_y = config.y;
//     } else if (new_y + current_resizing_client->height >
//                config.y + config.height) {
//       new_y = config.y + config.height - current_resizing_client->height;
//     }
//
//     if (USE_GRID) {
//       client_find_grid_pos(&new_x, &new_y);
//     }
//
//     current_resizing_client->x = new_x;
//     current_resizing_client->y = new_y;
//     current_resizing_client->shape.x = new_x;
//     current_resizing_client->shape.y = new_y;
//
//     uint32_t values[] = {(uint32_t)new_x, (uint32_t)new_y};
//     // xcb_configure_window(connection, current_resizing_client->frame,
//     //                      XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y,
//     values); xcb_flush(connection);
//   }
//
//   if (is_resizing && current_resizing_client) {
//     int32_t new_width = initial_width + (motion->root_x - start_x);
//     int32_t new_height = initial_height + (motion->root_y - start_y);
//
//     if (USE_GRID) {
//       client_find_grid_pos(&new_width, &new_height);
//     }
//
//     if (new_width < CLIENT_MIN_WIDTH)
//       new_width = CLIENT_MIN_WIDTH;
//     if (new_height < CLIENT_MIN_HEIGHT)
//       new_height = CLIENT_MIN_HEIGHT;
//
//     current_resizing_client->width = new_width;
//     current_resizing_client->height = new_height;
//     current_resizing_client->shape.width = new_width;
//     current_resizing_client->shape.height = new_height;
//
//     // uint32_t frame_values[] = {(uint32_t)new_width, (uint32_t)new_height};
//     // xcb_configure_window(connection, current_resizing_client->frame,
//     //                      XCB_CONFIG_WINDOW_WIDTH |
//     XCB_CONFIG_WINDOW_HEIGHT,
//     //                      frame_values);
//
//     uint32_t client_values[] = {
//         (uint32_t)(new_width - 2 * CLIENT_BORDER_SIZE),
//         (uint32_t)(new_height - 2 * CLIENT_BORDER_SIZE)};
//     xcb_configure_window(connection, current_resizing_client->window.id,
//                          XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT,
//                          client_values);
//
//     xcb_flush(connection);
//   }
// }

// void client_button_release(xcb_generic_event_t *event) {
//   if (is_resizing) {
//     std::cout << "Finished resizing client" << std::endl;
//     is_resizing = false;
//   }
//
//   if (is_moving) {
//     std::cout << "Finished moving client" << std::endl;
//     is_moving = false;
//   }
//
//   current_resizing_client = nullptr;
//   xcb_flush(connection);
// }

// void remove_client(xcb_generic_event_t *event) {
//     xcb_destroy_notify_event_t *destroy_notify = (xcb_destroy_notify_event_t
//     *)event; xcb_window_t window = destroy_notify->window;
//
//     for (auto it = workspaces[current_workspace].clients.begin(); it !=
//     workspaces[current_workspace].clients.end(); ++it) {
//         if (it->window.id == window || it->frame == window) {
//             std::cout << "Removing client " << window << " with frame " <<
//             it->frame << std::endl;
//
//             if (it->frame != 0) {
//                 xcb_destroy_window(connection, it->frame);
//             }
//
//             workspaces[current_workspace].clients.erase(it);
//             xcb_flush(connection);
//             return;
//         }
//     }
// }
