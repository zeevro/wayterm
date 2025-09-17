#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <wayland-server.h>
#include "xdg-shell-server-protocol.h"
#include "protocol-logger.h"

#include <chafa.h>

static const struct wl_compositor_interface wl_compositor_interface_impl;
static const struct wl_surface_interface wl_surface_interface_impl;
static const struct xdg_wm_base_interface xdg_wm_base_interface_impl;
static const struct xdg_surface_interface xdg_surface_interface_impl;
static const struct xdg_toplevel_interface xdg_toplevel_interface_impl;

struct wl_surface_data {
    struct wl_resource *buffer;
    struct wl_resource *xdg_surface;
};

struct xdg_surface_data {
    struct wl_resource *xdg_toplevel;
};

ChafaCanvasConfig *chafa_config;
ChafaCanvas *chafa_canvas;
GString *chafa_gs;

#pragma region wl_compositor
// =========================================================================
// wl_compositor interface implementation
// =========================================================================

static void wl_compositor_create_surface_handler(struct wl_client *client, struct wl_resource *resource, uint32_t id) {
    struct wl_resource *surface_resource = wl_resource_create(client, &wl_surface_interface, wl_resource_get_version(resource), id);
    if (!surface_resource) {
        wl_client_post_no_memory(client);
        return;
    }
    struct wl_surface_data *data = calloc(1, sizeof(*data));
    wl_resource_set_implementation(surface_resource, &wl_surface_interface_impl, data, NULL);
}

static const struct wl_compositor_interface wl_compositor_interface_impl = {
    .create_surface = wl_compositor_create_surface_handler,
    .create_region = NULL,
};

static void wl_compositor_bind_handler(struct wl_client *client, void *data, uint32_t version, uint32_t id) {
    struct wl_resource *resource = wl_resource_create(client, &wl_compositor_interface, version, id);
    if (!resource) {
        wl_client_post_no_memory(client);
        return;
    }
    wl_resource_set_implementation(resource, &wl_compositor_interface_impl, data, NULL);
}
#pragma endregion

#pragma region wl_surface
// =========================================================================
// wl_surface interface implementation
// =========================================================================

static void wl_surface_attach_handler(struct wl_client *client, struct wl_resource *resource, struct wl_resource *buffer, int32_t x, int32_t y) {
    struct wl_surface_data *data = wl_resource_get_user_data(resource);
    data->buffer = buffer;

    if (data->xdg_surface) xdg_surface_send_configure(data->xdg_surface, wl_display_next_serial(wl_client_get_display(client)));
}

static void wl_surface_commit_handler(struct wl_client *client, struct wl_resource *resource) {
    struct wl_shm_buffer *buffer = wl_shm_buffer_get(((struct wl_surface_data *)wl_resource_get_user_data(resource))->buffer);
    if (!buffer) return;

    wl_shm_buffer_begin_access(buffer);
    // struct argb_t {
    //     char b;
    //     char g;
    //     char r;
    //     char a;
    // };
    // int32_t width = wl_shm_buffer_get_width(buffer);
    // int32_t height = wl_shm_buffer_get_height(buffer);
    // struct argb_t *pixel = wl_shm_buffer_get_data(buffer);
    // for (int32_t row = 0; row < height; row++) {
    //     for (int32_t col = 0; col < width; col++) {
    //         printf("\e[38;2;%hhu;%hhu;%hhum██", pixel->r, pixel->g, pixel->b);
    //         pixel++;
    //     }
    //     printf("\e[0m\n");
    // }
    chafa_canvas_draw_all_pixels(
        chafa_canvas,
        CHAFA_PIXEL_RGBA8_UNASSOCIATED,
        wl_shm_buffer_get_data(buffer),
        wl_shm_buffer_get_width(buffer),
        wl_shm_buffer_get_height(buffer),
        wl_shm_buffer_get_stride(buffer)
    );
    chafa_gs = chafa_canvas_print(chafa_canvas, NULL);
    fwrite(chafa_gs->str, sizeof(char), chafa_gs->len, stdout);
    fputc('\n', stdout);
    fflush(stdout);
    wl_shm_buffer_end_access(buffer);
}

