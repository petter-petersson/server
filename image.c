#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>

#include <sys/socket.h>

#include "image.h"
#include "server.h"

void image_process(mem_buf_t * data) {
  
  printf("got data with size %d\n", (int) data->pos);

  //tmp. write the data to disk so we can verify it's integrity
  FILE * wrt_handle;
  wrt_handle = fopen("/tmp/image-server-data.jpg", "w");
  if(wrt_handle){
    fwrite(data->buf, data->pos, 1, wrt_handle);
    fclose(wrt_handle);
  }
out:
  mem_buf_destroy(data);
}


void print_help(const char * app_name){
  printf("\n");
  printf("\x1B[33mUsage: %s [-h] [-s socket]\n", app_name);
  printf("\t-s   Path to socket file. Deault file is <%s>\n", DEFAULT_SOCK_PATH);
  printf("\t-h   Print this info\x1B[0m\n");
  printf("\n");
}

int write_command(server_ctx_t * sctx, connection_t * conn){
  assert(conn != NULL);
  assert(sctx != NULL);
  int clientfd = fd_connection_t(conn);

  printf("server_write %d\n", clientfd);

  int d = dup(clientfd);
  if (d < 0){
    fprintf(stderr, "failed to dup client fd.\n");
    //return 1;
    goto out;
  }
  FILE * fd = fdopen(d, "w");
  if(fd){
    fprintf(fd,
        "Job <%d> init. received %d bytes in transfer. Goodbye.\n",
        clientfd,
        bytes_read_connection_t(conn));
    fflush(fd);
    fclose(fd);
  }
out:
  server_connection_delete_write(sctx, conn);
  return 0;
}

int read_command(struct server_ctx_s *sctx, connection_t * conn){
  char buffer[BUFSIZ];
  ssize_t n = 0;

  assert(conn != NULL);
  assert(sctx != NULL);
  int clientfd = fd_connection_t(conn);

  /* this should be handled in a different event
  if( event->flags & EV_ERROR){
    perror("event");
    printf("event->data: %ld\n", event->data);
    abort(); //or?
  }
  */


  if ((n = recv(clientfd, buffer, sizeof(buffer), 0)) <= 0) {
    if (n == 0) {
      
      printf("%d is done. Read %d bytes\n", clientfd, bytes_read_connection_t(conn));

      x_action_connection_t(conn) = write_command;

      server_connection_disable_read(sctx, conn);
      server_connection_enable_write(sctx, conn);

      return 0;
    } else {
      fprintf(stderr, "recv error on fd %d\n", clientfd);
      perror("recv");
      abort();
    }
  }
  x_bytes_read_connection_t(conn) = bytes_read_connection_t(conn) + n;

  //if(clientfd % 14 == 0)
  //  usleep(4000);

  server_connection_enable_read(sctx, conn);
  return 0;
}


//
int main(int argc, const char *argv[]) {

  int c;
  opterr = 0;
  char * sock_path = DEFAULT_SOCK_PATH;

  while ((c = getopt (argc, (char **)argv, "hs:")) != -1) {
    switch (c)
    {
      case 's':
        sock_path = optarg; 
        break;
      case 'h':
      case '?':
      default:
        print_help(argv[0]);
        exit(0);
        break;
    }
  }
  signal(SIGPIPE, SIG_IGN);
  //todo: alloc since we will destroy other resources anyway later on
  server_ctx_t server_context;
  server_ctx_t * sctx;
  x_default_action_server_ctx_t((struct server_ctx_s *) &server_context) = read_command;

  sctx = server_init(&server_context, sock_path);
  server_run(sctx);

  //FIXME: mem leak currently with server_connection not being free:d

  printf("Good bye.");
  return 0;
}
