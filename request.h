#ifndef __REQUEST_H
#define __REQUEST_H

#include "server.h"

#define REQ_BUF_SIZE 8 * 1024 //256

typedef struct req_arg_s {
  int fd;
  server_ctx_t * server_ctx;
} req_arg_t;

#ifdef DEBUG
#define deref_req_arg_t(_n) (assert((_n)!=0), (_n))
#else
#define deref_req_arg_t(_n) (_n)
#endif
#define x_server_ctx_req_arg_t(_n) (deref_req_arg_t(_n)->server_ctx)
#define server_ctx_req_arg_t(_n) ((void)0, x_server_ctx_req_arg_t(_n))
#define x_fd_req_arg_t(_n) (deref_req_arg_t(_n)->fd)
#define fd_req_arg_t(_n) ((void)0, x_fd_req_arg_t(_n))

void request_handler(void *arg);

#endif

