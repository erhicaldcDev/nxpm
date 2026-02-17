#ifndef GIT_H
#define GIT_H

int is_git_url(const char *url);
int git_prepare_source(const char *url, const char *pkg_name, const char *dest_dir);

#endif
