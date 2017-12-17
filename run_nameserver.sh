#g++ --std=c++11 -g -Wall csapp.c dns_helper.cpp nameserver.cpp -o nameserver -lpthread
#./nameserver  nameserver.log 7.0.0.1 7777 topos/topo2/topo2.servers topos/topo2/topo2.lsa
./nameserver -r nameserver_r.log 5.0.0.1 7777 topos/topo1/topo1.servers topos/topo1/topo1.lsa
