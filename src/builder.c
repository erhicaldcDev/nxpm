#include "builder.h"
#include "config.h"
#include "ui.h"
#include "fs.h"
#include "network.h"
#include "git.h"
#include "db.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>

static void open_editor(const char *filepath) {
    char *editor = getenv("EDITOR");
    if (!editor) editor = "vi";
    char sys_cmd[512];
    snprintf(sys_cmd, sizeof(sys_cmd), "%s %s", editor, filepath);
    if (system(sys_cmd) != 0) log_fail("Editor returned error");
}

static void edit_build_commands(cJSON *pkg) {
    log_step("EDIT", "Opening build configuration...");
    const char *temp_path = "/tmp/nxpm_build_edit.sh";
    
    FILE *f = fopen(temp_path, "w");
    if (!f) log_fail("Cannot create temp file");
    
    cJSON *commands = cJSON_GetObjectItemCaseSensitive(pkg, "build_commands");
    cJSON *cmd_item = NULL;
    cJSON_ArrayForEach(cmd_item, commands) {
        if (cJSON_IsString(cmd_item)) fprintf(f, "%s\n", cmd_item->valuestring);
    }
    fclose(f);

    open_editor(temp_path);

    f = fopen(temp_path, "r");
    if (!f) log_fail("Cannot read back edited configuration");

    cJSON *new_commands = cJSON_CreateArray();
    char line[2048];
    while (fgets(line, sizeof(line), f)) {
        line[strcspn(line, "\n")] = 0;
        if (strlen(line) > 0) cJSON_AddItemToArray(new_commands, cJSON_CreateString(line));
    }
    fclose(f);

    cJSON_ReplaceItemInObject(pkg, "build_commands", new_commands);
    log_success("Build configuration updated.");
}

static void resolve_dependencies(cJSON *pkg, cJSON *manifest, cJSON *db) {
    cJSON *deps = cJSON_GetObjectItemCaseSensitive(pkg, "dependencies");
    cJSON *dep = NULL;
    if (cJSON_IsArray(deps)) {
        cJSON_ArrayForEach(dep, deps) {
            if (cJSON_IsString(dep)) {
                log_info("Dependency required: %s", dep->valuestring);
                install_package_flow(dep->valuestring, manifest, db, 0);
            }
        }
    }
}

static void execute_build(cJSON *pkg) {
    char *url = cJSON_GetObjectItemCaseSensitive(pkg, "source_url")->valuestring;
    const char *name = cJSON_GetObjectItemCaseSensitive(pkg, "name")->valuestring;
    char cmd[2048];

    log_step("BUILD", "Preparing sandbox...");
    snprintf(cmd, sizeof(cmd), "rm -rf %s && mkdir -p %s", BUILD_DIR, BUILD_DIR);
    if (system(cmd) != 0) log_fail("Clean build dir failed");
    
    snprintf(cmd, sizeof(cmd), "rm -rf %s && mkdir -p %s", STAGE_DIR, STAGE_DIR);
    if (system(cmd) != 0) log_fail("Clean stage dir failed");

    if (is_git_url(url)) {
        git_prepare_source(url, name, BUILD_DIR);
    } else {
        char *filename = strrchr(url, '/');
        if (!filename) log_fail("Invalid source URL");
        filename++;
        char local_tarball[1024];
        snprintf(local_tarball, sizeof(local_tarball), "%s/%s", CACHE_DIR, filename);
        ensure_dir(CACHE_DIR);

        if (access(local_tarball, F_OK) != 0) {
            log_info("Source not in cache. Downloading...");
            download_file(url, local_tarball);
        } else {
            log_success("Using cached source: %s", filename);
        }
        snprintf(cmd, sizeof(cmd), "tar -xf %s -C %s", local_tarball, BUILD_DIR);
        if (system(cmd) != 0) log_fail("Extraction failed");
    }

    char src_dir[1024] = {0};
    struct dirent *de;
    DIR *dr = opendir(BUILD_DIR);
    if (!dr) log_fail("Could not open build dir");
    while ((de = readdir(dr)) != NULL) {
        if (de->d_type == DT_DIR && strcmp(de->d_name, ".") != 0 && strcmp(de->d_name, "..") != 0 && strcmp(de->d_name, "stage") != 0) {
            snprintf(src_dir, sizeof(src_dir), "%s/%s", BUILD_DIR, de->d_name);
            break;
        }
    }
    closedir(dr);
    
    if (src_dir[0] == 0) log_fail("Could not find extracted source directory.");
    if (chdir(src_dir) != 0) log_fail("Failed to enter source directory");
    if (setenv("DESTDIR", STAGE_DIR, 1) != 0) log_fail("Failed to set DESTDIR");

    cJSON *commands = cJSON_GetObjectItemCaseSensitive(pkg, "build_commands");
    cJSON *cmd_item = NULL;
    cJSON_ArrayForEach(cmd_item, commands) {
        if (cJSON_IsString(cmd_item)) {
            log_step("EXEC", cmd_item->valuestring);
            if (system(cmd_item->valuestring) != 0) log_fail("Command failed: %s", cmd_item->valuestring);
        }
    }
}

void install_package_flow(const char *pkg_name, cJSON *manifest, cJSON *db, int edit_mode) {
    if (is_installed(pkg_name, db)) {
        printf(YELLOW " [SKIP] " RESET "Package %s is already installed.\n", pkg_name);
        return;
    }

    cJSON *pkg = get_package_info(pkg_name, manifest);
    if (!pkg) log_fail("Package '%s' not found.", pkg_name);

    log_header(pkg_name);
    if (edit_mode) edit_build_commands(pkg);

    resolve_dependencies(pkg, manifest, db);
    execute_build(pkg);
    register_package(pkg, db, STAGE_DIR);
    
    log_success("Package %s installed successfully.", pkg_name);
}

void remove_package_flow(const char *pkg_name, cJSON *db) {
    cJSON *item = NULL;
    int index = 0;
    int found = 0;
    
    log_header("REMOVING PACKAGE");

    cJSON_ArrayForEach(item, db) {
        cJSON *name = cJSON_GetObjectItemCaseSensitive(item, "name");
        if (strcmp(name->valuestring, pkg_name) == 0) {
            cJSON *files = cJSON_GetObjectItemCaseSensitive(item, "files");
            cJSON *f = NULL;
            
            log_info("Deleting files for %s...", pkg_name);
            cJSON_ArrayForEach(f, files) {
                if (unlink(f->valuestring) != 0) {
                    printf(YELLOW " [WARN] " RESET "Could not remove %s\n", f->valuestring);
                }
            }
            cJSON_DeleteItemFromArray(db, index);
            found = 1;
            break;
        }
        index++;
    }

    if (found) {
        save_db(db);
        log_success("Package %s removed.", pkg_name);
    } else {
        log_fail("Package %s is not installed.", pkg_name);
    }
}
