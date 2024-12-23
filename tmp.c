#include <xcb/xcb.h>
#include <xcb/xproto.h>
#include <xcb/xcb_ewmh.h>
#include <stdio.h>
#include <stdlib.h>

// Estrutura para armazenar informações das janelas
typedef struct client {
    struct client *next;
    xcb_window_t window;
} client;

// Variáveis globais
static xcb_connection_t *connection;
static xcb_ewmh_connection_t *ewmh;
static xcb_screen_t *screen;
static xcb_window_t root;
static client *clients = NULL;

void add_window(xcb_window_t window) {
    client *c = (client *)malloc(sizeof(client));
    if (!c) {
        fprintf(stderr, "Erro ao alocar memória para cliente\n");
        exit(EXIT_FAILURE);
    }
    c->window = window;
    c->next = clients;
    clients = c;
}

void remove_window(xcb_window_t window) {
    client **c = &clients;
    while (*c) {
        if ((*c)->window == window) {
            client *tmp = *c;
            *c = (*c)->next;
            free(tmp);
            return;
        }
        c = &(*c)->next;
    }
}

void arrange_windows() {
    if (!clients) return;

    int window_count = 0;
    for (client *c = clients; c; c = c->next) {
        window_count++;
    }

    int screen_width = screen->width_in_pixels;
    int screen_height = screen->height_in_pixels;

    int master_width = screen_width / 2;
    int stack_width = screen_width - master_width;

    int stack_height = (window_count > 1) ? screen_height / (window_count - 1) : 0;

    int x = 0, y = 0;
    int i = 0;

    for (client *c = clients; c; c = c->next, i++) {
        if (i == 0) { // Master window
            xcb_configure_window(connection, c->window, XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y | XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT,
                                 (uint32_t[]){0, 0, master_width, screen_height});
        } else { // Stack windows
            xcb_configure_window(connection, c->window, XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y | XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT,
                                 (uint32_t[]){master_width, y, stack_width, stack_height});
            y += stack_height;
        }
    }

    xcb_flush(connection);
}

void handle_map_request(xcb_generic_event_t *event) {
    xcb_map_request_event_t *map_request = (xcb_map_request_event_t *)event;
    add_window(map_request->window);
    xcb_map_window(connection, map_request->window);
    arrange_windows();
    xcb_flush(connection);
}

void handle_destroy_notify(xcb_generic_event_t *event) {
    xcb_destroy_notify_event_t *destroy_notify = (xcb_destroy_notify_event_t *)event;
    remove_window(destroy_notify->window);
    arrange_windows();
}

void set_ewmh_desktop_properties() {
    xcb_ewmh_set_number_of_desktops(ewmh, 0, 4); // Define 4 desktops
    xcb_ewmh_set_current_desktop(ewmh, 0, 0);    // Define o desktop atual como 0

    // Configurar os nomes dos desktops
    const char *desktop_names[] = { "Desktop 1", "Desktop 2", "Desktop 3", "Desktop 4" };
    xcb_ewmh_set_desktop_names(ewmh, 0, 4, desktop_names);
}

void event_loop() {
    xcb_generic_event_t *event;
    while ((event = xcb_wait_for_event(connection))) {
        uint8_t response_type = event->response_type & ~0x80;
        switch (response_type) {
        case XCB_MAP_REQUEST:
            handle_map_request(event);
            break;
        case XCB_DESTROY_NOTIFY:
            handle_destroy_notify(event);
            break;
        default:
            break;
        }
        free(event);
    }
}

void setup() {
    connection = xcb_connect(NULL, NULL);
    if (xcb_connection_has_error(connection)) {
        fprintf(stderr, "Erro ao conectar ao servidor X\n");
        exit(EXIT_FAILURE);
    }

    // Inicializar EWMH
    ewmh = (xcb_ewmh_connection_t *)malloc(sizeof(xcb_ewmh_connection_t));
    if (!ewmh) {
        fprintf(stderr, "Erro ao alocar memória para EWMH\n");
        exit(EXIT_FAILURE);
    }

    xcb_intern_atom_cookie_t *ewmh_cookies = xcb_ewmh_init_atoms(connection, ewmh);
    if (!xcb_ewmh_init_atoms_replies(ewmh, ewmh_cookies, NULL)) {
        fprintf(stderr, "Erro ao inicializar os átomos EWMH\n");
        exit(EXIT_FAILURE);
    }

    // Obter a tela padrão
    screen = xcb_setup_roots_iterator(xcb_get_setup(connection)).data;
    root = screen->root;

    // Configurar para capturar eventos de janelas
    uint32_t values[] = { XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT | XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY };
    xcb_change_window_attributes(connection, root, XCB_CW_EVENT_MASK, values);
    xcb_flush(connection);

    // Configurar propriedades EWMH
    set_ewmh_desktop_properties();
}

int main() {
    setup();
    event_loop();

    // Limpar recursos EWMH
    xcb_ewmh_connection_wipe(ewmh);
    free(ewmh);

    // Fechar a conexão com o servidor X
    xcb_disconnect(connection);
    return 0;
}

