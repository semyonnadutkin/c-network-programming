/*
 * File: path_checkers.h
 * Author: Semyon Nadutkin
 *
 * Description: simple HTTP 1.1
 * response writer functions
 *
 * Copyright (C) 2025 Semyon Nadutkin
 */


#pragma once

#include <stdlib.h>
#include <string.h>


// Changes '/' to '\\' on Windows
void make_path_cross_platform(char* const cur_path);


/*
 * Checks if the path follows the specified criteria
 *
 * @path        Path to the resource
 * @prefix      Required path prefix (optional)
 * @max_ups     Maximum number of ".." moves allowed
 *
 * Returns:
 *      - Correct path:                 EXIT_SUCCESS
 *      - Prefix mismatch:              EXIT_FAILURE
 *      - Too much ".." moves:          -EXIT_FAILURE
 */
int check_path(const char* path, const char* prefix, const int max_ups);


#define MAX_EXT_LEN 64  // max file extension length


/*
 * Translates a path to resourse
 * to the related content type string
 */
const char* path_to_content_type(const char* const path);
