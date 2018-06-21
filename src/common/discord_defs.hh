#ifndef RPC_WINE_DISCORD_DEFS_HH
#define RPC_WINE_DISCORD_DEFS_HH

#include <cstddef>
#include <cstdint>

// Common definitions from Discord RPC

#define DISCORD_REPLY_NO 0
#define DISCORD_REPLY_YES 1
#define DISCORD_REPLY_IGNORE 2

struct discord_rich_presence {
    const char *state;               /* max 128 bytes */
    const char *details;             /* max 128 bytes */
    int64_t start_timestamp;
    int64_t end_timestamp;
    const char *large_image_key;     /* max 32 bytes */
    const char *large_image_text;    /* max 128 bytes */
    const char *small_image_key;     /* max 32 bytes */
    const char *small_image_text;    /* max 128 bytes */
    const char *party_id;            /* max 128 bytes */
    int party_size;
    int party_max;
    const char *match_secret;        /* max 128 bytes */
    const char *join_secret;         /* max 128 bytes */
    const char *spectate_secret;     /* max 128 bytes */
    int8_t instance;
};

struct discord_user {
    char *user_id;
    char *username;
    char *discrim;
    char *avatar;
};

struct discord_event_handlers {
    void (*ready)(const discord_user *request);
    void (*disconnected)(int error_code, const char *message);
    void (*errored)(int error_code, const char *message);
    void (*join_game)(const char *join_secret);
    void (*spectate_game)(const char *spectate_secret);
    void (*join_request)(const discord_user *request);
};

namespace rpc_wine {

    constexpr static size_t max_frame_size = 0x10000; // 64 * 1024 = 65536 = 0x10000

    enum class connection_error_code : int {
        SUCCESS = 0,
        PIPE_CLOSED = 1,
        READ_CORRUPT = 2
    };

    enum class connection_op_code : uint32_t {
        HANDSHAKE = 0,
        FRAME = 1,
        CLOSE = 2,
        PING = 3,
        PONG = 4
    };

    struct message_frame_header {
        connection_op_code op_code;
        uint32_t length;
    };

    struct message_frame : public message_frame_header {
        char message[rpc_wine::max_frame_size - sizeof(message_frame_header)];
    };

    enum class connection_state : uint32_t {
        DISCONNECTED = 0,
        SENT_HANDSHAKE = 1,
        AWAITING_RESPONSE = 2,
        CONNECTED = 3
    };

}

#endif  // RPC_WINE_DISCORD_DEFS_HH
