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
#include <time.h>

#include <fcntl.h>
//kqueue
#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>

#include "server.h"
#include "workqueue.h"

//TODO: remove accessors and have asserts at fn beginning instead?

void server_alloc_event(server_ctx_t * server_ctx) {
  struct kevent *m;

  //TODO: when deleting we should downsize
  x_num_connections_server_ctx_t(server_ctx) = num_connections_server_ctx_t(server_ctx) + 1;

  if (num_connections_server_ctx_t(server_ctx) >= avail_connections_server_ctx_t(server_ctx)) {
    int new_size = avail_connections_server_ctx_t(server_ctx) + SERVER_CTX_NUM_CONN_ALLOC;
    if(new_size == SERVER_CTX_NUM_CONN_ALLOC){
      printf("event malloc\n");
      m = malloc(new_size * sizeof(struct kevent));
    } else {
      printf("event realloc\n");
      m = realloc(events_server_ctx_t(server_ctx), new_size * sizeof(struct kevent));
    }
    if(m){
      x_events_server_ctx_t(server_ctx) = m;
      x_avail_connections_server_ctx_t(server_ctx) = new_size;
    } else {
      perror("malloc");
      abort();
    }
  }
}

int server_accept(server_ctx_t * sctx, connection_t * conn){
  socklen_t remote_size;
  int client_fd;
  struct sockaddr_un remote;
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

  //TODO: store in a bst too/instead - so we can dealloc properly at shutdown
  connection_t * client_conn = malloc(sizeof(connection_t));
  if(client_conn == NULL){
    perror("malloc");
    exit(errno);
  }
  x_fd_connection_t(client_conn) = client_fd;

  //setting default read write methods
  //TODO: restore read/write/error handlers
  x_action_connection_t(client_conn) = default_action_server_ctx_t(sctx);
  
  x_bytes_read_connection_t(client_conn) = 0;
  
  server_alloc_event(sctx);
  server_connection_enable_read(sctx, client_conn);

  return 1;
}

void server_destroy(server_ctx_t * sctx){
  printf("server destroy.\n");
  for (int i = 0; i < avail_connections_server_ctx_t(sctx); i++){
    printf("looping through connections: %d\n", i);

    struct kevent e = events_server_ctx_t(sctx)[i];

    //FIXME! udata can be garbage from allocation process - we must keep track of our conns ourselves 
    connection_t * conn = (connection_t *) e.udata;
    if(conn != NULL){ 
      printf("destroying connection with fd %d\n", conn->fd);
      if (fd_connection_t(conn) > 0 && (fcntl(fd_connection_t(conn), F_GETFD) != -1 || errno != EBADF)){
        printf("connection %d is open, attempting to close\n", fd_connection_t(conn));
        close(fd_connection_t(conn));

        //unpredictable FIXME
        //free(conn);
        //conn = NULL;
      }
    }
  }
  free(events_server_ctx_t(sctx));
  //TMP remove
  sleep(2);
}

void server_run(server_ctx_t * sctx){
  int new_events;

  while (1) {
    new_events = kevent(queue_server_ctx_t(sctx), 
                        NULL, 
                        0, 
                        events_server_ctx_t(sctx),
                        avail_connections_server_ctx_t(sctx), 
                        NULL);

    if (new_events < 0) {
      //control->c
      perror("kevent");
      goto out;
    }

    x_num_connections_server_ctx_t(sctx) = 0;

    for (int i = 0; i < new_events; i++) {

      struct kevent *e = &(events_server_ctx_t(sctx)[i]);
      assert(e != NULL);

      if( e->flags & EV_ERROR){
        //never called afaik
        perror("event");
        printf("e->data: %ld\n", e->data);
        //TODO exec an error handler?
        continue;
      }
      if( e->filter & EVFILT_PROC && e->fflags & NOTE_SIGNAL){
        //never called remove
        printf("got a signal\n");
      }

      connection_t * conn = (connection_t *) e->udata;
      //log this if this happens
      if (conn == NULL) continue;


      assert(conn->fd == e->ident);
      
      if(e->ident != sctx->fd){
        EV_SET(e, e->ident, e->filter, EV_DISABLE, 0, 0, conn);
        if (kevent(queue_server_ctx_t(sctx), e, 1, NULL, 0, NULL) < 0) {
          fprintf(stderr, "error %s:%d\n", __FILE__, __LINE__);
          perror("kevent");
          abort();
        }
      }

      server_wq_arg_t * task = (server_wq_arg_t *) malloc(sizeof(server_wq_arg_t));
      if(task == NULL) {
        perror("malloc");
        abort();
      }
      task->sctx = sctx;
      task->conn = conn;
      workqueue_add_task(sctx->w_queue, server_dispatch_connection_action, task);
    }
  }

out:
  workqueue_destroy(sctx->w_queue, NULL, NULL);
  printf("shutting down.\n");
  server_destroy(sctx);
}

