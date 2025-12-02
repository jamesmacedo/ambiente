#include "workspace/client.h"
#include <xcb/xcb.h>
#include <xcb/render.h>

class Barra: public Client{

public:
    Barra();
    xcb_window_t get_id(){ return this->id; };

    void damaged(xcb_damage_notify_event_t *dn);

private:
    xcb_window_t id;
    xcb_render_picture_t picture;
};

