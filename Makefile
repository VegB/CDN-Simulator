#
# Makefile for Proxy Lab 
#
# You may modify is file any way you like (except for the handin
# rule). Autolab will execute the command "make" on your specific 
# Makefile to build your proxy from sources.
#
CC = g++
CFLAGS = -g -Wall
LDFLAGS = -lpthread

all: nameserver proxy

csapp.o: csapp.c csapp.h
	$(CC) $(CFLAGS) -c csapp.c

mydns.o: mydns.cpp mydns.h dns_helper.hpp
	$(CC) $(CFLAGS) -c mydns.cpp

proxy.o: proxy.c csapp.h mydns.h
	$(CC) $(CFLAGS) -c proxy.c

dns_helper.o: dns_helper.cpp dns_helper.hpp
	$(CC) $(CFLAGS) -c dns_helper.cpp

proxy: proxy.o csapp.o mydns.o dns_helper.o

nameserver.o: nameserver.cpp dns_helper.hpp
	$(CC) $(CFLAGS) -c nameserver.cpp

nameserver: dns_helper.o csapp.o nameserver.o

# Creates a tarball in ../proxylab-handin.tar that you should then
# hand in to Autolab. DO NOT MODIFY THIS!
handin:
	(make clean; cd ..; tar cvf proxylab-handin.tar proxylab-handout --exclude tiny --exclude nop-server.py --exclude proxy --exclude driver.sh --exclude port-for-user.pl --exclude free-port.sh --exclude ".*")

clean:
	rm -f *~ *.o proxy nameserver core *.tar *.zip *.gzip *.bzip *.gz

