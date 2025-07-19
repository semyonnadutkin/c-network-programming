#include <stdlib.h>
#include <sys/select.h>
#define _CPSOCKS_DEBUG_ // to make debug logs
#include "headers/crossplatform_sockets.h"
#include <stdio.h>


/*
 * Allocates as much bytes as possible
 *
 * @dest Buffer for which memory is allocated
 * @mx Needed buffer size
 * @mn Minimum buffer size
 *
 * Description: tries allocating "mx" bytes,
 * "mx" / 2 bytes on fail and so on until
 * the min border is reached
 *
 * Returns:
 *      - Failure: -1
 *      - Success: number of bytes allocated
 */
int allocate_max(char** dest, const int mx, const int mn)
{
        if (!dest) {
#ifdef _CPSOCKS_DEBUG_
                _CPSOCKS_ERROR("Invalid pointer passed\n"
                               "\tFunction: allocate_max()");
#endif // _CPSOCKS_DEBUG_

                return -1;
        }

        if (mx < mn) {
#ifdef _CPSOCKS_DEBUG_
                _CPSOCKS_ERROR("Invalid bounds provided\n"
                               "\tFunction: allocate_max()");
#endif // _CPSOCKS_DEBUG_

                return -1;
        }
        
        // Allocate memory
        size_t dest_sz = (size_t) mx;
        while (dest_sz && mn <= (int) dest_sz) {
                *dest = (char*) calloc(dest_sz, sizeof(char));
                if (*dest) break;       // success

                dest_sz /= 2;
        }

        return (int) dest_sz;
}


// Starts socket API on Windows,
// does nothing on a Unix-based OS
int sockets_startup(void)
{
        int ret = 0;    // exit code

        // Startup Winsock
#ifdef _WIN32
        DWORD ws_ver = MAKEWORD(2, 2);  // required version: 2.2
        WSADATA wsd  = { 0 };           // Winsock data
        ret = WSAStartup(ws_ver, &wsd);

#ifdef _CPSOCKS_DEBUG_
        if (ret) {
                PSOCKERROR("WSAStartup() failed");
        }
#endif // _CPSOCKS_DEBUG_

#endif // _WIN32

        return ret;
}


// Cleans up socket API on Windows,
// does nothing on a Unix-based OS
int sockets_cleanup(void)
{
        int ret = 0;    // exit code

        // Cleanup Winsock
#ifdef _WIN32
        ret = WSACleanup();

#ifdef _CPSOCKS_DEBUG_
        if (ret) { 
                PSOCKERROR("WSACleanup() failed");
        }
#endif // _CPSOCKS_DEBUG_

#endif // _WIN32

        return ret;
}


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
                                   const int   family,
                                   const int   socktype,
                                   const int   flags)
{
        struct addrinfo hints = { 0 };
        hints.ai_family = family;
        hints.ai_socktype = socktype;
        hints.ai_flags = flags;

        struct addrinfo* res = NULL;
        int gai_res = getaddrinfo(addr, serv, &hints, &res);
#ifdef _CPSOCKS_DEBUG_
        if (gai_res) {
                PSOCKERROR("getaddrinfo() failed");
        }
#endif

        return res;
}


/*
 * Makes an IPv6 socket dual stack
 *
 * @fd Socket needed to be transformed
 *
 * Returns:
 *      - Success: 0
 *      - Failure: non-zero value
 */
int make_dual_stack(const SOCKET fd)
{
        int opt = 0;
        if (setsockopt(fd, IPPROTO_IPV6, IPV6_V6ONLY, &opt, sizeof(opt))) {
#ifdef _CPSOCKS_DEBUG_
            PSOCKERROR("setsockopt() failed");
#endif // _CPSOCKS_DEBUG_

            return EXIT_FAILURE;
        }

        return EXIT_SUCCESS;
}


/*
 * Starts the server
 *
 * @addr Bind address
 * @max_conn Maximum connection allowed to be handled at once
 *
 * Description: creates a socket,
 * binds it to the address, starts listening
 */
