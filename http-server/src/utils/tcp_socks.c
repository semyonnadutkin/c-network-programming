#include "../headers/tcp_socks.h"


// Initializes struct strinfo
// Sets all parameters to zero
int initialize_strinfo(struct strinfo* strinf)
{
        if (!strinf) {
#ifdef _CPSOCKS_DEBUG_
                _CPSOCKS_ERROR("Invalid string info\n"
                        "\tFunction: initialize_strinfo()");
#endif // _CPSOCKS_DEBUG_
                return EXIT_FAILURE;       
        }

        strinf->buf = NULL;
        strinf->sz = 0;
        strinf->len = 0;
        strinf->adv = 0;

        return EXIT_SUCCESS;
}


// Frees the buffer, sets the default values
int cleanup_strinfo(struct strinfo* strinf)
{
        if (!strinf) {
#ifdef _CPSOCKS_DEBUG_
                _CPSOCKS_ERROR("Invalid string info\n"
                        "\tFunction: cleanup_strinfo()");
#endif // _CPSOCKS_DEBUG_
                return EXIT_FAILURE;       
        }

        if (strinf->buf) {
                free(strinf->buf);
        }

        return initialize_strinfo(strinf);
}


// Initializes struct clientinfo
// Sets "client" to "INVALID_SOCKET",
// initalizes the related strinfo structures
int initialize_clientinfo(struct clientinfo* cinfo)
{
        if (!cinfo) {
#ifdef _CPSOCKS_DEBUG_
                _CPSOCKS_ERROR("Invalid client info\n"
                        "\tFunction: initialize_clientinfo()");
#endif // _CPSOCKS_DEBUG_
                return EXIT_FAILURE;
        }

        int ret = EXIT_SUCCESS;

        cinfo->client = INVALID_SOCKET;
        ret |= initialize_strinfo(&(cinfo->sdstr));
        ret |= initialize_strinfo(&(cinfo->rvstr));

        return ret;
}


// Closes the fd, cleans up the related strinfo structures
int cleanup_clientinfo(struct clientinfo* cinfo)
{
        if (!cinfo) {
#ifdef _CPSOCKS_DEBUG_
                _CPSOCKS_ERROR("Invalid string info\n"
                        "\tFunction: cleanup_clientinfo()");
#endif // _CPSOCKS_DEBUG_
                return EXIT_FAILURE;       
        }

        int ret = EXIT_SUCCESS;

        // Close the fd
        if (ISVALIDSOCK(cinfo->client)) {
                ret = CLOSESOCKET(cinfo->client);
#ifdef _CPSOCKS_DEBUG_
                if (ret) {
                        PSOCKERROR("close() failed");
                }
#endif // _CPSOCKS_DEBUG_
        }

        // Cleanup the related strinfo structures
        ret |= cleanup_strinfo(&(cinfo->sdstr));
        ret |= cleanup_strinfo(&(cinfo->rvstr));

        return ret;
}


// Initializes server and max fds with "serv",
// initializes the related clientinfo structures
int initialize_serverinfo(struct serverinfo* sinfo, const SOCKET serv)
{
        if (!sinfo) {
#ifdef _CPSOCKS_DEBUG_
                _CPSOCKS_ERROR("Invalid server info\n"
                        "\tFunction: initialize_serverinfo()");
#endif // _CPSOCKS_DEBUG_
                return EXIT_FAILURE;
        }

        int ret = EXIT_SUCCESS;

        sinfo->serv = serv;
        sinfo->max_fd = serv;
        for (size_t i = 0; i < MAX_CONN; ++i) {
                ret |= initialize_clientinfo(&(sinfo->clients[i]));
        }

        return ret;
}


// Closes the server fd,
// cleans up the related clientinfo structures
int cleanup_serverinfo(struct serverinfo* sinfo)
{
        if (!sinfo) {
#ifdef _CPSOCKS_DEBUG_
                _CPSOCKS_ERROR("Invalid server info\n"
                        "\tFunction: initialize_serverinfo()");
#endif // _CPSOCKS_DEBUG_
                return EXIT_FAILURE;
        }

        // Close the fd
        int ret = CLOSESOCKET(sinfo->serv);
#ifdef _CPSOCKS_DEBUG_
        if (ret) {
                PSOCKERROR("close() failed");
        }
#endif // _CPSOCKS_DEBUG_

        // Clean up clients
        for (size_t i = 0; i < MAX_CONN; ++i) {
                ret |= cleanup_clientinfo(&(sinfo->clients[i]));
        }

        // Initialize with "INVALID_SOCKET"
        ret |= initialize_serverinfo(sinfo, INVALID_SOCKET);
        return ret;
}


// Transforms a serverinfo structure to an fd_set containing
// connected clients' fds and the server fd
int sinfo_to_fd_set(const struct serverinfo* sinfo, fd_set* res)
{        
        if (!sinfo || !res) {
#ifdef _CPSOCKS_DEBUG_
                _CPSOCKS_ERROR("Invalid pointers passed\n"
                        "\tFunction: sinfo_to_fd_set()");
#endif // _CPSOCKS_DEBUG_
                return EXIT_FAILURE;  
        }

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

        return EXIT_SUCCESS;
}


// Handles clients' connections
int server_handle_clients(struct serverinfo* sinfo,
        const fd_set readfds,
        const fd_set writefds,
        void (*on_read_set)(struct serverinfo* sinfo, const SOCKET client),
        void (*on_write_set)(struct serverinfo* sinfo, const SOCKET client))
{
        if (!sinfo || !on_read_set || !on_write_set) {
#ifdef _CPSOCKS_DEBUG_
                _CPSOCKS_ERROR("Invalid server info\n"
                        "\tFunction: server_handle_clients()");
#endif // _CPSOCKS_DEBUG_
                return EXIT_FAILURE;
        }

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

        return EXIT_SUCCESS;
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
        if (!sinfo) {
#ifdef _CPSOCKS_DEBUG_
                _CPSOCKS_ERROR("Invalid server info\n"
                        "\tFunction: server_check_fds()");
#endif // _CPSOCKS_DEBUG_
                return EXIT_FAILURE;
        }

        // Configure master fd set
        fd_set master = { 0 };
        int stfs_res = sinfo_to_fd_set(sinfo, &master);
        if (stfs_res) {
#ifdef _CPSOCKS_DEBUG_
                _CPSOCKS_ERROR("sinfo_to_fd_set() failed");
#endif // _CPSOCKS_DEBUG_
                return EXIT_FAILURE;
        }

        // Select the fds ready for a read / write operation
        fd_set readfds = master;
        fd_set writefds = master;
        int slct_res = select(sinfo->max_fd + 1,
                &readfds, &writefds, NULL, NULL);
        if (slct_res <= 0) {    // must be positive
#ifdef _CPSOCKS_DEBUG_
                PSOCKERROR("select() failed");
#endif // _CPSOCKS_DEBUG_
                return EXIT_FAILURE;
        }

        // A new client is trying to connect
        if (FD_ISSET(sinfo->serv, &readfds)) {
                on_serv_set(sinfo);
        }

        // Handle clients
        int shc_res = server_handle_clients(sinfo, readfds, writefds,
                on_read_set, on_write_set);
        if (shc_res) {
#ifdef _CPSOCKS_DEBUG_
                _CPSOCKS_ERROR("server_handle_clients() failed");
#endif // _CPSOCKS_DEBUG_
                return EXIT_FAILURE;
        }

        return EXIT_SUCCESS;
}