#include <cerrno>
#include <sys/stat.h>

#include "utils.hh"

bool rpc_wine::utils::create_dir(const char *path) {
    int result = mkdir(path, 0755);
    if (result == 0)
        return true;

    return errno == EEXIST;
}
