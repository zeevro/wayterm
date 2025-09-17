#include <unistd.h>

char *PNG_MAGIC = {'\211', 'P', 'N', 'G', '\r', '\n', '\032', '\n'};

struct chunk_header {
    uint32_t length;
    uint32_t type;
}

struct chunk_footer {
    uint32_t crc;
}

