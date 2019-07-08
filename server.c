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

void server_help(const char * app_name){
  printf("\n");
  printf("\x1B[33mUsage: %s [-h] [-s socket]\n", app_name);
  printf("\t-s   Path to socket file. Deault file is <%s>\n", DEFAULT_SOCK_PATH);
  printf("\t-h   Print this info\x1B[0m\n");
  printf("\n");
}

//change init to update
void server_update_connection(server_ctx_t * server_ctx, int id, int filter, int flags, void *udata) {
  struct kevent evSet;
  struct kevent *m;

  if(flags & EV_DELETE) {
    printf("server_update_connection on id: %d\n", id);
    printf("udata: %p\n", udata);
    server_debug_print(server_ctx);
  }

  //FIXME: this should not automatically bumped? (if we delete for instance)
  x_num_connections_server_ctx_t(server_ctx) = num_connections_server_ctx_t(server_ctx) + 1;

  if(avail_connections_server_ctx_t(server_ctx) == 0){
    x_avail_connections_server_ctx_t(server_ctx) = SERVER_CTX_NUM_CONN_ALLOC;
    m = malloc(avail_connections_server_ctx_t(server_ctx) * sizeof(struct kevent));
    if(m){
      x_events_server_ctx_t(server_ctx) = m;
    } else {
      perror("malloc");
      abort();
    }
  }
  //todo downsize too?
  if (num_connections_server_ctx_t(server_ctx) >= avail_connections_server_ctx_t(server_ctx)) {
    int new_size = avail_connections_server_ctx_t(server_ctx) + SERVER_CTX_NUM_CONN_ALLOC;
    m = realloc(events_server_ctx_t(server_ctx), new_size * sizeof(struct kevent));
    if(m){
      x_events_server_ctx_t(server_ctx) = m;
      x_avail_connections_server_ctx_t(server_ctx) = new_size;
    } else {
      perror("malloc");
      abort();
    }
  }

  printf("EV_SET on id %d\n", id);
  EV_SET(&evSet, id, filter, flags, 0, 0, udata);
  if (kevent(queue_server_ctx_t(server_ctx), &evSet, 1, NULL, 0, NULL) < 0) {
    fprintf(stderr, "error %s:%d\n", __FILE__, __LINE__);
    perror("kevent");
    abort();
  }
}

//why is this func called prematurely?
int server_disconnect(server_ctx_t * sctx, struct kevent *event){

  assert(event != NULL);
  assert(event->udata != NULL);
  connection_t * conn = (connection_t *)event->udata;
  int clientfd = fd_connection_t(conn);

  assert(fd_connection_t(conn) == event->ident);
  //printf("server_disconnect %ld\n", event->ident);
  //
  //server_update_connection(sctx, event->ident, EVFILT_READ, EV_DELETE, NULL);
  //close(clientfd);
  return 0;
}

int server_write(server_ctx_t * sctx, struct kevent *event){

  assert(event != NULL);
  assert(event->udata != NULL);
  connection_t * conn = (connection_t *)event->udata;
  int clientfd = fd_connection_t(conn);

  assert(fd_connection_t(conn) == event->ident);
  printf("server_write %ld\n", event->ident);

  FILE * fd = fdopen(dup(clientfd), "w");
  if(fd){
    fprintf(fd,
        "Job <%d> init. received %d bytes in transfer. Goodbye.\n",
        clientfd,
        bytes_read_connection_t(conn));
    fflush(fd);
    fclose(fd);
  }
  server_update_connection(sctx, event->ident, EVFILT_WRITE, EV_DELETE, conn);
  close(clientfd);
  free(conn);
  return 0;
}

int server_read(server_ctx_t * sctx, struct kevent *event){
  char buffer[BUFSIZ];
  ssize_t n = 0;

  assert(event != NULL);
  assert(event->udata != NULL);
  connection_t * conn = (connection_t *)event->udata;
  int clientfd = fd_connection_t(conn);

  assert(fd_connection_t(conn) == event->ident);

  if( event->flags & EV_ERROR){
    perror("event");
    printf("event->data: %ld\n", event->data);
    abort(); //or?
  }

  if ((n = recv(clientfd, buffer, sizeof(buffer), 0)) <= 0) {
    if (n == 0) {
      
      printf("%ld is done. Read %d bytes\n", event->ident, bytes_read_connection_t(conn));
      server_update_connection(sctx, event->ident, EVFILT_READ, EV_DISABLE, conn);
      server_update_connection(sctx, event->ident, EVFILT_WRITE, EV_ADD | EV_ENABLE, conn);
      //close(clientfd);
      //free(conn);

      return 0;
    } else {
      perror("recv");
      abort();
    }
  }
  x_bytes_read_connection_t(conn) = bytes_read_connection_t(conn) + n;
  return 0;
}

