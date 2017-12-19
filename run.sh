#g++ --std=c++11 -g -Wall csapp.c dns_helper.cpp nameserver.cpp -o nameserver -lpthread
./nameserver "no" 7.7.7.7 7777 topos/topo1/topo1.servers topos/topo1/topo1.lsa 
