#include "./barra.h"
#include "config/config.h"
#include <iostream>
#include <string.h>
#include "utils/client.h"
#include <xcb/xcb_renderutil.h>

xcb_atom_t get_atom(const char *name) {
  xcb_intern_atom_cookie_t cookie =
      xcb_intern_atom(connection, 0, strlen(name), name);
  xcb_intern_atom_reply_t *reply =
      xcb_intern_atom_reply(connection, cookie, NULL);
  if (!reply) {
    fprintf(stderr, "Falha ao obter átomo: %s\n", name);
    return XCB_ATOM_NONE;
  }
  xcb_atom_t atom = reply->atom;
  free(reply);
  return atom;
};

Barra::Barra() {
  id = xcb_generate_id(connection);
  std::cout << "Id da barra: " << id << std::endl;
  uint32_t values[] = {screen->white_pixel,
                       XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_KEY_PRESS};

  xcb_create_window(connection,
                    XCB_COPY_FROM_PARENT,        // depth
                    id,                          // window ID
                    screen->root,                // parent
                    0, 0,                        // x, y
                    screen->width_in_pixels, 30, // width, height
                    0,                           // border
                    XCB_WINDOW_CLASS_INPUT_OUTPUT, screen->root_visual,
                    XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK, values);

  xcb_damage_damage_t damage = xcb_generate_id(connection);
  xcb_damage_create(connection, damage, id,
                    XCB_DAMAGE_REPORT_LEVEL_RAW_RECTANGLES);

  /* 4️⃣ Definir tipo da janela: _NET_WM_WINDOW_TYPE_DOCK */
  xcb_atom_t atom_wm_window_type = get_atom("_NET_WM_WINDOW_TYPE");
  xcb_atom_t atom_dock = get_atom("_NET_WM_WINDOW_TYPE_DOCK");
  xcb_change_property(connection, XCB_PROP_MODE_REPLACE, id,
                      atom_wm_window_type, XCB_ATOM_ATOM, 32, 1, &atom_dock);

  /* 5️⃣ Definir _NET_WM_STRUT para reservar espaço (exemplo: 30px no topo) */
  xcb_atom_t atom_strut = get_atom("_NET_WM_STRUT");
  uint32_t strut[4] = {0, 0, 30, 0}; // left, right, top, bottom
  xcb_change_property(connection, XCB_PROP_MODE_REPLACE, id, atom_strut,
                      XCB_ATOM_CARDINAL, 32, 4, strut);

  /* 6️⃣ Tornar visível */
  xcb_map_window(connection, id);
  this->init({id, 0, 0}, damage);
};


void Barra::damaged(xcb_damage_notify_event_t *dn){
    if (!this->get_window().picture) {

      xcb_get_window_attributes_cookie_t attr_cookie =
          xcb_get_window_attributes(connection, this->get_window().id);
      xcb_get_window_attributes_reply_t *attr_reply =
          xcb_get_window_attributes_reply(connection, attr_cookie, NULL);
      xcb_render_pictvisual_t *pict_format = xcb_render_util_find_visual_format(
          xcb_render_util_query_formats(connection), attr_reply->visual);

      std::pair<xcb_pixmap_t, xcb_render_picture_t> cli =
          create_picture_from_window(this->get_window().id, pict_format);

      this->update_visuals(cli.first, cli.second);

      std::cout << "PICTURE CRIADA " << cli.second << std::endl;

      free(attr_reply);
    }

    this->draw(root_buffer);

    xcb_damage_subtract(connection, dn->damage, XCB_XFIXES_REGION_NONE,
                        XCB_XFIXES_REGION_NONE);
    xcb_flush(connection);
}
