//
//  Nameserver.cpp
//  
//
//  Created by 朱芄蓉 on 16/12/2017.
//

#include <iostream>
#include <fstream>
#include <iomanip>
#include <string>
#include <cstdlib>
#include <vector>
#include <map>
#include "info.h"
using namespace std;

#define MAX 10
#define INF 1000000

// the graph
int Distance[MAX][MAX];
int pre[MAX][MAX];
int node_num = 0;

/*
 Load all content servers' IP.
 
 * Parameter:
     - filepath: where to read from.
 
 * Returns:
     a vector<string> that stores all IP.
 */
vector<string> LoadServersIP(string filepath){
    vector<string> server_ips;
    string ip;
    ifstream fin;
    fin.open(filepath);
    
    while(fin >> ip){
        server_ips.push_back(ip);
    }
    fin.close();
    
    return server_ips;
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
    string buff = "";
    vector<string> v;
    
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
map<string, int> LoadLSA(string filepath){
    // keep record of all nodes that appears, <key: node, value: id in graph>
    map<string, int> store;
    // keep record of time sequence of lsa, <key: node, value: its most recent time_stamp>
    map<string, int> lsa_time_stamp;
    pair<map<string, int>::iterator, bool> rtn;
    string node, tmp_neighbors;
    int time_stamp;
    ifstream fin;
    fin.open(filepath);
    
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
    return store;
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

int main(int argc, char* argv[]){
    int use_round_robin = NO;
    int allocated_port;
    string log_path, req_ip, port, server_ip_filepath, lsa_filepath;
    
    /* Read in parameters */
    if(argc == 6 || argc == 7){  // no '-r'
        if(argc == 7){
            if(strcmp(argv[1], "-r") != 0){
                cerr << "[Nameserver]: Wrong parameter '"<< argv[1] << "', expect '-r'." << endl;
            }
            else{
                use_round_robin = YES;
            }
        }
        log_path = argv[argc - 5];
        req_ip = argv[argc - 4];
        allocated_port = atoi(argv[argc - 3]);
        server_ip_filepath = argv[argc - 2];
        lsa_filepath = argv[argc - 1];
    }
    else{
        cerr << "[Nameserver]: Wrong number of parameters. Please input '[-r] <log> <ip> <port> <servers> <LSAs>'." << endl;
        return 0;
    }
    
    /* Load server replicas' IP */
    vector<string> server_ips = LoadServersIP(server_ip_filepath);
    
    /* Get LSA info and build up a graph with Distance info */
    init_Distance();
    map<string, int> nodes = LoadLSA(lsa_filepath);
#ifdef DEBUG
    print_Distance(nodes);
#endif
}
