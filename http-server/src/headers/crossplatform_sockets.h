#pragma once



/*
 * File: crossplatform_sockets.h
 *
 * Idea: Lewis Van Winkle
 * Implementation: Semyon Nadutkin
 *
 * Description: functionality to
 * abstract from Berkeley sockets and Winsock
 * obtained using defines and typedefs
 */



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

        // Defines for compatibility with Berkeley sockets
        #define CLOSESOCKET(s) closesocket(s)           // closes the socket
        #define ISVALIDSOCK(s) ((s) != INVALID_SOCKET)  // checks the socket
        #define GETSOCKERRNO() (WSAGetLastError())      // gets socket error

#else // _WIN32
        #include <sys/types.h>  // size_t, socklen_t, ...
        #include <sys/socket.h> // socket(), connect(), ...
        #include <arpa/inet.h>  // inet_ntoa(), inet_pron(), ...
        #include <netinet/in.h> // htons(), ntohs(), ...
        #include <netdb.h>      // getnameinfo(), ...
        #include <unistd.h>     // read(), write(), close(), ...
        #include <errno.h>      // errno

        // Defines for compatibility with WinSock2
        #define INVALID_SOCKET -1
        #define CLOSESOCKET(s) close(s)         // closes the socket
        #define ISVALIDSOCK(s) ((s) >= 0)       // checks the socket
        #define GETSOCKERRNO() (errno)          // gets last error

        // Not to use "int" on Windows
        typedef int SOCKET;

#endif // !_WIN32



// Debug defines
#ifdef _CPSOCKS_DEBUG_
        #include <stdio.h>      // to print logs to console
        #include <stdlib.h>     // to exit with "EXIT_FAILURE"

        // Prints the message and the last socket error to "stderr"
        #define PSOCKERROR(msg)                 \
                fprintf(stderr, "%s: %d\n", msg, GETSOCKERRNO())

        // Prints the message and the last socket error to "stderr"
        // and terminates the program (exit code: "EXIT_FAILURE")
        #define FATALSOCKERROR(msg)             \
                do {                            \
                        PSOCKERROR(msg);        \
                        exit(EXIT_FAILURE);     \
                } while (0)

        // Prints the error message "stderr"
        #define _CPSOCKS_ERROR(msg)             \
                fprintf(stderr, "%s\n", msg)

        // Prints the error message to "stderr"
        // and terminates the program (exit code: "EXIT_FAILURE")
        #define _CPSOCKS_FATALERROR(msg)        \
                do {                            \
                        fprintf(stderr, msg);   \
                        exit(EXIT_FAILURE);     \
                } while (0)

#endif // _CPSOCKS_DEBUG_
