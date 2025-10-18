#pragma  once

#include <xcb/xcb.h>

#include <xcb/render.h>
#include <xcb/damage.h>

struct entity {
    xcb_window_t id = 0;
    xcb_pixmap_t pixmap = 0;
    xcb_render_picture_t picture = 0;
};

class Client {

public:

    Client();
    Client(entity window, xcb_damage_damage_t damage);
    ~Client();

    void draw(xcb_render_picture_t buffer);
    entity& get_window(){ return window;};
    bool is_mapped(){ return mapped; };

    void update_visuals(xcb_pixmap_t pixmap, xcb_render_picture_t picture);
    void set_mapped(bool is_mapped){ mapped = is_mapped;};
    void update_topology(int x, int y, int width, int height);

    // void client_button_press(xcb_generic_event_t *event);
    // void client_button_release(xcb_generic_event_t *event);
    // void client_motion_handle(xcb_generic_event_t *event);
private:
    entity window;
    xcb_rectangle_t shape;
    xcb_damage_damage_t damage;
    int pinned;
    int mapped = false;
};
