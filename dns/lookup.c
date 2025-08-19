/*
 * Simple DNS lookup program utilizing
 * getaddrinfo(), getnameinfo() functions
 *
 * Copyright (C) 2025 Semyon Nadutkin
 */


#include "headers/cross_platform_sockets.h"
#include <stdio.h> // logging
#include <stdlib.h> // EXIT_FAILURE, EXIT_SUCCESS


// Gets all the IPv4 / IPv6 addresses
// associated with the "node"
struct addrinfo* get_addresses(const char* node)
{
	struct addrinfo hints = { 0 };
	hints.ai_flags = AI_ALL;

	struct addrinfo* addr = NULL;
	if (getaddrinfo(node, NULL, &hints, &addr)) {
		psockerror("getaddrinfo() failed");
	}

	return addr;
}


// Transforms each of the provided addresses
// into string and prints each one to "stdout"
int print_addresses(struct addrinfo* start)
{
	printf("Address:\n");

	do {
		// Transform to string
		char host[MAX_ADDRBUF_LEN];

		int gni_res = getnameinfo(start->ai_addr, start->ai_addrlen,
			host, sizeof(host), NULL, 0, NI_NUMERICHOST);
		if (gni_res) {
			psockerror("getnameinfo() failed");
			return gni_res;
		}

		// Print the host and service to "stdout"
		printf("\t%s\n", host);

		start = start->ai_next;
	} while (start->ai_next);

	return EXIT_SUCCESS;
}


// Prints all the IPv4 / IPv6 addresses
// associated with the "node"
int dns_lookup(const char* node)
{
	int sup_res = sockets_startup();
	if (sup_res) return sup_res;

	// Get the associated addresses
	struct addrinfo* addr = get_addresses(node);
	if (!addr) goto out_failure_sockets_cleanup;

	// Print the addresses to "stdout"
	int paddrs_res = print_addresses(addr);
	freeaddrinfo(addr);
	if (paddrs_res) goto out_failure_sockets_cleanup;

	return sockets_cleanup();

out_failure_sockets_cleanup:
	sockets_cleanup();
	return EXIT_FAILURE;
}


int main(const int argc, const char* argv[])
{
	if (argc != 2) { // { [executable name], [node] }
		const char* exec_name = argv[0];
		printf("Usage: %s [NODE]\n", exec_name);
		return EXIT_FAILURE;
	}

	const char* node = argv[1];
	return dns_lookup(node);
}
