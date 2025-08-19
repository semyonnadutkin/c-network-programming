/*
 * DNS client
 * Abondoned due to the complexity
 */


#include <stdint.h>
#include <stdlib.h>


#define DNS_MAX_NAME_LEN 255


/*
 * DNS query type
 *
 * @DNS_OPC_STD Standard query
 * @DNS_OPC_REV Reverse query
 * @DNS_OPC_SSTAT Server status query
 */
enum dns_opcode {
	DNS_OPC_STD = 0,
	DNS_OPC_REV = 1,
	DNS_OPC_SSTAT = 2
};


/*
 * DNS error code
 *
 * @DNS_RC_NO_ERR No error
 * @DNS_RC_FMT_ERR Format error
 * @DNS_RC_SFAIL Server failure
 * @DNS_RC_NAME_ERR Name error
 * @DNS_RC_NIMPL Not implemented
 * @DNS_RC_REFUSED Refused
 */
enum dns_rcode {
	DNS_RC_NO_ERR = 0,
	DNS_RC_FMT_ERR = 1,
	DNS_RC_SFAIL = 2,
	DNS_RC_NAME_ERR = 3,
	DNS_RC_NIMPL = 4,
	DNS_RC_REFUSED = 5
};


/*
 * DNS record type
 *
 * @DNS_REC_A IPv4 record
 * @DNS_REC_AAAA IPv6 record
 * @DNS_REC_MX Mail exchange record
 * @DNS_REC_TXT Text record
 * @DNS_REC_CNAME Canonical name
 * @DNS_REC_ALL All cached records
 */
enum dns_record {
	DNS_REC_A = 1,
	DNS_REC_AAAA = 28,
	DNS_REC_MX = 15,
	DNS_REC_TXT = 16,
	DNS_REC_CNAME = 28,
	DNS_REC_ALL = 255
};


// DNS record class
enum dns_class {
	DNS_CL_IN = 1,
	DNS_CL_CH = 3,
	DNS_CL_HS = 4,
	DNS_CL_NONE = 254,
	DNS_CL_ANY = 255
};


/*
 * DNS header
 *
 * @id Message ID
 * @qr Message type (query - 0, response - 1)
 * @opcode Query type
 * @aa Authoritative answer (response)
 * @tc Truncated (response)
 * @rd Recursion desired (query)
 * @ra Recursion supported (response)
 * @z Reserved
 * @rcode Error code (response)
 * @qdcount Questions number (query)
 * @ancount Answers number (response)
 * @nscount Nameservers number (response)
 * @arcount Additional records number (response)
 */
struct dns_header {
	uint16_t id;

	unsigned char qr : 1;
	enum dns_opcode opcode : 4;
	unsigned char aa : 1;
	unsigned char tc : 1;
	unsigned char rd : 1;
	unsigned char ra : 1;
	unsigned char z : 3;
	enum dns_rcode rcode : 4;

	uint16_t qdcount;
	uint16_t ancount;
	uint16_t nscount;
	uint16_t arcount;
};


/*
 * DNS question
 *
 * @name Requested domain name
 * @qtype Requested record type
 * @qclass Requested records class
 */
struct dns_question {
	char name[DNS_MAX_NAME_LEN + 1];

	enum dns_record qtype : 16;
	enum dns_class qclass : 16;
};


/*
 * DNS answer
 *
 * @name Requested domain name
 * @qtype Requested record type
 * @qclass Requested records class
 * @ttl Time-to-live
 * @rdlength Length of the "rdata" field
 * @rdata Data associated with the record
 */
struct dns_answer {
	char name[DNS_MAX_NAME_LEN + 1];
	size_t namelen;

	enum dns_record type : 16;
	enum dns_class class : 16;
	uint32_t ttl;

	uint16_t rdlength;
	const char* rdata;
};


struct dns_authority {
	void* data;
};


struct dns_additional {
	void* data;
};


struct dns_message {
	struct dns_header header;
	struct dns_question question;
	struct dns_answer answer;
	struct dns_authority authority;
	struct dns_additional additional;
};


int dns_client(void)
{

	return EXIT_SUCCESS;
}


int main(void)
{
	return dns_client();
}
