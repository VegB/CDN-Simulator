//
//  dns_helper.cpp
//  
//
//  Created by 朱芄蓉 on 19/12/2017.
//

#include "dns_helper.hpp"


#define MAX 10
#define INF 1000000

// the graph
int Distance[MAX][MAX];
int pre[MAX][MAX];
int node_num = 0;

// round-robin
vector<string>::iterator tmp_server;

/*
 Load all content servers' IP.
 
 * Parameter:
 - filepath: where to read from.
 
 * Returns:
 a vector<string> that stores all IP.
 */
void LoadServersIP(vector<string>& server_ips, string filepath){
    string ip;
    ifstream fin;
    fin.open(filepath.c_str());
    
    while(fin >> ip){
        server_ips.push_back(ip);
    }
    fin.close();
}

/*
 Split a string by a char and returns a vector.
 Called by LoadLSA().
 
 * Parameters:
 - s: a string that might contains multiple ips Distance by a char
 - c: should be ','
 
 * Returns:
 a vector<string> that contains all adjacent ips.
 */
vector<string> split(const string& s, const char& c){
    vector<string> v;
    string buff = "";
    
    for(int i = 0; i < s.length(); ++i){
        if(s[i] != c){
            buff+=s[i];
        }
        else if(s[i] == c && buff != "") {
            v.push_back(buff);
            buff = "";
        }
    }
    if(buff != ""){
        v.push_back(buff);
    }
    return v;
}

/*
 Read in LSA from 'filepath'.
 Update graph connection status accordingly.
 
 * Parameters:
 filepath: where LSA file is stored.
 
 * Returns:
 nodes that appears in map<string, int>.
 */
void LoadLSA(map<string, int> &store, string filepath){
    // keep record of all nodes that appears, <key: node, value: id in graph>
    // keep record of time sequence of lsa, <key: node, value: its most recent time_stamp>
    map<string, int> lsa_time_stamp;
    pair<map<string, int>::iterator, bool> rtn;
    string node, tmp_neighbors;
    int time_stamp;
    ifstream fin;
    fin.open(filepath.c_str());
    
    /* read in each LSA message */
    while(fin >> node >> time_stamp >> tmp_neighbors){
        /* make sure this lsa message is up-to-date */
        rtn = lsa_time_stamp.insert(make_pair(node, time_stamp));
        if(rtn.second == false && (rtn.first)->second >= time_stamp){  // this lsa is out-of-date
            continue;
        }
        rtn.first->second = time_stamp;  // update time_stamp
        
        /* get all neighbors */
        vector<string> neighbors = split(tmp_neighbors, ',');
        
        /* record all nodes */
        neighbors.push_back(node);  // add node into loop below
        for(vector<string>::iterator i = neighbors.begin(); i != neighbors.end(); ++i){
            pair<map<string, int>::iterator, bool> rtn = store.insert(make_pair(*i, node_num));
            if(rtn.second == true){  // this node appears for the first time
                node_num++;
                if(node_num >= MAX){
                    cerr << "[Nameserver]: Graph too small to store all nodes!" << endl;
                }
            }
        }
        neighbors.pop_back();  // remove node from its own neighbors
        
        /* update graph */
        for(int i = 0; i < node_num; ++i){
            Distance[store[node]][i] = INF;
            Distance[i][store[node]] = INF;
        }
        for(vector<string>::iterator i = neighbors.begin(); i != neighbors.end(); ++i){
            Distance[store[node]][store[*i]] = 1;
            Distance[store[*i]][store[node]] = 1;
        }
    }
    fin.close();
    
    /* Floyd */
    for(int i = 0; i < node_num; ++i){
        for(int j = 0; j < node_num; ++j){
            if(i == j){
                Distance[i][j] = 0;
            }
            if(i == j || Distance[i][j] < INF){
                pre[i][j] = i;
            }
            else if(Distance[i][j] == INF){
                pre[i][j] = -1;
            }
        }
    }
    for(int i = 0; i < node_num; ++i){
        for(int j = 0; j < node_num; ++j){
            for(int k = 0;k < node_num; ++k){
                if(Distance[i][j] > Distance[i][k] + Distance[k][j]){
                    Distance[i][j] = Distance[i][k] + Distance[k][j];
                    pre[i][j] = pre[k][j];
                }
            }
        }
    }
}

