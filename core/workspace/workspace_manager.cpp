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
  this->workspaces.emplace_back(this);
}

WorkspaceManager::~WorkspaceManager() {}

void WorkspaceManager::previous() {
  if (c_workspace <= 0) {
    return;
  }

  c_workspace--;
  this->current()->draw(connection);
}

void WorkspaceManager::next() {
  if (c_workspace == MAX_WORKSPACES) {
    return;
  }

  if (c_workspace == workspaces.size() - 1) {
    this->workspaces.emplace_back(this);
  }

  c_workspace++;
  this->current()->draw(connection);
}
