#ifndef __SERVER_H
#define __SERVER_H

#define DEFAULT_SOCK_PATH "/tmp/jamboree.sock"

typedef struct server_ctx_s {
} server_ctx_t;

#ifdef DEBUG
#define deref_server_ctx_t(_n) (assert((_n)!=0), (_n))
#else
#define deref_server_ctx_t(_n) (_n)
#endif

#define server_error(...) do { \
  fprintf(stderr, __VA_ARGS__); exit(1); \
} while (0)

extern char * sock_path;

void * flush(void *arg);
void sig_break_loop(int signo);
void print_help(const char * app_name);


#endif
