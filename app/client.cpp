#include <iostream>
#include <xcb/xcb.h>
#include <xcb/damage.h>
#include <xcb/composite.h>
#include <xcb/render.h>
#include <xcb/xcb_renderutil.h>

#include <cairo/cairo.h>
#include <cairo/cairo-xcb.h>

#include <algorithm>
#include <utility>
#include "client.h"
#include "math.h"

#include "./ui/utils.h"
#include "./workspace.h"
#include "./config.h"

client *current_resizing_client = nullptr;
client *active_client = nullptr;
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

client* find_client(xcb_window_t window) {
    for (auto it = workspaces[current_workspace].clients.begin(); it != workspaces[current_workspace].clients.end(); ++it) {
        if(it->frame == window){
            return &(*it);
        }   
    } 
    return nullptr;
}

void client::draw(xcb_render_picture_t buffer){
    // std::cout << "client " << this->child << " at " << this->shape.x << " " << this->shape.y << std::endl;
    // std::cout << "testando" << std::endl;

    uint32_t frame_position[] = { (uint32_t )this->shape.x, (uint32_t)this->shape.y, (uint32_t)this->shape.width, (uint32_t)this->shape.height + FRAME_BAR_HEIGHT, XCB_STACK_MODE_ABOVE};
    xcb_configure_window(
        connection,
        this->frame,
        XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y |XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT | XCB_CONFIG_WINDOW_STACK_MODE,
        frame_position 
    );

    uint32_t bar_position[] = { (uint32_t )this->shape.x, (uint32_t)this->shape.y, (uint32_t)this->shape.width, FRAME_BAR_HEIGHT, XCB_STACK_MODE_ABOVE};
    xcb_configure_window(
        connection,
        this->toolbar,
        XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y |XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT | XCB_CONFIG_WINDOW_STACK_MODE,
        bar_position 
    );
    xcb_render_composite(connection, XCB_RENDER_PICT_OP_OVER, this->bar.picture, XCB_RENDER_PICTURE_NONE, this->frame_v.picture, 0,0,0,0, this->shape.x, this->shape.y, this->shape.width, 30);

    uint32_t client_position[] = { (uint32_t )this->shape.x, (uint32_t)this->shape.y + FRAME_BAR_HEIGHT, (uint32_t)this->shape.width, (uint32_t)this->shape.height, XCB_STACK_MODE_ABOVE};
    xcb_configure_window(
        connection,
        this->child,
        XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y |XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT | XCB_CONFIG_WINDOW_STACK_MODE,
        client_position
    );
    xcb_render_composite(connection, XCB_RENDER_PICT_OP_OVER, this->window.picture, XCB_RENDER_PICTURE_NONE, this->frame_v.picture, 0,0,0,0, this->shape.x, this->shape.y, this->shape.width, this->shape.height);

    
    xcb_render_composite(connection, XCB_RENDER_PICT_OP_OVER, this->frame_v.picture, this->mask.picture, buffer, 0,0,0,0, this->shape.x, this->shape.y, this->shape.width, this->shape.height);

};

xcb_window_t ambi_create_frame(xcb_window_t child, int16_t width, int16_t height, uint32_t x, uint32_t y){
    xcb_window_t frame = xcb_generate_id(connection);
    uint32_t mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
    uint32_t frame_vals[3] = {
        0xff0000,
        XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_BUTTON_PRESS |
        XCB_EVENT_MASK_BUTTON_RELEASE | XCB_EVENT_MASK_POINTER_MOTION
    };

    xcb_create_window(connection,
      screen->root_depth,
      frame, root,
      x, y, width, height,
      0,
      XCB_WINDOW_CLASS_INPUT_OUTPUT,
      screen->root_visual,
      mask, frame_vals
    );

    xcb_reparent_window(connection, child, frame, x, y);

    return frame;
}

