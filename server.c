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

#include <fcntl.h>
//kqueue
#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>

#include "server.h"

bool shutting_down = false;
char * sock_path;

int queue; //todo: move to context
int s; //server fd, move to context and rename.

void sig_break_loop(int signo){
  printf("sig_break_loop: %d", signo);
  shutting_down = true;
}

void print_help(const char * app_name){
  printf("\n");
  printf("\x1B[33mUsage: %s [-h] [-s socket]\n", app_name);
  printf("\t-s   Path to socket file. Deault file is <%s>\n", DEFAULT_SOCK_PATH);
  printf("\t-h   Print this info\x1B[0m\n");
  printf("\n");
}

void server_init(){
  sock_path = DEFAULT_SOCK_PATH;
  //unsigned int t;
  struct sockaddr_un local;//, remote;

  queue = kqueue();

  if ((s = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
    fprintf(stderr, "Failed to create socket: %s", strerror(errno));
    exit(1);
  }
  int o = 1;
  setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &o, sizeof(o));

  memset(&local, 0, sizeof(local));
  local.sun_family = AF_UNIX;
  strncpy(local.sun_path, sock_path, sizeof(local.sun_path)-1);
  unlink(local.sun_path);

  int flags = fcntl(s, F_GETFL, 0);
  if (flags < 0) {
    server_error("F_GETFL: %s\n", strerror(errno));
  }
  if (fcntl(s, F_SETFL, flags | O_NONBLOCK) < 0) {
    server_error("O_NONBLOCK: %s\n", strerror(errno));
  }

  if (bind(s, (struct sockaddr *) &local,
                     sizeof(struct sockaddr_un)) == -1) {
    server_error("Failed to bind. %s\n", strerror(errno));
  }

  //default was 5
  if (listen(s, 30) == -1) {
    server_error("Failed to listen. %s\n", strerror(errno));
  }
}

int main(int argc, const char *argv[]) {

  int c;
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

  server_ctx_t server_context;
  
  //TODO: is this needed?
  //preventing client crash/abort to end this program:
  if (signal(SIGINT, sig_break_loop) == SIG_ERR){
    printf("signal");
    exit(1);
  }
  signal(SIGPIPE, SIG_IGN);


  while(1) {
    t = sizeof(remote);
    if ((s2 = accept(s, (struct sockaddr *)&remote, &t)) == -1) {
      fprintf(stderr, "accept call failed");
      exit(1);
    }
  }

  printf("Good bye.");
  return 0;
}
