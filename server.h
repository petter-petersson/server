#ifndef __SERVER_H
#define __SERVER_H

#include <assert.h>

#define DEFAULT_SOCK_PATH "/tmp/jamboree.sock"

typedef struct server_ctx_s {
  int queue;
  int fd;
  char * socket_path;
} server_ctx_t;

#ifdef DEBUG
#define deref_server_ctx_t(_n) (assert((_n)!=0), (_n))
#else
#define deref_server_ctx_t(_n) (_n)
#endif
#define x_queue_server_ctx_t(_n) (deref_server_ctx_t(_n)->queue)
#define queue_server_ctx_t(_n) ((void)0, x_queue_server_ctx_t(_n))
#define x_fd_server_ctx_t(_n) (deref_server_ctx_t(_n)->fd)
#define fd_server_ctx_t(_n) ((void)0, x_fd_server_ctx_t(_n))
#define x_socket_path_server_ctx_t(_n) (deref_server_ctx_t(_n)->socket_path)
#define socket_path_server_ctx_t(_n) ((void)0, x_socket_path_server_ctx_t(_n))

#define server_error(...) do { \
  fprintf(stderr, __VA_ARGS__); exit(1); \
} while (0)

extern char * sock_path;

void * flush(void *arg);
void sig_break_loop(int signo);
void print_help(const char * app_name);


#endif
