


/*
 * File: tcp_client.c
 *
 * Idea: Lewis Van Winkle
 * Implementation: Semyon Nadutkin
 *
 * Description: simple TCP client
 * that can be used for debugging
 * applications working over TCP
 */



#define _CPSOCKS_DEBUG_                     // for debug purposes
#include "headers/crossplatform_sockets.h"  // crossplatform settings


#include <stdio.h>  // to log the process flow
#include <stdlib.h> // to dynamically allocate memory
#include <string.h> // to parse strings

#ifdef _WIN32
        #include <conio.h>      // for _kbhit()
#endif  // _WIN32



/*
 * HELPER FUNCTIONS
 *
 * Can be used without socket
 * application context
 *
 * Purpose: to increase abstractions
 */



// Stores the string representation of an address
struct straddr {
        char addr[MAX_ADDRBUF_LEN];     // address
        char serv[MAX_SERVBUF_LEN];      // service
};


// Prompts user for a new address
struct straddr prompt_for_new_address(void)
{        
        struct straddr res = { 0 }; // the result address

        // Formatting for pretty-print
        const char* fmt = "-------------------";

        // Print info
        printf("\n%s\nNew address\n%s\n", fmt, fmt);

        // Scan the address
        printf("Address: ");
        scanf("%s", res.addr);

        // Scan the service
        printf("Service: ");
        scanf("%s", res.serv);

        // End with formatting
        printf("%s\n\n", fmt);

        return res;
}


/*
 * Allocates as much bytes of data as possible
 *
 * Description: tries allocating "mx" bytes,
 * "mx" / 2 bytes on fail and so on until
 * the min border is reached
 * 
 * Returns:
 *      - Failure: -1
 *      - Success: number of bytes allocated
 */
int allocate_max(char** dest, const int mx, const int mn)
{
        if (!dest) {    // invalid "dest" pointer
                _CPSOCKS_ERROR("Invalid pointer passed\n"
                               "\tFunction: allocate_max()");
                return -1;
        }

        if (mx < mn) {  // invalid bounds
                _CPSOCKS_ERROR("Invalid bounds provided\n"
                               "\tFunction: allocate_max()");
                return -1;
        }

        // Memory allocation process
        size_t dest_sz = (size_t) mx;
        while (dest_sz && ((size_t) mn) <= dest_sz) { // [ max(mn, 0), mx ]
                *dest = (char*) calloc(dest_sz, sizeof(char));
                if (*dest) break;       // successfully allocated

                dest_sz /= 2; 
        }

        return (int) dest_sz;
}



/*
 * SOCKET HELPER FUNCTIONS
 *
 * Can be used in crossplatform
 * socket applications
 *
 * Purpose: to increase abstractions
 */



// Starts socket API on Windows,
// does nothing on a Unix-based OS
int sockets_startup(void) 
{
        int ret = 0;    // exit code

        // Startup Winsock
#ifdef _WIN32
        DWORD ws_ver = MAKEWORD(2, 2);  // required version: 2.2
        WSADATA wsd  = { 0 };           // Winsock data

        ret = WSAStartup(ws_ver, &wsd);
        if (ret) {  // startup failed
                PSOCKERROR("WSAStartup() failed");
        }
#endif // _WIN32

        return ret;
}


// Cleans up socket API on Windows,
// does nothing on a Unix-based OS
int sockets_cleanup(void) 
{
        int ret = 0;    // exit code

        // Cleanup Winsock
#ifdef _WIN32
        ret = WSACleanup();
        if (ret) {  // cleanup failed
                PSOCKERROR("WSACleanup() failed");
        }
#endif // _WIN32

        return ret;
}


/*
 * Configures the remote address satisfying the provided parameters
 * 
 * Note 1: returns "NULL" on failure
 * Note 2: the return value should be freed with a call to freeaddrinfo()
 */
