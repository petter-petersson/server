#ifndef __SERVER_H
#define __SERVER_H

#include <assert.h>
#include <pthread.h>

#include "workqueue.h"
#include "connection.h"

#define SERVER_CTX_NUM_CONN_ALLOC 8

typedef struct server_ctx_s {
  int queue;
  int fd;
  char * socket_path;
  int avail_connections;
  int num_connections;
  struct kevent * events;
  int (*default_action) (struct server_ctx_s *sctx, connection_t * conn);
  workqueue_t * w_queue;
  connection_manager_t * connection_manager;
} server_ctx_t;

typedef struct server_wq_arg_s {
  server_ctx_t * sctx;
  connection_t * conn;
} server_wq_arg_t;

#ifdef DEBUG
#define deref_server_ctx_t(_n) (assert((_n)!=0), (_n))
#define deref_connection_t(_n) (assert((_n)!=0), (_n))
#else
#define deref_server_ctx_t(_n) (_n)
#define deref_connection_t(_n) (_n)
#endif
#define x_queue_server_ctx_t(_n) (deref_server_ctx_t(_n)->queue)
#define queue_server_ctx_t(_n) ((void)0, x_queue_server_ctx_t(_n))

#define x_fd_server_ctx_t(_n) (deref_server_ctx_t(_n)->fd)
#define fd_server_ctx_t(_n) ((void)0, x_fd_server_ctx_t(_n))

#define x_socket_path_server_ctx_t(_n) (deref_server_ctx_t(_n)->socket_path)
#define socket_path_server_ctx_t(_n) ((void)0, x_socket_path_server_ctx_t(_n))

#define x_num_connections_server_ctx_t(_n) (deref_server_ctx_t(_n)->num_connections)
#define num_connections_server_ctx_t(_n) ((void)0, x_num_connections_server_ctx_t(_n))

#define x_avail_connections_server_ctx_t(_n) (deref_server_ctx_t(_n)->avail_connections)
#define avail_connections_server_ctx_t(_n) ((void)0, x_avail_connections_server_ctx_t(_n))

#define x_events_server_ctx_t(_n) (deref_server_ctx_t(_n)->events)
#define events_server_ctx_t(_n) ((void)0, x_events_server_ctx_t(_n))

#define x_default_action_server_ctx_t(_n) (deref_server_ctx_t(_n)->default_action)
#define default_action_server_ctx_t(_n) ((void)0, x_default_action_server_ctx_t(_n))

#define x_connection_manager_server_ctx_t(_n) (deref_server_ctx_t(_n)->connection_manager)
#define connection_manager_server_ctx_t(_n) ((void)0, x_connection_manager_server_ctx_t(_n))

#define server_error(...) do { \
  fprintf(stderr, __VA_ARGS__); exit(1); \
} while (0)

#define server_debug_print(_n) do { \
  printf("allocated connections: %d\n", avail_connections_server_ctx_t(_n)); \
  printf("active connections: %d\n", num_connections_server_ctx_t(_n)); \
  printf("server fd: %d\n", fd_server_ctx_t(_n)); \
  printf("--\n"); \
} while (0)

//TODO: remove non-public methods
void server_alloc_event(server_ctx_t * server_ctx);
int server_accept(server_ctx_t * sctx, connection_t * conn);
void server_run(server_ctx_t * sctx);
server_ctx_t * server_init(server_ctx_t * server_ctx, char * sock_path);
void server_dispatch_connection_action(void * arg);

//TODO: improve this interface
void server_connection_enable_read(server_ctx_t * sctx, connection_t * conn);
void server_connection_enable_write(server_ctx_t * sctx, connection_t * conn);
void server_connection_disable_read(server_ctx_t * sctx, connection_t * conn);
void server_connection_delete_write(server_ctx_t * sctx, connection_t * conn);
#endif
