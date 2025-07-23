#include "../headers/libhttp.h"


// Frees the http_request structure
void free_http_request(struct http_request* req)
{
        if (req->method) free(req->method);
        if (req->url) free(req->url);
        if (req->conn) free(req->conn);
        if (req->ctype) free(req->ctype);
        if (req->content) free(req->content);

        req->method = NULL;
        req->url = NULL;
        req->conn = NULL;
        req->ctype = NULL;
        req->content = NULL;
        req->clen = 0;
}


/*
 * Parses a HTTP header
 *
 * @req    Full null-terminated HTTP request
 * @header Header which value is needed
 * @dest   NULL pointer where the header value should be written
 *
 * Description: takes a header,
 * parses the value, allocates memory to "dest",
 * writes the value to "dest"
 *
 * Returns:
 *      - Success: HTTP status code except 500
 *      - Failure: HTTP_INTERNAL_SERVER_ERROR
 */
int parse_http_header(const char* const req, char* header, char** dest)
{
        // Move to the start of the header
        char* start = strstr(req, header);
        if (!start) return HTTP_BAD_REQUEST;

        // Define header value bounds
        start += strlen(header) + 2; // + ": "
        char* end = strstr(start, "\r\n");
        if (!end) return HTTP_BAD_REQUEST;

        // Allocate memory for the header value
        size_t len = end - start;
        (*dest) = (char*) calloc(len + 1, sizeof(char)); // for '\0'
        if (!*dest) return HTTP_INTERNAL_SERVER_ERROR;

        // Copy the header value
        memcpy(*dest, start, end - start);

        return HTTP_OK;
}


// Moves a part after content to the beginning of request
int move_http_request(struct strinfo* reqstr, size_t content_len)
{
        char* end = strstr(reqstr->buf, "\r\n\r\n");
        if (!end) return EXIT_FAILURE;

        end += content_len + 4; // skip "\r\n\r\n"

        // Move the request
        size_t mv_len = strlen(end) + 1; // + '\0'
        memmove(reqstr->buf, end, mv_len);
        
        return EXIT_SUCCESS;
}


/*
 * Checks if the request is fully read
 *
 * Returns:
 *      - No memory: HTTP_INTERNAL_SERVER_ERROR
 *      - Invalid "Content-Length" header: HTTP_BAD_REQUEST
 *      - Fully read: HTTP_OK
 *      - Not read fully: 0
 */
int http_request_read_status(const char* const req)
{
        char* content = strstr(req, "\r\n\r\n");
        if (!content) return 0;

        content += 4; // "\r\n\r\n";

        char* content_len = NULL;
        int phh_res = parse_http_header(req, "Content-Length", &content_len);
        if (phh_res == HTTP_INTERNAL_SERVER_ERROR) return phh_res;

        if (content_len) {
                char* end = NULL;
                int len = strtol(content_len, &end, 10);
                
                char endval = *end;
                int cur_len = (int) strlen(content);
                free(content_len);

                if (endval != '\0' || len < 0) {
                        return HTTP_BAD_REQUEST;
                }

                if (len <= cur_len) return HTTP_OK;
                return 0; // Not read yet
        }

        return HTTP_OK;
}


// Parses a HTTP request
int parse_http_request(const char* const rbuf, struct http_request* req)
{
        // TODO: Complete HTTP request parser

        // Parse the "Connection" header
        int phh_res = parse_http_header(rbuf, "Connection", &(req->conn));
        if (phh_res == HTTP_INTERNAL_SERVER_ERROR) return phh_res;

        // Parse the "Content-Type" header
        phh_res = parse_http_header(rbuf, "Content-Type", &(req->ctype));
        if (phh_res == HTTP_INTERNAL_SERVER_ERROR) return phh_res;

        // Parse the "Content-Length" header
        char* content_len = NULL;
        phh_res = parse_http_header(rbuf, "Content-Length", &content_len);
        if (phh_res == HTTP_INTERNAL_SERVER_ERROR) return phh_res;

        if (content_len) {
                char* end = NULL;
                int len = strtol(content_len, &end, 10);

                char endval = *end;
                free(content_len);

                if (endval != '\0' || len < 0) return HTTP_BAD_REQUEST;
                req->clen = (size_t) len;
        }


        return HTTP_OK;
}
