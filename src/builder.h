#ifndef BUILDER_H
#define BUILDER_H

#include "cJSON.h"

void install_package_flow(const char *pkg_name, cJSON *manifest, cJSON *db, int edit_mode);
void remove_package_flow(const char *pkg_name, cJSON *db);

#endif
