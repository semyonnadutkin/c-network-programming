#include "../headers/sockshelp.h"


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
#ifdef _CPSOCKS_DEBUG_
                PSOCKERROR("getaddrinfo() failed");
#endif
        }

        return res;
}


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
