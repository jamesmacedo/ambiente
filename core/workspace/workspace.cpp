#include "config/config.h"
#include "workspace.h"
#include "client.h"

#include "utils/client.cpp"

#include <xcb/xcb.h>
#include <xcb/damage.h>
#include <xcb/render.h>
#include <xcb/composite.h>
#include <xcb/xcb_renderutil.h>
#include <iostream>
// #include <vector>

WorkspaceManager::WorkspaceManager() : workspaces{workspace{}} {}

WorkspaceManager::~WorkspaceManager() {}

void WorkspaceManager::arrange(xcb_render_picture_t buffer) {
  for (client c : workspaces[current_workspace].clients) {
    c.draw(buffer);
    // if(c.damage == damage){
    // }
  }
}

void WorkspaceManager::add_client(xcb_map_request_event_t *e) {

    xcb_window_t client_id = e->window;

    xcb_damage_damage_t damage = xcb_generate_id(connection);
    xcb_damage_create(connection, damage, client_id, XCB_DAMAGE_REPORT_LEVEL_RAW_RECTANGLES);

    // xcb_window_t frame = xcb_generate_id(connection);
    // uint32_t mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
    // uint32_t frame_vals[3] = {
    //     0xffffff,
    //     XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_BUTTON_PRESS |
    //     XCB_EVENT_MASK_BUTTON_RELEASE | XCB_EVENT_MASK_POINTER_MOTION
    // };

    int width = screen->width_in_pixels; 
    int height = screen->height_in_pixels;
    
    int x = 0;
    int y = 0;

    // xcb_create_window(connection,
    //   screen->root_depth,
    //   frame, root,
    //   x, y, width, height,
    //   0,
    //   XCB_WINDOW_CLASS_INPUT_OUTPUT,
    //   screen->root_visual,
    //   mask, frame_vals
    // );
    // xcb_reparent_window(connection, client_id, frame, CLIENT_BORDER_SIZE, CLIENT_BORDER_SIZE);
    
    uint32_t config_values[] = { 
        CLIENT_BORDER_SIZE, 
        CLIENT_BORDER_SIZE,
        (uint32_t)(width - 2 * CLIENT_BORDER_SIZE), // width (accounting for borders)
        (uint32_t)(height - 2 * CLIENT_BORDER_SIZE), // height (accounting for borders)
        0
    };

    xcb_configure_window(
        connection,
        client_id,
        XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y | XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT | XCB_CONFIG_WINDOW_STACK_MODE,
        config_values
    );

    // xcb_map_window(connection, frame);
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

    std::pair<xcb_pixmap_t, xcb_render_picture_t> cli = create_picture_from_window(client_id, pict_format);

    struct client c = {
        .window = {client_id, cli.first, cli.second},
        .shape = shape, 
        // .frame = frame,
        .damage = damage,
        .pinned = 0,
        .resizing = 0
    };

    workspaces[current_workspace].clients.insert(workspaces[current_workspace].clients.begin(), c); 
    
    free(attr_reply);
    xcb_flush(connection);
}

void WorkspaceManager::previous(){
    std::cout << "anterior" << std::endl;
    if(current_workspace > 0){
        current_workspace--;
    }
}

void WorkspaceManager::next(){
    std::cout << "proximo" << std::endl;
    if(current_workspace != MAX_WORKSPACES && current_workspace < workspaces.size()){
        workspaces.push_back({});
        current_workspace++;
    }
}
