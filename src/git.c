#include "git.h"
#include "config.h"
#include "ui.h"
#include "fs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

int is_git_url(const char *url) {
    size_t len = strlen(url);
    return (len > 4 && strcmp(url + len - 4, ".git") == 0);
}

int git_prepare_source(const char *url, const char *pkg_name, const char *dest_dir) {
    char cmd[2048];
    char git_cache_path[1024];
    snprintf(git_cache_path, sizeof(git_cache_path), "%s/%s", GIT_CACHE_DIR, pkg_name);

    ensure_dir(GIT_CACHE_DIR);

    struct stat st = {0};
    if (stat(git_cache_path, &st) == 0 && S_ISDIR(st.st_mode)) {
        log_info(ICON_GIT " Updating cached git repository...");
        snprintf(cmd, sizeof(cmd), "git -C %s pull", git_cache_path);
    } else {
        log_info(ICON_GIT " Cloning git repository...");
        snprintf(cmd, sizeof(cmd), "git clone --depth 1 %s %s", url, git_cache_path);
    }

    if (system(cmd) != 0) return 
    
    snprintf(cmd, sizeof(cmd), "mkdir -p %s && cp -r %s/. %s/", dest_dir, git_cache_path, dest_dir);
    return system(cmd);
}
