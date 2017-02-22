#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

char* build_CSP_request(char rtt, int num_probes, size_t msg_size, int delay){
    //FORMAT <PROTOCOL PHASE><WS><M−TYPE><WS><PROBES><WS><MSG SIZE><WS><SERVER DELAY>
    char* CSP_request = (char*)malloc(1000);
    static char* mtype;
    if(rtt) 
      mtype = "RTT";
    else
      mtype = "tput";
    
    sprintf(CSP_request,"s %s %d %d %d", mtype, num_probes, msg_size, delay);
    printf("generated %s\n",CSP_request);
    return CSP_request;

}

typedef struct CSP_request{
  char* mtype;
  int num_probes;
  size_t msg_size;
  int delay;
} CSP_request;

CSP_request* parse_CSP_request(char* request_string){
  //FORMAT <PROTOCOL PHASE><WS><M−TYPE><WS><PROBES><WS><MSG SIZE><WS><SERVER DELAY>
  CSP_request* incoming = (CSP_request*)malloc(sizeof(CSP_request));
  incoming->mtype = (char*)malloc(256);
  incoming->num_probes = 0;
  incoming->msg_size = 0; 
  incoming->delay = 0;


  char* token = strtok(request_string, " ");
  printf("tokenized %s\n", token); 
  
  token = strtok(NULL, " ");
  incoming->mtype = token;
  printf("tokenized %s\n", token); 
  
  token = atoi(strtok(NULL," "));
  incoming->num_probes = token;
  printf("tokenized %d\n", token); 
  
  token = atoi(strtok(NULL," "));
  incoming->msg_size = token;
  printf("tokenized %d\n", token); 
  
  token = atoi(strtok(NULL," "));
  incoming->delay = token; 
  printf("tokenized %d\n", token); 
  
  return incoming;
}



int main(){
  char* request;
  CSP_request* encoded;
  printf("building request\n");
  request = build_CSP_request(1, 1, 1, 0);
  char* request_copy = (char*) malloc(256);
  memcpy(request_copy,request,256);
  printf("decoding request \"%s\"\n",request);
  encoded = parse_CSP_request(request);
  printf("printing result\n");
  char* result = (char*)malloc(256);
  sprintf(result,"s %s %d %d %d", encoded->mtype, encoded->num_probes, encoded->msg_size, encoded->delay);
  if(strcmp(result,request_copy))
    printf("ERROR WITH STRINGS result:\"%s\" and requet\"%s\"\n",result, request);
}
