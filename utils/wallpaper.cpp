#include <stdlib.h>
#include <string>
#include <xcb/xcb.h>
#include <cairo/cairo.h>
#include <cairo/cairo-xcb.h>
#include <iostream>
#include <filesystem>
#include <random>

xcb_visualtype_t *get_visualtype(xcb_screen_t *screen) {
    xcb_depth_iterator_t depth_iter = xcb_screen_allowed_depths_iterator(screen);
    for (; depth_iter.rem; xcb_depth_next(&depth_iter)) {
        xcb_visualtype_iterator_t visual_iter = xcb_depth_visuals_iterator(depth_iter.data);
        for (; visual_iter.rem; xcb_visualtype_next(&visual_iter)) {
            if (screen->root_visual == visual_iter.data->visual_id) {
                return visual_iter.data;
            }
        }
    }
    return NULL;
}

std::string random_image(){
    std::filesystem::path dir = "/home/nemo/Documentos/wallpaper";  
    if(!std::filesystem::exists(dir) || !std::filesystem::is_directory(dir)){
        std::cerr << "Wallpaper directory does not exist or is not a directory" << std::endl;
        exit(1);
    }

    std::string path;
    std::vector<std::filesystem::path> png_files;
    size_t n = 1;

    for(const auto &entry: std::filesystem::recursive_directory_iterator(dir)){
        if(entry.path().extension() == ".png"){
            png_files.push_back(entry.path());
        }
    }

    if(png_files.empty()){
        std::cout << "No png files found in the directory" << std::endl;
    }

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist(0, png_files.size() - 1);

    return png_files[dist(gen)].string();
}

xcb_pixmap_t generate_wallpaper_pixmap(xcb_screen_t* screen, xcb_connection_t* connection, xcb_window_t root) {
    // const char *wallpaper_path = image.c_str();
    std::string wallpaper_string = random_image().c_str();
    std::cout << wallpaper_string << std::endl;
    const char *wallpaper_path = wallpaper_string.c_str();

    int screen_width = screen->width_in_pixels;
    int screen_height = screen->height_in_pixels;

    cairo_surface_t *img_surface = cairo_image_surface_create_from_png(wallpaper_path);
    if (cairo_surface_status(img_surface) != CAIRO_STATUS_SUCCESS) {
        fprintf(stderr, "Erro ao carregar wallpaper: %s\n", wallpaper_path);
        cairo_surface_destroy(img_surface);
        exit(1);
    }
    int img_width = cairo_image_surface_get_width(img_surface);
    int img_height = cairo_image_surface_get_height(img_surface);

    double scale_x = (double)screen_width / img_width;
    double scale_y = (double)screen_height / img_height;
    double scale = (scale_x > scale_y) ? scale_x : scale_y;
    int new_w = (int)(img_width * scale);
    int new_h = (int)(img_height * scale);
    int offset_x = (screen_width - new_w) / 2;
    int offset_y = (screen_height - new_h) / 2;

    xcb_pixmap_t pixmap = xcb_generate_id(connection);
    xcb_create_pixmap(connection, screen->root_depth, pixmap, root, screen_width, screen_height);

    xcb_visualtype_t *visual = get_visualtype(screen);
    cairo_surface_t *cairo_surface = cairo_xcb_surface_create(connection, pixmap, visual, screen_width, screen_height);
    cairo_t *cr = cairo_create(cairo_surface);

    cairo_set_source_rgb(cr, 0, 0, 0);
    cairo_paint(cr);

    cairo_save(cr);
    cairo_translate(cr, offset_x, offset_y);
    cairo_scale(cr, scale, scale);
    cairo_set_source_surface(cr, img_surface, 0, 0);
    cairo_paint(cr);
    cairo_restore(cr);

    cairo_destroy(cr);
    cairo_surface_destroy(cairo_surface);
    cairo_surface_destroy(img_surface);

    return pixmap;
}
