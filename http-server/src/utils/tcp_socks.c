/*
 * File: tcp_socks.c
 * Author: Semyon Nadutkin
 * Copyright (C) 2025 Semyon Nadutkin
 */


#include "../headers/tcp_socks.h"


// Initializes struct strinfo
// Sets all parameters to zero
void initialize_strinfo(struct strinfo* strinf)
{
        strinf->buf = NULL;
        strinf->sz = 0;
        strinf->len = 0;
        strinf->adv = 0;
}


// Frees the buffer, sets the default values
void cleanup_strinfo(struct strinfo* strinf)
{
        if (strinf->buf) {
                free(strinf->buf);
        }

        initialize_strinfo(strinf);
}


// Initializes struct clientinfo
// Sets "client" to "INVALID_SOCKET",
// initalizes the related strinfo structures
void initialize_clientinfo(struct clientinfo* cinfo)
{
        cinfo->client = INVALID_SOCKET;
        initialize_strinfo(&(cinfo->sdstr));
        initialize_strinfo(&(cinfo->rvstr));
}


// Closes the fd, cleans up the related strinfo structures
void cleanup_clientinfo(struct clientinfo* cinfo)
{
        // Close the fd
        if (ISVALIDSOCK(cinfo->client)) {
                int cs_res = CLOSESOCKET(cinfo->client);
                if (cs_res) {
                        PSOCKERROR("close() failed");
                }
        }

        // Cleanup the related strinfo structures
        cleanup_strinfo(&(cinfo->sdstr));
        cleanup_strinfo(&(cinfo->rvstr));
}


// Initializes server and max fds with "serv",
// initializes the related clientinfo structures
void initialize_serverinfo(struct serverinfo* sinfo, const SOCKET serv)
{
        sinfo->serv = serv;
        sinfo->max_fd = serv;
        for (size_t i = 0; i < MAX_CONN; ++i) {
                initialize_clientinfo(&(sinfo->clients[i]));
        }
}


// Closes the server fd,
// cleans up the related clientinfo structures
void cleanup_serverinfo(struct serverinfo* sinfo)
{
        // Close the fd
        int ret = CLOSESOCKET(sinfo->serv);
        if (ret) {
                PSOCKERROR("close() failed");
        }

        // Clean up clients
        for (size_t i = 0; i < MAX_CONN; ++i) {
                cleanup_clientinfo(&(sinfo->clients[i]));
        }

        // Initialize with "INVALID_SOCKET"
        initialize_serverinfo(sinfo, INVALID_SOCKET);
}


// Transforms a serverinfo structure to an fd_set containing
// connected clients' fds and the server fd
void sinfo_to_fd_set(const struct serverinfo* sinfo, fd_set* res)
{
        // Initialize, set the server fd
        FD_ZERO(res);
        FD_SET(sinfo->serv, res);

        // Set the clients
        for (size_t i = 0; i < MAX_CONN; ++i) {
                const SOCKET client = sinfo->clients[i].client;
                if (ISVALIDSOCK(client)) {
                        FD_SET(client, res);
                }
        }
}


// Handles clients' connections
void server_handle_clients(struct serverinfo* sinfo,
        const fd_set readfds,
        const fd_set writefds,
        void (*on_read_set)(struct serverinfo* sinfo, const SOCKET client),
        void (*on_write_set)(struct serverinfo* sinfo, const SOCKET client))
{
        for (size_t i = 0; i < MAX_CONN; ++i) {
                // Check for the ability to send the response first
                if (ISVALIDSOCK(sinfo->clients[i].client)) {
                        if (FD_ISSET(sinfo->clients[i].client, &writefds)) {
                                on_write_set(sinfo, sinfo->clients[i].client);
                        }
                }

                // Check if the client's fd was closed
                if (ISVALIDSOCK(sinfo->clients[i].client)) {
                        if (FD_ISSET(sinfo->clients[i].client, &readfds)) {
                                on_read_set(sinfo, sinfo->clients[i].client);
                        }
                }
        }
}


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
        void (*on_write_set)(struct serverinfo* sinfo, const SOCKET client))
{
        // Configure master fd set
        fd_set master = { 0 };
        sinfo_to_fd_set(sinfo, &master);

        // Select the fds ready for a read / write operation
        fd_set readfds = master;
        fd_set writefds = master;

        int slct_res = select(sinfo->max_fd + 1,
                &readfds, &writefds, NULL, NULL);
        if (slct_res <= 0) {    // must be positive
                PSOCKERROR("select() failed");
                return EXIT_FAILURE;
        }

        // A new client is trying to connect
        if (FD_ISSET(sinfo->serv, &readfds)) {
                on_serv_set(sinfo);
        }

        // Handle clients
        server_handle_clients(sinfo, readfds, writefds,
                on_read_set, on_write_set);

        return EXIT_SUCCESS;
}


int find_place_for_client(struct serverinfo* sinfo) {
        // Check for the free place
        for (size_t i = 0; i < MAX_CONN; ++i) {
                if (!ISVALIDSOCK(sinfo->clients[i].client)) {
                        return i;
                }
        }

        // No place for a new client
        return -EXIT_FAILURE;
}


void server_accept_client(struct serverinfo* sinfo)
{
        int cidx = find_place_for_client(sinfo);
        if (cidx < 0) {
                _CPSOCKS_ERROR("Too much clients\n"
                        "\tFunction: server_accept_client()");
                return;
        }

        // Accept the client
        struct sockaddr_storage caddr = { 0 };
        socklen_t caddr_len = sizeof(caddr);
        SOCKET client = accept(sinfo->serv,
                (struct sockaddr*) &caddr, &caddr_len);
        if (!ISVALIDSOCK(client)) {
                PSOCKERROR("accept() failed");
                return;
        }

        // Initialize the client's cell
        sinfo->clients[cidx].client = client;

        // Get the string representation
        char addr[MAX_ADDRBUF_LEN];
        char serv[MAX_SERVBUF_LEN];
        int gni_res = getnameinfo((struct sockaddr*) &caddr, caddr_len,
                addr, sizeof(addr), serv, sizeof(serv),
                NI_NUMERICHOST | NI_NUMERICHOST);
        if (gni_res) {
                PSOCKERROR("getnameinfo() failed");
        }
        
        fprintf(stdout, "Connection from %s:%s\n", addr, serv);
}


int drop_client(struct serverinfo* sinfo, const SOCKET client)
{
        // Find the client
        for (size_t i = 0; i < MAX_CONN; ++i) {
                if (sinfo->clients[i].client == client) {
                        cleanup_clientinfo(&(sinfo->clients[i]));
                }
        }

        return EXIT_FAILURE; // client was not found
}