void server_sig_break_loop(){
  //kevent will throw an error *sometimes* depending on state - see server_run
  //TODO: we need to stop loop too
  printf("sig break\n");
}

server_ctx_t * server_init(server_ctx_t * server_ctx, char * sock_path){
  struct sockaddr_un local;
  int s;

  if (signal(SIGINT, server_sig_break_loop) == SIG_ERR){
    printf("signal");
    exit(1);
  }

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
  if (listen(s, 32) == -1) {
    perror("listen");
    exit(errno);
  }

  workqueue_t * w_queue = workqueue_init(10);
  server_ctx->w_queue = w_queue;

  x_fd_server_ctx_t(server_ctx) = s;
  x_avail_connections_server_ctx_t(server_ctx) = 0;
  x_num_connections_server_ctx_t(server_ctx) = 0;


  //TODO: free this! (avoiding circular dependencies)
  connection_t * server_connection = malloc(sizeof(connection_t));
  if(server_connection == NULL){
    perror("malloc");
    exit(errno);
  }
  x_fd_connection_t(server_connection) = fd_server_ctx_t(server_ctx);
  x_action_connection_t(server_connection) = server_accept;

  server_alloc_event(server_ctx);
  server_connection_enable_read(server_ctx, server_connection);

  return server_ctx;
}

void server_dispatch_connection_action(void * arg){

  server_wq_arg_t * task = (server_wq_arg_t *) arg;
  assert(task != NULL);
  connection_t * conn = (connection_t *) task->conn;
  assert(conn != NULL);
  server_ctx_t * sctx = (server_ctx_t *) task->sctx;
  assert(sctx != NULL);

  if (action_connection_t(conn) != NULL) {
    while (action_connection_t(conn)(sctx, conn));
  }

  free(task);
}

//TODO: improve this interface
void server_connection_enable_read(server_ctx_t * sctx, connection_t * conn) {
  struct kevent evSet;
  assert(conn != NULL);
  assert(sctx != NULL);
  int clientfd = fd_connection_t(conn);

  EV_SET(&evSet, clientfd, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, conn);
  //TODO: macro (?)
  if (kevent(queue_server_ctx_t(sctx), &evSet, 1, NULL, 0, NULL) < 0) {
    fprintf(stderr, "error %s:%d\n", __FILE__, __LINE__);
    perror("kevent");
    abort();
  }
}

void server_connection_enable_write(server_ctx_t * sctx, connection_t * conn) {
  struct kevent evSet;
  assert(conn != NULL);
  assert(sctx != NULL);
  int clientfd = fd_connection_t(conn);

  EV_SET(&evSet, clientfd, EVFILT_WRITE, EV_ADD | EV_ENABLE, 0, 0, conn);
  //TODO: macro (?)
  if (kevent(queue_server_ctx_t(sctx), &evSet, 1, NULL, 0, NULL) < 0) {
    fprintf(stderr, "error %s:%d\n", __FILE__, __LINE__);
    perror("kevent");
    abort();
  }
}

void server_connection_disable_read(server_ctx_t * sctx, connection_t * conn) {
  struct kevent evSet;
  assert(conn != NULL);
  assert(sctx != NULL);
  int clientfd = fd_connection_t(conn);

  EV_SET(&evSet, clientfd, EVFILT_READ, EV_DISABLE, 0, 0, conn);
  //TODO: macro (?)
  if (kevent(queue_server_ctx_t(sctx), &evSet, 1, NULL, 0, NULL) < 0) {
    fprintf(stderr, "error %s:%d\n", __FILE__, __LINE__);
    perror("kevent");
    abort();
  }
}

//TODO: delete read??
void server_connection_delete_write(server_ctx_t * sctx, connection_t * conn) {
  struct kevent evSet;
  assert(conn != NULL);
  assert(sctx != NULL);
  int clientfd = fd_connection_t(conn);

  EV_SET(&evSet, clientfd, EVFILT_WRITE, EV_DELETE, 0, 0, NULL);
  //TODO: macro (?)
  if (kevent(queue_server_ctx_t(sctx), &evSet, 1, NULL, 0, NULL) < 0) {
    fprintf(stderr, "error %s:%d\n", __FILE__, __LINE__);
    perror("kevent");
    abort();
  }
  printf("closing and deleting %d\n", conn->fd);
  close(clientfd);
  free(conn);
}
