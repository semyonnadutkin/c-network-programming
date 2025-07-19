#pragma once


#include "crossplatform_sockets.h"
#include <stdlib.h>


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
 * Info about a TCP client
 *
 * @client Client's fd
 * @sdstr  Client's request
 * @rvstr  Response to the client
 */
struct clientinfo {
        SOCKET client;
        struct strinfo sdstr;
        struct strinfo rvstr;
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
int initialize_strinfo(struct strinfo* strinf);


// Frees the buffer, sets the default values
int cleanup_strinfo(struct strinfo* strinf);


// Initializes struct clientinfo
// Sets "client" to "INVALID_SOCKET",
// initalizes the related strinfo structures
int initialize_clientinfo(struct clientinfo* cinfo);


// Closes the fd, cleans up the related strinfo structures
int cleanup_clientinfo(struct clientinfo* cinfo);


// Initializes server and max fds with "serv",
// initializes the related clientinfo structures
int initialize_serverinfo(struct serverinfo* sinfo, const SOCKET serv);


// Closes the server fd,
// cleans up the related clientinfo structures
int cleanup_serverinfo(struct serverinfo* sinfo);


// Transforms a serverinfo structure to an fd_set containing
// connected clients' fds and the server fd
int sinfo_to_fd_set(const struct serverinfo* sinfo, fd_set* res);


// Handles clients' connections
int server_handle_clients(struct serverinfo* sinfo,
        const fd_set readfds,
        const fd_set writefds,
        void (*on_read_set)(struct serverinfo* sinfo, const SOCKET client),
        void (*on_write_set)(struct serverinfo* sinfo, const SOCKET client));


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