int server_accept(server_ctx_t * sctx, struct kevent *event){
  socklen_t remote_size;
  int client_fd;
  struct sockaddr_un remote;

  connection_t * conn = (connection_t * )event->udata;
  assert(conn != NULL);
  
  if(fd_connection_t(conn) != fd_server_ctx_t(sctx)){
    printf("unexpected: not a valid accept event\n");
    return 0;
  }
  
  remote_size = sizeof(remote);
  //(todo use conn->fd)
  client_fd = accept(fd_server_ctx_t(sctx), (struct sockaddr *)&remote, &remote_size);
  if (client_fd < 0) {
    if (errno == EWOULDBLOCK || errno == EAGAIN) {
      return 0;
    }
    perror("accept");
    exit(errno);
  }

  int flags = fcntl(client_fd, F_GETFL, 0);
  if (flags < 0) {
    perror("F_GETFL");
    exit(errno);
  }
  if (fcntl(client_fd, F_SETFL, flags | O_NONBLOCK) < 0) {
    perror("O_NONBLOCK");
    exit(errno);
  }

  connection_t * client_conn = malloc(sizeof(connection_t));
  if(client_conn == NULL){
    perror("malloc");
    abort();
  }
  x_fd_connection_t(client_conn) = client_fd;
  x_read_connection_t(client_conn) = server_read;
  x_write_connection_t(client_conn) = server_write;
  x_disconnect_connection_t(client_conn) = server_disconnect;
  x_bytes_read_connection_t(client_conn) = 0;
  server_update_connection(sctx, client_fd, EVFILT_READ, EV_ADD | EV_ENABLE, client_conn);
  //??
  server_update_connection(sctx, client_fd, EVFILT_WRITE, EV_ADD | EV_DISABLE, client_conn);

  return 1;
}

void server_run(server_ctx_t * sctx){

  printf("server_run\n");
  int new_events;

  while (1) {
    new_events = kevent(queue_server_ctx_t(sctx), 
                        NULL, 
                        0, 
                        events_server_ctx_t(sctx),
                        avail_connections_server_ctx_t(sctx), 
                        NULL);

    if (new_events < 0) {
      perror("kevent");
      abort();
    }
    x_num_connections_server_ctx_t(sctx) = 0;

    for (int i = 0; i < new_events; i++) {
      struct kevent *e = &(events_server_ctx_t(sctx)[i]);
      assert(e != NULL);
      connection_t * conn = (connection_t *) e->udata;

      if (conn == NULL) continue;
      //todo: remove disconnect call since it seems to not
      //occur after recv returns 0
      if (disconnect_connection_t(conn) != NULL && e->flags & EV_EOF) {
        while (disconnect_connection_t(conn)(sctx, e));
      }
      if (read_connection_t(conn) != NULL && e->filter == EVFILT_READ) {
        while (read_connection_t(conn)(sctx, e));
      }
      if (write_connection_t(conn) != NULL && e->filter == EVFILT_WRITE) {
        while (write_connection_t(conn)(sctx, e));
      }
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

  // setting timeout options on socket:
  struct timeval timeout;
  timeout.tv_sec = 10;
  timeout.tv_usec = 0;
  if (setsockopt (s, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout,
        sizeof(timeout)) < 0) {
    perror("setsockopt");
    exit(errno);
  }
  if (setsockopt (s, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout,
        sizeof(timeout)) < 0) {
    perror("setsockopt");
    exit(errno);
  }


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

  //usual value of 5 ignored, backlog max is 128 on BSD.
  if (listen(s, 64) == -1) {
    perror("listen");
    exit(errno);
  }

  x_fd_server_ctx_t(server_ctx) = s;
  x_avail_connections_server_ctx_t(server_ctx) = 0;
  x_num_connections_server_ctx_t(server_ctx) = 0;

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
        server_help(argv[0]);
        exit(0);
        break;
    }
  }
  //todo: alloc since we will destroy other resources anyway later on
  server_ctx_t server_context;
  server_ctx_t * sctx;

  sctx = server_init(&server_context, sock_path);

  connection_t server_connection = {
    .fd = fd_server_ctx_t(sctx),
    .read = server_accept,
    .write = NULL,
    .disconnect = NULL
  };
  server_update_connection(sctx, fd_server_ctx_t(sctx), EVFILT_READ, EV_ADD | EV_ENABLE, &server_connection);
  
  server_run(sctx);


  printf("Good bye.");
  return 0;
}
