#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <dirent.h> 
#include <curl/curl.h>
#include "cJSON.h"
#include <errno.h>

#define NXPM_VERSION "1.4-git"

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

#define RESET   "\033[0m"
#define BOLD    "\033[1m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define BLUE    "\033[34m"
#define CYAN    "\033[36m"
#define MAGENTA "\033[35m"
#define WHITE   "\033[37m"

#define ICON_CHECK  "\xE2\x9C\x93"
#define ICON_CROSS  "\xE2\x9C\x97"
#define ICON_ARROW  "\xE2\x9E\x9C"
#define ICON_BOX    "\xE2\x96\x88"
#define ICON_CLOUD  "\xE2\x98\x81"
#define ICON_GIT    "\xE2\x9B\x93"

void print_banner() {
    printf(CYAN BOLD);
    printf(" _   _  __  __ ____  __  __ \n");
    printf("| \\ | | \\ \\/ /|  _ \\|  \\/  |\n");
    printf("|  \\| |  \\  / | |_) | |\\/| |\n");
    printf("| |\\  |  /  \\ |  __/| |  | |\n");
    printf("|_| \\_| /_/\\_\\|_|   |_|  |_|\n");
    printf("        v%s (bootstraped)\n", NXPM_VERSION);
    printf(RESET "\n");
}

void log_header(const char *title) {
    printf("\n" MAGENTA BOLD ":: %s ::" RESET "\n", title);
    printf(MAGENTA "%.*s" RESET "\n", (int)strlen(title) + 6, "========================================");
}

void log_info(const char *fmt, ...) {
    va_list args;
    printf(BLUE " %s " RESET, ICON_ARROW);
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    printf("\n");
}

void log_success(const char *fmt, ...) {
    va_list args;
    printf(GREEN BOLD " %s " RESET, ICON_CHECK);
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    printf("\n");
}

void log_step(const char *step, const char *msg) {
    printf(YELLOW BOLD " [%s] " RESET "%s\n", step, msg);
}

void log_fail(const char *fmt, ...) {
    va_list args;
    printf(RED BOLD " %s [ERROR] " RESET, ICON_CROSS);
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    printf("\n");
    exit(1);
}

static void format_size(double size, char *out_buf) {
    const char *units[] = {"B", "KB", "MB", "GB"};
    int i = 0;
    while (size > 1024 && i < 3) {
        size /= 1024;
        i++;
    }
    sprintf(out_buf, "%.2f %s", size, units[i]);
}

static size_t write_file_cb(void *ptr, size_t size, size_t nmemb, FILE *stream) {
    return fwrite(ptr, size, nmemb, stream);
}

static size_t write_memory_cb(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    char **p_response = (char **)userp;
    *p_response = realloc(*p_response, strlen(*p_response ? *p_response : "") + realsize + 1);
    if(*p_response == NULL) return 0;
    strncat(*p_response, (char *)contents, realsize);
    return realsize;
}

static int progress_cb(void *clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow) {
    (void)clientp; (void)ultotal; (void)ulnow;
    if (dltotal <= 0) return 0;
    
    const int bar_width = 25;
    double fraction = (double)dlnow / (double)dltotal;
    int filled = (int)(fraction * bar_width);
    int percentage = (int)(fraction * 100);

    char current_fmt[32];
    char total_fmt[32];
    format_size((double)dlnow, current_fmt);
    format_size((double)dltotal, total_fmt);

    printf("\r" CYAN " [DL] " RESET "[");
    for (int i = 0; i < bar_width; i++) {
        if (i < filled) printf(ICON_BOX);
        else printf(" ");
    }
    printf("] %3d%% (%s / %s)", percentage, current_fmt, total_fmt);
    fflush(stdout);
    return 0;
}

static void ensure_dir(const char *path) {
    struct stat st = {0};
    if (stat(path, &st) == -1) {
        char cmd[1024];
        snprintf(cmd, sizeof(cmd), "mkdir -p %s", path);
        if (system(cmd) != 0) log_fail("Failed to create directory: %s", path);
    }
}

