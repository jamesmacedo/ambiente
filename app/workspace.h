#pragma once
#include <vector>
class client;

typedef struct workspace {
    std::vector<client> clients;
} workspace;

extern int current_workspace;
extern std::vector<workspace> workspaces;
