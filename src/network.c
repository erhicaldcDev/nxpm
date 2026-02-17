#include "network.h"
#include "ui.h"
#include "config.h" // dla user agenta jesli potrzebny
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>

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

int download_file(const char *url, const char *output_path) {
    CURL *curl_handle;
    CURLcode res;
    FILE *pagefile;

    // curl_global_init powinno byc w main, ale tu dla bezpieczenstwa:
    curl_global_init(CURL_GLOBAL_ALL);
    curl_handle = curl_easy_init();
    if (!curl_handle) return -1;

    pagefile = fopen(output_path, "wb");
    if (!pagefile) {
        curl_easy_cleanup(curl_handle);
        return -1;
    }

    curl_easy_setopt(curl_handle, CURLOPT_URL, url);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_file_cb);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, pagefile);
    curl_easy_setopt(curl_handle, CURLOPT_XFERINFOFUNCTION, progress_cb);
    curl_easy_setopt(curl_handle, CURLOPT_NOPROGRESS, 0L);
    curl_easy_setopt(curl_handle, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "nxpm/1.5");
    curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYHOST, 0L);

    printf("\n");
    res = curl_easy_perform(curl_handle);
    printf("\n");

    fclose(pagefile);
    curl_easy_cleanup(curl_handle);
    // curl_global_cleanup(); // Zostawiamy dla main

    if (res != CURLE_OK) {
        remove(output_path);
        return 1;
    }
    return 0;
}

char *download_to_memory(const char *url) {
    CURL *curl;
    CURLcode res;
    char *response = NULL;

    curl = curl_easy_init();
    if(curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_memory_cb);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "nxpm/1.5");
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
        
        res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
        
        if (res != CURLE_OK) {
            if(response) free(response);
            return NULL;
        }
    }
    return response;
}
