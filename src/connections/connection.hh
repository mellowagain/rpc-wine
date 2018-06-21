#ifndef RPC_WINE_CONNECTION_HH
#define RPC_WINE_CONNECTION_HH

#include <cstddef>

namespace rpc_wine {

    class base_connection {
    private:
        int socket_connection = -1;

    public:
        bool is_open = false;

        bool open_connection();
        bool close_connection();

        bool read(void *data, size_t length);
        bool write(void *data, size_t length);

        // Static builder methods
        static base_connection *create();
        static void destroy(base_connection *&connection);
    };

}

#endif //RPC_WINE_CONNECTION_HH
