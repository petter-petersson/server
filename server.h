#ifndef __SERVER_H
#define __SERVER_H

#include <assert.h>

#define DEFAULT_SOCK_PATH "/tmp/jamboree.sock"
#define SERVER_CTX_NUM_CONN_ALLOC 8


typedef struct server_ctx_s {
  int queue;
  int fd;
  char * socket_path;
  int avail_connections;
  int num_connections;
  struct kevent * events;
} server_ctx_t;

typedef struct connection_s {
  int fd;
  int (*read) (server_ctx_t *sctx, struct kevent *event);
} connection_t;

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

#define x_fd_connection_t(_n) (deref_connection_t(_n)->fd)
#define fd_connection_t(_n) ((void)0, x_fd_connection_t(_n))
#define x_read_connection_t(_n) (deref_connection_t(_n)->read)
#define read_connection_t(_n) ((void)0, x_read_connection_t(_n))

#define server_error(...) do { \
  fprintf(stderr, __VA_ARGS__); exit(1); \
} while (0)

#define server_debug_print(_n) do { \
  printf("allocated connections: %d\n", avail_connections_server_ctx_t(_n)); \
  printf("active connections: %d\n", num_connections_server_ctx_t(_n)); \
  printf("server fd: %d\n", fd_server_ctx_t(_n)); \
} while (0)

#endif
