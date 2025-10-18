#pragma once
#include <vector>
#include <string>
#include <xcb/xcb.h>

#include <xcb/render.h>
#include <xcb/damage.h>

class Client;

class Workspace {
public:
  Workspace(std::string wallpaper);
  ~Workspace();

Client* find_client(xcb_window_t window);
void arrange();
void draw(xcb_connection_t* connection);
void add_client(xcb_map_request_event_t *e);
void damaged(xcb_damage_notify_event_t *dn);
std::vector<Client>& get_clients(){ return clients; };

private:
    std::vector<Client> clients;

    xcb_render_picture_t root_picture = 0;
    xcb_render_picture_t root_buffer = 0;
    xcb_render_picture_t root_tile = 0;
};
