#ifndef __CONNECTION_H
#define __CONNECTION_H

#include "server.h"

//separate file + init & destroy method
typedef struct connection_s {
  int fd; // redundant since we have event->ident
  int bytes_read;
  int (*read) (server_ctx_t *sctx, struct kevent *event);
  int (*write) (server_ctx_t *sctx, struct kevent *event);

  int (*disconnect) (server_ctx_t *sctx, struct kevent *event);
} connection_t;

//connection_t accessors
#define x_fd_connection_t(_n) (deref_connection_t(_n)->fd)
#define fd_connection_t(_n) ((void)0, x_fd_connection_t(_n))

#define x_read_connection_t(_n) (deref_connection_t(_n)->read)
#define read_connection_t(_n) ((void)0, x_read_connection_t(_n))

#define x_write_connection_t(_n) (deref_connection_t(_n)->write)
#define write_connection_t(_n) ((void)0, x_write_connection_t(_n))

#define x_disconnect_connection_t(_n) (deref_connection_t(_n)->disconnect)
#define disconnect_connection_t(_n) ((void)0, x_disconnect_connection_t(_n))

#define x_bytes_read_connection_t(_n) (deref_connection_t(_n)->bytes_read)
#define bytes_read_connection_t(_n) ((void)0, x_bytes_read_connection_t(_n))
#endif
