#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <stdbool.h>
#include <getopt.h>
#include <time.h>

#include "server.h"
#include "request.h"
#include "threadpool.h"
#include "token_generator.h"

bool shutting_down = false;
char * sock_path;

// In order to move past accept we need this function.
void * flush(void *arg){
  printf("Shutting down, sending internal signal: %s.\n", sock_path);

  int s;
  struct sockaddr_un remote;

  if ((s = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
    printf("socket");
    exit(1);
  }

  remote.sun_family = AF_UNIX;
  strcpy(remote.sun_path, sock_path);
  if (connect(s, (struct sockaddr *)&remote, sizeof(struct sockaddr_un)) == -1) {
    printf("connect");
    exit(1);
  }

  int ret;
  if ((ret = send(s, 0, 0, 0)) == -1) {
    printf("send");
    exit(1);
  }
  close(s);

  return 0;
}


void sig_break_loop(int signo){
  printf("sig_break_loop: %d", signo);
  shutting_down = true;
  flush(NULL); //will trigger accept by hopefully accepting a final connetion
}

void print_help(const char * app_name){
  printf("\n");
  printf("\x1B[33mUsage: %s [-h] [-s socket]\n", app_name);
  printf("\t-s   Path to socket file. Deault file is <%s>\n", DEFAULT_SOCK_PATH);
  printf("\t-h   Print this info\x1B[0m\n");
  printf("\n");
}

int main(int argc, const char *argv[]) {

  struct timeval timeout;
  int c;
  sock_path = DEFAULT_SOCK_PATH;
  threadpool * pool;
  int s, s2;
  unsigned int t;
  struct sockaddr_un local, remote;
  req_arg_t * rarg = NULL;

  // Parse options
  opterr = 0;
  while ((c = getopt (argc, (char **)argv, "hs:")) != -1) {
    switch (c)
    {
      case 's':
        sock_path = optarg; 
        break;
      case 'h':
      case '?':
      default:
        print_help(argv[0]);
        exit(0);
        break;
    }
  }
  //setup context
  server_ctx_t server_context;
  token_generator_t generator;
  token_generator_init(&generator);
  server_context.token_generator = &generator;
  
  //TODO: is this needed?
  //preventing client crash/abort to end this program:
  if (signal(SIGINT, sig_break_loop) == SIG_ERR){
    printf("signal");
    exit(1);
  }
  signal(SIGPIPE, SIG_IGN);

  //socket..
  if ((s = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
    fprintf(stderr, "Failed to create socket");
    exit(1);
  }
  // setting timeout options on socket:
  timeout.tv_sec = 10;
  timeout.tv_usec = 0;
  if (setsockopt (s, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout,
                sizeof(timeout)) < 0) {
    fprintf(stderr, "setsockopt failed\n");
  }

  if (setsockopt (s, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout,
        sizeof(timeout)) < 0) {
    fprintf(stderr, "setsockopt failed\n");
  }

  //binding..
  memset(&local, 0, sizeof(local));
  local.sun_family = AF_UNIX;
  strncpy(local.sun_path, sock_path, sizeof(local.sun_path)-1);
  unlink(local.sun_path);
  if (bind(s, (struct sockaddr *) &local,
                     sizeof(struct sockaddr_un)) == -1) {

    fprintf(stderr, "Failed to bind");
    exit(1);
  }

  //listen.. default was 5 nb
  if (listen(s, 30) == -1) {
    fprintf(stderr, "Listen call failed");
    exit(1);
  }

  //num worker threads that receive requests
  pool = threadpool_create(5);

  while(1) {
    t = sizeof(remote);
    if ((s2 = accept(s, (struct sockaddr *)&remote, &t)) == -1) {
      fprintf(stderr, "accept call failed");
      exit(1);
    }
    if(shutting_down){
      goto shutdown;
    }
    
    rarg = malloc(sizeof(req_arg_t));
    if(rarg == NULL){
      fprintf(stderr, "malloc failure\n");
      exit(1);
    }
    rarg->fd = s2;
    threadpool_dispatch(pool, request_handler, rarg);
  }

shutdown:
  threadpool_destroy(pool);

  printf("Good bye.");
  sleep(1);
  return 0;
}
