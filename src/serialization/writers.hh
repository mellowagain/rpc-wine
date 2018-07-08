#ifndef RPC_WINE_WRITERS_HH
#define RPC_WINE_WRITERS_HH

#include "../common/discord_defs.hh"
#include "serialization.hh"

namespace rpc_wine::serialization {

    class write_object {
    private:
        json_writer &writer;

    public:
        explicit write_object(json_writer &writer);
        ~write_object();

        template <typename T>
        write_object(json_writer &writer, T &name);
    };

    class write_array {
    private:
        json_writer &writer;

    public:
        template <typename T>
        write_array(json_writer &writer, T &name);

        ~write_array();
    };

    template <typename T>
    void write_key(json_writer &writer, T &key);

    template <typename T>
    void write_optional_string(json_writer &writer, T &key, const char *value);

    void write_nonce(json_writer &writer, int nonce);

    size_t write_rich_presence(char *destination, size_t max_length, int nonce, int pid, const discord_rich_presence *presence);

    size_t write_handshake(char *destination, size_t max_length, int version, const char *app_id);

    size_t write_subscribe(char *destination, size_t max_length, int nonce, const char *event_name);

    size_t write_unsubscribe(char *destination, size_t max_length, int nonce, const char *event_name);

    size_t write_join_reply(char *destination, size_t max_length, const char *user_id, int reply, int nonce);

}

#endif //RPC_WINE_WRITERS_HH
