//
//  Nameserver.cpp
//  
//
//  Created by 朱芄蓉 on 16/12/2017.
//

#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>
#include <vector>
#include <map>
#include "info.h"
using namespace std;

#define MAX 10

// the graph
bool connected[MAX][MAX] = {0};

/*
 Load all content servers' IP.
 
 * Parameter:
     - filepath: where to read from.
 
 * Returns:
     a vector<string> that stores all IP.
 */
void LoadServersIP(string filepath, vector<string>& server_ips){
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
     - s: a string that might contains multiple ips connected by a char
     - c: should be ','
 
 * Returns:
     a vector<string> that contains all adjacent ips.
 */
vector<string> split(const string& s, const char& c){
    string buff = "";
    vector<string> v;
    
    for(int i = 0;i < s.length(); ++i){
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
    string node, tmp_neighbors;
    int time_stamp, id = 0;
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
            pair<map<string, int>::iterator, bool> rtn = store.insert(make_pair(*i, id));
            if(rtn.second == true){  // this node appears for the first time
                id++;
                if(id >= MAX){
                    cerr << "[Nameserver]: Graph too small to store all nodes!" << endl;
                }
            }
        }
        neighbors.pop_back();  // remove node from its own neighbors
        
        /* update graph */
        for(int i = 0; i < MAX; ++i){
            connected[store[node]][i] = 0;
        }
        for(vector<string>::iterator i = neighbors.begin(); i != neighbors.end(); ++i){
            connected[store[node]][store[*i]] = 1;
        }
    }
    fin.close();
    
    return store;
}

int main(int argc, char* argv[]){
    int use_round_robin = NO;
    int allocated_port;
    string log_path, req_ip, port, server_ip_filepath, lsa_filepath;
    
    /* Read in parameters */
    if(argc == 6 || argc == 7){  // no '-r'
        if(argc == 7){
            if(strcmp(r, "-r") != 0){
                cerr << "[Nameserver]: Wrong parameter '"<< r << "', expect '-r'." << endl;
            }
            else{
                use_round_robin = YES;
            }
        }
        log_path = argv[argc - 5];
        req_ip = argv[argc - 4]
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
    
    /* Get LSA info and build up a graph with distance info */
    map<string, int> nodes = LoadLSA(lsa_filepath);
}