static char *read_file(const char *filename) {
    FILE *f = fopen(filename, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long length = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *buffer = malloc(length + 1);
    if (!buffer) { fclose(f); return NULL; }
    fread(buffer, 1, length, f);
    fclose(f);
    buffer[length] = '\0';
    return buffer;
}

static void save_json(const char *filename, cJSON *json) {
    char *string = cJSON_Print(json);
    if (!string) log_fail("Failed to print JSON");
    FILE *f = fopen(filename, "w");
    if (!f) { free(string); log_fail("Failed to open DB for writing"); }
    fprintf(f, "%s", string);
    fclose(f);
    free(string);
}

static void download_url(const char *url, const char *output_path) {
    CURL *curl_handle;
    CURLcode res;
    FILE *pagefile;

    curl_global_init(CURL_GLOBAL_ALL);
    curl_handle = curl_easy_init();
    if (!curl_handle) log_fail("Curl init failed");

    pagefile = fopen(output_path, "wb");
    if (!pagefile) log_fail("Cannot open file for download: %s", output_path);

    curl_easy_setopt(curl_handle, CURLOPT_URL, url);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_file_cb);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, pagefile);
    curl_easy_setopt(curl_handle, CURLOPT_XFERINFOFUNCTION, progress_cb);
    curl_easy_setopt(curl_handle, CURLOPT_NOPROGRESS, 0L);
    curl_easy_setopt(curl_handle, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "nxpm/1.3");
    curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYHOST, 0L);

    printf("\n");
    res = curl_easy_perform(curl_handle);
    printf("\n");

    fclose(pagefile);
    curl_easy_cleanup(curl_handle);
    curl_global_cleanup();

    if (res != CURLE_OK) {
        remove(output_path);
        log_fail("Download failed: %s", curl_easy_strerror(res));
    }
}

static void cmd_update() {
    log_header("SYSTEM UPDATE");
    ensure_dir(ROOT_DIR);
    log_info("Fetching repository manifest...");
    log_info("URL: %s", MIRROR_URL);
    download_url(MIRROR_URL, MANIFEST_PATH);
    log_success("Repository updated.");
}

static void cmd_check_self_update() {
    log_header("SELF-CHECK");
    log_info("Current Version: " BOLD WHITE "%s" RESET, NXPM_VERSION);
    log_info("Checking remote version...");

    CURL *curl;
    CURLcode res;
    char *response = NULL;

    curl = curl_easy_init();
    if(curl) {
        curl_easy_setopt(curl, CURLOPT_URL, VERSION_URL);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_memory_cb);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "nxpm/1.3");
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
        
        res = curl_easy_perform(curl);
        if(res != CURLE_OK) {
            log_fail("Check failed: %s", curl_easy_strerror(res));
        }
        curl_easy_cleanup(curl);
    }

    if (response) {
        response[strcspn(response, "\n")] = 0;
        
        if (strcmp(response, NXPM_VERSION) == 0) {
            log_success("NXPM is up to date (v%s).", NXPM_VERSION);
        } else {
            printf(YELLOW BOLD " [!] " RESET "New version available: " BOLD GREEN "v%s" RESET "\n", response);
            printf("     Run the bootstrap script to upgrade.\n");
        }
        free(response);
    } else {
        log_fail("Empty response from server.");
    }
}

static int is_installed(const char *pkg_name, cJSON *db) {
    cJSON *item = NULL;
    cJSON_ArrayForEach(item, db) {
        cJSON *name = cJSON_GetObjectItemCaseSensitive(item, "name");
        if (cJSON_IsString(name) && (strcmp(name->valuestring, pkg_name) == 0)) {
            return 1;
        }
    }
    return 0;
}

static cJSON *get_package_info(const char *pkg_name, cJSON *manifest) {
    cJSON *item = NULL;
    cJSON_ArrayForEach(item, manifest) {
        cJSON *name = cJSON_GetObjectItemCaseSensitive(item, "name");
        if (cJSON_IsString(name) && (strcmp(name->valuestring, pkg_name) == 0)) {
            return item;
        }
    }
    return NULL;
}

static void install_package(const char *pkg_name, cJSON *manifest, cJSON *db, int edit_mode);

static void resolve_dependencies(cJSON *pkg, cJSON *manifest, cJSON *db) {
    cJSON *deps = cJSON_GetObjectItemCaseSensitive(pkg, "dependencies");
    cJSON *dep = NULL;
    if (cJSON_IsArray(deps)) {
        cJSON_ArrayForEach(dep, deps) {
            if (cJSON_IsString(dep)) {
                log_info("Dependency required: %s", dep->valuestring);
                install_package(dep->valuestring, manifest, db, 0);
            }
        }
    }
}

