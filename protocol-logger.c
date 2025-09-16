#include "protocol-logger.h"

void protocol_logger_func(void *user_data, enum wl_protocol_logger_type direction, const struct wl_protocol_logger_message *message) {
    fprintf(
        stderr,
        "%s #%d %s::%s",
        direction ? "💥" : "💬",
        message->resource->object.id,
        message->resource->object.interface->name,
        message->message->name
    );
    if (message->arguments_count) {
        const char *sig = message->message->signature;
        while ('0' <= *sig && *sig <= '9') sig++; // Skip "since version" prefix

        for (int i = 0; i < message->arguments_count; i++, sig++) {
            const union wl_argument *arg = &message->arguments[i];

            fprintf(stderr, i ? ", ": "(");

            if (*sig == '?') sig++; // Skip "nullable" prefix

            switch (*sig) {
            case 'i':
                fprintf(stderr, "%d", arg->i);
                break;
            case 'u':
                fprintf(stderr, "%uu", arg->u);
                break;
            case 'f':
                fprintf(stderr, "%df", arg->f);
                break;
            case 's':
                if (arg->s == NULL) {
                    fprintf(stderr, "NULL");
                } else {
                    fprintf(stderr, "\"%s\"", arg->s);
                }
                break;
            case 'o':
                fprintf(stderr, "(%s)#%u", arg->o->interface->name, arg->o->id);
                break;
            case 'n':
                const struct wl_interface *type = message->message->types[i];
                if (type == NULL) {
                    fprintf(stderr, "#%u", arg->n);
                } else {
                    fprintf(stderr, "(%s)#%u", type->name, arg->n);
                }
                break;
            case 'a':
                if (arg->a->size == 0) {
                    fprintf(stderr, "{}");
                } else {
                    int *n = arg->a->data;
                    for (int i = 0; i < arg->a->size / sizeof(int); i++) {
                        fprintf(stderr, "%s%d", i ? ", " : "{", *n++);
                    }
                    fprintf(stderr, "}");
                }
                break;
            case 'h':
                fprintf(stderr, "&%u", arg->h);
                break;
            default:
                fprintf(stderr, "⁉️ %x", *(int *)arg);
                break;
            }
        }
        fprintf(stderr, ")");
    }
    fprintf(stderr, "\n");
}
