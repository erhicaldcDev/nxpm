#ifndef DB_H
#define DB_H

#include "cJSON.h"

int is_installed(const char *pkg_name, cJSON *db);
cJSON *get_package_info(const char *pkg_name, cJSON *manifest);
void register_package(cJSON *pkg, cJSON *db, const char *stage_dir);
void save_db(cJSON *db);
cJSON *load_db();

#endif
