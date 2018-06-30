#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

#include "connection.hh"

static rpc_wine::base_connection connection;
static sockaddr_un pipe_address {};
static int message_flags = 0;

rpc_wine::base_connection *rpc_wine::base_connection::create() {
    pipe_address.sun_family = AF_UNIX;
    return &connection;
}

void rpc_wine::base_connection::destroy(rpc_wine::base_connection *&connection) {
    connection->close_connection();
    delete connection;
}

bool rpc_wine::base_connection::open_connection() {
    // I'm so sorry for this
    const char *tmp_path = getenv("XDG_RUNTIME_DIR");
    if (tmp_path == nullptr) {
        tmp_path = getenv("TMPDIR");
        if (tmp_path == nullptr) {
            tmp_path = getenv("TMP");
            if (tmp_path == nullptr) {
                tmp_path = getenv("TEMP");
                if (tmp_path == nullptr) {
                    tmp_path = "/tmp";
                }
            }
        }
    }

    this->socket_connection = socket(AF_UNIX, SOCK_STREAM, 0);
    if (this->socket_connection == -1)
        return false;

    fcntl(this->socket_connection, F_SETFL, O_NONBLOCK);

    for (unsigned int i = 0; i < 10; ++i) {
        snprintf(pipe_address.sun_path, sizeof(pipe_address.sun_path), "%s/discord-ipc-%d", tmp_path, i);

        int return_value = connect(this->socket_connection, (const sockaddr*) &pipe_address, sizeof(pipe_address));
        if (return_value == 0) {
            this->is_open = true;
            return true;
        }
    }

    this->close_connection();
    return false;
}

bool rpc_wine::base_connection::close_connection() {
    if (this->socket_connection == -1)
        return false;

    if (close(this->socket_connection) == -1)
        return false;

    this->socket_connection = -1;
    this->is_open = false;

    return true;
}

bool rpc_wine::base_connection::read(void *data, size_t length) {
    if (this->socket_connection == -1)
        return false;

    int response = (int) recv(this->socket_connection, data, length, message_flags);
    if (response < 0) {
        if (errno == EAGAIN)
            return false;

        this->close_connection();
    }

    return response == (int) length;
}

bool rpc_wine::base_connection::write(void *data, size_t length) {
    if (this->socket_connection == -1)
        return false;

    ssize_t sent_bytes = send(this->socket_connection, data, length, message_flags);
    if (sent_bytes < 0) {
        this->close_connection();
        return false;
    }

    return sent_bytes == (int) length;
}