static void edit_build_commands(cJSON *pkg) {
    log_step("EDIT", "Opening build configuration...");
    const char *temp_path = "/tmp/nxpm_build_edit.sh";
    
    FILE *f = fopen(temp_path, "w");
    if (!f) log_fail("Cannot create temp file for editing");
    
    cJSON *commands = cJSON_GetObjectItemCaseSensitive(pkg, "build_commands");
    cJSON *cmd_item = NULL;
    cJSON_ArrayForEach(cmd_item, commands) {
        if (cJSON_IsString(cmd_item)) {
            fprintf(f, "%s\n", cmd_item->valuestring);
        }
    }
    fclose(f);

    char *editor = getenv("EDITOR");
    if (!editor) editor = "vi";
    
    char sys_cmd[512];
    snprintf(sys_cmd, sizeof(sys_cmd), "%s %s", editor, temp_path);
    if (system(sys_cmd) != 0) log_fail("Editor returned error");

    f = fopen(temp_path, "r");
    if (!f) log_fail("Cannot read back edited configuration");

    cJSON *new_commands = cJSON_CreateArray();
    char line[2048];
    while (fgets(line, sizeof(line), f)) {
        line[strcspn(line, "\n")] = 0;
        if (strlen(line) > 0) {
            cJSON_AddItemToArray(new_commands, cJSON_CreateString(line));
        }
    }
    fclose(f);

    cJSON_ReplaceItemInObject(pkg, "build_commands", new_commands);
    log_success("Build configuration updated.");
}

// .git detector
static int is_git_url(const char *url) {
    size_t len = strlen(url);
    if (len > 4 && strcmp(url + len - 4, ".git") == 0) return 1;
    return 0;
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
        ensure_dir(GIT_CACHE_DIR);
        char git_cache_path[1024];
        snprintf(git_cache_path, sizeof(git_cache_path), "%s/%s", GIT_CACHE_DIR, name);

        struct stat st = {0};
        if (stat(git_cache_path, &st) == 0 && S_ISDIR(st.st_mode)) {
            log_info(ICON_GIT " Updating cached git repository...");
            snprintf(cmd, sizeof(cmd), "git -C %s pull", git_cache_path);
            if (system(cmd) != 0) log_fail("Git pull failed");
        } else {
            log_info(ICON_GIT " Cloning git repository...");
            snprintf(cmd, sizeof(cmd), "git clone --depth 1 %s %s", url, git_cache_path);
            if (system(cmd) != 0) log_fail("Git clone failed");
        }

        char build_src_path[1024];
        snprintf(build_src_path, sizeof(build_src_path), "%s/%s", BUILD_DIR, name);
        snprintf(cmd, sizeof(cmd), "mkdir -p %s && cp -r %s/. %s/", build_src_path, git_cache_path, build_src_path);
        if (system(cmd) != 0) log_fail("Failed to copy git source to build dir");

    } 
    else {
        char *filename = strrchr(url, '/');
        if (!filename) log_fail("Invalid source URL");
        filename++;

        char local_tarball[1024];
        snprintf(local_tarball, sizeof(local_tarball), "%s/%s", CACHE_DIR, filename);

        ensure_dir(CACHE_DIR);
        if (access(local_tarball, F_OK) != 0) {
            log_info("Source not in cache. Downloading...");
            download_url(url, local_tarball);
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

static void install_files_to_root() {
    log_step("INSTALL", "Moving files from stage to system root...");
    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "cp -a %s/* / 2>/dev/null", STAGE_DIR);
    if (system(cmd) != 0) {} 
}

static void register_package(cJSON *pkg, cJSON *db) {
    cJSON *new_entry = cJSON_CreateObject();
    cJSON_AddStringToObject(new_entry, "name", cJSON_GetObjectItem(pkg, "name")->valuestring);
    cJSON_AddStringToObject(new_entry, "version", cJSON_GetObjectItem(pkg, "version")->valuestring);
    
    cJSON *files = cJSON_CreateArray();
    collect_files(STAGE_DIR, files);
    cJSON_AddItemToObject(new_entry, "files", files);
    
    cJSON_AddItemToArray(db, new_entry);
    save_json(DB_PATH, db);
    
    install_files_to_root();
}

static void install_package(const char *pkg_name, cJSON *manifest, cJSON *db, int edit_mode) {
    if (is_installed(pkg_name, db)) {
        printf(YELLOW " [SKIP] " RESET "Package %s is already installed.\n", pkg_name);
        return;
    }

    cJSON *pkg = get_package_info(pkg_name, manifest);
    if (!pkg) {
        log_fail("Package '%s' not found in manifest.", pkg_name);
    }

    log_header(pkg_name);

    if (edit_mode) {
        edit_build_commands(pkg);
    }

    resolve_dependencies(pkg, manifest, db);
    execute_build(pkg);
    register_package(pkg, db);
    
    log_success("Package %s installed successfully.", pkg_name);
}

static void cmd_install(const char *pkg_name, int edit_mode) {
    char *manifest_data = read_file(MANIFEST_PATH);
    if (!manifest_data) log_fail("Manifest not found. Run 'nxpm update' first.");
    cJSON *manifest = cJSON_Parse(manifest_data);
    if (!manifest) log_fail("Invalid manifest JSON");

    ensure_dir(ROOT_DIR);
    char *db_data = read_file(DB_PATH);
    cJSON *db = db_data ? cJSON_Parse(db_data) : cJSON_CreateArray();

    install_package(pkg_name, manifest, db, edit_mode);

    cJSON_Delete(manifest);
    cJSON_Delete(db);
    free(manifest_data);
    if (db_data) free(db_data);
}

static void cmd_remove(const char *pkg_name) {
    char *db_data = read_file(DB_PATH);
    if (!db_data) log_fail("Database empty or missing.");
    
    cJSON *db = cJSON_Parse(db_data);
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
        save_json(DB_PATH, db);
        log_success("Package %s removed.", pkg_name);
    } else {
        log_fail("Package %s is not installed.", pkg_name);
    }

    cJSON_Delete(db);
    free(db_data);
}

