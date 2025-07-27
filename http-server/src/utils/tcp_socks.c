#include "../headers/tcp_socks.h"


void initialize_strinfo(struct strinfo* strinf)
{
        strinf->buf = NULL;
        strinf->sz = 0;
        strinf->len = 0;
        strinf->adv = 0;
}


void cleanup_strinfo(struct strinfo* strinf)
{
        if (strinf->buf) {
                free(strinf->buf);
        }

        initialize_strinfo(strinf);
}


void initialize_clientinfo(struct clientinfo* cinfo)
{
        cinfo->client = INVALID_SOCKET;
        cinfo->state = CS_IDLE;
        cinfo->add_data = NULL;
        initialize_strinfo(&(cinfo->sdstr));
        initialize_strinfo(&(cinfo->rvstr));
}


void cleanup_clientinfo(struct clientinfo* cinfo)
{
        // Close the fd
        if (validate_socket(cinfo->client)) {
                int cs_res = closesocket(cinfo->client);
                cinfo->client = INVALID_SOCKET;
                cinfo->state = CS_IDLE;
                cinfo->add_data = NULL;
                if (cs_res) {
                        psockerror("close() failed");
                }
        }

        // Cleanup the related strinfo structures
        cleanup_strinfo(&(cinfo->sdstr));
        cleanup_strinfo(&(cinfo->rvstr));
}


void initialize_serverinfo(struct serverinfo* sinfo, const SOCKET serv)
{
        sinfo->serv = serv;
        sinfo->max_fd = serv;
        for (size_t i = 0; i < MAX_CONN; ++i) {
                initialize_clientinfo(&(sinfo->clients[i]));
        }
}


void cleanup_serverinfo(struct serverinfo* sinfo)
{
        // Close the fd
        int ret = closesocket(sinfo->serv);
        if (ret) {
                psockerror("close() failed");
        }

        // Clean up clients
        for (size_t i = 0; i < MAX_CONN; ++i) {
                cleanup_clientinfo(&(sinfo->clients[i]));
        }

        // Initialize with "INVALID_SOCKET"
        initialize_serverinfo(sinfo, INVALID_SOCKET);
}


int server_receive_request(struct serverinfo* sinfo, struct clientinfo* cinfo,
        int (*on_disconnect)(struct serverinfo*, const SOCKET client))
{
        const SOCKET client = cinfo->client;

        // Allocate memory
        if (!cinfo->rvstr.buf) {
                int amx_res = allocate_max(&(cinfo->rvstr.buf),
                        MIN_NETBUF_LEN, MAX_NETBUF_LEN);
                if (amx_res <= 0) return -EXIT_FAILURE;
                cinfo->rvstr.sz = (size_t) amx_res;
        }

        // Check for room for '\0'
        if (cinfo->rvstr.sz < 1) return -EXIT_FAILURE;

        // Receive the request
        int recvd = recv(client, cinfo->rvstr.buf + cinfo->rvstr.len,
                cinfo->rvstr.sz - cinfo->rvstr.len - 1, 0);
        if (recvd <= 0) { // client has disconnected
                if (on_disconnect(sinfo, client)) {
                        fprintf(stderr, "on_disconnect() failed\n");
                        return -EXIT_FAILURE;
                }

                return EXIT_SUCCESS;
        }

        printf("Received %d BYTES\n", recvd);
        cinfo->rvstr.len += (size_t) recvd;
        cinfo->rvstr.buf[cinfo->rvstr.len] = '\0';

        return recvd;
}


void server_accept_client(struct serverinfo* sinfo)
{
        int cidx = find_place_for_client(sinfo);
        if (cidx < 0) {
                fprintf(stderr, "Too much clients: server_accept_client()\n");
                return;
        }

        // Accept the client
        struct sockaddr_storage caddr = { 0 };
        socklen_t caddr_len = sizeof(caddr);
        SOCKET client = accept(sinfo->serv,
                (struct sockaddr*) &caddr, &caddr_len);
        if (!validate_socket(client)) {
                psockerror("accept() failed");
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
                psockerror("getnameinfo() failed");
        }
        
        printf("Connection from %s:%s\n", addr, serv);
}


int drop_client(struct serverinfo* sinfo, const SOCKET client)
{
        // Find the client
        for (size_t i = 0; i < MAX_CONN; ++i) {
                if (sinfo->clients[i].client == client) {
                        cleanup_clientinfo(&(sinfo->clients[i]));
                        printf("Client was dropped\n");
                        return EXIT_SUCCESS;
                }
        }

        return EXIT_FAILURE; // client was not found
}


void sinfo_to_fd_set(const struct serverinfo* sinfo, fd_set* res)
{
        // Initialize, set the server fd
        FD_ZERO(res);
        FD_SET(sinfo->serv, res);

        // Set the clients
        for (size_t i = 0; i < MAX_CONN; ++i) {
                const SOCKET client = sinfo->clients[i].client;
                if (validate_socket(client)) {
                        FD_SET(client, res);
                }
        }
}


void server_handle_clients(struct serverinfo* sinfo,
        const fd_set readfds,
        const fd_set writefds,
        void (*on_read_set)(struct serverinfo* sinfo, const SOCKET client),
        void (*on_write_set)(struct serverinfo* sinfo, const SOCKET client))
{
        for (size_t i = 0; i < MAX_CONN; ++i) {
                // Check for the ability to send the response first
                if (validate_socket(sinfo->clients[i].client)) {
                        if (FD_ISSET(sinfo->clients[i].client, &writefds)) {
                                on_write_set(sinfo, sinfo->clients[i].client);
                        }
                }

                // Check if the client's fd was closed
                if (validate_socket(sinfo->clients[i].client)) {
                        if (FD_ISSET(sinfo->clients[i].client, &readfds)) {
                                on_read_set(sinfo, sinfo->clients[i].client);
                        }
                }
        }
}


void update_max_fd(struct serverinfo* sinfo)
{
        SOCKET mx = sinfo->serv;
        for (size_t i = 0; i < MAX_CONN; ++i) {
                if (mx < sinfo->clients[i].client) {
                        mx = sinfo->clients[i].client;
                }
        }

        sinfo->max_fd = mx;
}


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
                psockerror("select() failed");
                return EXIT_FAILURE;
        }

        // A new client is trying to connect
        if (FD_ISSET(sinfo->serv, &readfds)) {
                on_serv_set(sinfo);
        }

        // Handle clients
        server_handle_clients(sinfo, readfds, writefds,
                on_read_set, on_write_set);
        update_max_fd(sinfo);

        return EXIT_SUCCESS;
}


int find_place_for_client(struct serverinfo* sinfo) {
        // Check for the free place
        for (size_t i = 0; i < MAX_CONN; ++i) {
                if (!validate_socket(sinfo->clients[i].client)) {
                        return i;
                }
        }

        // No place for a new client
        return -EXIT_FAILURE;
}
