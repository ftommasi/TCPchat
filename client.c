// Fausto Tommasi
//
// code heavily modified from bind(2) example

#include <sys/socket.h>
#include <sys/un.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#define MY_SOCK_PATH "./somepath"
#define LISTEN_BACKLOG 50

#define handle_error(msg) \
    do { perror(msg); exit(EXIT_FAILURE); } while (0)

struct writer_args {
    int sfd;
};

void* writer(void* args) {
    struct writer_args* my_args = (struct writer_args*) args;
    FILE* socket = fdopen(my_args->sfd, "r");

    char buf[255];
    while (fgets(buf, 255, socket) != NULL) {
        printf("%s", buf);
    }
    return NULL;
}

char verify_args(char* argv[]){
  //verify IP & socket number
  return 1;
}


char* build_request(char type, char rtt, int num_probes, size_t msg_size, int delay){
    //FORMAT <PROTOCOL PHASE><WS><M−TYPE><WS><PROBES><WS><MSG SIZE><WS><SERVER DELAY>
    char* request = (char*)malloc(1000);
    static char* mtype;
    if(type == 't'){
      return "t\n\0";
    }
    if(rtt) 
      mtype = "RTT";
    else
      mtype = "tput";
    
    sprintf(request,"%c %s %d %d %d\n",type,  mtype, num_probes, msg_size, delay);
    //printf("generated %s\n",request);
    return request;

}


char verify_response_code(const char* response){
  if(!strcmp(response,"200 OK: Ready")){
    return 1;
  }
  
  else if(!strcmp(response,"404 ERROR: Invalid Connection Setup Message")){
    return 0;    
  }
  
  else{
    return 0;
  }
}

void usage_error_print(){
  printf("Usage client <IP address> <port number>\n");
}

int main(int argc, char *argv[])
{
    if(argc < 3 || argc > 3){
      usage_error_print();
      return -1;
    }
    if(!verify_args(argv)){
      usage_error_print();
      return -1;
    }
    char IP_address[256];
    strcpy(&IP_address,argv[1]);
    char port_num[256];
    strcpy(port_num,argv[2]);
    
    int sfd, cfd;
    struct sockaddr_in my_addr, peer_addr;
    socklen_t peer_addr_size;

    sfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sfd == -1)
        handle_error("socket");

    memset(&my_addr, 0, sizeof(struct sockaddr_in));
                        /* Clear structure */
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(atoi(port_num));
    //inet_aton(IP_address,&my_addr.sin_addr);
    my_addr.sin_addr.s_addr = inet_addr(IP_address);
    
//    strncpy(my_addr.sun_path, MY_SOCK_PATH,           sizeof(my_addr.sun_path) - 1);
    //printf("IP: %s socket: %d",IP_address, atoi(port_num));
    if (connect(sfd, (struct sockaddr *) &my_addr,
            sizeof(struct sockaddr_in)) == -1)
        handle_error("connect");

    pthread_t id;
    struct writer_args arg;
    arg.sfd = sfd;
    if (pthread_create(&id, NULL, writer, (void*) &arg) != 0) {
        handle_error("pthread_create error");
    }

    FILE* socket = fdopen(sfd, "w");

    char buf[255];
    while (fgets(buf, 255, stdin) != NULL) {
        
        fprintf(socket, "%s", buf);
        fflush(socket);

        if (strncmp("quit\n", buf, 5) == 0) {
            fclose(socket);
            exit(0);
        }
    }
}

