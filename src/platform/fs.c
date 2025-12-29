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

/*
 * Debug resource monitoring
 */

#ifdef DEBUG_RESOURCES

#include <stdio.h>
#include <dirent.h>

#ifdef PLATFORM_LINUX
#include <sys/resource.h>
#endif

void debug_log_resources(const char *label) {
    int fd_count = 0;

#ifdef PLATFORM_LINUX
    // Count open FDs via /proc/self/fd
    DIR *dir = opendir("/proc/self/fd");
    if (dir != NULL) {
        while (readdir(dir) != NULL) {
            fd_count++;
        }
        closedir(dir);
        fd_count -= 2;  // Subtract . and ..
    }

    // Get memory usage from /proc/self/status
    long rss_kb = 0;
    FILE *status = fopen("/proc/self/status", "r");
    if (status != NULL) {
        char line[256];
        while (fgets(line, sizeof(line), status)) {
            if (strncmp(line, "VmRSS:", 6) == 0) {
                sscanf(line + 6, "%ld", &rss_kb);
                break;
            }
        }
        fclose(status);
    }

    printf("[DEBUG] %s: FDs=%d, RSS=%ld KB\n", label, fd_count, rss_kb);

#elif defined(PLATFORM_MACOS)
    // macOS: count FDs by iterating possible range
    struct stat st;
    for (int i = 0; i < 1024; i++) {
        if (fstat(i, &st) == 0) {
            fd_count++;
        }
    }

    // RSS via getrusage
    struct rusage usage;
    long rss_kb = 0;
    if (getrusage(RUSAGE_SELF, &usage) == 0) {
        rss_kb = usage.ru_maxrss / 1024;  // macOS reports bytes
    }

    printf("[DEBUG] %s: FDs=%d, RSS=%ld KB\n", label, fd_count, rss_kb);
#else
    (void)label;
    (void)fd_count;
#endif
}

#else
// No-op when DEBUG_RESOURCES not defined
void debug_log_resources(const char *label) {
    (void)label;
}
#endif
