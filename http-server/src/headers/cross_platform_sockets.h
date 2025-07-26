/*
 * File: cross_platform_sockets.h
 *
 * Description: functionality to
 * abstract from Berkeley sockets and Winsock
 * obtained using defines and typedefs
 * 
 * Created by Semyon Nadutkin, 2025
 */


#pragma once



/*
 * USEFULL DEFINES FOR NUMERIC VALUES
 * Purpose: make the logic behind
 * the buffer allocation clearer
 */



#define MAX_ADDRBUF_LEN 129     // max buffer length for an address string
#define MAX_SERVBUF_LEN 33      // max buffer length for a service string

#define MAX_NETBUF_LEN  16384   // max buffer length for send() / recv()
#define MIN_NETBUF_LEN  1025    // min buffer length for send() / recv()

#define MAX_CONN        64      // max connections for listen()



/*
 * CROSSPLATFORM SETTINGS
 */



#ifdef _WIN32
        #include <winsock2.h>                   // basic socket functionality
        #include <ws2tcpip>                     // TCP/IP functionality
        #pragma comment(lib, "ws2_32.lib")      // linking the WinSock2 library

#else  // _WIN32
        #include <sys/types.h>  // size_t, socklen_t, ...
        #include <sys/socket.h> // socket(), connect(), ...
        #include <arpa/inet.h>  // inet_ntoa(), inet_pron(), ...
        #include <netinet/in.h> // htons(), ntohs(), ...
        #include <netdb.h>      // getnameinfo(), ...
        #include <unistd.h>     // read(), write(), close(), ...
        #include <errno.h>      // errno

        // Compatibility with WinSock2
        typedef int SOCKET;
        #define INVALID_SOCKET -1
        #define closesocket(s) close(s)
#endif // !_WIN32


// Gets socket error
int sockerrno();


// Checks if the socket is valid
static inline
int validate_socket(const SOCKET s)
{
#ifdef _WIN32
        return s != INVALID_SOCKET;
#else   // _WIN32
        return s >= 0;
#endif  // !_WIN32
}



/*
 * LOGGING FUNCTIONALITY
 */



#include <stdio.h>  // fprintf(), printf(), ...
#include <stdlib.h> // exit()
#include <stdarg.h> // va_arg(), va_list(), ...


// Prints the message and socket errno to "stderr"
void psockerror(const char* format, ...);


// Prints the message to "stderr"
// with socket errno specification
// and exits with "EXIT_FAILURE" code
void fatal_socket_error(const char* format, ...);


// Prints the message to "stderr"
// and exits with "EXIT_FAILURE" code
void pfatal(const char* format, ...);
