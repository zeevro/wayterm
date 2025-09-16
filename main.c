#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>

#include <wayland-server.h>
#include "xdg-shell-server-protocol.h"

#include "protocol-logger.h"

// Forward declare interfaces
static const struct xdg_wm_base_interface xdg_wm_base_interface_impl;
static const struct xdg_surface_interface xdg_surface_interface_impl;
static const struct xdg_toplevel_interface xdg_toplevel_interface_impl;
static const struct wl_compositor_interface compositor_interface;
static const struct wl_surface_interface surface_interface;
static const struct wl_shm_interface shm_interface;

// Data structure to store info about a surface
struct my_surface_data {
    struct wl_resource *resource;
};

// Data structure to store info about an xdg_surface (top-level window)
struct my_xdg_surface_data {
    struct wl_resource *xdg_surface_resource;
    struct wl_resource *surface_resource;
    struct wl_resource *toplevel_resource;
};

#pragma region wl_compositor
// =========================================================================
// wl_compositor interface implementation
// =========================================================================

static void create_surface_handler(struct wl_client *client, struct wl_resource *resource, uint32_t id) {
    struct wl_resource *surface_resource = wl_resource_create(client, &wl_surface_interface, wl_resource_get_version(resource), id);
    if (!surface_resource) {
        wl_client_post_no_memory(client);
        return;
    }
    struct my_surface_data *data = calloc(1, sizeof(*data));
    data->resource = surface_resource;
    wl_resource_set_implementation(surface_resource, &surface_interface, data, NULL);
    printf("Client created a wl_surface (ID: %u).\n", wl_resource_get_id(surface_resource));
}

static void bind_compositor_handler(struct wl_client *client, void *data, uint32_t version, uint32_t id) {
    struct wl_resource *resource = wl_resource_create(client, &wl_compositor_interface, version, id);
    if (!resource) {
        wl_client_post_no_memory(client);
        return;
    }
    wl_resource_set_implementation(resource, &compositor_interface, data, NULL);
    printf("Client bound to wl_compositor.\n");
}

static const struct wl_compositor_interface compositor_interface = {
    .create_surface = create_surface_handler,
    .create_region = NULL,
};
#pragma endregion

#pragma region wl_surface
// =========================================================================
// wl_surface interface implementation
// =========================================================================

static void attach_handler(struct wl_client *client, struct wl_resource *resource, struct wl_resource *buffer, int32_t x, int32_t y) {
    printf("Client attached a buffer to a surface.\n");
}

static void commit_handler(struct wl_client *client, struct wl_resource *resource) {
    printf("Client committed a surface.\n");
}

static const struct wl_surface_interface surface_interface = {
    .destroy = NULL,
    .attach = attach_handler,
    .damage = NULL,
    .frame = NULL,
    .set_opaque_region = NULL,
    .set_input_region = NULL,
    .commit = commit_handler,
    .set_buffer_transform = NULL,
    .set_buffer_scale = NULL,
    .damage_buffer = NULL,
};
#pragma endregion

#pragma region xdg_wm_base
// =========================================================================
// xdg_wm_base interface implementation
// =========================================================================

static void get_xdg_surface_handler(struct wl_client *client, struct wl_resource *resource, uint32_t id, struct wl_resource *surface_resource) {
    printf("Client requested xdg_surface for wl_surface (ID: %u).\n", wl_resource_get_id(surface_resource));
    struct my_xdg_surface_data *data = calloc(1, sizeof(*data));
    if (!data) {
        wl_client_post_no_memory(client);
        return;
    }

    struct wl_resource *xdg_surface_resource = wl_resource_create(client, &xdg_surface_interface, wl_resource_get_version(resource), id);
    if (!xdg_surface_resource) {
        free(data);
        wl_client_post_no_memory(client);
        return;
    }

    data->surface_resource = surface_resource;
    data->xdg_surface_resource = xdg_surface_resource;

    wl_resource_set_implementation(xdg_surface_resource, &xdg_surface_interface_impl, data, NULL);
    wl_resource_set_user_data(surface_resource, data);
}

