#include <stdio.h>
#include <wayland-server.h>

void protocol_logger_func(void *user_data, enum wl_protocol_logger_type direction, const struct wl_protocol_logger_message *message);
