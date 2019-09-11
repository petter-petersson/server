#ifndef __CONNECTION_H
#define __CONNECTION_H

#include <assert.h>

struct server_ctx_s; //fwd

typedef struct connection_s {
  int fd; 
  int bytes_read;
  int (*action) (struct server_ctx_s *sctx, struct connection_s * conn);
} connection_t;

#define x_fd_connection_t(_n) (deref_connection_t(_n)->fd)
#define fd_connection_t(_n) ((void)0, x_fd_connection_t(_n))

#define x_action_connection_t(_n) (deref_connection_t(_n)->action)
#define action_connection_t(_n) ((void)0, x_action_connection_t(_n))

#define x_bytes_read_connection_t(_n) (deref_connection_t(_n)->bytes_read)
#define bytes_read_connection_t(_n) ((void)0, x_bytes_read_connection_t(_n))

#endif
