#include "../headers/path_checkers.h"


void make_path_cross_platform(char* const cur_path) {
        // To mitigate GCC warnings
        const size_t path_len = strlen(cur_path);
        for (size_t i = 0; i < path_len; ++i) {
                if ((cur_path)[i] == '/') {
#ifdef _WIN32
                        (cur_path)[i] = '\\';
#endif
                }
        }
}


int check_path(const char* path, const char* prefix, const int max_ups)
{
        // Check the prefix
        if (prefix) {
                if (strncmp(path, prefix, strlen(prefix))) {
                        return EXIT_FAILURE;
                }
        }

        // Check for ".." moves
        int ups = 0;
        const char* begin = path + (path[0] == '/');
        const char* end = strchr(path, '/');
        while (end) {                 
                if (end - begin == 2 && begin[0] == '.' && begin[1] == '.') {
                        ++ups;
                } else {
                        --ups;
                }

                begin = end + 1;
                end = strchr(end + 1, '/');
        }

        // Check for .. at the end (optional, fopen would fail)
        if (!strcmp(begin, "..")) {
                ++ups;
        }

        if (max_ups < ups) return -EXIT_FAILURE;

        // OK
        return EXIT_SUCCESS;
}


#define MAX_EXT_LEN 64  // max file extension length


const char* path_to_content_type(const char* const path)
{
        const char* last_dot = NULL;
        size_t path_len = strlen(path);
        for (int i = (int) path_len - 1; i >= 0; i--) {
                if (path[i] == '.') {
                        last_dot = path + i;
                        break;
                }
        }

        // Transform
        const char* ext = "text/plain";
        if (MAX_EXT_LEN <= path - last_dot) {
                return ext;
        }

        // Programming
        if (!strcmp(last_dot, ".html")) ext = "text/html";
        else if (!strcmp(last_dot, ".css")) ext = "text/css";
        else if (!strcmp(last_dot, ".js")) ext = "text/javascript";
        else if (!strcmp(last_dot, ".md")) ext = "text/markdown";
        else if (!strcmp(last_dot, ".json")) ext = "application/json";
        // Images
        else if (!strcmp(last_dot, ".png")) ext = "image/png";
        else if (!strcmp(last_dot, ".jpeg")) ext = "image/jpeg";
        else if (!strcmp(last_dot, ".jpg")) ext = "image/jpeg";
        else if (!strcmp(last_dot, ".ico")) ext = "image/vnd.microsoft.icon";
        else if (!strcmp(last_dot, ".svg")) ext = "image/svg+xml";

        return ext;
}
