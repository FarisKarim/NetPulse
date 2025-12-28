#include "platform/platform.h"

#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>
#include <pwd.h>

#define NETPULSE_DIR ".netpulse"

int mkdir_p(const char *path) {
    if (path == NULL || path[0] == '\0') {
        return -1;
    }

    char *path_copy = strdup(path);
    if (path_copy == NULL) {
        return -1;
    }

    char *p = path_copy;

    // Skip leading slash for absolute paths
    if (*p == '/') {
        p++;
    }

    while (*p != '\0') {
        // Find next slash
        while (*p != '\0' && *p != '/') {
            p++;
        }

        char saved = *p;
        *p = '\0';

        // Try to create directory
        if (mkdir(path_copy, 0755) != 0) {
            if (errno != EEXIST) {
                free(path_copy);
                return -1;
            }
        }

        *p = saved;
        if (*p != '\0') {
            p++;
        }
    }

    free(path_copy);
    return 0;
}

char *expand_home_path(const char *path) {
    if (path == NULL) {
        return NULL;
    }

    // If doesn't start with ~, just duplicate
    if (path[0] != '~') {
        return strdup(path);
    }

    const char *home = getenv("HOME");
    if (home == NULL) {
        // Fall back to passwd entry
        struct passwd *pw = getpwuid(getuid());
        if (pw == NULL) {
            return NULL;
        }
        home = pw->pw_dir;
    }

    // Skip the ~ character
    const char *rest = path + 1;

    size_t home_len = strlen(home);
    size_t rest_len = strlen(rest);
    size_t total_len = home_len + rest_len + 1;

    char *result = malloc(total_len);
    if (result == NULL) {
        return NULL;
    }

    memcpy(result, home, home_len);
    memcpy(result + home_len, rest, rest_len + 1);

    return result;
}

char *get_data_dir(void) {
    char *home = expand_home_path("~/" NETPULSE_DIR);
    if (home == NULL) {
        return NULL;
    }

    // Create directory if it doesn't exist
    if (mkdir_p(home) != 0) {
        // Check if it already exists
        struct stat st;
        if (stat(home, &st) != 0 || !S_ISDIR(st.st_mode)) {
            free(home);
            return NULL;
        }
    }

    return home;
}
