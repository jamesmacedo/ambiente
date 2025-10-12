#pragma once
#include <vector>
#include <xcb/xcb.h>

#include <xcb/render.h>
#include <xcb/damage.h>
class client;

struct workspace {
    std::vector<client> clients;
};

class WorkspaceManager {
public:
  WorkspaceManager();
  ~WorkspaceManager();

void arrange(xcb_render_picture_t buffer);
void add_client(xcb_map_request_event_t *e);
void arrange_clients();

void previous();
void next();

private:
    int current_workspace = 0;
    std::vector<workspace> workspaces;
    int width, height;
};
