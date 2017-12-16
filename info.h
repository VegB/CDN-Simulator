#define YES 1
#define NO 0

/* Header for DNS messages */
struct DNS_Header{
    uint16_t ID;
    
    uint8_t RD:1;
    uint8_t TC:1;
    uint8_t AA:1;
    uint8_t OPCODE:4;
    uint8_t QR:1;
    
    uint8_t RCODE:4;
    uint8_t Z:3;
    uint8_t RA:1;
    
    uint16_t QDCOUNT;
    uint16_t ANCOUNT;
    uint16_t NSCOUNT;
    uint16_t ARCOUNT;
};

