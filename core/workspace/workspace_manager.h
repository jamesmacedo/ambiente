#pragma once
#include <vector>
#include <xcb/xcb.h>

#include <xcb/damage.h>
#include <xcb/render.h>
#include "workspace.h"
#include <memory.h>

class Client;

class WorkspaceManager {
public:
  WorkspaceManager(int width, int height);
  WorkspaceManager();
  ~WorkspaceManager();

  Workspace* current() { return &workspaces[c_workspace]; };
  void add_client(Client c);

  void previous();
  void next();

private:
  int c_workspace = 0;
  std::vector<Workspace> workspaces = {};
  int width, height;
};
