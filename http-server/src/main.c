#include "headers/cross_platform_sockets.h"
#include "headers/http_parsers.h"
#include "headers/tcp_socks.h"
#include "headers/http_writers.h"

#include <stdio.h>      // fprintf(), ...
#include <stdlib.h>     // EXIT_SUCCESS / EXIT_FAILURE


/*
 * Executes a HTTP request, writes the response
 *
 * Returns:
 *      - Success: HTTP status code
 *      - Failure: -EXIT_FAILURE
 */
enum http_code execute_http_request(struct http_request req,
        struct strinfo* respstr)
{
        /* TODO: Implement the function */
        
        int whr_res = write_http_response(respstr, "HTTP/1.1 200 OK",
                "text/plain", "Hello World!",
                1, "Connection: close");
        if (whr_res) return -EXIT_FAILURE;

        return HTTP_OK;
}


/*
 * Parses HTTP request, executes the request,
 * and writes the response to the related send buffer
 * 
 * Returns:
 *      - Success: written HTTP status code
 *      - Failure: -EXIT_FAILURE
 */
int process_http_request(struct clientinfo* cinfo)
{
        // Parse the request
        struct http_request req = { 0 };
        int parse_res = parse_http_request(cinfo->rvstr.buf, &req);
        if (parse_res != HTTP_OK) {
                if (parse_res == HTTP_INTERNAL_SERVER_ERROR) {
                        fprintf(stderr, "parse_http_request() failed\n");
                        return -EXIT_FAILURE;
                }

                /* TODO: Write the error message */

                cinfo->state = CS_IDLE;
                return parse_res;
        }

        // Write the execution result to send string
        int exec_res = execute_http_request(req, &(cinfo->sdstr));
        if (exec_res < 0) {
                fprintf(stderr, "Failed to execute HTTP request\n");
                return exec_res;
        }

        // Move to a new request
        move_http_request(&(cinfo->rvstr), req.clen);
        cinfo->state = CS_IDLE;

        return exec_res;
}


// Called when a client's fd is set for a read operation
void handle_http_input(struct serverinfo* sinfo, const SOCKET client)
{
        for (size_t i = 0; i < MAX_CONN; ++i) {
                struct clientinfo* cinfo = &(sinfo->clients[i]);
                if (cinfo->client != client) continue;

                // Set the state to "RECEIVING"
                if (cinfo->state == CS_IDLE) {
                        cinfo->state = CS_RECEIVING;
                } else {
                        return; // wait for another operation
                }

                int rr_res = server_receive_request(sinfo, cinfo, drop_client);
                if (rr_res < 0) { // bug
                        pfatal("Invalid data: receive_request()");
                } else if (rr_res == 0) { // received disconnect
                        return;
                }

                // Check if the request was fully scanned
                int req_status = http_request_read_status(cinfo->rvstr.buf);
                if (req_status == HTTP_OK) {
                        printf("\nReceived request (%zu bytes)\n%s\n",
                                cinfo->rvstr.len, cinfo->rvstr.buf);

                        if (process_http_request(cinfo) < 0) { // error
                                fprintf(stderr, "process_request() failed\n");
                        }
                } else if (req_status == HTTP_BAD_REQUEST) {
                        // TODO: write 400

                        cinfo->state = CS_IDLE;
                }

                return;
        }

        fprintf(stderr, "Client was not found: handle_http_input()");
}


// Called when a client's fd is set for a write operation
void send_http_response(struct serverinfo* sinfo, const SOCKET client)
{
        for (size_t i = 0; i < MAX_CONN; ++i) {
                if (sinfo->clients[i].client != client) continue;

                struct clientinfo* cinfo = &(sinfo->clients[i]);
                if (!cinfo->sdstr.buf) return; // nothing to send

                // Set the state to "SENDING"
                if (cinfo->state == CS_IDLE) {
                        cinfo->state = CS_SENDING;
                } else {
                        return; // wait for another operation
                }

                struct strinfo* sdstr = &cinfo->sdstr;
                int sent = send(client, sdstr->buf + sdstr->adv,
                        sdstr->len - sdstr->adv, 0);
                if (sent <= 0) {
                        psockerror("send() failed");
                }

                sdstr->adv += sent; // move the cursor
                if (sdstr->adv == sdstr->len) { // fully sent
                        printf("\nSent response (%zu bytes)\n%s\n",
                                sdstr->len, sdstr->buf);
                        cleanup_strinfo(sdstr); // -> optional <- cleanup
                        cinfo->state = CS_IDLE;
                }

                return;
        }

        fprintf(stderr, "Client was not found: send_http_response()\n");
}


int http_server_handle_communication(const SOCKET serv)
{
        struct serverinfo sinfo = { 0 };
        initialize_serverinfo(&sinfo, serv);

        // Start accepting connections
        while (1) {
                int scfds_res = server_check_fds(&sinfo, server_accept_client,
                        handle_http_input, send_http_response);
                if (scfds_res) {
                        fprintf(stderr, "server_check_fds() failed");
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
        if (!validate_socket(serv)) {
                goto out_failure_sockets_cleanup;
        }

        // Run the server
        int hshc_res = http_server_handle_communication(serv);
        if (hshc_res) {
                fprintf(stderr, "server_handle_communication() failed");
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
            pfatal("Usage:\n\thttp_server [PORT]");
        }

        setvbuf(stdout, NULL, _IONBF, 0);

        const char* port = argv[1];
        return http_server(port);
}