static const struct wl_surface_interface wl_surface_interface_impl = {
    .destroy = NULL,
    .attach = wl_surface_attach_handler,
    .damage = NULL,
    .frame = NULL,
    .set_opaque_region = NULL,
    .set_input_region = NULL,
    .commit = wl_surface_commit_handler,
    .set_buffer_transform = NULL,
    .set_buffer_scale = NULL,
    .damage_buffer = NULL,
};
#pragma endregion

#pragma region xdg_wm_base
// =========================================================================
// xdg_wm_base interface implementation
// =========================================================================

static void xdg_wm_base_get_xdg_surface_handler(struct wl_client *client, struct wl_resource *resource, uint32_t id, struct wl_resource *surface) {
    struct xdg_surface_data *data = calloc(1, sizeof(*data));
    if (!data) {
        wl_client_post_no_memory(client);
        return;
    }

    struct wl_resource *xdg_surface = wl_resource_create(client, &xdg_surface_interface, wl_resource_get_version(resource), id);
    if (!xdg_surface) {
        free(data);
        wl_client_post_no_memory(client);
        return;
    }

    struct wl_surface_data *surface_data = wl_resource_get_user_data(surface);
    surface_data->xdg_surface = xdg_surface;

    wl_resource_set_implementation(xdg_surface, &xdg_surface_interface_impl, data, NULL);
    wl_resource_set_user_data(surface, data);

    xdg_surface_send_configure(xdg_surface, wl_display_next_serial(wl_client_get_display(client)));
}

static const struct xdg_wm_base_interface xdg_wm_base_interface_impl = {
    .destroy = NULL,
    .get_xdg_surface = xdg_wm_base_get_xdg_surface_handler,
    .pong = NULL,
};

static void xdg_wm_base_bind_handler(struct wl_client *client, void *data, uint32_t version, uint32_t id) {
    struct wl_resource *resource = wl_resource_create(client, &xdg_wm_base_interface, version, id);
    if (!resource) {
        wl_client_post_no_memory(client);
        return;
    }
    wl_resource_set_implementation(resource, &xdg_wm_base_interface_impl, data, NULL);
}
#pragma endregion

#pragma region xdg_surface
// =========================================================================
// xdg_surface interface implementation
// =========================================================================

static void xdg_surface_get_toplevel_handler(struct wl_client *client, struct wl_resource *resource, uint32_t id) {
    struct xdg_surface_data *data = wl_resource_get_user_data(resource);
    data->xdg_toplevel = wl_resource_create(client, &xdg_toplevel_interface, wl_resource_get_version(resource), id);
    if (!data->xdg_toplevel) {
        wl_client_post_no_memory(client);
        return;
    }
    wl_resource_set_implementation(data->xdg_toplevel, &xdg_toplevel_interface_impl, data, NULL);

    int mystate = 0;
    struct wl_array states = {sizeof(mystate), 0, &mystate};
    xdg_toplevel_send_configure(data->xdg_toplevel, 0, 0, &states);
    xdg_surface_send_configure(resource, wl_display_next_serial(wl_client_get_display(client)));
}

static void xdg_surface_ack_configure_handler(struct wl_client *client, struct wl_resource *resource, uint32_t serial) {
    // This is where a real server would apply the configuration.
}

static const struct xdg_surface_interface xdg_surface_interface_impl = {
    .destroy = NULL,
    .get_toplevel = xdg_surface_get_toplevel_handler,
    .get_popup = NULL,
    .set_window_geometry = NULL,
    .ack_configure = xdg_surface_ack_configure_handler,
};
#pragma endregion

#pragma region xdg_toplevel
// =========================================================================
// xdg_toplevel interface implementation
// =========================================================================

