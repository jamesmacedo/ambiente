#ifndef CLIENT_LOGIC_H
#define CLIENT_LOGIC_H

#include "../core/services/workspace_service.h"
#include "../render/render_client.h"
#include <xcb/xcb.h>

enum class InteractionMode {
    NONE,
    MOVING,
    RESIZING
};

struct InteractionState {
    InteractionMode mode = InteractionMode::NONE;
    ClientModel* target_client = nullptr;
    Point start_position;
    Point initial_position;
    Size initial_size;
};

class ClientLogic {
private:
    Workspace workspace;
    InteractionState interaction_state;
    
    Rectangle screen_bounds;
    bool use_grid_snapping;
    int grid_spacing;
    
public:
    CairoClientRenderer renderer;
    ClientLogic(const RenderContext& render_ctx, const Rectangle& screen) 
        : renderer(render_ctx), screen_bounds(screen), 
          use_grid_snapping(false), grid_spacing(20) {}
    
    xcb_render_picture_t generateMask(xcb_connection_t *connection,xcb_drawable_t drawable, int width, int height);
    void draw(ClientModel* client, xcb_render_picture_t buffer);

    void handleMapRequest(xcb_generic_event_t* event);
    void handleDestroyNotify(xcb_generic_event_t* event);
    void handleDamageNotify(xcb_generic_event_t* event);
    
    void handleButtonPress(xcb_generic_event_t* event);
    void handleButtonRelease(xcb_generic_event_t* event);
    void handleMotionNotify(xcb_generic_event_t* event);
    
    void focusClient(ClientModel* client);
    void closeClient(ClientModel* client);
    void toggleClientFixed(ClientModel* client);
    
    void setGridSnapping(bool enabled) { use_grid_snapping = enabled; }
    void setGridSpacing(int spacing) { grid_spacing = spacing; }
    
    Workspace& getWorkspace() { return workspace; }
    
private:
    std::pair<xcb_pixmap_t, xcb_render_picture_t> createPictureFromWindow(xcb_window_t client, xcb_render_pictvisual_t *pict_format);

    void startMoving(ClientModel* client, const Point& start_pos);
    void startResizing(ClientModel* client, const Point& start_pos);
    void updateMoving(const Point& current_pos);
    void updateResizing(const Point& current_pos);
    void finishInteraction();
    
    bool isInResizeZone(const ClientModel* client, const Point& position);
};

#endif