static void cmd_list_installed() {
    char *db_data = read_file(DB_PATH);
    if (!db_data) {
        log_info("No packages installed.");
        return;
    }
    cJSON *db = cJSON_Parse(db_data);
    cJSON *item = NULL;

    printf("\n" BOLD WHITE " %-25s %-15s" RESET "\n", "INSTALLED PACKAGE", "VERSION");
    printf(BLUE " ---------------------------------------------" RESET "\n");

    cJSON_ArrayForEach(item, db) {
        printf(" %-24s %-15s\n", 
            cJSON_GetObjectItem(item, "name")->valuestring,
            cJSON_GetObjectItem(item, "version")->valuestring);
    }
    printf(BLUE " ---------------------------------------------" RESET "\n");
    printf(" Total: %d packages\n\n", cJSON_GetArraySize(db));

    cJSON_Delete(db);
    free(db_data);
}

static void cmd_list_online() {
    char *data = read_file(MANIFEST_PATH);
    if (!data) {
        log_fail("Manifest not found. Run 'nxpm update' first.");
        return;
    }
    cJSON *manifest = cJSON_Parse(data);
    cJSON *item = NULL;
    char *db_data = read_file(DB_PATH);
    cJSON *db = db_data ? cJSON_Parse(db_data) : cJSON_CreateArray();

    printf("\n" BOLD WHITE " %-25s %-15s %-10s" RESET "\n", "AVAILABLE PACKAGE", "VERSION", "STATUS");
    printf(BLUE " --------------------------------------------------------" RESET "\n");

    cJSON_ArrayForEach(item, manifest) {
        const char *name = cJSON_GetObjectItem(item, "name")->valuestring;
        const char *ver = cJSON_GetObjectItem(item, "version")->valuestring;
        int installed = is_installed(name, db);

        printf(" %-24s %-15s %s\n", name, ver, installed ? GREEN "Installed" RESET : WHITE "Available" RESET);
    }
    printf(BLUE " --------------------------------------------------------" RESET "\n");
    printf(" Total available: %d\n\n", cJSON_GetArraySize(manifest));

    cJSON_Delete(manifest);
    cJSON_Delete(db);
    free(data);
    if (db_data) free(db_data);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        print_banner();
        printf("Usage: nxpm [command] <args>\n\n");
        printf("Commands:\n");
        printf("  update [--check-nxpm-update] Update mirrorlist [check app self-update]\n");
        printf("  install [--edit-conf] <pkg> Install a package [edit ./configure and make configuration]\n");
        printf("  remove <pkg>               Uninstall a package\n");
        printf("  list [--online]            Show installed packages [with online]\n");
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

        if (argc >= 4) {
            if (strcmp(argv[2], "--edit-conf") == 0) {
                edit_conf = 1;
                pkg_name = argv[3];
            } else if (strcmp(argv[3], "--edit-conf") == 0) {
                edit_conf = 1;
                pkg_name = argv[2];
            } else {
                 log_fail("Invalid arguments for install");
            }
        } else if (argc >= 3) {
            pkg_name = argv[2];
        } else {
            log_fail("Usage: nxpm install [--edit-conf] <package_name>");
        }
        cmd_install(pkg_name, edit_conf);
    } else if (strcmp(argv[1], "remove") == 0) {
        if (argc < 3) log_fail("Usage: nxpm remove <package_name>");
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
