/*
 * File: http_parsers.c
 * Author: Semyon Nadutkin
 * Copyright (C) 2025 Semyon Nadutkin
 */


#include "../headers/http_parsers.h"


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


enum http_code parse_http_header(const char* const req,
        char* header, char** dest)
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


enum http_code parse_http_method(const char* const req, char** method)
{
        // Allocate memory for the longest method (OPTIONS / CONNECT)
        *method = (char*) calloc(8, sizeof(char));
        if (!*method) return HTTP_INTERNAL_SERVER_ERROR; // no memory

        char* mp = strchr(req, ' '); // first space (e.g GET_)
        if (!mp) return HTTP_BAD_REQUEST;

        memcpy(*method, req, mp - req); // copy the method

        // Validate the method
        if (strcmp(*method, "GET")) return HTTP_OK;
        if (strcmp(*method, "POST")) return HTTP_OK;
        if (strcmp(*method, "PUT")) return HTTP_OK;
        if (strcmp(*method, "DELETE")) return HTTP_OK;
        if (strcmp(*method, "PATCH")) return HTTP_OK;
        if (strcmp(*method, "HEAD")) return HTTP_OK;
        if (strcmp(*method, "OPTIONS")) return HTTP_OK;
        if (strcmp(*method, "TRACE")) return HTTP_OK;
        if (strcmp(*method, "CONNECT")) return HTTP_OK;


        // Free the allocated memory on fail
        free(*method);
        return HTTP_BAD_REQUEST;
}


enum http_code parse_http_url(const char* const req, char** url)
{
        char* start = strchr(req, ' ');
        if (!start) return HTTP_BAD_REQUEST;

        start += 1;
        char* end = strchr(start,  ' ');
        if (!end) return HTTP_BAD_REQUEST;

        *url = (char*) calloc(end - start + 1, sizeof(char)); // + '\0'
        if (!*url) return HTTP_INTERNAL_SERVER_ERROR;

        memcpy(*url, start, end - start);
        return HTTP_OK;
}


enum http_code parse_http_content(const char* const req,
        char** content, const size_t clen)
{
        if (!clen) return HTTP_OK; // nothing to parse

        char* start = strstr(req, "\r\n\r\n");
        if (!start) return HTTP_BAD_REQUEST;

        start += 4;
        if (strlen(start) < clen) return HTTP_BAD_REQUEST;

        *content = (char*) calloc(clen, sizeof(char));\
        if (!*content) return HTTP_INTERNAL_SERVER_ERROR;

        memcpy(*content, start, clen);
        return HTTP_OK;
}


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


enum http_code parse_http_request(const char* const rbuf,
        struct http_request* req)
{
        // Parse the method
        int code = parse_http_method(rbuf, &req->method);    
        if (code != HTTP_OK) goto out_cleanup_req_return_err;

        // Parse the URL
        code = parse_http_url(rbuf, &(req->url));
        if (code != HTTP_OK) goto out_cleanup_req_return_err;

        // Parse the "Connection" header
        code = parse_http_header(rbuf, "Connection", &(req->conn));
        if (code != HTTP_OK) goto out_cleanup_req_return_err;

        // Parse the "Content-Type" header
        code = parse_http_header(rbuf, "Content-Type", &(req->ctype));
        if (code != HTTP_OK) goto out_cleanup_req_return_err;

        // Parse the "Content-Length" header
        char* content_len = NULL;
        code = parse_http_header(rbuf, "Content-Length", &content_len);
        if (code != HTTP_OK) goto out_cleanup_req_return_err;

        if (content_len) {
                char* end = NULL;
                int len = strtol(content_len, &end, 10);

                char endval = *end;
                free(content_len);

                if (endval != '\0' || len < 0) {
                        code = HTTP_BAD_REQUEST;
                        goto out_cleanup_req_return_err;
                }
                req->clen = (size_t) len;
        }

        // Parse the content
        code = parse_http_content(rbuf, &(req->content), req->clen);
        if (code != HTTP_OK) goto out_cleanup_req_return_err;

        return HTTP_OK;

out_cleanup_req_return_err:
        free_http_request(req);
        return code;
}
