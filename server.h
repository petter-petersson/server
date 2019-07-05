#ifndef __SERVER_H
#define __SERVER_H
#include "token_generator.h"

#define DEFAULT_SOCK_PATH "/tmp/jamboree.sock"

typedef struct server_ctx_s {
  token_generator_t * token_generator;
} server_ctx_t;

extern char * sock_path;

void * flush(void *arg);
void sig_break_loop(int signo);
void print_help(const char * app_name);


#endif
