#define _CPSOCKS_DEBUG_ // to make debug logs
#include "headers/crossplatform_sockets.h"
#include "headers/sockshelp.h"
#include "headers/tcp_socks.h"


void on_client_connect(struct serverinfo* sinfo)
{

}


void on_client_read(struct serverinfo* sinfo, const SOCKET client)
{
        
}


void on_client_write(struct serverinfo* sinfo, const SOCKET client)
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

        struct serverinfo sinfo = { 0 };
        if (initialize_serverinfo(&sinfo, serv)) {
#ifdef _CPSOCKS_DEBUG_
                _CPSOCKS_ERROR("initialize_serverinfo() failed");
#endif // _CPSOCKS_DEBUG_
                return EXIT_FAILURE;
        }

        // Start accepting connections
        while (1) {
                int scfds_res = server_check_fds(&sinfo, on_client_connect,
                        on_client_read, on_client_write);
                if (scfds_res) {
#ifdef _CPSOCKS_DEBUG_
                        _CPSOCKS_ERROR("server_check_fds() failed");
#endif // _CPSOCKS_DEBUG_
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
#ifdef _CPSOCKS_DEBUG_
            _CPSOCKS_ERROR("Usage:\n\thttp_server [PORT]");
#endif
            return EXIT_FAILURE;
        }

        const char* port = argv[1];
        return http_server(port);
}
