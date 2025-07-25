#include "../headers/cross_platform_sockets.h"



/*
 * CROSS-PLATFORM SETTINGS
 */



// Gets socket error
int sockerrno()
{
#ifdef _WIN32
        return WSAGetLastError();
#else   // _WIN32
        return errno;
#endif  // !_WIN32
}



/*
 * LOGGING FUNCTIONALITY
 */



// Prints the message and socket errno to "stderr"
void psockerror(const char* format, ...)
{
        va_list args = { 0 };
        va_start(args, format);

        vfprintf(stderr, format, args);
        fprintf(stderr, ": %d\n", sockerrno());

        va_end(args);
}


// Prints the message to "stderr"
// with socket errno specification
// and exits with "EXIT_FAILURE" code
void fatal_socket_error(const char* format, ...)
{
        va_list args = { 0 };
        va_start(args, format);

        vfprintf(stderr, format, args);
        fprintf(stderr, ": %d\n", sockerrno());

        va_end(args);

        exit(EXIT_FAILURE);
}


// Prints the message to "stderr"
// and exits with "EXIT_FAILURE" code
void pfatal(const char* format, ...)
{
        va_list args = { 0 };
        va_start(args, format);
        vfprintf(stderr, format, args);
        va_end(args);

        exit(EXIT_FAILURE);
}
