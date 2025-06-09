#ifndef CLIENT_MODEL_H
#define CLIENT_MODEL_H

#include <string>
#include <xcb/xcb.h>
#include <xcb/damage.h>

struct Point {
    int x, y;
    Point(int x = 0, int y = 0) : x(x), y(y) {}
};

struct Size {
    int width, height;
    Size(int w = 0, int h = 0) : width(w), height(h) {}
};

struct Rectangle {
    Point position;
    Size size;
    
    Rectangle(int x = 0, int y = 0, int w = 0, int h = 0) 
        : position(x, y), size(w, h) {}
    
    bool contains(const Point& point) const {
        return point.x >= position.x && 
               point.x <= position.x + size.width &&
               point.y >= position.y && 
               point.y <= position.y + size.height;
    }
};

struct Entity {
    xcb_window_t id;
    xcb_pixmap_t pixmap;
    xcb_render_picture_t picture;
};

class ClientModel {
private:
    Entity client;
    Entity frame;
    // xcb_window_t window_id;
    // xcb_window_t frame_id;
    xcb_damage_damage_t damage_id;
    xcb_render_picture_t mask;
    
    Rectangle bounds;
    std::string title;
    bool is_fixed;
    bool is_visible;
    bool is_focused;
    
public:
    ClientModel(Entity client, const Rectangle& rect) 
        : client(client), damage_id(0), 
          bounds(rect), is_fixed(false), is_visible(true), is_focused(false) {}
    
    xcb_window_t getWindowId() const { return client.id; }
    xcb_window_t getFrameId() const { return frame.id; }
    xcb_damage_damage_t getDamageId() const { return damage_id; }
    Rectangle getBounds() const { return bounds; }

    xcb_render_picture_t getFramePicture() const { return frame.picture; }
    xcb_render_picture_t getClientPicture() const { return client.picture; }

    xcb_render_picture_t getMask() { return mask; }
    Point getPosition() const { return bounds.position; }
    Size getSize() const { return bounds.size; }
    std::string getTitle() const { return title; }
    bool isFixed() const { return is_fixed; }
    bool isVisible() const { return is_visible; }
    bool isFocused() const { return is_focused; }
    
    void setMask(xcb_render_picture_t m) { mask = m; }
    void setFrame(Entity f) { frame = f; }
    void setDamageId(xcb_damage_damage_t id) { damage_id = id; }
    void setPosition(const Point& pos) { bounds.position = pos; }
    void setSize(const Size& size) { bounds.size = size; }
    void setBounds(const Rectangle& rect) { bounds = rect; }
    void setTitle(const std::string& t) { title = t; }
    void setFixed(bool fixed) { is_fixed = fixed; }
    void setVisible(bool visible) { is_visible = visible; }
    void setFocused(bool focused) { is_focused = focused; }
    
    void move(int dx, int dy) {
        bounds.position.x += dx;
        bounds.position.y += dy;
    }
    
    void resize(int dw, int dh) {
        bounds.size.width += dw;
        bounds.size.height += dh;
    }
    
    bool containsPoint(const Point& point) const {
        return bounds.contains(point);
    }
    
    Rectangle getResizeZone(int zone_size = 20) const {
        return Rectangle(
            bounds.position.x + bounds.size.width - zone_size,
            bounds.position.y + bounds.size.height - zone_size,
            zone_size, zone_size
        );
    }
};

#endif
