#ifndef NETWORK_H
#define NETWORK_H

int download_file(const char *url, const char *output_path);
char *download_to_memory(const char *url);

#endif
