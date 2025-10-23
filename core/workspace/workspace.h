#pragma once
#include <vector>
#include <string>
#include <xcb/xcb.h>

#include <xcb/render.h>
#include <xcb/damage.h>

class Client;

class Workspace {
public:
  Workspace();
  ~Workspace();

std::pair<Client*, size_t> find_client(xcb_window_t window);
void arrange();
void draw(xcb_connection_t* connection);
void add_client(xcb_map_request_event_t *e);
void remove_client(xcb_destroy_notify_event_t *e);
void configure_client(xcb_configure_notify_event_t *e);
void damaged(xcb_damage_notify_event_t *dn);
void focused(xcb_focus_in_event_t *e); 
void pin_client();
void switch_client();
std::vector<Client>& get_clients(){ return clients; };

private:
    std::vector<Client> clients;

    xcb_render_picture_t root_picture = 0;
    xcb_render_picture_t root_buffer = 0;
    xcb_render_picture_t root_tile = 0;
    Client* pinned_client = nullptr;
};
