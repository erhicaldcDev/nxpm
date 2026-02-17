#ifndef CONFIG_H
#define CONFIG_H
#define NXPM_VERSION "1.5"
#define REPO_BASE "https://raw.githubusercontent.com/erhicaldcDev/nxpm/refs/heads/main"
#define MIRROR_URL REPO_BASE "/mirrorlist/mirrorlist.json"
#define VERSION_URL REPO_BASE "/version.txt"
#define ROOT_DIR "/var/lib/nxpm"
#define DB_PATH "/var/lib/nxpm/installed.json"
#define MANIFEST_PATH "/var/lib/nxpm/packages.json"
#define CACHE_DIR "/var/cache/nxpm/sources"
#define GIT_CACHE_DIR "/var/cache/nxpm/sources/git"
#define BUILD_DIR "/tmp/nxpm-build"
#define STAGE_DIR "/tmp/nxpm-build/stage"
#endif
