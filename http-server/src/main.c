#include <string.h> // to parse HTTP requests
#include "headers/crossplatform_sockets.h"
#include "headers/sockshelp.h"
#include "headers/tcp_socks.h"


/*
 * HTTP request
 *
 * @method  GET / POST / ...
 * @url     Route
 * @conn    Connection
 * @ctype   Content-Type
 * @content Content
 * @clen    Content-Length
 */
struct http_request {
        char* method;
        char* url;
        char* conn;
        char* ctype;
        char* content;
        size_t clen;
};


struct http_request parse_http_request()
{
        struct http_request req = { 0 };

        return req;
}


// Finds a place for a new client
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


// Accepts a new client, initializes the free cell
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


// Disconnects the given client and clears the related clientinfo structure
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


// Called when a client's fd is set for a read operation
void scan_http_request(struct serverinfo* sinfo, const SOCKET client)
{
        for (size_t i = 0; i < MAX_CONN; ++i) {
                struct clientinfo* cinfo = &(sinfo->clients[i]);
                if (cinfo->client != client) continue;

                if (!cinfo->rvstr.buf) { // allocate memory
                        int amx_res = allocate_max(&(cinfo->rvstr.buf),
                                MIN_NETBUF_LEN, MAX_NETBUF_LEN);
                        if (amx_res) return;
                }

                if (cinfo->rvstr.sz < 1) return; // check for room for '\0'

                int recvd = recv(client, cinfo->rvstr.buf + cinfo->rvstr.len,
                        cinfo->rvstr.sz - cinfo->rvstr.len - 1, 0);
                if (recvd <= 0) { // client has disconnected
                        drop_client(sinfo, client);
                        return;
                }

                cinfo->rvstr.len += (size_t) recvd;
                cinfo->rvstr.buf[cinfo->rvstr.len] = '\0'; // for strstr()
                if (strstr(cinfo->rvstr.buf, "\r\n\r\n")) {
                        
                }
        }

        _CPSOCKS_ERROR("Client was not found\n"
                "\tFunction: on_client_read()");
}


// Called when a client's fd is set for a write operation
void on_client_write(struct serverinfo* sinfo, const SOCKET client)
{

}


int http_server_handle_communication(const SOCKET serv)
{
        struct serverinfo sinfo = { 0 };
        initialize_serverinfo(&sinfo, serv);

        // Start accepting connections
        while (1) {
                int scfds_res = server_check_fds(&sinfo, server_accept_client,
                        scan_http_request, on_client_write);
                if (scfds_res) {
                        _CPSOCKS_ERROR("server_check_fds() failed");
                        goto out_failure_cleanup_serverinfo;
                }
        }

        cleanup_serverinfo(&sinfo);
        return EXIT_SUCCESS;

out_failure_cleanup_serverinfo:
        cleanup_serverinfo(&sinfo);
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
                _CPSOCKS_ERROR("Could not run the server");
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
            _CPSOCKS_FATALERROR("Usage:\n\thttp_server [PORT]");
        }

        const char* port = argv[1];
        return http_server(port);
}
