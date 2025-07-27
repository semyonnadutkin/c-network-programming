#pragma once
#include <stdlib.h> // memory management
#include <string.h> // strcmp()
#include <stdio.h>  // making logs


/*
 * HTTP route and the related handler function storage
 *
 * @route Route used by a client
 * @path  Related handler function
 */
struct http_route {
        const char* route;
        int (*handler)(void*, const size_t argc, ...);
};


/*
 * HTTP routes list node
 *
 * @rt HTTP route and handler
 * @next Pointer to the next node
 *
 * Singly linked since
 * no remove operations are expected
 */
struct http_rts_lnode {
        struct http_route rt;
        struct http_rts_lnode* next;
};


// List of HTTP routes
struct http_rts_list {
        struct http_rts_lnode* head;
};


// Initializes HTTP routes list
void http_rts_list_init(struct http_rts_list* list);


// Adds a new HTTP route node to the list of routes
int http_rts_list_add(struct http_rts_list* list, struct http_route rt);


// Frees the list of HTTP routes
void http_rts_list_free(struct http_rts_list* list);


// Gets the HTTP router
struct http_rts_list* get_http_router();


// Sets the HTTP route
int set_http_route(struct http_route rt);


// Gets HTTP route
struct http_route* get_http_route(const char* route);
