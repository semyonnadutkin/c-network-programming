#include <stdio.h>                  // making logs
#include <stdlib.h>                 // memory management
#include <stdarg.h>
#include <string.h>
#include "headers/http_parsers.h"
#include "headers/http_writers.h"   // write_http_from_code(), ...
#include "headers/path_checkers.h"


/*
 * Reads the file to rbuf
 *
 * Returns:
 *      - Success: Number of bytes read
 *      - Failure: -EXIT_FAILURE
 */
int read_file(FILE* f, char** rbuf)
{
        // Calculate the file size
        if (fseek(f, 0, SEEK_END)) return -EXIT_FAILURE;

        long sz = ftell(f);
        if (sz < 0) return -EXIT_FAILURE;

        if (fseek(f, 0, SEEK_SET)) return -EXIT_FAILURE;

        // Read the contents
        *rbuf = (char*) calloc((size_t) sz + 1, sizeof(char));
        if (!*rbuf) return -EXIT_FAILURE;

        unsigned long read = fread(*rbuf, sizeof(char), sz, f);
        if (read != (size_t) sz) {
                free(*rbuf);
                return -EXIT_FAILURE;
        }

        return read;
}


/*
 * Gets page 404 contents
 *
 * Returns: Number of bytes read
 */
size_t get_404_page(char** page)
{
        const char* path = "public/frontend/templates/not_found.html";
        FILE* f = fopen(path, "rb");
        if (!f) return 0;

        int read = read_file(f, page);

        fclose(f);
        return read == -EXIT_FAILURE ? 0 : read;
}


// Writes 404 response with a page
int write_404_page(struct strinfo* dest)
{
        char* page404 = NULL;
        size_t read = get_404_page(&page404);
        char* ctype = (page404 ? "text/html" : NULL);
        int w404_res = write_http_from_code(HTTP_NOT_FOUND, dest,
                ctype, read, page404, "close");
        if (page404) free(page404);
        if (w404_res) return EXIT_FAILURE;

        return EXIT_SUCCESS;
}


// Writes 505 respose with a short plain text description
int write_500_page(struct strinfo* dest)
{
        const char* const content = "Internal Server Error";
        const size_t content_len = strlen(content);

        return write_http_from_code(HTTP_INTERNAL_SERVER_ERROR, dest,
                "text/plain", content_len, content, NULL);
}


/*
* Gets the resource and writes the response
 *
 * @dest        Send string (struct strinfo*)
 * @path        Path to the resource
 * @prefix      Required path prefix (optional)
 * @req         Client's HTTP request (optional)
 * 
 * Returns:
 *      - Success: EXIT_SUCCESS
 *      - Failure: EXIT_FAILURE
 */
int process_default_resource_request(void* dest,
        char* path, const char* prefix, struct http_request* req)
{
        if (!path || check_path(path, prefix, 0)) { // write 404
                return write_404_page(dest);
        }

        // Check if the target is a file
        if (!strchr(path, '.')) return write_404_page(dest);

        make_path_cross_platform(path);
        FILE* f = fopen(path, "rb");
        if (!f) return write_404_page(dest);

        // Read the file
        char* rbuf = NULL;
        int bytes_read = read_file(f, &rbuf);
        fclose(f);
        if (bytes_read < 0) goto out_write_500;

        // Write the response
        const char* connection = (req ? req->conn : "close"); // same as client
        int w200_res = write_http_from_code(HTTP_OK, dest,
                path_to_content_type(path), bytes_read, rbuf, connection);
        free(rbuf);
        return w200_res;

out_write_500:
        return write_500_page(dest);
}


/*
 * Gets the front page and writes the response
 *
 * @dest Send string (struct strinfo*)
 * @...  Client's HTTP request (optional, struct http_request*)
 * 
 * Returns:
 *      - Success: EXIT_SUCCESS
 *      - Failure: EXIT_FAILURE
 */
int get_front_page(void* dest, const size_t argc, ...)
{
        // TODO: accept HTTP request as an argument
        struct http_request* req = NULL;

        if (argc) {
                va_list args = { 0 };
                va_start(args, argc);
                req = va_arg(args, struct http_request*);
                va_end(args);
        }

        char* path = "public/frontend/templates/index.html";
        return process_default_resource_request(dest, path, NULL, req);
}
