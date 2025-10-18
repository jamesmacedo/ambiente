#include "workspace.h"
#include "client.h"
#include "config/config.h"

#include "utils/client.cpp"

#include "config/config.h"
#include "utils/wallpaper.cpp"
#include <algorithm>
#include <iostream>
#include <xcb/composite.h>
#include <xcb/damage.h>
#include <xcb/render.h>
#include <xcb/xcb.h>
#include <xcb/xcb_renderutil.h>

int avail_width = 1280 - (WORKSPACE_GAP_SIZE*2), avail_height = 720 - (WORKSPACE_GAP_SIZE*2);

Workspace::Workspace(std::string wallpaper) {
  xcb_pixmap_t wallpaper_pix =
      generate_wallpaper_pixmap(wallpaper, screen, connection, root);
  root_tile = xcb_generate_id(connection);
  xcb_render_create_picture(connection, root_tile, wallpaper_pix, root_format,
                            0, NULL);
}

Workspace::~Workspace() {}

void Workspace::damaged(xcb_damage_notify_event_t *dn) {
  printf("Recebi DamageNotify: level=%u, área=(%d, %d, %u x %u)\n", dn->level,
         dn->area.x, dn->area.y, dn->area.width, dn->area.height);

  if (Client *c = this->find_client(dn->drawable)) {
    if (!c->get_window().picture) {
      xcb_get_window_attributes_cookie_t attr_cookie =
          xcb_get_window_attributes(connection, c->get_window().id);
      xcb_get_window_attributes_reply_t *attr_reply =
          xcb_get_window_attributes_reply(connection, attr_cookie, NULL);
      xcb_render_pictvisual_t *pict_format = xcb_render_util_find_visual_format(
          xcb_render_util_query_formats(connection), attr_reply->visual);

      std::pair<xcb_pixmap_t, xcb_render_picture_t> cli =
          create_picture_from_window(c->get_window().id, pict_format);

      c->update_visuals(cli.first, cli.second);

      std::cout << "PICTURE CRIADA " << cli.second << std::endl;

      free(attr_reply);
    }

    c->draw(root_buffer);

    // c->draw(buffer);
    //
    // xcb_damage_subtract(connection, dn->damage, XCB_XFIXES_REGION_NONE,
    //                     XCB_XFIXES_REGION_NONE);
    xcb_flush(connection);
  }
}

void Workspace::arrange() {
  std::cout << "Arranging clients" << std::endl;
  // int cursor_x = WORKSPACE_GAP_SIZE, cursor_y = WORKSPACE_GAP_SIZE;
  // int i = 1;
    
  std::vector<Client*> vis;

    vis.reserve(this->get_clients().size());
    for(auto& c: this->get_clients()){
        if(c.is_mapped()) vis.push_back(&c);
    }
    if (vis.empty()) return;

    if(vis.size() == 1){
        vis[0]->update_topology(WORKSPACE_GAP_SIZE, WORKSPACE_GAP_SIZE, avail_width, avail_height);
        return;
    }

    int w_left = (avail_width - (WORKSPACE_GAP_SIZE)/2)/2;
    int w_right = avail_width - (WORKSPACE_GAP_SIZE)/2 - w_left;

    int x_left = WORKSPACE_GAP_SIZE;
    int x_right = WORKSPACE_GAP_SIZE + w_left + (WORKSPACE_GAP_SIZE/2);
    int y = WORKSPACE_GAP_SIZE;
    int h = avail_height;

    vis[0]->update_topology(x_left, y, w_left, h);
    vis[1]->update_topology(x_right, y, w_right, h);

    this->draw(connection);
}

void Workspace::draw(xcb_connection_t *connection) {

  if (!root_buffer) {
    root_buffer = xcb_generate_id(connection);

    xcb_pixmap_t root_tmp = xcb_generate_id(connection);

    xcb_create_pixmap(connection, screen->root_depth, root_tmp, root,
                      screen->width_in_pixels, screen->height_in_pixels);
    xcb_render_create_picture(connection, root_buffer, root, root_format, 0,
                              NULL);

    xcb_free_pixmap(connection, root_tmp);
  }

  if (root_tile) {
    xcb_render_composite(connection, XCB_RENDER_PICT_OP_SRC, root_tile,
                         XCB_RENDER_PICTURE_NONE, root_buffer, 0, 0, 0, 0, 0, 0,
                         screen->width_in_pixels, screen->height_in_pixels);
  }

  for (auto &client : get_clients()) {
    if (client.is_mapped())
      client.draw(root_buffer);
  }

  xcb_render_composite(connection, XCB_RENDER_PICT_OP_SRC, root_buffer,
                       XCB_RENDER_PICTURE_NONE, root_picture, 0, 0, 0, 0, 0, 0,
                       screen->width_in_pixels, screen->height_in_pixels);

  xcb_flush(connection);
}

Client *Workspace::find_client(xcb_window_t window) {
  std::vector<Client> &c = get_clients();
  auto it = std::find_if(c.begin(), c.end(), [&](Client &c) {
    std::cout << c.get_window().id << std::endl;
    return c.get_window().id == window;
  });
  return (it == c.end()) ? nullptr : &(*it);
}

void Workspace::add_client(xcb_map_request_event_t *e) {

  xcb_window_t client_id = e->window;

  xcb_damage_damage_t damage = xcb_generate_id(connection);
  xcb_damage_create(connection, damage, client_id,
                    XCB_DAMAGE_REPORT_LEVEL_RAW_RECTANGLES);

  entity en = {client_id, 0, 0};
  Client c(en, damage);
  c.set_mapped(true);
  clients.insert(clients.begin(), c);

  this->arrange();

  xcb_map_window(connection, client_id);
  xcb_flush(connection);

  // xcb_window_t frame = xcb_generate_id(connection);
  // uint32_t mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
  // uint32_t frame_vals[3] = {
  //     0xffffff,
  //     XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_BUTTON_PRESS |
  //     XCB_EVENT_MASK_BUTTON_RELEASE | XCB_EVENT_MASK_POINTER_MOTION
  // };

  // xcb_create_window(connection,
  //   screen->root_depth,
  //   frame, root,
  //   x, y, width, height,
  //   0,
  //   XCB_WINDOW_CLASS_INPUT_OUTPUT,
  //   screen->root_visual,
  //   mask, frame_vals
  // );
  // xcb_reparent_window(connection, client_id, frame, CLIENT_BORDER_SIZE,
  // CLIENT_BORDER_SIZE);

  // xcb_map_window(connection, frame);
}
