#include "config.h"
#include "ui.h"
#include "fs.h"
#include "network.h"
#include "db.h"
#include "builder.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>

static void cmd_update() {
    log_header("SYSTEM UPDATE");
    ensure_dir(ROOT_DIR);
    log_info("Fetching repository manifest...");
    log_info("URL: %s", MIRROR_URL);
    download_file(MIRROR_URL, MANIFEST_PATH);
    log_success("Repository updated.");
}

static void cmd_check_self_update() {
    log_header("SELF-CHECK");
    log_info("Current Version: " BOLD WHITE "%s" RESET, NXPM_VERSION);
    char *response = download_to_memory(VERSION_URL);
    if (response) {
        response[strcspn(response, "\n")] = 0;
        if (strcmp(response, NXPM_VERSION) == 0) {
            log_success("NXPM is up to date.");
        } else {
            printf(YELLOW BOLD " [!] " RESET "New version: " BOLD GREEN "v%s" RESET "\n", response);
        }
        free(response);
    } else {
        log_fail("Empty response from server.");
    }
}

static void cmd_install(const char *pkg_name, int edit_mode) {
    char *manifest_data = read_file(MANIFEST_PATH);
    if (!manifest_data) log_fail("Manifest not found. Run 'nxpm update' first.");
    cJSON *manifest = cJSON_Parse(manifest_data);
    
    ensure_dir(ROOT_DIR);
    cJSON *db = load_db();

    install_package_flow(pkg_name, manifest, db, edit_mode);

    cJSON_Delete(manifest);
    cJSON_Delete(db);
    free(manifest_data);
}

static void cmd_remove(const char *pkg_name) {
    cJSON *db = load_db();
    if (cJSON_GetArraySize(db) == 0) log_fail("Database empty.");
    remove_package_flow(pkg_name, db);
    cJSON_Delete(db);
}

static void cmd_list_installed() {
    cJSON *db = load_db();
    cJSON *item = NULL;
    printf("\n" BOLD WHITE " %-25s %-15s" RESET "\n", "INSTALLED PACKAGE", "VERSION");
    printf(BLUE " ---------------------------------------------" RESET "\n");
    cJSON_ArrayForEach(item, db) {
        printf(" %-24s %-15s\n", 
            cJSON_GetObjectItem(item, "name")->valuestring,
            cJSON_GetObjectItem(item, "version")->valuestring);
    }
    printf("\n");
    cJSON_Delete(db);
}

static void cmd_list_online() {
    char *data = read_file(MANIFEST_PATH);
    if (!data) log_fail("Manifest not found.");
    cJSON *manifest = cJSON_Parse(data);
    cJSON *db = load_db();
    
    printf("\n" BOLD WHITE " %-25s %-15s %-10s" RESET "\n", "AVAILABLE PACKAGE", "VERSION", "STATUS");
    printf(BLUE " --------------------------------------------------------" RESET "\n");

    cJSON *item;
    cJSON_ArrayForEach(item, manifest) {
        const char *name = cJSON_GetObjectItem(item, "name")->valuestring;
        const char *ver = cJSON_GetObjectItem(item, "version")->valuestring;
        printf(" %-24s %-15s %s\n", name, ver, is_installed(name, db) ? GREEN "Installed" RESET : WHITE "Available" RESET);
    }
    printf("\n");
    cJSON_Delete(manifest);
    cJSON_Delete(db);
    free(data);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        print_banner();
        printf("Usage: nxpm [command] <args>\n");
        printf("Commands:\n");
        printf("  update [--check-nxpm-update]\n");
        printf("  install [--edit-conf] <pkg>\n");
        printf("  remove <pkg>\n");
        printf("  list [--online]\n");
        return 1;
    }

    if (strcmp(argv[1], "update") == 0) {
        if (argc >= 3 && strcmp(argv[2], "--check-nxpm-update") == 0) {
            cmd_check_self_update();
        } else {
            cmd_update();
        }
    } else if (strcmp(argv[1], "install") == 0) {
        int edit_conf = 0;
        char *pkg_name = NULL;
        if (argc >= 4 && strcmp(argv[2], "--edit-conf") == 0) {
            edit_conf = 1;
            pkg_name = argv[3];
        } else if (argc >= 3) {
            pkg_name = argv[2];
        } else {
            log_fail("Usage: nxpm install [--edit-conf] <package>");
        }
        cmd_install(pkg_name, edit_conf);
    } else if (strcmp(argv[1], "remove") == 0) {
        if (argc < 3) log_fail("Usage: nxpm remove <package>");
        cmd_remove(argv[2]);
    } else if (strcmp(argv[1], "list") == 0) {
        if (argc >= 3 && strcmp(argv[2], "--online") == 0) {
            cmd_list_online();
        } else {
            cmd_list_installed();
        }
    } else {
        log_fail("Unknown command: %s", argv[1]);
    }

    return 0;
}
