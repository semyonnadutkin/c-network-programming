#include "headers/sockshelp.h"
#include "headers/http_parsers.h"


int execute_http_request(struct http_request req, char** resp)
{
        return EXIT_SUCCESS;
}


// Called when a client's fd is set for a read operation
void handle_http_input(struct serverinfo* sinfo, const SOCKET client)
{
        // TODO: split the function
        
        for (size_t i = 0; i < MAX_CONN; ++i) {
                struct clientinfo* cinfo = &(sinfo->clients[i]);
                if (cinfo->client != client) continue;

                if (!cinfo->rvstr.buf) { // allocate memory
                        int amx_res = allocate_max(&(cinfo->rvstr.buf),
                                MIN_NETBUF_LEN, MAX_NETBUF_LEN);
                        if (amx_res <= 0) return; // failed to allocate memory
                        cinfo->rvstr.sz = (size_t) amx_res;
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

                // Check if the request was fully scanned
                if (strstr(cinfo->rvstr.buf, "\r\n\r\n")) {
                        struct http_request req = { 0 };
                        int phr_res = parse_http_request(cinfo->rvstr.buf,
                                &req);


                        // Write the execution result to send string
                        int resp_sz = execute_http_request(req,
                                &(cinfo->sdstr.buf));
                        if (resp_sz <= 0) return;

                        cinfo->sdstr.len = (size_t) resp_sz;

                        // Move to a new request
                        move_http_request(&(cinfo->rvstr), req.clen);
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
                        handle_http_input, on_client_write);
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
