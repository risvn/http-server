#include<sys/socket.h>
#include<strings.h>
#include <netdb.h>
#include <arpa/inet.h>  
#include <netinet/in.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <sys/sendfile.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>




#define PORT "3490"
#define BACKLOG 10 


//extracting the addr ipv4 or ipv6
void* get_addr_info(struct sockaddr* sa)
{
  if(sa->sa_family==AF_INET){
    return &(((struct sockaddr_in *)sa)->sin_addr);
  }
  else{
    return &(((struct sockaddr_in6 *)sa)->sin6_addr);
  }
}


char* get_mime_type(char* path)
{
  char* ext = strrchr(path,'.');
  if(strcmp(ext,".html")==0){return "text/html";}
  if(strcmp(ext,".jpg")==0||strcmp(ext,".jpeg")==0){return "image/jpg";}
  else{return "text/html";}
}


char* req_parser(char* buffer)
{
  char* client_req=buffer+4;
  *strchr(client_req,' ')=0;

  // if the request path is root "/" send the index.html file
  if (strcmp(client_req, "/") == 0 ) {
    return client_req = "index.html";
} else {
    if (client_req[0] == '/') client_req++;
          return client_req;
    }
}



int send_file(int client_fd,char* req_file)
{
  int open_fd=open(req_file,O_RDONLY);
  // TODO: replace the print with http status code replaying to the client
  if(open_fd<0){
    printf("file not found\n");

    const char* not_found = "HTTP/1.1 404 Not Found\r\n"
                                "Content-Type: text/html\r\n"
                                "Content-Length: 23\r\n"
                                "Connection: close\r\n\r\n"
                                "<h1>404 Not Found</h1>";

        send(client_fd, not_found, strlen(not_found), 0);
        shutdown(client_fd, SHUT_WR);  // optional, signals end of data
        close(client_fd);
        return 0;
    }

  //get the file size
  struct stat st;
  fstat(open_fd,&st);

  //get the mime type of the request
  char* mime = get_mime_type(req_file);
  printf("content type:%s\n",mime);
  //construct http-headers
  char header[512];
  snprintf(header, sizeof(header),
             "HTTP/1.1 200 OK\r\n"
             "Content-Type: %s\r\n"
             "Content-Length: %ld\r\n"
             "Connection: close\r\n\r\n",
             mime, st.st_size);

   // send header to the browser
  send(client_fd, header, strlen(header), 0);
  int status = sendfile(client_fd,open_fd,0,st.st_size);
  
      if(status<0){printf("server failed! to send file\n");}
      else{printf("file sent successfully!\n");}

    close(open_fd);   
    close(client_fd);  



return 0;
}



int main()
{
struct addrinfo  *res, *p;
//for holding ipv6 or ipv4 with out knowing in advance
struct sockaddr_storage their_addr;
socklen_t  addr_size;

struct addrinfo hints ={0};
  hints.ai_family=AF_UNSPEC;
  hints.ai_socktype=SOCK_STREAM;
  hints.ai_flags=AI_PASSIVE;
  
  //DNS resolver it will get ip by host name in case of server pass NULL
  //incase of client pass server name eg:www.google.com
    int s = getaddrinfo(NULL,PORT, &hints, &res);
    if (s != 0) {
               fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
               exit(1);
          }


  int sockfd =socket(res->ai_family,res->ai_socktype,res->ai_protocol);

  int opt = 1;
  setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

  bind(sockfd,res->ai_addr,res->ai_addrlen);

  if (listen(sockfd, BACKLOG) == -1) {
    perror("listen");
    exit(1);
  }

printf(" Server running on port %s\n", PORT);
  
//accept loop for each client fork new process
while(1)
  {

  //TODO: make the process async and multi-thread to handle clients 

  addr_size=sizeof(their_addr);
    //accept(fd,sockadddr,addr_size)
  int client_fd=accept(sockfd,(struct sockaddr *)&their_addr,&addr_size);

    char ip_str[INET_ADDRSTRLEN];
    inet_ntop(their_addr.ss_family,get_addr_info((struct sockaddr *)&their_addr),ip_str,sizeof(ip_str));
    //extracting port
    int port;
        if (their_addr.ss_family == AF_INET)
            port = ntohs(((struct sockaddr_in *)&their_addr)->sin_port);
        else
            port = ntohs(((struct sockaddr_in6 *)&their_addr)->sin6_port);
    printf("Client connected from %s:%d\n",ip_str,port);
    

    
    //TODO:only accept only GET requestes
    char buff[1024];
    memset(buff,0,sizeof buff);
    int bytes_recv=recv(client_fd,buff,sizeof(buff)-1,0);
      printf("bytes recvd: %d\n",bytes_recv);
      printf("client request >>:%s\n",buff);
     char* client_req = req_parser(buff);
      printf("requsest>>:%s\n",client_req);


      send_file(client_fd,client_req);

  }
  
}
