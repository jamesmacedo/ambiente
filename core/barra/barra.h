#include <xcb/xcb.h>

class Barra {

public:
    Barra();
    xcb_window_t get_id(){ return this->id; };

    void draw();

private:
    xcb_window_t id;
};

