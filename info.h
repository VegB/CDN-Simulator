#define YES 1
#define NO 0

#define REQUEST 1
#define RESPONSE 0

#define BUFFER_SIZE 200
/* Header for DNS messages */
struct DNS_Header{
    uint8_t AA;
    uint8_t RD;
    uint8_t RA;
    uint8_t Z;
    uint8_t NSCOUNT;
    uint8_t ARCOUNT;
    uint8_t QTYPE;
    uint8_t QCLASS;
    uint8_t TYPE;
    uint8_t CLASS;
    uint8_t TTL;
};

struct DNS_Packet{
    DNS_Header header;
    char src_addr[20];
    /* for response */
    char ip[20];
    /* for request */
    char url[50];
};
