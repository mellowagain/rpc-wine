#ifndef RPC_WINE_RPC_REGISTER_HH
#define RPC_WINE_RPC_REGISTER_HH

namespace rpc_wine {

    void register_app(const char *app_id, const char *cmd);

    void register_steam_game(const char *app_id, const char *steam_id);

}

#endif //RPC_WINE_RPC_REGISTER_HH