SOCKET start_server(struct addrinfo* addr, const int max_conn)
{
        // Create a socket
        SOCKET serv = socket(addr->ai_family,
            addr->ai_socktype, addr->ai_protocol);
        if (!ISVALIDSOCK(serv)) {
#ifdef _CPSOCKS_DEBUG_
                PSOCKERROR("socket() failed");
#endif // _CPSOCKS_DEBUG_
                return serv;    // = INVALID_SOCKET (-1)
        }

        if (make_dual_stack(serv)) {
#ifdef _CPSOCKS_DEBUG_
                _CPSOCKS_ERROR("Failed to make dual stack");
#endif // _CPSOCKS_DEBUG_
                return serv;    // not critical error
        }

        // Bind the socket to the provided address
        int bnd_res = bind(serv, addr->ai_addr, addr->ai_addrlen);
        if (bnd_res) {
#ifdef _CPSOCKS_DEBUG_
                PSOCKERROR("bind() failed");
#endif // _CPSOCKS_DEBUG_
                return INVALID_SOCKET;
        }

        // Start listenning
        int lsn_res = listen(serv, max_conn);
        if (lsn_res) {
#ifdef _CPSOCKS_DEBUG_
                PSOCKERROR("listen() failed");
#endif // _CPSOCKS_DEBUG_
                return INVALID_SOCKET;
        }

        return serv;
}


struct serverinfo {
        SOCKET serv;

        SOCKET clients[MAX_CONN];
        char* sdbs[MAX_CONN];
        char* rvbs[MAX_CONN];

        fd_set readfds;
        fd_set writefds;
};


int initialize_serverinfo(struct serverinfo* sinfo, const SOCKET serv)
{

}


int http_server_handle_communication(const SOCKET serv)
{
        if (!ISVALIDSOCK(serv)) {
#ifdef _CPSOCKS_DEBUG_
                _CPSOCKS_ERROR("Invalid server fd\n"
                        "\tFunction: http_server_handle_communication()");
#endif // _CPSOCKS_DEBUG_
                return EXIT_FAILURE;
        }

        fd_set master = { 0 };
        FD_ZERO(&master);
        FD_SET(serv, &master);

        SOCKET max_fd = serv;
        struct serverinfo sinfo = { 0 };
        if (initialize_serverinfo(&sinfo, serv)) {
                
        }

        while (1) {
                fd_set readfds = master;
                fd_set writefds = master;

                if (select(max_fd, &readfds, &writefds, NULL, NULL) <= 0) {
#ifdef _CPSOCKS_DEBUG_
                        PSOCKERROR("select() failed");
#endif
                        goto out_failure_serverinfo_cleanup;
                }
        }

        return EXIT_SUCCESS;

out_failure_serverinfo_cleanup:
        return EXIT_FAILURE;
}


// Starts a HTTP server on the provided port
int http_server(const char* port)
{
        sockets_startup();

        // Configure local address
        struct addrinfo* addr = configure_address(NULL,
            port, AF_INET6, SOCK_STREAM, AI_PASSIVE);
        if (!addr) {
                goto out_failure_sockets_cleanup;
        }

        // Create a socket
        SOCKET serv = start_server(addr, MAX_CONN);
        freeaddrinfo(addr);
        if (!ISVALIDSOCK(serv)) {
                goto out_failure_sockets_cleanup;
        }

        // Run the server
        int hshc_res = http_server_handle_communication(serv);
        if (hshc_res) {
#ifdef _CPSOCKS_DEBUG_
                _CPSOCKS_ERROR("Could not run the server");
#endif // _CPSOCKS_DEBUG_
                goto out_failure_sockets_cleanup;
        }

        return sockets_cleanup();

out_failure_sockets_cleanup:
        sockets_cleanup();
        return EXIT_FAILURE;
}


int main(int argc, const char* argv[])
{
        if (argc != 2) {
            printf("Usage:\n\thttp_server [PORT]\n");
            return EXIT_FAILURE;
        }

        const char* port = argv[1];
        return http_server(port);
}