struct addrinfo* configure_address(const char* addr,
                                   const char* serv,
                                   const int   family,
                                   const int   socktype,
                                   const int   flags)
{
        struct addrinfo hints = { 0 };
        hints.ai_family = family;
        hints.ai_socktype = socktype;
        hints.ai_flags = flags;

        struct addrinfo* res = NULL;
        int gai_res = getaddrinfo(addr, serv, &hints, &res);
        if (gai_res) {
                PSOCKERROR("getaddrinfo() failed");
        }

        return res;
}


// Starts a TCP / UDP client, connects to the remote peer
SOCKET start_client(const char* addr,
                    const char* serv,
                    const int   family,
                    const int   socktype,
                    const int   flags)
{
        printf("Configuring the remote address...\n");
        struct addrinfo* remote_addr = configure_address(addr, serv, family,
                                                         socktype, flags);
        if (!remote_addr) {     // configuration failed
                _CPSOCKS_ERROR("Failed to configure remote address");
                return INVALID_SOCKET;
        }

        printf("Creating a socket...\n");
        SOCKET remote = socket(remote_addr->ai_family,
                         remote_addr->ai_socktype, remote_addr->ai_protocol);
        if (!ISVALIDSOCK(remote)) {     // socket creation failed
                PSOCKERROR("socket() failed");
                return remote;  // set to an invalid value by socket()
        }

        printf("Connecting to the remote peer...\n");
        int conn_res = connect(remote,
                         remote_addr->ai_addr, remote_addr->ai_addrlen);
        freeaddrinfo(remote_addr);      // no more needed
        if (conn_res) {         // connection failed
                PSOCKERROR("connect() failed");
                return INVALID_SOCKET;
        }

        printf("Client was started\n"); // OK
        return remote;
}


/*
 * Ready-for-operation fds selection result
 *
 * Purpose: to split the selection logic
 * and a communication handler function
 */
struct client_select_result {
        int ret;         // value returned by a function
        fd_set readfds;  // result read fds set
        fd_set writefds; // result write fds set
};


// Selects fds ready for an operation
struct client_select_result client_select_fds(const SOCKET remote,
                                              struct timeval* timeout)
{
        struct client_select_result res = { 0 };

        // Configure the fd sets
        fd_set readfds = { 0 };
        FD_ZERO(&readfds);
        FD_SET(remote, &readfds); // input from the peer
        FD_SET(fileno(stdin), &readfds); // input from console (Unix-based)

        fd_set writefds = { 0 };
        FD_ZERO(&writefds);
        FD_SET(remote, &writefds);  // peer is ready for input

        // Select the fds ready for an operation
        int slct_res = select(remote + 1,
                              &readfds, &writefds, NULL, timeout);
        if (slct_res < 0) {
                perror("select() failed");
                res.ret = slct_res;
        }

        // Set the fields
        res.readfds = readfds;
        res.writefds = writefds;

        return res;
}


/*
 * Stores a network client info
 *
 * Purpose: not to pass many buffers
 * and their sizes between functions
 */
struct net_client_info {
        SOCKET remote_peer;

        char* send_buf; // stores a client request
        char* recv_buf; // stores a message from a remote peer

        size_t sdblen;  // send buffer message length
        size_t sdbsent; // bytes sent to the remote peer

        size_t sdbsz;   // size of the "send_buf"
        size_t rvbsz;   // size of the "recv_buf"
};


/* 
 * Checks for input from console and remote peer
 * and for an ability to send a request to the remote peer
 *
 * Description:
 * Selects the fds ready for an operation using "select()""
 * After the call to "select()"":
 *      remote peer fd is set for read: call "on_peer_read()"
 *      remote peer fd is set for write: call "on_peer_write()"
 *      input from console: call "on_console_input"
 *
 * Return: 0 on success, non-zero value on failure
 */
