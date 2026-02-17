#include "fs.h"
#include "ui.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>

void ensure_dir(const char *path) {
    struct stat st = {0};
    if (stat(path, &st) == -1) {
        char cmd[1024];
        snprintf(cmd, sizeof(cmd), "mkdir -p %s", path);
        if (system(cmd) != 0) log_fail("Failed to create directory: %s", path);
    }
}

char *read_file(const char *filename) {
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
