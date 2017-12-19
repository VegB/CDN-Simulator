//
//  mydns.cpp
//  
//
//  Created by 朱芄蓉 on 16/12/2017.
//

#include "mydns.h"

int clientfd;

/**
 * Initialize your client DNS library with the IP address and port number of
 * your DNS server.
 *
 * @param  dns_ip  The IP address of the DNS server.
 * @param  dns_port  The port number of the DNS server.
 * @param  client_ip  The IP address of the client
 *
 * @return 0 on success, -1 otherwise
 */
int init_mydns(const char *dns_ip, unsigned int dns_port, const char *client_ip){
    char port[10];
    sprintf(port, "%d", dns_port);
    clientfd = open_clientfd(dns_ip, dns_port);
    if(clientfd == -1){
        return -1;
    }
    return 0;
}


/**
 * Resolve a DNS name using your custom DNS server.
 *
 * Whenever your proxy needs to open a connection to a web server, it calls
 * resolve() as follows:
 *
 * struct addrinfo *result;
 * int rc = resolve("video.pku.edu.cn", "8080", null, &result);
 * if (rc != 0) {
 *     // handle error
 * }
 * // connect to address in result
 * mydns_freeaddrinfo(result);
 *
 *
 * @param  node  The hostname to resolve.
 * @param  service  The desired port number as a string.
 * @param  hints  Should be null. resolve() ignores this parameter.
 * @param  res  The result. resolve() should allocate a struct addrinfo, which
 * the caller is responsible for freeing.
 *
 * @return 0 on success, -1 otherwise
 */

int resolve(const char *node, const char *service,
            const struct addrinfo *hints, struct addrinfo **res){
    char buffer[BUFFER_SIZE];
    DNS_Packet request, response;
    int clientfd;
    rio_t rio;
    
    (*res) = (addrinfo*) malloc(sizeof(addrinfo));
    (*res)->ai_flags = AI_PASSIVE;
    (*res)->ai_family = AF_INET;
    (*res)->ai_socktype = SOCK_STREAM;
    (*res)->ai_protocol = 0;
    (*res)->ai_addrlen = (socklen_t)sizeof(sockaddr);
    (*res)->ai_canonname = (char*)node;
    (*res)->ai_next = NULL;
    (*res)->ai_addr = (sockaddr*) malloc(sizeof(sockaddr));
    ((sockaddr_in*) (*res)->ai_addr)->sin_port = ntohs(atoi(service));
    
    /* Create request packet */
    init_dns_request(request, node);
    dns_packet_to_char(request, buffer);

    /* Connect to DNS server */
    if((clientfd = open_clientfd(node, service)) < 0){
        cerr << "[mydns]: open clientfd failed!" << endl;
        return -1;
    }
    
    /* send request to server */
    Rio_readinitb(&rio, clientfd);
    Rio_writen(clientfd, buffer, sizeof(request));
    
    /* recieve the response from the server */
    memset(buffer, 0, sizeof(response));
    while(rio_readnb(&rio, buffer, BUFFER_SIZE) > 0) {
        char_to_dns_packet(buffer, response);
    }
    close(clientfd);

    /* store result into res */
    ((sockaddr_in*)((*res)->ai_addr))->sin_addr.s_addr = inet_addr(response.ip);
    ((sockaddr_in*)((*res)->ai_addr))->sin_family = AF_INET;
    return 0;
}

/**
 * Release the addrinfo structure.
 *
 * @param  p  the addrinfo structure to release
 *
 * @return 0 on success, -1 otherwise
 */
int mydns_freeaddrinfo(struct addrinfo *p){
    if(p->ai_addr != NULL){
        free(p->ai_addr);
    }
    if(p != NULL){
        free(p);
    }
    return 0;
}
