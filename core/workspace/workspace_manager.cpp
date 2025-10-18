#include "workspace_manager.h"
#include "client.h"
#include "config/config.h"
#include "workspace.h"

#include <algorithm>
#include <iostream>
#include <memory>
#include <xcb/composite.h>
#include <xcb/damage.h>
#include <xcb/render.h>
#include <xcb/xcb.h>
#include <xcb/xcb_renderutil.h>

WorkspaceManager::WorkspaceManager() {}

WorkspaceManager::WorkspaceManager(int width, int height)
    : width(width), height(height) {
  this->workspaces.emplace_back(Workspace("/home/nemo/Documentos/wallpaper/after.png"));
}

WorkspaceManager::~WorkspaceManager() {}

void WorkspaceManager::previous() {
  std::cout << "anterior" << std::endl;
  if (c_workspace > 0) {
    c_workspace--;
  }
}

void WorkspaceManager::next() {
  std::cout << "proximo" << std::endl;
  if (c_workspace  != MAX_WORKSPACES &&
      c_workspace < workspaces.size()) {
  this->workspaces.emplace_back(Workspace("/home/nemo/Documentos/wallpaper/forest.png"));
    c_workspace++;
  }
}
