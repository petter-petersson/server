#ifndef __REQUEST_H
#define __REQUEST_H

#define REQ_BUF_SIZE 8 * 1024 //256

typedef struct req_arg_s {
  int fd;
} req_arg_t;

void request_handler(void *arg);

#endif

