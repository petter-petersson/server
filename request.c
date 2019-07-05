#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <pthread.h>
#include <stdbool.h>
#include <assert.h>
#include <errno.h>

#include "request.h"
#include "image.h"
#include "mem_buf.h"
#include "token_generator.h"

void request_handler(void *arg) {

  req_arg_t * rarg = (req_arg_t *) arg;
  int clientfd = fd_req_arg_t(rarg);
  token_generator_t * tg = token_generator_server_ctx_t(server_ctx_req_arg_t(rarg));

  ssize_t n;
  char buffer[BUFSIZ];
  char tok[20];
  //int enough?
  int bytes_read = 0;
  mem_buf_t * data = mem_buf_create(BUFSIZ);

  //timeout? slow clients will block the server
  while(true){
    if ((n = recv(clientfd, buffer, sizeof(buffer), 0)) <= 0) {
      if (n == 0) {
        // connection closed
        break;
      } else {
        fprintf(stderr, "Err: recv on clientfd %d\n", clientfd);
        int err = errno;
        fprintf(stderr, "errno: %d\n", err);
        goto abort;
      }
    }
    mem_buf_append(data, (uint8_t *) buffer, n);
    //usleep(8000);
    bytes_read += n;
  }

  if(bytes_read == 0){
    //disconnect call
    goto abort;
  }

  assert(bytes_read == data->pos);

  token_generate(tg, tok, sizeof(tok));
  //TODO: dispatch to another threadpool
  image_process(data);

  //dup
  //very important: dup() see :out
  //tmp, binary server should return binary data too (?)
  FILE * fd = fdopen(dup(clientfd), "w"); 
  if(fd){
    fprintf(fd, 
            "Job <%d:%s> init. received %d bytes in transfer. Goodbye.\n",
            clientfd,
            tok,
            bytes_read);
    fflush(fd);
    fclose(fd);
  }
  goto out;

abort:
  mem_buf_destroy(data);
out:
  //NOTE: writing a response without dup:ing the socket seem to create Errno::EPIPE
  printf("closing %d\n", clientfd);
  close(clientfd);
  free(rarg);
}
