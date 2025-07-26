#include "../headers/http_writers.h"


size_t calculate_http_response_size(const char* response,
        const char* content_type,
        const char* content,
        const size_t argc,
        va_list args)
{
        size_t total_sz = 1; // '\0'
        total_sz += strlen(response) + 2; // + "\r\n"

        int count_contents = (content_type != NULL && content != NULL);
        if (count_contents) {
                size_t content_len = (size_t) strlen(content);

                total_sz += strlen("Content-Type: ")
                + strlen(content_type) + 2;

                char clen_str[32];
                memset(clen_str, 0, sizeof(clen_str));
                sprintf(clen_str, "%zu", content_len);
                total_sz += strlen("Content-Length: ") + strlen(clen_str) + 2;

                total_sz += 2;  // blank line
                total_sz += content_len;
        }

        va_list chrs_args = { 0 };
        va_copy(chrs_args, args);
        for (size_t i = 0; i < argc; ++i) {
                const char* arg = va_arg(chrs_args, const char*);
                total_sz += strlen(arg) + 2;
        }
        va_end(chrs_args);

        return total_sz;
}


int write_http_response(struct strinfo* sstr,
        const char* response,
        const char* content_type,
        const char* content,
        const size_t argc,
        ...)
{
        // Calculate the total response size
        va_list args = { 0 };
        va_start(args, argc);
        size_t total_sz = calculate_http_response_size(response,
                content_type, content, argc, args);

        // Allocate the memory
        if (sstr->buf) {
                cleanup_strinfo(sstr);
        }
        sstr->buf = (char*) calloc(total_sz, 1);
        if (!sstr->buf) return EXIT_FAILURE;
        sstr->sz = total_sz;
        sstr->len = sstr->sz - 1;

        // Write the response
        char* cur = sstr->buf;
        cur += sprintf(cur, "%s\r\n", response);

        // Write content related info
        int write_contents = (content_type != NULL && content != NULL);
        if (write_contents) {
                cur += sprintf(cur,
                        "Content-Type: %s\r\n"
                        "Content-Length: %zu\r\n",
                        content_type, (size_t) strlen(content));
        }

        // Write the additional headers
        for (size_t i = 0; i < argc; ++i) {
                const char* arg = va_arg(args, const char*);
                cur += sprintf(cur, "%s\r\n", arg);
        }
        va_end(args);

        // Write the contents
        if (write_contents) {
                cur += sprintf(cur, "\r\n%s", content);
        }

        return EXIT_SUCCESS;
}
