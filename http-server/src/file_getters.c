#include <stdio.h>                  // making logs
#include <stdlib.h>                 // memory management
#include <stdarg.h>
#include "headers/http_parsers.h"
#include "headers/http_writers.h"   // write_http_from_code(), ...
#include "headers/path_checkers.h"


/*
 * Reads the file to rbuf
 *
 * Returns:
 *      - Success: EXIT_SUCCESS
 *      - Failure: EXIT_FAILURE
 */
int read_file(FILE* f, char** rbuf)
{
        // Calculate the file size
        if (fseek(f, 0, SEEK_END)) return EXIT_FAILURE;

        long sz = ftell(f);
        if (sz < 0) EXIT_FAILURE;

        if (fseek(f, 0, SEEK_SET)) return EXIT_FAILURE;

        // Read the contents
        *rbuf = (char*) calloc((size_t) sz + 1, sizeof(char)); // + '\0'
        if (!*rbuf) return EXIT_FAILURE;

        unsigned long read = fread(*rbuf, sizeof(char), sz, f);
        if (read != (size_t) sz) {
                free(*rbuf);
                return EXIT_FAILURE;
        }

        return EXIT_SUCCESS;
}


// Gets 404 page contents
char* get_404_page(void)
{
        
        return NULL;
}


// Writes 404 response with a page
int write_404_page(struct strinfo* dest)
{
        char* page404 = get_404_page();
        char* ctype = (page404 ? "text/html; charset=UTF-8" : NULL);
        int w404_res = write_http_from_code(HTTP_NOT_FOUND, dest,
                page404, ctype, NULL);
        if (page404) free(page404);
        if (w404_res) return EXIT_FAILURE;

        return EXIT_SUCCESS;
}


/*
* Gets the resource and writes the response
 *
 * @dest Send string (struct strinfo*)
 * @...  Client's HTTP request (optional)
 * 
 * Returns:
 *      - Success: EXIT_SUCCESS
 *      - Failure: EXIT_FAILURE
 */
int process_default_resource_request(void* dest, const char* path,
    struct http_request* req)
{
        const char* prefix = "public/"; // required prefix
        if (check_path(path, prefix, 0)) { // write 404
                return write_404_page(dest);
        }

        FILE* f = fopen(path, "rb");
        if (!f) return write_404_page(dest);

        // Read the file
        char* rbuf = NULL;
        int rf_res = read_file(f, &rbuf);
        fclose(f);
        if (rf_res) goto out_write_500;

        // Write the response
        const char* connection = (req ? req->conn : "close"); // same as client
        int w200_res = write_http_from_code(HTTP_OK, dest,
                path_to_content_type(path), rbuf, connection);
        free(rbuf);
        return w200_res;

out_write_500:
        return write_http_from_code(HTTP_INTERNAL_SERVER_ERROR, dest,
                NULL, NULL, NULL);
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

        const char* path = "public/frontend/templates/index.html";
        return process_default_resource_request(dest, path, req);
}
