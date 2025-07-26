/*
 * File: http_writers.h
 * Author: Semyon Nadutkin
 *
 * Description: simple HTTP 1.1
 * response writer functions
 *
 * Copyright (C) 2025 Semyon Nadutkin
 */


#pragma once


#include <stdio.h>      // sprintf()
#include <string.h>     // strlen()
#include <stdarg.h>     // va_list, va_arg(), ...
#include <stdlib.h>     // EXIT_SUCCESS, EXIT_FAILURE
#include "tcp_socks.h"  // struct strinfo


// HTTP response size calculator for write_http_response() function
size_t calculate_http_response_size(const char* response,
        const char* content_type,
        const char* content,
        const size_t argc,
        va_list args);


/*
 * Writes a HTTP response to the send string
 *
 * Notes:
 *      - All string arguments should be null-terminated
 *      - Headers should not contain "\r\n" ending
 *      - Required: "response", other arguments may be NULL
 *
 * @response            Response string
 * @content_type        Type of the content
 * @content             Contents send to the client
 * @argc                Additional headers count
 * @...                 Additional headers with values
 *
 * Returns:
 *      - Success:      EXIT_SUCCESS
 *      - No memory:    EXIT_FAILURE
 */
int write_http_response(struct strinfo* sstr,
        const char* response,
        const char* content_type,
        const char* content,
        const size_t argc,
        ...);