void init_Distance(){
    for(int i = 0; i < MAX; ++i){
        for(int j = 0; j < MAX; ++j){
            Distance[i][j] = INF;
        }
    }
}

void print_Distance(map<string, int> nodes){
    cout << "[Nameserver]: Printing Graph details." << endl;
    cout << "Nodes and id: " << endl;
    for(map<string, int>::iterator i = nodes.begin(); i != nodes.end(); ++i){
        cout << i->first << ": " << i->second << endl;
    }
    cout << "Distance: " << endl;
    for(int i = 0; i < node_num; ++i){
        for(int j = 0; j < node_num; ++j){
            cout << setw(5) << Distance[i][j] << " ";
        }
        cout << endl;
    }
}

/*
 Select which server to handle request.
 
 * Parameters:
 - src_ip: IP of the client that sends the request
 - nodes: <key: IP, value id>, used to turn ip into index in Distance[][]
 - server_ips: IP of all available servers
 - use_round_robin: whether to use round-robin or select based on distance
 
 * Returns:
 the ip of the selected server
 */
string select_server(string src_ip, map<string, int> nodes,
                     vector<string> server_ips, int use_round_robin){
    string rst;
    /* Round_robin */
    if(use_round_robin == YES){
        rst = *tmp_server;
        tmp_server++;
        if(tmp_server == server_ips.end()){
            tmp_server = server_ips.begin();
        }
    }
    /* Distance based selection */
    else{
        int min_dist = INF;
        for(vector<string>::iterator i = server_ips.begin(); i != server_ips.end(); ++i){
            if(Distance[nodes[src_ip]][nodes[*i]] < min_dist){
                rst = *i;
                min_dist = Distance[nodes[src_ip]][nodes[*i]];
            }
        }
    }
    return rst;
}

void init_dns_request(DNS_Packet& packet, const char* src_addr, const char* url){
    init_dns_header(packet, REQUEST);
    strcpy(packet.src_addr, src_addr);
    /* set URL */
    //packet.url_len = url.length();
    strcpy(packet.url, url);
    cout << "Request URL: " << packet.url << endl;
}

void init_dns_response(DNS_Packet& packet, const char* src_addr, const char* ip){
    init_dns_header(packet, RESPONSE);
    strcpy(packet.src_addr, src_addr);
    /* Set IP */
    //packet.ip_len = ip.length();
    strcpy(packet.ip, ip);
    cout << "Response IP: " << packet.ip << endl;
}

/* Initialize a packet used for communication between DNS server and client side */
void init_dns_header(DNS_Packet& packet, int is_request){
    DNS_Header* header = &(packet.header);
    memset(header, 0, sizeof(struct DNS_Header));
    header->AA = 0;
    header->RD = 0;
    header->RA = 0;
    header->Z = 0;
    header->NSCOUNT = 0;
    header->ARCOUNT = 0;
    header->TTL = 0;
    
    if(is_request == YES){
        header->AA = 0;
        header->QTYPE = 1;
        header->QCLASS = 1;
    }
    else{
        header->AA = 1;
        header->TYPE = 1;
        header->CLASS = 1;
    }
}

void dns_packet_to_char(DNS_Packet& packet, char* buffer){
    memcpy((void*)buffer, (const void*)(&packet), sizeof(packet));
    char* p = buffer + sizeof(struct DNS_Header);
    cout << "src_addr: " << p << endl;
    p += 20;
    cout << "ip: " << p << enl;
    p += 20;
    cout << "url: " << p << endl;
}

void char_to_dns_packet(char* buffer, DNS_Packet& packet){
    memcpy((void*)buffer, (const void*)(&packet), sizeof(packet));
}
