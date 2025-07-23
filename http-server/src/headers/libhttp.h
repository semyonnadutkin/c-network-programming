#include "tcp_socks.h"
#include <stdlib.h>
#include <string.h>


// HTTP status code
enum http_code {
        HTTP_INTERNAL_SERVER_ERROR = 500,
        HTTP_NOT_FOUND = 404,
        HTTP_BAD_REQUEST = 400,
        HTTP_OK = 200
};


/*
 * HTTP request
 *
 * @method  GET / POST / ...
 * @url     Route
 * @conn    Connection
 * @ctype   Content-Type
 * @content Content
 * @clen    Content-Length
 */
struct http_request {
        char* method;
        char* url;
        char* conn;
        char* ctype;
        char* content;
        size_t clen;
};


// Frees the http_request structure
void free_http_request(struct http_request* req);


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
int parse_http_header(const char* const req, char* header, char** dest);


// Moves a part after content to the beginning of request
int move_http_request(struct strinfo* reqstr, size_t content_len);


/*
 * Checks if the request is fully read
 *
 * Returns:
 *      - No memory: HTTP_INTERNAL_SERVER_ERROR
 *      - Invalid "Content-Length" header: HTTP_BAD_REQUEST
 *      - Fully read: HTTP_OK
 *      - Not read fully: 0
 */
int http_request_read_status(const char* const req);


// Parses a HTTP request
int parse_http_request(const char* const rbuf, struct http_request* req);