xcb_window_t ambi_create_bar(xcb_window_t parent, int width, int height, int x, int y){
    xcb_window_t toolbar = xcb_generate_id(connection);
    uint32_t mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
    uint32_t frame_vals[3] = {
        0xffffff,
        XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_BUTTON_PRESS |
        XCB_EVENT_MASK_BUTTON_RELEASE | XCB_EVENT_MASK_POINTER_MOTION
    };

    xcb_reparent_window(connection, toolbar, parent, x, y);
    
    xcb_create_window(connection,
      screen->root_depth,
      toolbar, root,
      x, y, width, height,
      0,
      XCB_WINDOW_CLASS_INPUT_OUTPUT,
      screen->root_visual,
      mask, frame_vals
    );
    
    uint32_t config_values[] = { (uint32_t )x, (uint32_t)y, (uint32_t)width, (uint32_t)height, XCB_STACK_MODE_ABOVE};
                    
    xcb_configure_window(
        connection,
        toolbar,
        XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y |XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT | XCB_CONFIG_WINDOW_STACK_MODE,
        config_values
    );

    return toolbar;
}

std::pair<xcb_pixmap_t, xcb_render_picture_t> ambi_picture_from_window(xcb_window_t client, xcb_render_pictvisual_t *pict_format){

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

    int base_width = 500; 
    int base_height = 500;

    int x = 0;
    int y = 0; 

    xcb_window_t frame_id = ambi_create_frame(client_id, base_width, base_height, x, y);
    xcb_window_t bar_id = ambi_create_bar(frame_id, base_width, FRAME_BAR_HEIGHT, x, FRAME_BAR_HEIGHT);

    // xcb_map_window(connection, client_id); 
    xcb_map_window(connection, bar_id); 
    xcb_map_window(connection, frame_id);

    xcb_get_window_attributes_cookie_t attr_cookie = xcb_get_window_attributes(connection, client_id);
    xcb_get_window_attributes_reply_t *attr_reply = xcb_get_window_attributes_reply(connection, attr_cookie, NULL);
    xcb_render_pictvisual_t  *pict_format = xcb_render_util_find_visual_format(xcb_render_util_query_formats(connection), attr_reply->visual);

    xcb_render_picture_t mask_picture = ambi_generate_mask(connection, root, base_width, base_height);
    
    // std::pair<xcb_pixmap_t, xcb_render_picture_t> cli = ambi_picture_from_window(client_id, pict_format);
    std::pair<xcb_pixmap_t, xcb_render_picture_t> f = ambi_picture_from_window(frame_id, pict_format);
    std::pair<xcb_pixmap_t, xcb_render_picture_t> b = ambi_picture_from_window(bar_id, pict_format);

    xcb_rectangle_t shape = {
       .x = (int16_t) x,
       .y = (int16_t) y,
       .width = (uint16_t) base_width,
       .height = (uint16_t) base_height 
    };

    struct client c = {frame_id, client_id, bar_id, shape, damage, 
                        .window={client_id, 0, 0}, 
                        .frame_v={frame_id, f.first, f.second},
                        .mask={0,0,mask_picture},
                        .bar={bar_id,b.first,b.second}
    };

    workspaces[current_workspace].clients.insert(workspaces[current_workspace].clients.begin(), c);
}

void client_button_press(xcb_generic_event_t *event) {
    xcb_button_press_event_t *be = (xcb_button_press_event_t *)event;
    for (client c : workspaces[current_workspace].clients) {
        if (be->event == c.bar.id) {
            if(!is_moving){
                is_resizing = true;
                active_client = &c;
            }
        }

        if (be->event == c.frame) {
            std::cout << "press frame" << std::endl;
            if(be->detail == 1 && (be->event_x > c.shape.width - CLIENT_BORDER_SIZE && be->event_y < c.shape.width)) {
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
        if (!is_moving || !active_client) return;

        xcb_get_geometry_reply_t *geometry = xcb_get_geometry_reply(
            connection, xcb_get_geometry(connection, motion->event), NULL);

        if (!geometry) return; 

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

        int32_t new_width = (motion->root_x - current_resizing_client->shape.x);
        int32_t new_height = (motion->root_y - current_resizing_client->shape.y);

        // if(USE_GRID)
        //     client_find_grid_pos(&new_width, &new_height);

        // if (new_width < CLIENT_MIN_WIDTH) new_width = CLIENT_MIN_WIDTH;
        // if (new_height < CLIENT_MIN_HEIGHT) new_height = CLIENT_MIN_HEIGHT;

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
        current_resizing_client->shape.width = geometry->width;
        current_resizing_client->shape.height = geometry->height;
        current_resizing_client = nullptr;
    }

    if(is_moving){
        is_moving = false;
        current_resizing_client->shape.x = geometry->x;
        current_resizing_client->shape.y = geometry->y;
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
