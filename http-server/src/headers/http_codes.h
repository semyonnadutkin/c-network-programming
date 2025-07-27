/*
 * File: http_codes.h
 * Author: Semyon Nadutkin
 * 
 * Description: HTTP status codes
 * and related functionality
 * 
 * Copyright (C) 2025 Semyon Nadutkin
 */


#pragma once


// HTTP status code
enum http_code {
        HTTP_INTERNAL_SERVER_ERROR = 500,
        HTTP_NOT_FOUND = 404,
        HTTP_BAD_REQUEST = 400,
        HTTP_OK = 200
};


// Translates HTTP status code to string
const char* http_code_to_str_1_1(const enum http_code code);
