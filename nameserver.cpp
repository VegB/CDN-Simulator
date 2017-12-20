//
//  Nameserver.cpp
//
//
//  Created by 朱芄蓉 on 16/12/2017.
//

#include "dns_helper.hpp"

int use_round_robin = NO;
string log_path, my_ip, port, server_ip_filepath, lsa_filepath;
map<string, int> nodes;
vector<string> server_ips;

int load_parameters(int argc, char* argv[]){
    if(argc == 6 || argc == 7){  // no '-r'
        if(argc == 7){
            if(strcmp(argv[1], "-r") != 0){
                cerr << "[Nameserver]: Wrong parameter '"
                << argv[1] << "', expect '-r'." << endl;
            }
            else{
                use_round_robin = YES;
            }
        }
        log_path = argv[argc - 5];
        my_ip = argv[argc - 4];
        port = argv[argc - 3];
        server_ip_filepath = argv[argc - 2];
        lsa_filepath = argv[argc - 1];
    }
    else{
        cerr << "[Nameserver]: Wrong number of parameters. \
        Please input '[-r] <log> <ip> <port> <servers> <LSAs>'." << endl;
        return -1;
    }
    return 0;
}

int handle_request(int fd){
    rio_t rio;
    char buffer[BUFFER_SIZE];
    DNS_Packet request, response;
    
    /* Read packet */
    Rio_readinitb(&rio, fd);
    if (!Rio_readlineb(&rio, buffer, BUFFER_SIZE)){
        cerr << "[Nameserver]: Rio_readlineb() failed!" << endl;
        return -1;
    }
    char_to_dns_packet(buffer, request);
    string src_url(request.url);
    
    cout << "[Nameserver]: handling request from " << request.src_addr << " for "
    << src_url << endl;
    
    /* Select server to return */
    string selected_server = select_server(src_url, nodes,
                                           server_ips, use_round_robin);
    cout << "[Nameserver]: Selected " << selected_server << endl;
    // LOGGING
    // cout << "time " << request.src_addr << " " << src_url << " " << selected_server << endl;
    
    /* Create response */
    init_dns_response(response, my_ip.c_str(), selected_server.c_str());
    dns_packet_to_char(response, buffer);
    
    /* Send response */
    Rio_writen(fd, buffer, sizeof(response));
    cout << "[Nameserver]: response sent" << endl;
    return 0;
}

void *thread(void *vargp){
    int connfd = *((int *)vargp);
    pthread_detach(pthread_self());
    free(vargp);
    handle_request(connfd);
    close(connfd);
    return NULL;
}

int main(int argc, char* argv[]){
    /* Read in parameters */
    if(load_parameters(argc, argv) == -1){
        return 0;
    }
    cout << "[Nameserver]: IP = " << my_ip << ", port = " << port << endl;
    
    /* Load server replicas' IP */
    cout << "[Nameserver]: Loading server replicas' IP" << endl;
    LoadServersIP(server_ips, server_ip_filepath);
    // string tmp_server = server_ips.begin();  // used in round-robin
    
    /* Get LSA info and build up a graph with Distance info */
    cout << "[Nameserver]: Get LSA info and build up a graph with Distance info" << endl;
    init_Distance();
    LoadLSA(nodes, lsa_filepath);
    
    /* Main Process */
    int listenfd = Open_listenfd((char*)port.c_str());
    if(listenfd == -1){
        cerr << "[Nameserver]: Open_listenfd() failed!" << endl;
        return -1;
    }
    pthread_t tid;
    struct sockaddr_in clientaddr;
    while (1) {
        cout << "[Nameserver]: Waiting for connection." << endl;
        socklen_t clientaddr_len = sizeof(clientaddr);
        int* connfd = (int*)malloc(sizeof(int));
        *connfd = Accept(listenfd, (SA *)&clientaddr, &clientaddr_len);
        pthread_create(&tid, NULL, thread, connfd);
    }
    return 0;
}
