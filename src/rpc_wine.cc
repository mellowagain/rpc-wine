#include <cstdio>
#include <sys/types.h>
#include <unistd.h>

#include "common/message_queue.hh"
#include "common/rpc_register.hh"
#include "connections/io_thread.hh"
#include "connections/rpc_connection.hh"
#include "rpc_wine.hh"
#include "serialization/writers.hh"
#include "utils/backoff.hh"

static connection *rpc_connection = nullptr;

static discord_event_handlers queued_handlers {};
static discord_event_handlers global_handlers {};

static std::atomic_bool was_just_connected { false };
static std::atomic_bool was_just_disconnected { false };
static std::atomic_bool got_error_message { false };

static std::atomic_bool was_join_game { false };
static std::atomic_bool was_spectate_game { false };

static char join_game_secret[256];
static char spectate_game_secret[256];

static io_thread_holder *io_thread = nullptr;
static backoff reconnect_time_ms(500, 60 * 1000);
static auto next_reconnect = std::chrono::system_clock::now();
static pid_t process_id = 0;
static int nonce = 0;

static queued_message queued_presence {};
static message_queue<queued_message, 8> send_queue;
static message_queue<discord_user, 8> join_ask_queue;

static int last_error_code = 0;
static char last_error_message[256];

static int last_disconnect_error_code = 0;
static char last_disconnect_error_message[256];

static std::mutex presence_mutex;
static std::mutex handler_mutex;

static discord_user connected_user;

extern "C" { // Prevent mangle of function names (Wine can't find them if mangled by C++ compiler)

// Discord Rich Presence API - discord_rpc.h

void rpcw_clear_presence() {
    //printf("== ! == rpcw_clear_presence called\n");
    rpcw_update_presence(nullptr);
}

void rpcw_initialize(const char *app_id, discord_event_handlers *handlers, int auto_register, const char *steam_id) {
    printf("== ! == rpcw_initialize called\n");

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
    //printf("== ! == rpcw_respond called\n");

    if (rpc_connection == nullptr || !rpc_connection->is_connected())
        return;

    queued_message *queued_msg = send_queue.get_next_add_message();

    if (queued_msg != nullptr) {
        queued_msg->length = serialization::write_join_reply(
                queued_msg->buffer, sizeof(queued_msg->buffer), user_id, reply, nonce++
        );
        send_queue.commit_add();

        if (io_thread != nullptr)
            io_thread->notify();
    }
}

void rpcw_run_callbacks() {
    //printf("== ! == rpcw_run_callbacks called\n");

    if (rpc_connection == nullptr)
        return;

    bool was_disconnected = was_just_disconnected.exchange(false);
    bool is_connected = rpc_connection->is_connected();

    if (is_connected) {
        std::lock_guard<std::mutex> guard(handler_mutex);

        if (was_disconnected && global_handlers.disconnected != nullptr)
            global_handlers.disconnected(last_disconnect_error_code, last_disconnect_error_message);
    }

    if (was_just_connected.exchange(false)) {
        std::lock_guard<std::mutex> guard(handler_mutex);

        if (global_handlers.ready != nullptr) {
            discord_user user { connected_user.user_id, connected_user.username, connected_user.discrim, connected_user.avatar };
            global_handlers.ready(&user);
        }
    }

    if (got_error_message.exchange(false)) {
        std::lock_guard<std::mutex> guard(handler_mutex);

        if (global_handlers.errored != nullptr)
            global_handlers.errored(last_error_code, last_error_message);
    }

    if (was_join_game.exchange(false)) {
        std::lock_guard<std::mutex> guard(handler_mutex);

        if (global_handlers.join_game != nullptr)
            global_handlers.join_game(join_game_secret);
    }

    if (was_spectate_game.exchange(false)) {
        std::lock_guard<std::mutex> guard(handler_mutex);

        if (global_handlers.spectate_game != nullptr)
            global_handlers.spectate_game(spectate_game_secret);
    }

    if (join_ask_queue.has_pending_sends()) {
        auto request = join_ask_queue.get_next_send_message();

        {
            std::lock_guard<std::mutex> guard(handler_mutex);

            if (global_handlers.join_request != nullptr) {
                discord_user user { request->user_id, request->username, request->discrim, request->avatar };
                global_handlers.join_request(&user);
            }
        }

        join_ask_queue.commit_send();
    }

    if (!is_connected) {
        std::lock_guard<std::mutex> guard(handler_mutex);

        if (was_disconnected && global_handlers.disconnected != nullptr)
            global_handlers.disconnected(last_disconnect_error_code, last_disconnect_error_message);
    }
}

void rpcw_shutdown() {
    //printf("== ! == rpcw_shutdown called\n");
}

void rpcw_update_connection() {
    //printf("== ! == rpcw_update_connection called\n");
    // TODO: Unimplemented stub Discord_UpdateConnection
}

void rpcw_update_handlers(discord_event_handlers *handlers) {
    //printf("== ! == rpcw_update_handlers called\n");
    // TODO: Unimplemented stub Discord_UpdateHandlers
}

void rpcw_update_presence(const discord_rich_presence *presence) {
    //printf("== ! == rpcw_update_presence called");

    std::lock_guard<std::mutex> guard(presence_mutex);

    queued_presence.length = serialization::write_rich_presence(
            queued_presence.buffer, sizeof(queued_presence.buffer), nonce++, process_id, presence
    );

    if (io_thread != nullptr)
        io_thread->notify();
}

// Discord Register API - discord_register.h

void rpcw_register(const char *app_id, const char *cmd) {
    printf("== ! == rpcw_register called\n");
    rpc_wine::register_app(app_id, cmd);
}

void rpcw_register_steam_game(const char *app_id, const char *steam_id) {
    printf("== ! == rpcw_register_steam_game called\n");
    rpc_wine::register_steam_game(app_id, steam_id);
}

};
