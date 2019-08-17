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

//FIXME:
workqueue_t * w_queue;
//TODO: remove accessors and have asserts at fn beginning instead?

void server_alloc_event(server_ctx_t * server_ctx) {
  struct kevent *m;

  //TODO: when deleting we should downsize
  x_num_connections_server_ctx_t(server_ctx) = num_connections_server_ctx_t(server_ctx) + 1;

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

  connection_t * client_conn = malloc(sizeof(connection_t));
  if(client_conn == NULL){
    perror("malloc");
    exit(errno);
  }
  x_fd_connection_t(client_conn) = client_fd;
  //TODO: store in a bst instead of udata?

  //setting default read write methods
  x_action_connection_t(client_conn) = default_action_server_ctx_t(sctx);
  
  x_bytes_read_connection_t(client_conn) = 0;
  
  server_alloc_event(sctx);
  server_connection_enable_read(sctx, client_conn);

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
      //log this if this happens
      if (conn == NULL) continue;

      if( e->flags & EV_ERROR){
        perror("event");
        printf("e->data: %ld\n", e->data);
        //TODO exec an error handler
        continue;
      }
      assert(conn->fd == e->ident);
      
      if(e->ident != sctx->fd){
        EV_SET(e, e->ident, e->filter, EV_DISABLE, 0, 0, conn);
        if (kevent(queue_server_ctx_t(sctx), e, 1, NULL, 0, NULL) < 0) {
          fprintf(stderr, "error %s:%d\n", __FILE__, __LINE__);
          perror("kevent");
          abort();
        }
      }
      server_add_task_to_workqueue(w_queue, conn);

      /*
      if(e->flags & EV_EOF){
        printf("kqueue EV_EOF\n");
      } 
      // e->filter == EVFILT_WRITE
      */
      //TODO!!! pass filter value to action/connection ??
      // or - add default error handler, default read + write (again)
      // to server ctx so we can switch on those here.. that would make impl.
      // less versatile though - we still need an error handler in any event.
      //
      //if (action_connection_t(conn) != NULL) {
      //  while (action_connection_t(conn)(sctx, conn));
      //}
      //
    }
    pthread_mutex_lock(&(w_queue->mutex));
    //while(w_queue->t_head != NULL || w_queue->available_threads < 1){
    while(w_queue->available_threads < 1){
      printf("waiting for available threads\n");
      pthread_cond_wait(&(w_queue->thread_available), &(w_queue->mutex));
    }
    pthread_mutex_unlock(&(w_queue->mutex));
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
  //this value is only a request too.
  if (listen(s, 64) == -1) {
    perror("listen");
    exit(errno);
  }

  //just one workqueue to begin with
  //FIXME free / destroy method (+ init method) + move to separate files!
  w_queue = (workqueue_t *) malloc(sizeof(workqueue_t));
  if(w_queue == NULL){
    fprintf(stderr, "Unable to create workqueue\n");
    abort();
  }
  w_queue->t_head = NULL;
  w_queue->t_tail = NULL;
  pthread_mutex_init(&(w_queue->mutex), NULL);
  pthread_cond_init(&(w_queue->q_not_empty), NULL);
  pthread_cond_init(&(w_queue->thread_available), NULL);

  w_queue->sctx = server_ctx;

  //TODO: constant
  int num_threads = 19;
  w_queue->work_threads = (pthread_t *)malloc(num_threads * sizeof(pthread_t));
  if(w_queue->work_threads == NULL) {
    perror("malloc");
    abort();
  }
  for(int i = 0; i < num_threads; i++) {
    if (pthread_create(&(w_queue->work_threads[i]), NULL, server_thread_work, w_queue) != 0) {
      perror("pthread_created");
      abort();
    }
  } 
  w_queue->available_threads = num_threads;
  //END w_queue

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

void server_add_task_to_workqueue(workqueue_t * w_queue, connection_t * conn) {

  //printf("server_add_task_to_workqueue\n");

  assert(w_queue != NULL);

  task_t *current_task;
  current_task = (task_t *) malloc(sizeof(task_t));
  if(current_task == NULL) {
    fprintf(stderr, "Failed to create task\n");
    return;
  }
  current_task->connection = conn;
  current_task->next = NULL;

  pthread_mutex_lock(&(w_queue->mutex));

  if (w_queue->t_head == NULL) {
    w_queue->t_head = current_task;
  } else {
    w_queue->t_tail->next = current_task;
  }
  w_queue->t_tail = current_task;

  //WORKQUEUE_DISPLAY_TASKS(w_queue);
  pthread_cond_signal(&(w_queue->q_not_empty));
  pthread_mutex_unlock(&(w_queue->mutex));
}

void * server_thread_work(void * arg){
  workqueue_t * wq = (workqueue_t *) arg;
  task_t * current;

  printf("server_thread_work\n");
  assert(wq != NULL);

  while(1) {

    pthread_mutex_lock(&(wq->mutex));
    while(wq->t_head == NULL){
      pthread_cond_wait(&(wq->q_not_empty), &(wq->mutex));
    }

    current = wq->t_head;
    wq->t_head = current->next;

    if (wq->t_head == NULL) {
      wq->t_tail = NULL;
    }

    wq->available_threads--;
    assert(wq->available_threads >= 0);
    
    pthread_mutex_unlock(&(wq->mutex));

    //printf("exec connection methods %p from %p\n", current, pthread_self());

    connection_t * conn = current->connection;
    assert(conn != NULL);

    if (action_connection_t(conn) != NULL) {
      while (action_connection_t(conn)(wq->sctx, conn));
    }
    //todo: on_exec_complete(pthread_t, arg)  ???

    pthread_mutex_lock(&(wq->mutex));
    wq->available_threads++;
    pthread_cond_signal(&(w_queue->thread_available));
    pthread_mutex_unlock(&(wq->mutex));
    
    free(current);
    current = NULL;
  }
}

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
  close(clientfd);
  free(conn);
}
