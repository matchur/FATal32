#include "utils.h"
#include <unistd.h>

bool file_exists(const char *path) {
    return access(path, F_OK) == 0;
}