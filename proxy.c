#include <stdio.h>
#include <stdlib.h>
#include "csapp.h"
#include <sys/time.h>
#include "mydns.h"
/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* You won't lose style points for including these long lines in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";
static const char *accept_hdr = "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n";
static const char *accept_encoding_hdr = "Accept-Encoding: gzip, deflate\r\n";

void doit(int fd);
int open_clientfd_bind_fake_ip(char *hostname, char *port, char *fake_ip);
int uri_found_f4m(char *uri, char *uri_nolist);
void read_requesthdrs(rio_t *rp);
int parse_uri(char *uri, char *hostname, int *port);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg);
int powerten(int i);
void *thread(void *vargp);
static void request_hdr(char *buf, char *buf2ser, char *hostname);
void parse_bitrates(char *xml);
int choose_bitrate(char *uri, char *uri_choose_bitrate);
float char2float(char* c);

// global variables
sem_t mutex_;
float alpha;
char *listen_port;
char *fake_ip;
char *www_ip;
char server_ip[MAXLINE];
char *video_pku = "video.pku.edu.cn";
char xml[MAXLINE];
int bitrate_array[50] = {0};
int bitrate_cnt = 0;
struct timeval start;
struct timeval end_t;
float throughput_current = 0;
float throughput_new = 0;
FILE *fp;
char *log_filename;
char *dns_ip, *dns_port;
int dns = 0;
int main(int argc, char **argv) 
{
    signal(SIGPIPE, SIG_IGN); // ignore sigpipe

    int listenfd;
    int *connfd;

    char hostname[MAXLINE], port[MAXLINE];
    socklen_t clientlen;
    struct sockaddr_in clientaddr;
    pthread_t tid;

    /* Check command line args */
    if (argc < 7) {
	    fprintf(stderr, "usage: %s <log> <alpha> <listen-port> <fake-ip> <dns-ip> <dns-port> [<www-ip>]\n", argv[0]);
	    exit(1);
    }
    log_filename = argv[1];
    alpha = char2float(argv[2]);
    listen_port = argv[3];
    fake_ip = argv[4];
    dns_ip = argv[5];
    dns_port = argv[6];
    if(argc == 8){
        www_ip = argv[7];
        dns = 0;
    }
    else{
        dns = 1;
        init_mydns(dns_ip, atoi(dns_port), fake_ip);
    }
    printf("fake_ip = %s\n", fake_ip);
    printf("www_ip = %s\n", www_ip);
    sem_init(&mutex_, 0, 1);
    listenfd = Open_listenfd(listen_port);
    while (1) {
	    clientlen = sizeof(clientaddr);
        connfd = (int*)malloc(sizeof(int));
	    *connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
        Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0);
        printf("Accepted connection from (%s, %s)\n", hostname, port);

        pthread_create(&tid, NULL, thread, connfd);
    }
    return 0;
}
float char2float(char* c){
    if(strcmp(c, "0.1") == 0)
        return 0.1;
    if(strcmp(c, "0.5") == 0)
        return 0.5;
    if(strcmp(c, "0.9") == 0)
        return 0.9;
    return 0;
}
/*
 * doit - handle one HTTP request/response transaction
 */

