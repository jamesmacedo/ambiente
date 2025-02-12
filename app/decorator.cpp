#include <cairo/cairo.h>
#include <cairo/cairo-xcb.h>

// void init_wallpaper_cover() {
//     const char *wallpaper_path = "/home/nemo/Documentos/wallpaper/evening-sky.png";
//
//     int screen_width = screen->width_in_pixels;
//     int screen_height = screen->height_in_pixels;
//
//     cairo_surface_t *img_surface = cairo_image_surface_create_from_png(wallpaper_path);
//     if (cairo_surface_status(img_surface) != CAIRO_STATUS_SUCCESS) {
//         fprintf(stderr, "Erro ao carregar wallpaper: %s\n", wallpaper_path);
//         cairo_surface_destroy(img_surface);
//         return;
//     }
//     int img_width = cairo_image_surface_get_width(img_surface);
//     int img_height = cairo_image_surface_get_height(img_surface);
//
//     double scale_x = (double)screen_width / img_width;
//     double scale_y = (double)screen_height / img_height;
//     double scale = (scale_x > scale_y) ? scale_x : scale_y;
//     int new_w = (int)(img_width * scale);
//     int new_h = (int)(img_height * scale);
//     int offset_x = (screen_width - new_w) / 2;
//     int offset_y = (screen_height - new_h) / 2;
//
//     xcb_pixmap_t pixmap = xcb_generate_id(connection);
//     xcb_create_pixmap(connection, screen->root_depth, pixmap, root, screen_width, screen_height);
//
//     xcb_visualtype_t *visual = get_visualtype(screen);
//     cairo_surface_t *cairo_surface = cairo_xcb_surface_create(connection, pixmap, visual, screen_width, screen_height);
//     cairo_t *cr = cairo_create(cairo_surface);
//
//     cairo_set_source_rgb(cr, 0, 0, 0);
//     cairo_paint(cr);
//
//     cairo_save(cr);
//     cairo_translate(cr, offset_x, offset_y);
//     cairo_scale(cr, scale, scale);
//     cairo_set_source_surface(cr, img_surface, 0, 0);
//     cairo_paint(cr);
//     cairo_restore(cr);
//
//     cairo_destroy(cr);
//     cairo_surface_destroy(cairo_surface);
//     cairo_surface_destroy(img_surface);
//
//     uint32_t value = pixmap;
//     xcb_change_window_attributes(connection, root, XCB_CW_BACK_PIXMAP, &value);
//
//     xcb_clear_area(connection, 0, root, 0, 0, 0, 0);
//     xcb_flush(connection);
// }