static void pong_handler(struct wl_client *client, struct wl_resource *resource, uint32_t serial) {
    printf("Client responded to ping.\n");
}

static void bind_xdg_wm_base_handler(struct wl_client *client, void *data, uint32_t version, uint32_t id) {
    struct wl_resource *resource = wl_resource_create(client, &xdg_wm_base_interface, version, id);
    if (!resource) {
        wl_client_post_no_memory(client);
        return;
    }
    wl_resource_set_implementation(resource, &xdg_wm_base_interface_impl, data, NULL);
    printf("Client bound to xdg_wm_base.\n");
    // Send a ping to the client
    xdg_wm_base_send_ping(resource, 1234);
}

static const struct xdg_wm_base_interface xdg_wm_base_interface_impl = {
    .destroy = NULL,
    .get_xdg_surface = get_xdg_surface_handler,
    .pong = pong_handler,
};
#pragma endregion

#pragma region xdg_surface + xdg_toplevel
// =========================================================================
// xdg_surface and xdg_toplevel interface implementation
// =========================================================================

static void get_toplevel_handler(struct wl_client *client, struct wl_resource *resource, uint32_t id) {
    struct my_xdg_surface_data *data = wl_resource_get_user_data(resource);
    data->toplevel_resource = wl_resource_create(client, &xdg_toplevel_interface, wl_resource_get_version(resource), id);
    if (!data->toplevel_resource) {
        wl_client_post_no_memory(client);
        return;
    }
    wl_resource_set_implementation(data->toplevel_resource, &xdg_toplevel_interface_impl, data, NULL);
    printf("Client requested a toplevel for its surface.\n");
}

static void xdg_surface_ack_configure_handler(struct wl_client *client, struct wl_resource *resource, uint32_t serial) {
    // This is where a real server would apply the configuration.
    printf("Client acknowledged configure serial %u.\n", serial);
}

static const struct xdg_surface_interface xdg_surface_interface_impl = {
    .destroy = NULL,
    .get_toplevel = get_toplevel_handler,
    .get_popup = NULL,
    .set_window_geometry = NULL,
    .ack_configure = xdg_surface_ack_configure_handler,
};

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

#pragma region wl_shm
// =========================================================================
// wl_shm interface implementation
// =========================================================================

static void create_pool_handler(struct wl_client *client, struct wl_resource *resource, uint32_t id, int fd, int32_t size) {
    // A real implementation would manage the fd and create a wl_shm_pool resource
    printf("Client requested to create a shared memory pool with fd %d and size %d.\n", fd, size);
    close(fd);
}

static void bind_shm_handler(struct wl_client *client, void *data, uint32_t version, uint32_t id) {
    struct wl_resource *resource = wl_resource_create(client, &wl_shm_interface, version, id);
    if (!resource) {
        wl_client_post_no_memory(client);
        return;
    }
    wl_resource_set_implementation(resource, &shm_interface, data, NULL);
    printf("Client bound to wl_shm.\n");
}

static const struct wl_shm_interface shm_interface = {
    .create_pool = create_pool_handler,
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

    // Add a socket for clients to connect
    const char *socket_name = wl_display_add_socket_auto(display);
    if (!socket_name)
    {
        fprintf(stderr, "Failed to add Wayland socket\n");
        wl_display_destroy(display);
        return 1;
    }
    printf("Wayland display running on WAYLAND_DISPLAY=%s\n", socket_name);

    wl_display_add_protocol_logger(display, protocol_logger_func, NULL);

    // Create the global singletons.
    wl_global_create(display, &wl_compositor_interface, 4, NULL, bind_compositor_handler);
    wl_global_create(display, &wl_shm_interface, 1, NULL, bind_shm_handler);
    wl_global_create(display, &xdg_wm_base_interface, 1, NULL, bind_xdg_wm_base_handler);

    printf("Running... Press Ctrl+C to exit.\n");

    wl_display_run(display);

    wl_display_destroy(display);
    return 0;
}
#pragma endregion
