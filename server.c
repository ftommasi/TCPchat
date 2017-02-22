// Christopher Lucas
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

typedef struct Node {
  int cfd;
  char* name;
  struct Node* next;
} Node;

Node* head;

typedef struct request{
  char* mtype;
  int num_probes;
  size_t msg_size;
  int delay;
  char valid;
} request;

request* parse_request(char* request_string){
  //FORMAT <PROTOCOL PHASE><WS><M−TYPE><WS><PROBES><WS><MSG SIZE><WS><SERVER DELAY>
  request* incoming = (request*)malloc(sizeof(request));
  incoming->mtype = (char*)malloc(256);
  incoming->num_probes = -1;
  incoming->msg_size = 0; 
  incoming->delay = -1;

  incoming->valid = 0;

  char* token = strtok(request_string, " ");
  if(!strcmp(token,"t\n")){
    //terminate connection
    incoming->valid = 1;
  }

  //printf("tokenized %s\n", token); 
  if(token) {
    token = strtok(NULL, " ");
    incoming->mtype = token;
    //printf("tokenized %s\n", token); 
  }
  if(token){ 
    token = atoi(strtok(NULL," "));
    incoming->num_probes = token;
    //printf("tokenized %d\n", token); 
  } 
  if(token) {
    token = atoi(strtok(NULL," "));
    incoming->msg_size = token;
    //printf("tokenized %d\n", token); 
  }
  if(token) {
    token = atoi(strtok(NULL," "));
    incoming->delay = token; 
    //printf("tokenized %d\n", token); 
  }

  incoming->valid = 1; //If I can parse the whole thing the format should be correct
  return incoming;
}

const char* generate_response_code(request* req){
  const char* ok200 = "200 OK: Ready";
  const char* bad404 = "404 ERROR: Invalid Connection Setup Message";
  if(!req->valid){
    return bad404;
  }
  
  if(!req->mtype){
    
    return bad404;
  }
  
  if(!strcmp(req->mtype,'m') || !strcmp(req->mtype, 's')){
    return bad404;
  }

  if(req->num_probes < 0 ){
    return bad404;
  }

  if(req->msg_size < 1){
    return bad404;
  }

  if(req->delay < 0){
    return bad404; 
  }

  return ok200;

}

struct forwarder_args {
  int cfd;
  pthread_mutex_t* mutex;
};

void* forwarder(void* args) {
  struct forwarder_args* my_args = (struct forwarder_args*) args;
  int cfd = my_args->cfd;


  FILE* socket = fdopen(cfd, "r");
  if (socket == NULL) {
    handle_error("fdopen");
  }

  Node* self = head;
  while (self != NULL && self->cfd != cfd) {
    self = self->next;
  }

  char buf[255];
  while (fgets(buf, 255, socket) != NULL) {

    if (pthread_mutex_lock(my_args->mutex)) {
      handle_error("lock error");
    }

    if (strncmp("quit\n", buf, 5) == 0) {
      // run quit code below
      break;
    }  


    else {
      request* r = malloc(sizeof(request));
      r= parse_request(buf);
      char* response = generate_response_code(r);

      // forward message
      Node* curr = head->next;
      while (curr != NULL) {
        //if (curr->cfd != cfd) {
        if (
            write(curr->cfd, self->name, strlen(self->name)) == -1 ||
            write(curr->cfd, ": ", 2) == -1 ||
            write(curr->cfd, response, strlen(buf)) == -1
           ) {
          handle_error("forward write failed");
        }
        //}
        curr = curr->next;
      }

      if (pthread_mutex_unlock(my_args->mutex)) {
        handle_error("forward mutex unlock");
      }
    }
  }

  // handle quit or socket break
  // write has quit message
  Node* curr = head->next;
  while (curr != NULL) {
    if (curr->cfd != cfd) {
      if (
          write(curr->cfd, self->name, strlen(self->name)) == -1 ||
          write(curr->cfd, " has quit\n", 10) == -1
         ) {
        handle_error("quit write failed");
      }
    }
    curr = curr->next;
  }

  // remove node
  curr = head;
  while (curr != NULL && curr->next != NULL && curr->next->cfd != cfd) {
    curr = curr->next;
  }
  Node* next = curr->next;
  curr->next = curr->next->next;
  free(next->name);
  free(next);

  // close socket
  if (fclose(socket)) {
    handle_error("fclose");
  }

  if (pthread_mutex_unlock(my_args->mutex)) {
    handle_error("quit mutex unlock");
  }
  return NULL;
}

char verify_args(char* argv[]){
  //verify IP & socket number
  return 1;
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
  strcpy(&port_num,argv[2]);

  head = (struct Node*) malloc(sizeof(Node));
  if (head == NULL) {
    handle_error("error mallocing head");
  }
  pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

  int sfd, cfd;
  struct sockaddr_in my_addr, peer_addr;
  socklen_t peer_addr_size;

  sfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sfd == -1)
    handle_error("socket");

  memset(&my_addr, 0, sizeof(struct sockaddr_in));
  /* Clear structure */
  my_addr.sin_family = AF_INET;
  //strncpy(my_addr.sun_path, MY_SOCK_PATH,     sizeof(my_addr.sun_path) - 1);
  my_addr.sin_port = htons(atoi(port_num));
  my_addr.sin_addr.s_addr = inet_addr(IP_address);
  if (bind(sfd, (struct sockaddr *) &my_addr,
        sizeof(struct sockaddr_in)) == -1)
    handle_error("bind");

  if (listen(sfd, LISTEN_BACKLOG) == -1)
    handle_error("listen");

  peer_addr_size = sizeof(struct sockaddr_in);

  for(;;) {
    cfd = accept(sfd, (struct sockaddr *) &peer_addr,
        &peer_addr_size);
    if (cfd == -1)
      handle_error("accept");

    Node* curr = head;
    while (curr->next != NULL) {
      curr = curr->next;
    }
    curr->next = (struct Node*) malloc(sizeof(Node));
    if (curr->next == NULL) {
      handle_error("error mallocing node");
    }
    curr = curr->next;
    curr->cfd = cfd;
    curr->name = (char*) malloc(8);
    if (curr->name == NULL) {
      handle_error("error mallocing initial name");
    }
    strncpy(curr->name, "unnamed\0", 8);

    pthread_t id;
    struct forwarder_args arg;
    arg.cfd = cfd;
    arg.mutex = &mutex;

    if (pthread_create(&id, NULL, forwarder, (void*) &arg) != 0) {
      handle_error("pthread_create error for forwarder");
    }

  }

  if (unlink(MY_SOCK_PATH)) {
    handle_error("unlink");
  }

}

