#ifndef RPC_WINE_RPC_WINE_HH
#define RPC_WINE_RPC_WINE_HH

#include "common/discord_defs.hh"

#if defined(__cplusplus)
extern "C" { // Prevent mangle of function names (Wine can't find them if mangled by C++ compiler)
#endif

// Discord Rich Presence API - discord_rpc.h

void rpcw_clear_presence();

void rpcw_initialize(const char *app_id, discord_event_handlers *handlers, int auto_register, const char *steam_id);

void rpcw_respond(const char *user_id, int reply);

void rpcw_run_callbacks();

void rpcw_shutdown();

void rpcw_update_connection();

void rpcw_update_handlers(discord_event_handlers *handlers);

void rpcw_update_presence(const discord_rich_presence *presence);

// Discord Register API - discord_register.h

void rpcw_register(const char *app_id, const char *cmd);

void rpcw_register_steam_game(const char *app_id, const char *steam_id);

#if defined(__cplusplus)
};
#endif

#endif //RPC_WINE_RPC_WINE_HH
