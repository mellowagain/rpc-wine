#ifndef RPC_WINE_DISCORD_DEFS_HH
#define RPC_WINE_DISCORD_DEFS_HH

#include <cstdint>

/* Common definitions from Discord RPC */

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
    const char *user_id;
    const char *username;
    const char *discrim;
    const char *avatar;
};

struct discord_event_handlers {
    void (*ready)(const discord_user *request);
    void (*disconnected)(int error_code, const char *message);
    void (*errored)(int error_code, const char *message);
    void (*join_game)(const char *join_secret);
    void (*spectate_game)(const char *spectate_secret);
    void (*join_request)(const discord_user *request);
};

#endif  // RPC_WINE_DISCORD_DEFS_HH
