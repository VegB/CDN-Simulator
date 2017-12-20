#define YES 1
#define NO 0

#define REQUEST 1
#define RESPONSE 0

#define BUFFER_SIZE 200
/* Header for DNS messages */
struct DNS_Header{
    unsigned char AA;
    unsigned char RD;
    unsigned char RA;
    unsigned char Z;
    unsigned char NSCOUNT;
    unsigned char ARCOUNT;
    unsigned char QTYPE;
    unsigned char QCLASS;
    unsigned char TYPE;
    unsigned char CLASS;
    unsigned char TTL;
};

struct DNS_Packet{
    DNS_Header header;
    char src_addr[20];
    /* for response */
    char ip[20];
    /* for request */
    char url[50];
};
