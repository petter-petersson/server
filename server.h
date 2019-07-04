#ifndef __SERVER_H
#define __SERVER_H

#define DEFAULT_SOCK_PATH "/tmp/jamboree.sock"

extern int running;
extern char * sock_path;

void * flush(void *arg);
void sig_break_loop(int signo);
void print_help(const char * app_name);

#endif
