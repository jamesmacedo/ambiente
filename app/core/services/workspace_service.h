#ifndef CLIENT_SERVICE_H
#define CLIENT_SERVICE_H

#include "../models/client.h"
#include <vector>
#include <memory>
#include <xcb/xcb.h>

class Workspace {
private:
    std::vector<std::unique_ptr<ClientModel>> clients;
    
public:
    ClientModel* addClient(Entity cli, const Rectangle& bounds);
    bool removeClient(xcb_window_t window_id);
    ClientModel* findClient(int id);
    ClientModel* findFrameClient(xcb_window_t frame_id);
    
    void moveClient(ClientModel* client, const Point& position);
    void resizeClient(ClientModel* client, const Size& size);
    void snapToGrid(Point& position, int grid_spacing = 20);
    void constrainToBounds(ClientModel* client, const Rectangle& screen_bounds);
    
    std::vector<ClientModel*> getClients();
    std::vector<ClientModel*> getVisibleClients() const;
    ClientModel* getClientAt(const Point& position) const;
    size_t getClientCount() const { return clients.size(); }
    
    std::string getWindowTitle(xcb_window_t window_id);
    void updateClientTitle(ClientModel* client);
};

#endif
