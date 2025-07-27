#include "../headers/sockshelp.h"


int allocate_max(char** dest, const int mn, const int mx)
{        
        // Allocate memory
        size_t dest_sz = (size_t) mx;
        while (dest_sz && mn <= (int) dest_sz) {
                *dest = (char*) calloc(dest_sz, sizeof(char));
                if (*dest) return (int) dest_sz; // success

                dest_sz /= 2;
        }

        return -EXIT_FAILURE;
}


int sockets_startup(void)
{
        int ret = 0;    // exit code

        // Startup Winsock
#ifdef _WIN32
        DWORD ws_ver = MAKEWORD(2, 2);  // required version: 2.2
        WSADATA wsd  = { 0 };           // Winsock data

        ret = WSAStartup(ws_ver, &wsd);
        if (ret) {
                PSOCKERROR("WSAStartup() failed");
        }
#endif // _WIN32

        return ret;
}


int sockets_cleanup(void)
{
        int ret = 0;    // exit code

        // Cleanup Winsock
#ifdef _WIN32
        ret = WSACleanup();
        if (ret) { 
                PSOCKERROR("WSACleanup() failed");
        }
#endif // _WIN32

        return ret;
}


struct addrinfo* configure_address(const char* addr,
        const char* serv,
        const int family,
        const int socktype,
        const int flags)
{
        struct addrinfo hints = { 0 };
        hints.ai_family = family;
        hints.ai_socktype = socktype;
        hints.ai_flags = flags;

        struct addrinfo* res = NULL;
        int gai_res = getaddrinfo(addr, serv, &hints, &res);
        if (gai_res) {  // to avoid a warning from GCC
                psockerror("getaddrinfo() failed");
        }

        return res;
}


int make_dual_stack(const SOCKET fd)
{
        int opt = 0;
        if (setsockopt(fd, IPPROTO_IPV6, IPV6_V6ONLY, &opt, sizeof(opt))) {
            psockerror("setsockopt() failed");
            return EXIT_FAILURE;
        }

        printf("Server socket was made dual stack\n");
        return EXIT_SUCCESS;
}


SOCKET start_server(struct addrinfo* addr, const int max_conn)
{
        // Create a socket
        SOCKET serv = socket(addr->ai_family,
            addr->ai_socktype, addr->ai_protocol);
        if (!validate_socket(serv)) {
                psockerror("socket() failed");
                return serv;    // = INVALID_SOCKET (-1)
        }

        if (make_dual_stack(serv)) {
                fprintf(stderr, "Failed to make dual stack\n");
        }

        int opt = 1;
        if (setsockopt(serv, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
                psockerror("setsockopt() failed");
        }

        // Bind the socket to the provided address
        int bnd_res = bind(serv, addr->ai_addr, addr->ai_addrlen);
        if (bnd_res) {
                psockerror("bind() failed");
                if (closesocket(serv)) {
                        psockerror("close() failed");
                }
                
                return INVALID_SOCKET;
        }

        // Start listenning
        if (addr->ai_socktype == SOCK_STREAM) {
                int lsn_res = listen(serv, max_conn);
                if (lsn_res) {
                        psockerror("listen() failed");
                        if (closesocket(serv)) {
                                psockerror("close() failed");
                        }

                        return INVALID_SOCKET;
                }
        }

        printf("Server was started\n");
        return serv;
}
