/*
 * File: tcp_socks.h
 * Author: Semyon Nadutkin
 *
 * Description: abstractions for TCP programs
 *
 * Copyright (C) 2025 Semyon Nadutkin
 */


#pragma once


#include "cross_platform_sockets.h"
#include "sockshelp.h"
#include <stdlib.h>


/*
 * Client's current state
 *
 * @CS_READY     Client's request was fully read
 * @CS_EXECUTING Client's request is being processed
 * @CS_IDLE      Client is being idle
 * @CS_SENDING   Sending a response to the client
 * @CS_RECEIVING Receiving a client's request
 */
enum client_state {
        CS_READY,
        CS_EXECUTING,
        CS_IDLE,
        CS_SENDING,
        CS_RECEIVING
};


/*
 * Info about a string
 *
 * @buf String buffer
 * @sz  Buffer size
 * @len Length of the string
 * @adv Advance, temporary start shift
 */
struct strinfo {
        char* buf;
        size_t sz;
        size_t len;
        size_t adv;
};


/*
 * Info about a TCP client
 *
 * @client      Client's fd
 * @state       State of the client
 * @sdstr       Client's request
 * @rvstr       Response to the client
 * @add_data    Additional data
 */
struct clientinfo {
        SOCKET client;
        enum client_state state;

        struct strinfo sdstr;
        struct strinfo rvstr;

        void* add_data;
};


/*
 * Stores info about a server
 *
 * @serv        Server fd
 * @max_fd      Max fd among the connected clients and server
 * @clients     Info about clients
 * @readfds     Set of fds ready for read operation
 * @writefds    Set of fds ready for write operation
 */
struct serverinfo {
        SOCKET serv;
        SOCKET max_fd;

        struct clientinfo clients[MAX_CONN];

        fd_set readfds;
        fd_set writefds;
};


// Initializes struct strinfo
// Sets all parameters to zero
void initialize_strinfo(struct strinfo* strinf);


// Frees the buffer, sets the default values
void cleanup_strinfo(struct strinfo* strinf);


// Initializes struct clientinfo
// Sets "client" to "INVALID_SOCKET",
// initalizes the related strinfo structures
void initialize_clientinfo(struct clientinfo* cinfo);


// Closes the fd, cleans up the related strinfo structures
void cleanup_clientinfo(struct clientinfo* cinfo);


// Initializes server and max fds with "serv",
// initializes the related clientinfo structures
void initialize_serverinfo(struct serverinfo* sinfo, const SOCKET serv);


// Closes the server fd,
// cleans up the related clientinfo structures
void cleanup_serverinfo(struct serverinfo* sinfo);


/*
 * Appends a request from a TCP client to the existing buffer
 * 
 * Returns:
 *      - Success:      bytes read
 *      - Disconnect:   EXIT_SUCCESS  (0)
 *      - Error:        -EXIT_FAILURE (-1)
 */
int server_receive_request(struct serverinfo* sinfo, struct clientinfo* cinfo,
        int (*on_disconnect)(struct serverinfo*, const SOCKET client));


// Finds a place for a new client
int find_place_for_client(struct serverinfo* sinfo);


// Accepts a new client, initializes the free cell
void server_accept_client(struct serverinfo* sinfo);


// Disconnects the given client and clears the related clientinfo structure
int drop_client(struct serverinfo* sinfo, const SOCKET client);


// Handles clients' connections
void server_handle_clients(struct serverinfo* sinfo,
        const fd_set readfds,
        const fd_set writefds,
        void (*on_read_set)(struct serverinfo* sinfo, const SOCKET client),
        void (*on_write_set)(struct serverinfo* sinfo, const SOCKET client));


// Transforms a serverinfo structure to an fd_set containing
// connected clients' fds and the server fd
void sinfo_to_fd_set(const struct serverinfo* sinfo, fd_set* res);


// Sets only the server and clients which are idle
// or from which input is being handled
void sinfo_to_read_fds(const struct serverinfo* sinfo, fd_set* res);


// Sets only the clients which are ready for response
// or to which input is being sent
void sinfo_to_write_fds(const struct serverinfo* sinfo, fd_set* res);


// Updates max fd value for select() call
void update_max_fd(struct serverinfo* sinfo);


/*
 * Blocks until at least one fd has input
 *
 * @sinfo        Info about the TCP server
 * @on_serv_set  Function called if a client is trying to connect
 * @on_read_set  Function called if a client's fd is set for read operation
 * @on_write_set Function called if a client's fd is set for write operation
 */
int server_check_fds(struct serverinfo* sinfo,
        void (*on_serv_set)(struct serverinfo* sinfo),
        void (*on_read_set)(struct serverinfo* sinfo, const SOCKET client),
        void (*on_write_set)(struct serverinfo* sinfo, const SOCKET client));
