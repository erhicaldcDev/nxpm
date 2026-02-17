#ifndef UI_H
#define UI_H
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

void print_banner();
void log_header(const char *title);
void log_info(const char *fmt, ...);
void log_success(const char *fmt, ...);
void log_step(const char *step, const char *msg);
void log_fail(const char *fmt, ...);
void format_size(double size, char *out_buf);

#endif