static void xdg_toplevel_destroy_handler(struct wl_client *client, struct wl_resource *resource) {}
static void xdg_toplevel_set_parent_handler(struct wl_client *client, struct wl_resource *resource, struct wl_resource *parent) {}
static void xdg_toplevel_set_title_handler(struct wl_client *client, struct wl_resource *resource, const char *title) {}
static void xdg_toplevel_set_app_id_handler(struct wl_client *client, struct wl_resource *resource, const char *app_id) {}
static void xdg_toplevel_show_window_menu_handler(struct wl_client *client, struct wl_resource *resource, struct wl_resource *seat, uint32_t serial, int32_t x, int32_t y) {}
static void xdg_toplevel_move_handler(struct wl_client *client, struct wl_resource *resource, struct wl_resource *seat, uint32_t serial) {}
static void xdg_toplevel_resize_handler(struct wl_client *client, struct wl_resource *resource, struct wl_resource *seat, uint32_t serial, uint32_t edges) {}
static void xdg_toplevel_set_max_size_handler(struct wl_client *client, struct wl_resource *resource, int32_t width, int32_t height) {}
static void xdg_toplevel_set_min_size_handler(struct wl_client *client, struct wl_resource *resource, int32_t width, int32_t height) {}
static void xdg_toplevel_set_maximized_handler(struct wl_client *client, struct wl_resource *resource) {}
static void xdg_toplevel_unset_maximized_handler(struct wl_client *client, struct wl_resource *resource) {}
static void xdg_toplevel_set_fullscreen_handler(struct wl_client *client, struct wl_resource *resource, struct wl_resource *output) {}
static void xdg_toplevel_unset_fullscreen_handler(struct wl_client *client, struct wl_resource *resource) {}
static void xdg_toplevel_set_minimized_handler(struct wl_client *client, struct wl_resource *resource) {}

static const struct xdg_toplevel_interface xdg_toplevel_interface_impl = {
    .destroy = xdg_toplevel_destroy_handler,
    .set_parent = xdg_toplevel_set_parent_handler,
    .set_title = xdg_toplevel_set_title_handler,
    .set_app_id = xdg_toplevel_set_app_id_handler,
    .show_window_menu = xdg_toplevel_show_window_menu_handler,
    .move = xdg_toplevel_move_handler,
    .resize = xdg_toplevel_resize_handler,
    .set_max_size = xdg_toplevel_set_max_size_handler,
    .set_min_size = xdg_toplevel_set_min_size_handler,
    .set_maximized = xdg_toplevel_set_maximized_handler,
    .unset_maximized = xdg_toplevel_unset_maximized_handler,
    .set_fullscreen = xdg_toplevel_set_fullscreen_handler,
    .unset_fullscreen = xdg_toplevel_unset_fullscreen_handler,
    .set_minimized = xdg_toplevel_set_minimized_handler,
};
#pragma endregion

#pragma region main
// =========================================================================
// Main loop
// =========================================================================

int main() {
    struct wl_display *display = wl_display_create();
    if (!display)
    {
        fprintf(stderr, "Failed to create Wayland display\n");
        return 1;
    }

    const char *socket_name = wl_display_add_socket_auto(display);
    if (!socket_name)
    {
        fprintf(stderr, "Failed to add Wayland socket\n");
        wl_display_destroy(display);
        return 1;
    }

    wl_display_add_protocol_logger(display, protocol_logger_func, NULL);

    wl_global_create(display, &wl_compositor_interface, 4, NULL, wl_compositor_bind_handler);
    wl_display_init_shm(display);
    wl_global_create(display, &xdg_wm_base_interface, 1, NULL, xdg_wm_base_bind_handler);

    fprintf(stderr, "Wayland display running on WAYLAND_DISPLAY=%s\n", socket_name);

    chafa_config = chafa_canvas_config_new();
    // chafa_canvas_config_set_geometry(chafa_config, 100, 100);
    chafa_canvas_config_set_pixel_mode(chafa_config, CHAFA_PIXEL_MODE_SIXELS);
    chafa_canvas_config_set_cell_geometry(chafa_config, 10, 20);
    chafa_canvas = chafa_canvas_new(chafa_config);

    wl_display_run(display);

    wl_display_destroy(display);
    return 0;
}
#pragma endregion
