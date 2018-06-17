#include <cstdio>
#include <sys/types.h>
#include <unistd.h>

#include "io_thread.hh"
#include "rpc_register.hh"
#include "rpc_wine.hh"

static io_thread_holder* io_thread = nullptr;
static pid_t process_id = 0;

extern "C" { // Prevent mangle of function names (Wine can't find them if mangled by C++ compiler)

// Discord Rich Presence API - discord_rpc.h

void rpcw_clear_presence() {
    // TODO: Implement Discord_ClearPresence
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

    // Create RPC connection


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
    // TODO: Implement Discord_UpdateConnection
}

void rpcw_update_handlers(discord_event_handlers *handlers) {
    // TODO: Implement Discord_UpdateHandlers
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
