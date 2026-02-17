#include "ui.h"
#include "config.h"
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

void print_banner() {
    printf(CYAN BOLD);
    printf(" _   _  __  __ ____  __  __ \n");
    printf("| \\ | | \\ \\/ /|  _ \\|  \\/  |\n");
    printf("|  \\| |  \\  / | |_) | |\\/| |\n");
    printf("| |\\  |  /  \\ |  __/| |  | |\n");
    printf("|_| \\_| /_/\\_\\|_|   |_|  |_|\n");
    printf("            v%s\n", NXPM_VERSION);
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

void format_size(double size, char *out_buf) {
    const char *units[] = {"B", "KB", "MB", "GB"};
    int i = 0;
    while (size > 1024 && i < 3) {
        size /= 1024;
        i++;
    }
    sprintf(out_buf, "%.2f %s", size, units[i]);
}
