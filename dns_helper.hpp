//
//  dns_helper.hpp
//  
//
//  Created by 朱芄蓉 on 19/12/2017.
//

#ifndef dns_helper_hpp
#define dns_helper_hpp

#include <stdio.h>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <string>
#include <cstdlib>
#include <vector>
#include <map>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include "info.h"
#include "csapp.h"

using namespace std;

void LoadServersIP(vector<string>& server_ips, string filepath);
vector<string> split(const string& s, const char& c);
void LoadLSA(map<string, int> &nodes, string filepath);
void init_Distance();
void print_Distance(map<string, int> nodes);
string select_server(string src_ip, map<string, int> nodes,
                     vector<string> server_ips, int use_round_robin);
void init_dns_header(DNS_Packet& packet, int is_request);
void init_dns_request(DNS_Packet& packet, const char* src_addr, const char* ip);
void init_dns_response(DNS_Packet& packet, const char* src_addr, const char* url);
void dns_packet_to_char(DNS_Packet& packet, char* buffer);
void char_to_dns_packet(char* buffer, DNS_Packet& packet);
#endif /* dns_helper_hpp */
