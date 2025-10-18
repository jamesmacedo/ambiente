#pragma once
#include <X11/keysym.h>
#include <functional>
#include <vector>
#include <xcb/xcb.h>
#include <xcb/xcb_keysyms.h>

struct KeyBinding {
  uint16_t modifiers;
  xcb_keysym_t keysym;
  std::function<void()> callback;
};

class KeyManager {
public:
  KeyManager(xcb_connection_t *connection, xcb_window_t root);
  ~KeyManager();

  void add_binding(uint16_t modifiers, xcb_keysym_t keysym,
                   std::function<void()> callback);
  void grab_all();
  void handle_event(xcb_key_press_event_t *e);

private:
  xcb_connection_t *connection;
  xcb_window_t root;
  xcb_key_symbols_t *keysyms;
  std::vector<KeyBinding> bindings;
};
