#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>

#include <wayland-server.h>
#include "xdg-shell-server-protocol.h"

#include "protocol-logger.h"

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
    wl_resource_set_implementation(surface_resource, &wl_surface_interface_impl, data, (wl_resource_destroy_func_t)free);
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
}

static void wl_surface_commit_handler(struct wl_client *client, struct wl_resource *resource) {
    struct wl_shm_buffer *buffer = wl_shm_buffer_get(((struct wl_surface_data *)wl_resource_get_user_data(resource))->buffer);
    if (!buffer) return;

    wl_shm_buffer_begin_access(buffer);
    struct argb_t {
        char b;
        char g;
        char r;
        char a;
    };
    int32_t width = wl_shm_buffer_get_width(buffer);
    int32_t height = wl_shm_buffer_get_height(buffer);
    struct argb_t *pixel = wl_shm_buffer_get_data(buffer);
    for (int32_t row = 0; row < height; row++) {
        for (int32_t col = 0; col < width; col++) {
            printf("\e[38;2;%hhu;%hhu;%hhum██", pixel->r, pixel->g, pixel->b);
            pixel++;
        }
        printf("\e[0m\n");
    }
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

    xdg_surface_send_configure(xdg_surface, 5678);
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

static const struct xdg_toplevel_interface xdg_toplevel_interface_impl = {
    .destroy = NULL,
    .set_parent = NULL,
    .set_title = NULL,
    .set_app_id = NULL,
    .show_window_menu = NULL,
    .move = NULL,
    .resize = NULL,
    .set_maximized = NULL,
    .unset_maximized = NULL,
    .set_fullscreen = NULL,
    .unset_fullscreen = NULL,
    .set_minimized = NULL,
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

    wl_display_init_shm(display);

    wl_global_create(display, &wl_compositor_interface, 4, NULL, wl_compositor_bind_handler);
    wl_global_create(display, &xdg_wm_base_interface, 1, NULL, xdg_wm_base_bind_handler);

    fprintf(stderr, "Wayland display running on WAYLAND_DISPLAY=%s\n", socket_name);

    wl_display_run(display);

    wl_display_destroy(display);
    return 0;
}
#pragma endregion