int client_check_fds(struct net_client_info* cinfo,
                     struct timeval* timeout,
                     int (*on_peer_read)(struct net_client_info* cinfo),
                     int (*on_peer_write)(struct net_client_info* cinfo),
                     int (*on_console_input)(struct net_client_info* cinfo))
{
        if (!cinfo || !timeout
            || !on_peer_read || !on_peer_write || !on_console_input) {
                _CPSOCKS_ERROR("NULL pointer(s) passed: client_check_fds()");
                return EXIT_FAILURE;
        }

        // Select the fds ready for an operation
        struct client_select_result res
                = client_select_fds(cinfo->remote_peer, timeout);
        if (res.ret) {
                _CPSOCKS_ERROR("Error from client_select_fds()");
                return res.ret;
        }

        // Check for input from the remote peer
        if (FD_ISSET(cinfo->remote_peer, &(res.readfds))) {
                int opr_res = on_peer_read(cinfo);
                if (opr_res) {
                        _CPSOCKS_ERROR("Error from on_peer_read()");
                        return opr_res;
                }
        }

        // Check for an ability to write to the remote peer
        if (FD_ISSET(cinfo->remote_peer, &(res.writefds))) {
                int opw_res = on_peer_write(cinfo);
                if (opw_res) {
                        _CPSOCKS_ERROR("Error from on_peer_write()");
                        return opw_res;
                }
        }
        
        // Check for input from console
#ifdef _WIN32
        if (_kbhit()) {
#else
        if (FD_ISSET(fileno(stdin), &(res.readfds))) {
#endif
                int oci_res = on_console_input(cinfo);
                if (oci_res) {
                        _CPSOCKS_ERROR("Error from on_console_input()");
                        return oci_res;
                }
        }

        return EXIT_SUCCESS;
}


// Receives a message from the remote peer
int client_recv(struct net_client_info* cinfo) {
        if (!cinfo || !cinfo->recv_buf) {
                _CPSOCKS_ERROR("NULL pointer passed: client_recv()");
                return EXIT_FAILURE;
        }

        int recvd = recv(cinfo->remote_peer, cinfo->recv_buf, cinfo->rvbsz, 0);
        if (recvd <= 0) {
                _CPSOCKS_ERROR("Remote peer has disconnected");
                return EXIT_FAILURE;
        }

        printf("Received a message from the remote peer\n");
        printf("Content:\n%.*s\n", recvd, cinfo->recv_buf);

        return EXIT_SUCCESS;
}


// Sends a message to the remote peer
int client_send(struct net_client_info* cinfo) {
        if (!cinfo || !cinfo->send_buf) {
                _CPSOCKS_ERROR("NULL pointer passed: client_send()");
                return EXIT_FAILURE;
        }

        if (cinfo->sdblen == 0) { // nothing to send
                return EXIT_SUCCESS;
        }

        int sent = send(cinfo->remote_peer, cinfo->send_buf + cinfo->sdbsent,
                        cinfo->sdblen - cinfo->sdbsent, 0);
        if (sent <= 0) {
                PSOCKERROR("send() failed");
                return EXIT_FAILURE;
        }

        printf("Sent %d bytes\n", sent);

        cinfo->sdbsent += (size_t) sent;
        if (cinfo->sdbsent == cinfo->sdblen) {  // fully sent the request
                cinfo->sdbsent = (size_t) 0;
                cinfo->sdblen = (size_t) 0;
        }

        return EXIT_SUCCESS;
}


/*
 * Parses a "--from-file" command
 * Structure: "$\\ --from-file [path] $\\"
 *
 * Return:
 *      - Null-terminated string on success
 *      - NULL on failure
 */
char* parse_from_file_command(const char* buf)
{
        char* start = strstr(buf, "$\\ --from-file ");
        if (!start) return NULL;

        start += strlen("$\\ --from-file ");
        char* end = strstr(start, " $\\");
        if (!end) return NULL;

        const size_t reslen = end - start;
        char* res = (char*) malloc(reslen + 1); // room for '\0'
        if (!res) {
                perror("malloc() failed");
                return NULL;
        }

        // Copy the content
        for (size_t i = 0; i < reslen; ++i) {
                res[i] = start[i];
        }

        res[reslen] = '\0';
        return res;
}


// Executes the "--from-file" command
// and writes the result to the end of send buffer
int scan_request_from_file(const char* path, struct net_client_info* cinfo)
{
        FILE* f = fopen(path, "r");
        if (!f) {
                perror("fopen() failed");
                return EXIT_FAILURE;
        }

        fscanf(f, "%s", cinfo->send_buf + cinfo->sdblen);

        int fc_res = fclose(f);
        if (fc_res) {
                perror("fclose() failed");
                return fc_res;
        }

        return EXIT_FAILURE;
}


// Gets input from console and writes it to the end of send buffer
int client_scanf(struct net_client_info* cinfo) {
        if (!cinfo || !cinfo->send_buf) {
                _CPSOCKS_ERROR("NULL pointer passed: client_scanf()");
                return EXIT_FAILURE;
        }

        // Get the input
        if (!fgets(cinfo->send_buf + cinfo->sdblen,
                   cinfo->sdbsz - cinfo->sdblen, stdin)) {
                perror("fgets() failed");
                return EXIT_FAILURE;
        }

        printf("Scanned: %s\n", cinfo->send_buf);
        cinfo->sdblen += strlen(cinfo->send_buf + cinfo->sdblen);

        // Check input for containing the "--from-file" command
        const char* path = parse_from_file_command(cinfo->send_buf);
        
        if (path) {
                int srff_res = scan_request_from_file(path, cinfo);
                if (srff_res) {
                        _CPSOCKS_ERROR("--from-file command was not executed");
                        return srff_res;
                }
        }

        return EXIT_SUCCESS;
}


// Handles communication between client and the remote peer
void client_handle_communication(const SOCKET remote)
{
        // Configure client info
        struct net_client_info cinfo = { 0 };
        cinfo.remote_peer = remote;

        // Allocate memory for buffers
        int sdbsz = allocate_max(&(cinfo.send_buf),
                MAX_NETBUF_LEN, MIN_NETBUF_LEN);
        int rvbsz = allocate_max(&(cinfo.recv_buf),
                MAX_NETBUF_LEN, MIN_NETBUF_LEN);
        if (sdbsz < 0 || rvbsz < 0) {
                _CPSOCKS_ERROR("Not enough memory\n"
                               "Function: client_handle_communication()");
                goto out_send_recv_buffs_cleanup;
        }

        cinfo.sdbsz = (size_t) sdbsz;
        cinfo.rvbsz = (size_t) rvbsz;

        // Start the communication
        while (1) {
                struct timeval timeout = { 0 };
                // To check for console input on Windows
                timeout.tv_usec = 100000;

                // Check the fds
                int ccf_res = client_check_fds(&cinfo, &timeout,
                                               client_recv,
                                               client_send,
                                               client_scanf);
                if (ccf_res) {
                        _CPSOCKS_ERROR("Error from client_check_fds()");
                        break;
                }
        }

        // Cleans up the buffers and exits the function
        out_send_recv_buffs_cleanup:
                if (cinfo.send_buf) free(cinfo.send_buf);
                if (cinfo.recv_buf) free(cinfo.recv_buf);
}


int tcp_client(void)
{
        sockets_startup();  // start the used socket API

        // Scanning the remote address
        struct straddr remote_addr = prompt_for_new_address();
        printf("Scanned: %s:%s\n", remote_addr.addr, remote_addr.serv);

        // Starting the client
        SOCKET remote = start_client(remote_addr.addr, remote_addr.serv,
                                     AF_INET, SOCK_STREAM, 0);
        
        // Starting the communication
        client_handle_communication(remote);

        return sockets_cleanup();   // clean up the socket API
}


int main(void)
{
        return tcp_client();
}