/* $begin doit */
void doit(int fd) 
{
    int serverfd, len;

    int *port;
    char port2[10]="8080";
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], uri_nolist[MAXLINE], version[MAXLINE];
    char filename[MAXLINE];         // client request filename
    char hostname[MAXBUF];          // client request hostname
    char buf2ser[MAXLINE];          // proxy to server
    char ser_response[MAXLINE];     // server to proxy
    rio_t rio, rio_ser;             // rio: between client and proxy
                                    // rio_ser: between proxy and server
    port = (int*)malloc(sizeof(int));
    *port = 80;                      // default port 80

    memset(buf2ser, 0, sizeof(buf2ser)); 
    memset(filename, 0, sizeof(filename)); 
    memset(hostname, 0, sizeof(hostname)); 
    memset(ser_response, 0, sizeof(ser_response));
    memset(uri, 0, sizeof(uri));
    memset(method, 0, sizeof(method));
    memset(buf, 0, sizeof(buf));
    memset(version, 0, sizeof(version));

    // step1: obtain request from client and parse the request
    Rio_readinitb(&rio, fd);
    if (!Rio_readlineb(&rio, buf, MAXLINE))  
        return;
    gettimeofday(&start, NULL);
    
    printf("request from client: %s\n", buf);

    // parse request into method, uri, version
    sscanf(buf, "%s %s %s", method, uri, version);       
    
    // check HTTP version, if 1.1, change it into 1.0
    if (!strcasecmp(version, "HTTP/1.1")) {
        strcpy(version, "HTTP/1.0");
    }

    // we only need GET method
    if (strcasecmp(method, "GET")) {     
        clienterror(fd, method, "501", "Not Implemented",
                    "This proxy does not implement this method");
        return;
    }               
    read_requesthdrs(&rio);

    /* Parse URI from GET request */
    parse_uri(uri, hostname, port);       
    strcpy(filename, uri);
    sprintf(buf2ser, "%s %s %s\r\n", method, filename, version);
    //printf("proxy to server: %s\n", buf2ser);

    // request header
    request_hdr(buf, buf2ser, hostname);
    if(dns == 0){
        if(strcmp(hostname, video_pku) == 0){
            strcpy(hostname,www_ip);
        }
        else{
            fprintf(stderr, "reverse proxy mode\n");
            strcpy(hostname,www_ip);
        }
    }
    // find .f4m in uri
    if (uri_found_f4m(uri, uri_nolist) != 0){
        // request .f4m from server, but do not give it to client
        cout << "[Proxy]: before binding, hostname: " << hostname << ", port2: " << port2 << endl;
        if ((serverfd = open_clientfd_bind_fake_ip(hostname, port2, fake_ip)) < 0){
            fprintf(stderr, "open server fd error\n");
            return;
        }
        Rio_readinitb(&rio_ser, serverfd);
        Rio_writen(serverfd, buf2ser, strlen(buf2ser));
        while ((len = rio_readnb(&rio_ser, ser_response,sizeof(ser_response))) > 0) {
            strcpy(xml, ser_response);
            parse_bitrates(xml);
            int i;
            for(i = 0; i < bitrate_cnt; i++){
                printf("bitrate = %d\n",bitrate_array[i]);
            }
            memset(ser_response, 0, sizeof(ser_response));
        }
        close(serverfd);
        // request _nolist.f4m from server, give it to client this time
        strcpy(hostname,video_pku);
        sprintf(buf2ser, "%s %s %s\r\n", method, uri_nolist, version);
        request_hdr(buf, buf2ser, hostname);
        if(dns == 0){
            if(strcmp(hostname, video_pku) == 0){
                strcpy(hostname,www_ip);
            }
            else{
                fprintf(stderr, "reverse proxy mode\n");
                strcpy(hostname,www_ip);
            }
        }
        if ((serverfd = open_clientfd_bind_fake_ip(hostname, port2, fake_ip)) < 0){
            fprintf(stderr, "open server fd error\n");
            return;
        }
        Rio_readinitb(&rio_ser, serverfd);
        Rio_writen(serverfd, buf2ser, strlen(buf2ser));
        while ((len = rio_readnb(&rio_ser, ser_response,sizeof(ser_response))) > 0) {
            Rio_writen(fd, ser_response, len);
            printf("no_list=\n%s\n",ser_response);
            memset(ser_response, 0, sizeof(ser_response));
        }
        close(serverfd);
        return;
    }
    // other requests
    char uri_choose_bitrate[MAXLINE];
    int rate = choose_bitrate(uri, uri_choose_bitrate);
    
    strcpy(hostname,video_pku);
    sprintf(buf2ser, "%s %s %s\r\n", method, uri_choose_bitrate, version);
    request_hdr(buf, buf2ser, hostname);
    if(dns == 0){
        if(strcmp(hostname, video_pku) == 0){
            strcpy(hostname,www_ip);
        }
        else{
            fprintf(stderr, "reverse proxy mode\n");
            strcpy(hostname,www_ip);
        }
    }
    if ((serverfd = open_clientfd_bind_fake_ip(hostname, port2, fake_ip)) < 0){
        fprintf(stderr, "open server fd error\n");
        return;
    }
        
    Rio_readinitb(&rio_ser, serverfd);
    printf("proxy to server: %s\n", buf2ser);
    // send request to server
    Rio_writen(serverfd, buf2ser, strlen(buf2ser));
        
    // step3: recieve the response from the server
    int chunk_size = 0;
    while ((len = rio_readnb(&rio_ser, ser_response,sizeof(ser_response))) > 0) {
        chunk_size += len;
        Rio_writen(fd, ser_response, len);
        memset(ser_response, 0, sizeof(ser_response));
    }
    gettimeofday(&end_t, NULL);
    float time_use = (end_t.tv_sec-start.tv_sec)*1000000+(end_t.tv_usec-start.tv_usec);
    printf("time_use = %.1f us\n",time_use);
    printf("chunk_size = %.1f Kb\n",(float)chunk_size*8/1000);
    throughput_new=(float)chunk_size*8000/time_use;
    printf("throughput_new = %.1f Kbps\n",throughput_new);
    if(throughput_current == 0)
        throughput_current = bitrate_array[0];
    else
        throughput_current = alpha * throughput_new + (1 - alpha) * throughput_current;
    printf("throughput_current = %.1f Kbps\n",throughput_current);
    close(serverfd);
    
    // output log
    int time_epoch = end_t.tv_sec+end_t.tv_usec/1000000;
    float time_duration = time_use/1000000;
    // tput = throughput_new;
    // avg-tput = throughput_current;
    // bitrate = rate
    // server-ip = www_ip
    // chunkname = uri_choose_bitrate
    
    if((fp = fopen(log_filename,"a+")) == NULL){
        printf("file cannot be opened/n");
        return;
    }
    // <time> <duration> <tput> <avg-tput> <bitrate> <server-ip> <chunkname>
    fprintf(fp, "%d %f %f %f %d %s %s\n", time_epoch, time_duration, throughput_new,
            throughput_current, rate, www_ip, uri_choose_bitrate);
    printf("%d %f %f %f %d %s %s\n", time_epoch, time_duration, throughput_new,
            throughput_current, rate, www_ip, uri_choose_bitrate);
    fclose(fp);
    
    
    
    
    
}
int choose_bitrate(char *uri, char *uri_choose_bitrate){
    int i;
    int choosen_bitrate = 0;
    char bitrate_char[10];
    for(i = bitrate_cnt - 1; i >= 0; i--){
        if(throughput_current / 1.5 >= bitrate_array[i]){
            choosen_bitrate = bitrate_array[i];
            break;
        }
    }
    if(choosen_bitrate == 0)
        choosen_bitrate = bitrate_array[0];
    sprintf(bitrate_char, "%d", choosen_bitrate);
    printf("choose bitrate = %s\n", bitrate_char);
    char *p;
    int len = 0;

    int flag = 0;
    char uri_part_1[50], uri_part_2[50];
    for(p = uri; *p; p++){
        if(strncmp(p, "Seg", strlen("Seg")) == 0){
            flag = 1;
            break;
        }
    }
    if(flag == 0) {
        strcpy(uri_choose_bitrate, uri);
        return choosen_bitrate;
    }
    strcpy(uri_part_2, p);
    p--;
    while(*p >= '0' && *p <= '9')
        p--;
    len = p - uri + 1;
    for(i = 0; i < len; i++){
        uri_part_1[i] = uri[i];
    }
    uri_part_1[i] = 0;
    //strcat(uri_part_1, "\0");
    
    printf("uri=%s\n",uri);
    strcat(uri_part_1, bitrate_char);
    strcat(uri_part_1, uri_part_2);
    printf("final_uri=%s\n",uri_part_1);
    strcpy(uri_choose_bitrate, uri_part_1);
    return choosen_bitrate;
}
/* $end doit */
void parse_bitrates(char *xml){
    char* p;
    int array_index = 0;
    for(p = xml; *p; p++){
        if(strncmp(p, "bitrate=\"", strlen("bitrate=\"")) == 0){
            p += strlen("bitrate=\"");
            char tmp_bitrate[10];
            int index = 0;
            while(*p >= '0' && *p <= '9'){
                tmp_bitrate[index] = *p;
                index++;
                p++;
            }
            tmp_bitrate[index] = '\0';
            bitrate_array[array_index]=atoi(tmp_bitrate);
            array_index++;
            bitrate_cnt++;
        }
    }
}


