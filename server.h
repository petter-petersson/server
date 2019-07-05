#ifndef __SERVER_H
#define __SERVER_H
#include "token_generator.h"

#define DEFAULT_SOCK_PATH "/tmp/jamboree.sock"

typedef struct server_ctx_s {
  token_generator_t * token_generator;
} server_ctx_t;

#ifdef DEBUG
#define deref_server_ctx_t(_n) (assert((_n)!=0), (_n))
#else
#define deref_server_ctx_t(_n) (_n)
#endif
#define x_token_generator_server_ctx_t(_n) (deref_server_ctx_t(_n)->token_generator)
#define token_generator_server_ctx_t(_n) ((void)0, x_token_generator_server_ctx_t(_n))

extern char * sock_path;

void * flush(void *arg);
void sig_break_loop(int signo);
void print_help(const char * app_name);


#endif
