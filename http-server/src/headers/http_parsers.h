/*
 * File: http_parsers.h
 * Author: Semyon Nadutkin
 * 
 * Description: simple functionality
 * to parse most common parts of HTTP 1.1 requests
 * 
 * Copyright (C) 2025 Semyon Nadutkin
 */


#pragma once


#include "tcp_socks.h"
#include <stdlib.h>
#include <string.h>


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
enum http_code parse_http_header(const char* const req,
        char* header, char** dest);


// Parses HTTP method
enum http_code parse_http_method(const char* const req, char** method);


// Parses the URL part of the HTTP request
enum http_code parse_http_url(const char* const req, char** url);


// Parses HTTP request contents
enum http_code parse_http_content(const char* const req,
        char** content, const size_t clen);


// Moves everything after the content section to the beginning of the request
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
enum http_code parse_http_request(const char* const rbuf,
        struct http_request* req);
