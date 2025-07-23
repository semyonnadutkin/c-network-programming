#include "headers/crossplatform_sockets.h"
#include "headers/sockshelp.h"
#include "headers/http_parsers.h"
#include "headers/tcp_socks.h"
#include <stdlib.h>


/*
 * Executes a HTTP request 
 *
 * Returns:
 *      - Success: HTTP status code
 *      - Failure: -EXIT_FAILURE
 */
enum http_code execute_http_request(struct http_request req,
        struct strinfo* respstr)
{

        return EXIT_SUCCESS;
}


/*
 * Appends a request from a TCP client to the existing buffer
 * 
 * Returns:
 *      - Success: 0
 *      - No memory: -EXIT_FAILURE
 *      - Invalid arguments: EXIT_FAILURE
 */
int receive_request(struct serverinfo* sinfo, struct clientinfo* cinfo,
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
        if (cinfo->rvstr.sz < 1) return EXIT_FAILURE;

        // Receive the request
        int recvd = recv(client, cinfo->rvstr.buf + cinfo->rvstr.len,
                cinfo->rvstr.sz - cinfo->rvstr.len - 1, 0);
        if (recvd <= 0) { // client has disconnected
                if (drop_client(sinfo, client)) {
                        _CPSOCKS_ERROR("Failed to drop client");
                        return EXIT_FAILURE;
                }

                _CPSOCKS_LOG("Received disconnect\n");
        }

        cinfo->rvstr.len += (size_t) recvd;
        cinfo->rvstr.buf[cinfo->rvstr.len] = '\0';

        return EXIT_SUCCESS;
}


/*
 * Parses and executes HTTP request
 * 
 * Returns:
 *      - Success: written HTTP status code
 *      - Failure: -EXIT_SUCCESS
 */
int process_request(struct clientinfo* cinfo)
{
        // Parse the request
        struct http_request req = { 0 };
        int parse_res = parse_http_request(cinfo->rvstr.buf, &req);
        if (parse_res != HTTP_OK) return parse_res; 

        // Write the execution result to send string
        int exec_res = execute_http_request(req, &(cinfo->sdstr));
        if (exec_res < 0) {
                _CPSOCKS_ERROR("Failed to execute HTTP request");
                return exec_res;
        }

        // Move to a new request
        move_http_request(&(cinfo->rvstr), req.clen);

        return exec_res;
}


// Called when a client's fd is set for a read operation
void handle_http_input(struct serverinfo* sinfo, const SOCKET client)
{
        // TODO: split the function
        
        for (size_t i = 0; i < MAX_CONN; ++i) {
                struct clientinfo* cinfo = &(sinfo->clients[i]);
                if (cinfo->client != client) continue;

                int rr_res = receive_request(sinfo, cinfo, drop_client);
                if (rr_res < 0) { // bug
                        _CPSOCKS_FATALERROR("Invalid client / server data");
                }

                // Check if the request was fully scanned
                int req_status = http_request_read_status(cinfo->rvstr.buf);
                if (req_status == HTTP_OK) {
                        process_request(cinfo);
                } else if (req_status == HTTP_BAD_REQUEST) {
                        // TODO: write 400
                }
        }

        _CPSOCKS_ERROR("Client was not found\n"
                "\tFunction: handle_http_input()");
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
