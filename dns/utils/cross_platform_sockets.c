#include "../headers/cross_platform_sockets.h"
#include <stdlib.h>



/*
 * CROSS-PLATFORM SETTINGS
 */



int sockerrno(void)
{
#ifdef _WIN32
        return WSAGetLastError();
#else // _WIN32
        return errno;
#endif // !_WIN32
}


int sockets_startup(void)
{
        int ret = 0; // exit code

#ifdef _WIN32
        DWORD ws_ver = MAKEWORD(2, 2); // required version: 2.2
        WSADATA wsd  = { 0 }; // WinSock data

        ret = WSAStartup(ws_ver, &wsd);
        if (ret) {
                psockerror("WSAStartup() failed");
        }
#endif // _WIN32

        return ret;
}


int sockets_cleanup(void)
{
        int ret = 0; // exit code

#ifdef _WIN32
        ret = WSACleanup();
        if (ret) { 
                psockerror("WSACleanup() failed");
        }
#endif // _WIN32

        return ret;
}



/*
 * LOGGING FUNCTIONALITY
 */



void psockerror(const char* format, ...)
{
        va_list args = { 0 };
        va_start(args, format);

        vfprintf(stderr, format, args);
        fprintf(stderr, ": %d\n", sockerrno());

        va_end(args);
}


void fatal_socket_error(const char* format, ...)
{
        va_list args = { 0 };
        va_start(args, format);

        vfprintf(stderr, format, args);
        fprintf(stderr, ": %d\n", sockerrno());

        va_end(args);

        exit(EXIT_FAILURE);
}


void pfatal(const char* format, ...)
{
        va_list args = { 0 };
        va_start(args, format);
        vfprintf(stderr, format, args);
        va_end(args);

        exit(EXIT_FAILURE);
}
