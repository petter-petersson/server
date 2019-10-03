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
//TODO: update threadpool lib and remove
#include "workqueue.h"

//TODO: remove accessors and have asserts at fn beginning instead?

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

  connection_t * client_conn = connection_manager_get_connection(
      connection_manager_server_ctx_t(sctx), client_fd);

  //TODO: restore read/write/error handlers
  //TODO: `create` method so we don't have to do this?
  x_action_connection_t(client_conn) = default_action_server_ctx_t(sctx);
  server_connection_enable_read(sctx, client_conn);

  return 1;
}

void server_destroy(server_ctx_t * sctx){
  printf("server destroy.\n");
  //TMP remove(?)
  sleep(2);
  connection_manager_destroy(connection_manager_server_ctx_t(sctx));
}

void server_run(server_ctx_t * sctx){
  int new_events;

  while (1) {
    new_events = kevent(queue_server_ctx_t(sctx), 
                        NULL, 
                        0, 
                        sctx->events,
                        SERVER_CTX_NUM_EVENTS, 
                        NULL);

    if (new_events < 0) {
      //control->c
      perror("kevent");
      goto out;
    }

    //printf("new events: %d\n", new_events);
    for (int i = 0; i < new_events && i < SERVER_CTX_NUM_EVENTS-1; i++) {

      //printf("event list: %d\n", i);
      assert(i < (SERVER_CTX_NUM_EVENTS-1));

      struct kevent e = sctx->events[i];

      if( e.flags & EV_ERROR){
        //never called?
        perror("event");
        printf("e.data: %ld\n", e.data);
        //TODO exec an error handler?
        continue;
      }

      connection_t * conn = connection_manager_get_connection(
        connection_manager_server_ctx_t(sctx), e.ident);

      if(!conn){
        fprintf(stderr, "Connection manager did not return connection obj for fd %ld\n", e.ident);
        abort();
      }
      assert(fd_connection_t(conn) == e.ident);
      
      if(e.ident != fd_server_ctx_t(sctx)){
        EV_SET(&e, e.ident, e.filter, EV_DISABLE, 0, 0, NULL);
        if (kevent(queue_server_ctx_t(sctx), &e, 1, NULL, 0, NULL) < 0) {
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

  x_connection_manager_server_ctx_t(server_ctx) = connection_manager_init();
  //or maybe create connection after all..
  connection_t * server_connection = connection_manager_get_connection(
      connection_manager_server_ctx_t(server_ctx), s);

  x_action_connection_t(server_connection) = server_accept;
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

  EV_SET(&evSet, clientfd, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, NULL);
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

  EV_SET(&evSet, clientfd, EVFILT_WRITE, EV_ADD | EV_ENABLE, 0, 0, NULL);
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

  EV_SET(&evSet, clientfd, EVFILT_READ, EV_DISABLE, 0, 0, NULL);
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
  connection_manager_delete_connection(connection_manager_server_ctx_t(sctx), conn);
}
