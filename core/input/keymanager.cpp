#include "keymanager.h"
#include <iostream>

KeyManager::KeyManager(xcb_connection_t *connection, xcb_window_t root)
    : connection(connection), root(root) {
  keysyms = xcb_key_symbols_alloc(connection);
}

KeyManager::~KeyManager() { xcb_key_symbols_free(keysyms); }

void KeyManager::add_binding(uint16_t modifiers, xcb_keysym_t keysym,
                             std::function<void()> callback) {
  bindings.push_back({modifiers, keysym, callback});
}

void KeyManager::grab_all() {
  for (auto &bind : bindings) {
    xcb_keycode_t *keycodes = xcb_key_symbols_get_keycode(keysyms, bind.keysym);
    if (!keycodes)
      continue;
    for (xcb_keycode_t *kc = keycodes; *kc; ++kc) {
      xcb_grab_key(connection, 1, root, bind.modifiers, *kc, XCB_GRAB_MODE_ASYNC,
                   XCB_GRAB_MODE_ASYNC);
    }
    free(keycodes);
  }
  xcb_flush(connection);
}

void KeyManager::handle_event(xcb_key_press_event_t *e) {
  xcb_keysym_t sym = xcb_key_symbols_get_keysym(keysyms, e->detail, 0);
  for (auto &bind : bindings) {
    if (sym == bind.keysym && (e->state & bind.modifiers)) {
      bind.callback();
      return;
    }
  }
}