int uri_found_f4m(char *uri, char *uri_nolist){
    char uri_tmp[MAXLINE];
    strcpy(uri_tmp, uri);
    char *p;
    for (p = uri_tmp; *p; p++){
        if (strncmp(p, ".f4m", strlen(".f4m")) == 0){
            strcpy(p, "_nolist.f4m");
            strcpy(uri_nolist, uri_tmp);
            printf("uri find f4m! and convert to nolist\n");
            return 1;
        }
    }
    return 0;
}
/* $begin open_clientfd_bind_fake_ip */
int open_clientfd_bind_fake_ip(char *hostname, char *port, char *fake_ip) {
    int clientfd;
    struct addrinfo hints, *listp, *p;
    
    /* Get a list of potential server addresses */
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_socktype = SOCK_STREAM;  /* Open a connection */
    hints.ai_flags = AI_NUMERICSERV;  /* ... using a numeric port arg. */
    hints.ai_flags |= AI_ADDRCONFIG;  /* Recommended for connections */
    if(dns == 1){
        resolve(hostname, port, &hints, &listp);
		cout << ((sockaddr_in*)(listp->ai_addr))->sin_addr.s_addr;
}
    else{
        Getaddrinfo(hostname, port, &hints, &listp);
    }
    
    /* Walk the list for one that we can successfully connect to */
    for (p = listp; p; p = p->ai_next) {
        /* Create a socket descriptor */
        if ((clientfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0)
            continue; /* Socket failed, try the next */
        /* Bind fake_ip*/
        struct sockaddr_in saddr;
        memset((void *) &saddr, 0, sizeof(saddr));
        saddr.sin_family = AF_INET;
        saddr.sin_port = htons(0);
        saddr.sin_addr.s_addr = inet_addr(fake_ip);
        if (bind(clientfd, (struct sockaddr *) &saddr, sizeof(saddr)) < 0) {
            fprintf(stderr, "Bind clientfd error.\n");
            Close(clientfd);
            continue;
        }
        /* Connect to the server */
        Inet_ntop(AF_INET,&((struct sockaddr_in*)(p->ai_addr))->sin_addr,server_ip, p->ai_addrlen);
        if (connect(clientfd, p->ai_addr, p->ai_addrlen) != -1)
            break; /* Success */
        Close(clientfd); /* Connect failed, try another */  //line:netp:openclientfd:closefd
    }
    
    /* Clean up */
    if(dns == 1){
        mydns_freeaddrinfo(listp);
    }
    else{
        Freeaddrinfo(listp);
    }
    if (!p) /* All connects failed */
        return -1;
    else    /* The last connect succeeded */
        return clientfd;
}
/* $end open_clientfd_bind_fake_ip */

/*
 * request_hdr - request header
 * if the request does not contain header, add request header
 */
static void request_hdr(char *buf, char *buf2ser, char *hostname)
{
    if(strcmp(buf, "Host"))
    {
          strcat(buf2ser, "Host: ");
          strcat(buf2ser, hostname);
          strcat(buf2ser, "\r\n");
    }
    if(strcmp(buf, "Accept:")) {
        strcat(buf2ser, accept_hdr);
    }
    if(strcmp(buf, "Accept-Encoding:")) {
        strcat(buf2ser, accept_encoding_hdr);
    }
    if(strcmp(buf, "User-Agent:")) {
        strcat(buf2ser, user_agent_hdr);
    }
    if(strcmp(buf, "Proxy-Connection:")) {
        strcat(buf2ser, "Proxy-Connection: close\r\n");
    }
    if(strcmp(buf, "Connection:")) {
        strcat(buf2ser, "Connection: close\r\n");
    }
    memset(buf, 0, sizeof(buf));
    strcat(buf2ser, "\r\n");
}

/*
 * thread - thread funciton
 *
 */
void *thread(void *vargp)
{
    int connfd = *((int *)vargp);
    pthread_detach(pthread_self());
    free(vargp);
    doit(connfd);
    close(connfd);
    return NULL;
}

/*
 * read_requesthdrs - read HTTP request headers
 */
/* $begin read_requesthdrs */
void read_requesthdrs(rio_t *rp) 
{
    char buf[MAXLINE];

    Rio_readlineb(rp, buf, MAXLINE);
    printf("%s", buf);
    while(strcmp(buf, "\r\n")) {      
        Rio_readlineb(rp, buf, MAXLINE);
        printf("%s", buf);
    }
    return;
}
/* $end read_requesthdrs */

/*
 * parse_uri - parse URI into filename and CGI args
 *             return 0 if dynamic content, 1 if static
 */
/* $begin parse_uri */
int parse_uri(char *uri, char *hostname, int *port) 
{
    // in this lab all requests are static 

    char tmp[MAXLINE];          // holds local copy of uri
    char *buf;                  // ptr that traverses uri
    char *endbuf;               // ptr to end of the cmdline string
    int port_tmp[10];
    int i, j;                   // loop
    char num[2];                // store port value

    buf = tmp;
    for (i = 0; i < 10; i++) {
        port_tmp[i] = 0;
    }
    (void) strncpy(buf, uri, MAXLINE);
    endbuf = buf + strlen(buf);
    buf += 7;                   // 'http://' has 7 characters
    while (buf < endbuf) {
    // take host name out
        if (buf >= endbuf) {
            strcpy(uri, "");
            strcat(hostname, " ");
            // no other character found
            break;
        }
        if (*buf == ':') {  // if port number exists
            buf++;
            *port = 0;
            i = 0;
            while (*buf != '/') {
                num[0] = *buf;
                num[1] = '\0';
                port_tmp[i] = atoi(num);
                buf++;
                i++;
            }
            j = 0;
            while (i > 0) {
                *port += port_tmp[j] * powerten(i - 1);
                j++;
                i--;
            }
        }
        if (*buf != '/') {

            sprintf(hostname, "%s%c", hostname, *buf);
        }
        else { // host name done
            strcat(hostname, "\0");
            strcpy(uri, buf);
            break;
        }
        buf++;
    }
    return 1;
}
/* $end parse_uri */

/*
 * powerten - return ten to the power of i
 */
int powerten(int i) {
    int num = 1;
    while (i > 0) {
        num *= 10;
        i--;
    }
    return num;
}

/*
 * clienterror - returns an error message to the client
 */
/* $begin clienterror */
void clienterror(int fd, char *cause, char *errnum, 
		 char *shortmsg, char *longmsg) 
{
    char buf[MAXLINE], body[MAXBUF];

    /* Build the HTTP response body */
    sprintf(body, "<html><title>Tiny Error</title>");
    sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body);
    sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
    sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
    sprintf(body, "%s<hr><em>The Tiny Web server</em>\r\n", body);

    /* Print the HTTP response */
    sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-type: text/html\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
    Rio_writen(fd, buf, strlen(buf));
    Rio_writen(fd, body, strlen(body));
}
/* $end_t clienterror */
