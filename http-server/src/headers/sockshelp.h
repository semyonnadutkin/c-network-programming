/*
 * File: sockshelp.h
 * Author: Semyon Nadutkin
 *
 * Description: functionality for easy
 * TCP / UDP program startup
 *
 * Copyright (C) 2025 Semyon Nadutkin
 */


#pragma once


#include "crossplatform_sockets.h"
#include <stdlib.h>


/*
 * Allocates as much bytes as possible
 *
 * @dest Buffer for which memory is allocated
 * @mn Minimum buffer size
 * @mx Needed buffer size
 *
 * Description: tries allocating "mx" bytes,
 * "mx" / 2 bytes on fail and so on until
 * the min border is reached
 *
 * Returns:
 *      - Failure: -1
 *      - Success: number of bytes allocated
 */
int allocate_max(char** dest, const int mn, const int mx);


// Starts sockets API on Windows,
// does nothing on a Unix-based OS
int sockets_startup(void);


// Cleans up sockets API on Windows,
// does nothing on a Unix-based OS
int sockets_cleanup(void);


/*
 * Configures an address using the provided parameters
 *
 * @addr Requested address
 * @serv Requested service
 * @family IPv4 / IPv6
 * @socktype SOCK_STREAM / SOCK_DGRAM
 * @flags Requested address flags
 *
 * Returns:
 *      Success: Non-null struct addrinfo pointer
 *      Failure: NULL
 */
struct addrinfo* configure_address(const char* addr,
        const char* serv,
        const int family,
        const int socktype,
        const int flags);


/*
 * Makes an IPv6 socket dual stack
 *
 * @fd Socket needed to be transformed
 *
 * Returns:
 *      - Success: 0
 *      - Failure: non-zero value
 */
int make_dual_stack(const SOCKET fd);


/*
 * Starts the server
 *
 * @addr Bind address
 * @max_conn Maximum number of connections allowed to be handled at once
 *
 * Description: creates a socket,
 * binds it to the address, starts listening
 */
SOCKET start_server(struct addrinfo* addr, const int max_conn);
