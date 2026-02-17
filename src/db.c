#include "db.h"
#include "config.h"
#include "fs.h"
#include "ui.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

int is_installed(const char *pkg_name, cJSON *db) {
    cJSON *item = NULL;
    cJSON_ArrayForEach(item, db) {
        cJSON *name = cJSON_GetObjectItemCaseSensitive(item, "name");
        if (cJSON_IsString(name) && (strcmp(name->valuestring, pkg_name) == 0)) {
            return 1;
        }
    }
    return 0;
}

cJSON *get_package_info(const char *pkg_name, cJSON *manifest) {
    cJSON *item = NULL;
    cJSON_ArrayForEach(item, manifest) {
        cJSON *name = cJSON_GetObjectItemCaseSensitive(item, "name");
        if (cJSON_IsString(name) && (strcmp(name->valuestring, pkg_name) == 0)) {
            return item;
        }
    }
    return NULL;
}

static void collect_files(const char *base_path, cJSON *file_list) {
    char cmd[2048];
    snprintf(cmd, sizeof(cmd), "find %s -type f", base_path);
    FILE *fp = popen(cmd, "r");
    if (!fp) log_fail("Failed to scan staged files");
    
    char path[4096];
    size_t base_len = strlen(base_path);
    while (fgets(path, sizeof(path), fp) != NULL) {
        path[strcspn(path, "\n")] = 0;
        if (strlen(path) > base_len) {
            cJSON_AddItemToArray(file_list, cJSON_CreateString(path + base_len));
        }
    }
    pclose(fp);
}

void register_package(cJSON *pkg, cJSON *db, const char *stage_dir) {
    cJSON *new_entry = cJSON_CreateObject();
    cJSON_AddStringToObject(new_entry, "name", cJSON_GetObjectItem(pkg, "name")->valuestring);
    cJSON_AddStringToObject(new_entry, "version", cJSON_GetObjectItem(pkg, "version")->valuestring);
    
    cJSON *files = cJSON_CreateArray();
    collect_files(stage_dir, files);
    cJSON_AddItemToObject(new_entry, "files", files);
    
    cJSON_AddItemToArray(db, new_entry);
    save_db(db);
    
    log_step("INSTALL", "Moving files from stage to system root...");
    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "cp -a %s/* / 2>/dev/null", stage_dir);
    if (system(cmd) != 0) {} // Ignorujemy błędy kopiowania
}

void save_db(cJSON *db) {
    char *string = cJSON_Print(db);
    FILE *f = fopen(DB_PATH, "w");
    if (!f) { free(string); log_fail("Failed to open DB for writing"); }
    fprintf(f, "%s", string);
    fclose(f);
    free(string);
}

cJSON *load_db() {
    char *data = read_file(DB_PATH);
    if (!data) return cJSON_CreateArray();
    cJSON *json = cJSON_Parse(data);
    free(data);
    return json ? json : cJSON_CreateArray();
}
