#ifndef __CONNECTION_H
#define __CONNECTION_H

#include "binary_search_tree.h"

struct server_ctx_s; //fwd

typedef struct connection_s {
  int fd; 
  int bytes_read;
  int (*action) (struct server_ctx_s *sctx, struct connection_s * conn);
} connection_t;

typedef struct connection_manager_s {
  bst_t * store;
} connection_manager_t;

#define x_fd_connection_t(_n) (deref_connection_t(_n)->fd)
#define fd_connection_t(_n) ((void)0, x_fd_connection_t(_n))

#define x_action_connection_t(_n) (deref_connection_t(_n)->action)
#define action_connection_t(_n) ((void)0, x_action_connection_t(_n))

#define x_bytes_read_connection_t(_n) (deref_connection_t(_n)->bytes_read)
#define bytes_read_connection_t(_n) ((void)0, x_bytes_read_connection_t(_n))


connection_t * connection_manager_get_connection(connection_manager_t * m, int fd);
connection_manager_t * connection_manager_init();
void connection_manager_destroy();
void connection_manager_delete_connection(connection_manager_t * m, connection_t * conn);

#endif
