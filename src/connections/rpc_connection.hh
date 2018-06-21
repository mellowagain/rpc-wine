#ifndef RPC_WINE_RPC_CONNECTION_HH
#define RPC_WINE_RPC_CONNECTION_HH

#include "../common/discord_defs.hh"
#include "../serialization/serialization.hh"
#include "connection.hh"

namespace rpc_wine {

    class connection {
    private:
        base_connection *connection = nullptr;
        char app_id[64] {};
        message_frame send_frame {};

    public:
        connection_state state = connection_state::DISCONNECTED;
        void (*on_connect)(serialization::json_document &message) { nullptr };
        void (*on_disconnect)(int error_code, const char *message) { nullptr };
        int last_error_code = 0;
        char last_error_message[256] = {};

        void open_connection();
        void close_connection();

        bool read(serialization::json_document &message);
        bool write(void *data, size_t length);

        inline bool is_connected() const {
            return this->state == connection_state::CONNECTED;
        }

        // Static builder methods
        static rpc_wine::connection *create(const char *app_id);
        static void destroy(rpc_wine::connection *&connection);
    };

}

#endif //RPC_WINE_RPC_CONNECTION_HH
