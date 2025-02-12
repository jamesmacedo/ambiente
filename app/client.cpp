#include <iostream>
#include <xcb/xcb.h>
#include <algorithm>
#include "client.h"
#include "./workspace.h"

client *current_resizing_client = nullptr;

void swap_client() {
    if (workspaces[current_workspace].clients.size() >= 2) {
        std::swap(workspaces[current_workspace].clients[current_workspace], workspaces[current_workspace].clients[1]);
    }
}

void add_client(xcb_window_t client) {
    struct client c = {client, client, false};
    workspaces[current_workspace].clients.insert(workspaces[current_workspace].clients.begin(), c);
}

void handle_destroy_notify(xcb_generic_event_t *event) {
    xcb_destroy_notify_event_t *destroy_notify = (xcb_destroy_notify_event_t *)event;

    std::cout << "Janela %d foi destruída.\n" << std::endl;

    xcb_window_t window = destroy_notify->window;

    std::cout << "Removendo janela " << window << std::endl;
    workspaces[0].clients.erase(
        std::remove_if(workspaces[0].clients.begin(), workspaces[0].clients.end(),
            [window](const client& client) {
                return client.child == window;
            }),
        workspaces[0].clients.end()
    );

    // remove_client(destroy_notify->window);
    // arrange_clients();
}
