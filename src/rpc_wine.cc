#include <cstdio>
#include <sys/types.h>
#include <unistd.h>

#include "common/rpc_register.hh"
#include "connections/io_thread.hh"
#include "connections/rpc_connection.hh"
#include "rpc_wine.hh"
#include "utils/backoff.hh"

static connection *rpc_connection = nullptr;

static discord_event_handlers queued_handlers {};
static discord_event_handlers global_handlers {};

static std::atomic_bool was_just_connected { false };
static std::atomic_bool was_just_disconnected { false };
static std::atomic_bool got_error_message { false };

static io_thread_holder *io_thread = nullptr;
static backoff reconnect_time_ms(500, 60 * 1000);
static auto next_reconnect = std::chrono::system_clock::now();
static pid_t process_id = 0;
static int nonce = 0;

static int last_disconnect_error_code = 0;
static char last_disconnect_error_message[256];

static std::mutex handler_mutex;

static discord_user connected_user;

extern "C" { // Prevent mangle of function names (Wine can't find them if mangled by C++ compiler)

// Discord Rich Presence API - discord_rpc.h

void rpcw_clear_presence() {
    // TODO: Unimplemented stub Discord_ClearPresence
}

void rpcw_initialize(const char *app_id, discord_event_handlers *handlers, int auto_register, const char *steam_id) {
    io_thread = new io_thread_holder();
    if (io_thread == nullptr)
        return;

    if (auto_register) {
        if (steam_id != nullptr && steam_id[0]) {
            rpc_wine::register_steam_game(app_id, steam_id);
        } else {
            rpc_wine::register_app(app_id, nullptr);
        }
    }

    process_id = getpid();

    {
        std::lock_guard<std::mutex> guard(handler_mutex);

        if (handlers != nullptr) {
            queued_handlers = *handlers;
        } else {
            queued_handlers = {};
        }

        global_handlers = {};
    }

    if (rpc_connection != nullptr)
        return;

    rpc_connection = connection::create(app_id);

    rpc_connection->on_connect = [](serialization::json_document &message) {
        rpcw_update_handlers(&queued_handlers);

        memset(&connected_user, 0, sizeof(discord_user));

        auto data = serialization::get_object_member(&message, "data");
        auto user = serialization::get_object_member(data, "user");

        const char *user_id = serialization::get_string_member(user, "id");
        const char *username = serialization::get_string_member(user, "username");

        if (user_id != nullptr && username != nullptr) {
            strcpy(connected_user.user_id, user_id);
            strcpy(connected_user.username, username);

            const char *discrim = serialization::get_string_member(user, "discriminator");
            if (discrim != nullptr) {
                strcpy(connected_user.discrim, discrim);
            }

            const char *avatar = serialization::get_string_member(user, "avatar");
            if (avatar != nullptr) {
                strcpy(connected_user.avatar, avatar);
            } else {
                connected_user.avatar[0] = 0;
            }
        }

        was_just_connected.exchange(true);
        reconnect_time_ms.reset();
    };

    rpc_connection->on_disconnect = [](int error, const char *message) {
        last_disconnect_error_code = error;
        strcpy(last_disconnect_error_message, message);

        {
            std::lock_guard<std::mutex> guard(handler_mutex);
            global_handlers = {};
        }

        was_just_disconnected.exchange(true);
        next_reconnect = std::chrono::system_clock::now() + std::chrono::duration<int64_t, std::milli>{ reconnect_time_ms.next_delay() };
    };

    io_thread->start();
}

void rpcw_respond(const char *user_id, int reply) {
    printf("\n\nRPCWine_Respond called\n");
    printf("Param: %s, %i\n\n", user_id, reply);
}

void rpcw_run_callbacks() {
    printf("\n\nRPCWine_RunCallbacks called\n\n");
}

void rpcw_shutdown() {
    printf("\n\nRPCWine_Shutdown called\n\n");
}

void rpcw_update_connection() {
    // TODO: Unimplemented stub Discord_UpdateConnection
}

void rpcw_update_handlers(discord_event_handlers *handlers) {
    // TODO: Unimplemented stub Discord_UpdateHandlers
}

void rpcw_update_presence(const discord_rich_presence *presence) {
    printf("\n\nRPCWine_UpdatePresence called\n");
    printf("Arguments: %p\n\n", &presence);
}

// Discord Register API - discord_register.h

void rpcw_register(const char *app_id, const char *cmd) {
    rpc_wine::register_app(app_id, cmd);
}

void rpcw_register_steam_game(const char *app_id, const char *steam_id) {
    rpc_wine::register_steam_game(app_id, steam_id);
}

};
