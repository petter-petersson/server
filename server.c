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
#include <stdbool.h>
#include <getopt.h>
#include <time.h>

#include <fcntl.h>
//kqueue
#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>

#include "server.h"

void sig_break_loop(int signo){
  printf("sig_break_loop: %d", signo);
  //shutting_down = true;
}

void print_help(const char * app_name){
  printf("\n");
  printf("\x1B[33mUsage: %s [-h] [-s socket]\n", app_name);
  printf("\t-s   Path to socket file. Deault file is <%s>\n", DEFAULT_SOCK_PATH);
  printf("\t-h   Print this info\x1B[0m\n");
  printf("\n");
}

void server_run(server_ctx_t * server_ctx){
  socklen_t remote_size;
  int client_fd;
  struct sockaddr_un remote;

  while(true) {
    remote_size = sizeof(remote);
    client_fd = accept(fd_server_ctx_t(server_ctx), (struct sockaddr *)&remote, &remote_size);
    if (client_fd < 0) {
      if (errno == EWOULDBLOCK || errno == EAGAIN) {
        continue;
      }
      perror("accept");
      exit(errno);
    }
  }
}

server_ctx_t * server_init(server_ctx_t * server_ctx, char * sock_path){
  struct sockaddr_un local;
  int s;

  x_socket_path_server_ctx_t(server_ctx) = sock_path;
  x_queue_server_ctx_t(server_ctx) = kqueue();

  if ((s = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
    perror("socket");
    exit(errno);
  }
  int o = 1;
  setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &o, sizeof(o));

  memset(&local, 0, sizeof(local));
  local.sun_family = AF_UNIX;
  strncpy(local.sun_path, sock_path, sizeof(local.sun_path)-1);
  unlink(local.sun_path);

  int flags = fcntl(s, F_GETFL, 0);
  if (flags < 0) {
    perror("F_GETFL");
    exit(errno);
  }
  if (fcntl(s, F_SETFL, flags | O_NONBLOCK) < 0) {
    perror("O_NONBLOCK");
    exit(errno);
  }

  if (bind(s, (struct sockaddr *) &local,
                     sizeof(struct sockaddr_un)) == -1) {
    perror("bind");
    exit(errno);
  }

  //default was 5
  if (listen(s, 30) == -1) {
    perror("listen");
    exit(errno);
  }

  x_fd_server_ctx_t(server_ctx) = s;

  return server_ctx;
}

int main(int argc, const char *argv[]) {

  int c;
  opterr = 0;
  char * sock_path = DEFAULT_SOCK_PATH;

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

  //todo: alloc since we will destroy other resources anyway later on
  server_ctx_t server_context;
  server_ctx_t * sctx;

  sctx = server_init(&server_context, sock_path);
  
  server_run(sctx);


  printf("Good bye.");
  return 0;
}
