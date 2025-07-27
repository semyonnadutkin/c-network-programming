#include "../headers/http_codes.h"


const char* http_code_to_str_1_1(const enum http_code code)
{
        switch (code) {
        case HTTP_INTERNAL_SERVER_ERROR:
                return "HTTP/1.1 500 Internal Server Error";
        case HTTP_NOT_FOUND:
                return "HTTP/1.1 404 Not Found";
        case HTTP_BAD_REQUEST:
                return "HTTP/1.1 400 Bad Request";
        case HTTP_OK:
                return "HTTP/1.1 200 OK";
        default:
                return "HTTP/1.1 418 I'm a teapot";
        }
}
