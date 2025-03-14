#ifndef WORKSPACE_H
#define WORKSPACE_H

#include <vector>
#include "client.h"

typedef struct workspace {
    std::vector<client> clients;
} workspace;

extern long unsigned int current_workspace;
extern std::vector<workspace> workspaces;

#endif
