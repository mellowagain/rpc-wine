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
    rpcw_update_presence(nullptr);
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
    if (rpc_connection == nullptr)
        return;

    rpc_connection->on_connect = nullptr;
    rpc_connection->on_disconnect = nullptr;

    global_handlers = {};

    if (io_thread != nullptr) {
        io_thread->stop();
        delete io_thread;
    }

    connection::destroy(rpc_connection);
}

void rpcw_update_connection() {
    if (rpc_connection == nullptr)
        return;

    if (!rpc_connection->is_connected()) {
        if (std::chrono::system_clock::now() >= next_reconnect) {
            next_reconnect = std::chrono::system_clock::now() + std::chrono::duration<int64_t, std::milli>{ reconnect_time_ms.next_delay() };
            rpc_connection->open_connection();
        }

        return;
    }

    while (true) {
        serialization::json_document message;

        if (!rpc_connection->read(message))
            break;

        const char *event = serialization::get_string_member(&message, "evt");
        const char *nonce = serialization::get_string_member(&message, "nonce");

        if (nonce == nullptr) {
            if (event == nullptr)
                continue;

            serialization::json_value *data = serialization::get_object_member(&message, "data");

            if (strcmp(event, "ACTIVITY_JOIN") == 0) {
                const char *secret = serialization::get_string_member(data, "secret");

                if (secret != nullptr) {
                    strcpy(join_game_secret, secret);
                    was_join_game.store(true);
                }
            } else if (strcmp(event, "ACTIVITY_SPECTATE") == 0) {
                const char *secret = serialization::get_string_member(data, "secret");

                if (secret != nullptr) {
                    strcpy(spectate_game_secret, secret);
                    was_spectate_game.store(true);
                }
            } else if (strcmp(event, "ACTIVITY_JOIN_REQUEST") == 0) {
                serialization::json_value *user = serialization::get_object_member(data, "user");

                const char *user_id = serialization::get_string_member(user, "id");
                const char *username = serialization::get_string_member(user, "username");
                const char *avatar = serialization::get_string_member(user, "avatar");

                discord_user *request = join_ask_queue.get_next_add_message();

                if (user_id != nullptr && username != nullptr && request != nullptr) {
                    strcpy(request->user_id, user_id);
                    strcpy(request->username, username);

                    const char *discrim = serialization::get_string_member(user, "discriminator");
                    if (discrim != nullptr)
                        strcpy(request->discrim, discrim);

                    if (avatar != nullptr) {
                        strcpy(request->avatar, avatar);
                    } else {
                        request->avatar[0] = 0;
                    }

                    join_ask_queue.commit_add();
                }
            }
        } else {
            // This is only sent with responses to our payloads
            if (event != nullptr && strcmp(event, "ERROR") == 0) {
                serialization::json_value *data = serialization::get_object_member(&message, "data");

                last_error_code = serialization::get_int_member(data, "code");
                strcpy(last_error_message, serialization::get_string_member(data, "message", ""));

                got_error_message.store(true);
            }
        }
    }

    if (queued_presence.length != 0) {
        queued_message local_queued_presence {};

        {
            std::lock_guard<std::mutex> guard(presence_mutex);
            local_queued_presence.copy(queued_presence);
            queued_presence.length = 0;
        }

        if (!rpc_connection->write(local_queued_presence.buffer, local_queued_presence.length)) {
            std::lock_guard<std::mutex> guard(presence_mutex);
            queued_presence.copy(local_queued_presence);
        }
    }

    while (send_queue.has_pending_sends()) {
        queued_message *message = send_queue.get_next_send_message();
        rpc_connection->write(message->buffer, message->length);
        send_queue.commit_send();
    }
}

inline bool register_event_handler(const char *event_name) {
    queued_message *message = send_queue.get_next_add_message();

    if (message != nullptr) {
        message->length = serialization::write_subscribe(
                message->buffer, sizeof(message->buffer), nonce++, event_name
        );

        send_queue.commit_add();

        if (io_thread != nullptr)
            io_thread->notify();

        return true;
    }

    return false;
}

inline bool unregister_event_handler(const char *event_name) {
    queued_message *message = send_queue.get_next_add_message();

    if (message != nullptr) {
        message->length = serialization::write_unsubscribe(
                message->buffer, sizeof(message->buffer), nonce++, event_name
        );

        send_queue.commit_add();

        if (io_thread != nullptr)
            io_thread->notify();

        return true;
    }

    return false;
}

void rpcw_update_handlers(discord_event_handlers *handlers) {
    #define HANDLE_EVENT_REGISTRATION(handler_name, event)                                          \
    if (global_handlers.handler_name == nullptr && handlers->handler_name != nullptr) {             \
        register_event_handler(event);                                                              \
    } else if (global_handlers.handler_name != nullptr && handlers->handler_name == nullptr) {      \
        unregister_event_handler(event);                                                            \
    }                                                                                               \

    if (handlers != nullptr) {
        std::lock_guard<std::mutex> guard(handler_mutex);

        HANDLE_EVENT_REGISTRATION(join_game, "ACTIVITY_JOIN");
        HANDLE_EVENT_REGISTRATION(spectate_game, "ACTIVITY_SPECTATE");
        HANDLE_EVENT_REGISTRATION(join_request, "ACTIVITY_JOIN_REQUEST");

        global_handlers = *handlers;
    } else {
        std::lock_guard<std::mutex> guard(handler_mutex);
        global_handlers = {};
    }

    #undef HANDLE_EVENT_REGISTRATION
}

void rpcw_update_presence(const discord_rich_presence *presence) {
    std::lock_guard<std::mutex> guard(presence_mutex);

    queued_presence.length = serialization::write_rich_presence(
            queued_presence.buffer, sizeof(queued_presence.buffer), nonce++, process_id, presence
    );

    if (io_thread != nullptr)
        io_thread->notify();
}

// Discord Register API - discord_register.h

void rpcw_register(const char *app_id, const char *cmd) {
    rpc_wine::register_app(app_id, cmd);
}

void rpcw_register_steam_game(const char *app_id, const char *steam_id) {
    rpc_wine::register_steam_game(app_id, steam_id);
}

};
