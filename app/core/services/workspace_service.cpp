#include "workspace_service.h"
#include <xcb/xcb_ewmh.h>
#include <algorithm>
#include <iostream>

extern xcb_connection_t* connection;
extern xcb_ewmh_connection_t* ewmh;

ClientModel* Workspace::addClient(Entity cli, const Rectangle& bounds) {
    auto client = std::make_unique<ClientModel>(cli, bounds);
    
    client->setTitle(getWindowTitle(cli.id));
    
    ClientModel* client_ptr = client.get();
    clients.push_back(std::move(client));
    
    std::cout << "Client added: " << cli.id << " at (" 
              << bounds.position.x << "," << bounds.position.y << ")" << std::endl;
    
    return client_ptr;
}

bool Workspace::removeClient(xcb_window_t window_id) {
    auto it = std::find_if(clients.begin(), clients.end(),
        [window_id](const std::unique_ptr<ClientModel>& client) {
            return client->getWindowId() == window_id || client->getFrameId() == window_id;
        });
        
    if (it != clients.end()) {
        std::cout << "Client removed: " << window_id << std::endl;
        clients.erase(it);
        return true;
    }
    
    return false;
}

ClientModel* Workspace::findClient(int id) {
    for (auto& client : clients) {
        if (client->getDamageId() == id || client->getWindowId() == id || client->getFrameId() == id) {
            return client.get();
        }
    }
    return nullptr;
}

void Workspace::moveClient(ClientModel* client, const Point& position) {
    if (!client) return;
    
    Point constrained_pos = position;
    
    client->setPosition(constrained_pos);
}

void Workspace::resizeClient(ClientModel* client, const Size& size) {
    if (!client) return;
    
    Size constrained_size = size;
    // Aplicar tamanhos mínimos
    if (constrained_size.width < 100) constrained_size.width = 100;
    if (constrained_size.height < 50) constrained_size.height = 50;
    
    client->setSize(constrained_size);
}

void Workspace::snapToGrid(Point& position, int grid_spacing) {
    position.x = (position.x / grid_spacing) * grid_spacing;
    position.y = (position.y / grid_spacing) * grid_spacing;
}

std::string Workspace::getWindowTitle(xcb_window_t window_id) {
    xcb_ewmh_get_utf8_strings_reply_t title_reply;
    
    if (xcb_ewmh_get_wm_name_reply(ewmh, 
            xcb_ewmh_get_wm_name(ewmh, window_id),
            &title_reply, NULL)) {
        std::string title(title_reply.strings, title_reply.strings_len);
        xcb_ewmh_get_utf8_strings_reply_wipe(&title_reply);
        return title;
    }
    
    return "Sem Título";
}
