#include <climits>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <pwd.h>
#include <unistd.h>

#include "rpc_register.hh"
#include "utils.hh"

// Add a desktop file and update some mime handlers so that xdg-open does the right thing.
void rpc_wine::register_app(const char *app_id, const char *cmd) {
    const char *home_dir = getenv("HOME");
    if (home_dir == nullptr) {
        passwd *pwd = getpwuid(getuid());
        if (pwd == nullptr || pwd->pw_dir == nullptr)
            return;

        home_dir = pwd->pw_dir;
    }

    if (cmd == nullptr || !cmd[0]) {
        char game_path[PATH_MAX];
        if (readlink("/proc/self/exe", game_path, sizeof(game_path)) <= 0)
            return;

        cmd = game_path;
    }

    const char *desktop_file_format = "[Desktop Entry]\n"
                                      "Name=Game %s\n"
                                      "Exec=%s %%u\n"
                                      "Type=Application\n"
                                      "NoDisplay=true\n"
                                      "Categories=Discord;Games;\n"
                                      "MimeType=x-scheme-handler/discord-%s;\n";

    char desktop_file[PATH_MAX];
    int file_length = snprintf(desktop_file, sizeof(desktop_file), desktop_file_format, app_id, cmd, app_id);
    if (file_length <= 0)
        return;

    char desktop_file_name[PATH_MAX];
    snprintf(desktop_file_name, sizeof(desktop_file_name), "/discord-%s.desktop", app_id);

    char desktop_file_path[PATH_MAX];
    snprintf(desktop_file_path, sizeof(desktop_file_path), "%s/.local", home_dir);
    if (utils::create_dir(desktop_file_path))
        return;

    strcat(desktop_file_path, "/share");
    if (utils::create_dir(desktop_file_path))
        return;

    strcat(desktop_file_path, "/applications");
    if (utils::create_dir(desktop_file_path))
        return;

    strcat(desktop_file_path, desktop_file_name);

    FILE *file = fopen(desktop_file_path, "w");
    if (file == nullptr)
        return;

    fwrite(desktop_file, 1, static_cast<size_t>(file_length), file);
    fclose(file);

    char xdg_mime_cmd[1024];
    snprintf(
            xdg_mime_cmd, sizeof(xdg_mime_cmd),
            "xdg-mime default discord-%s.desktop x-scheme-handler/discord-%s",
            app_id, app_id
    );

    if (system(xdg_mime_cmd) != 0)
        fprintf(stderr, "Failed to register mime handler\n");
}

void rpc_wine::register_steam_game(const char *app_id, const char *steam_id) {
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "xdg-open steam://rungameid/%s", steam_id);

    register_app(app_id, cmd);
}
