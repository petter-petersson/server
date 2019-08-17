#ifndef __SERVER_H
#define __SERVER_H

#include <assert.h>
#include <pthread.h>

#define SERVER_CTX_NUM_CONN_ALLOC 8

struct connection_s; //forward decl.

typedef struct server_ctx_s {
  int queue;
  int fd;
  char * socket_path;
  int avail_connections;
  int num_connections;
  struct kevent * events;
  int (*default_action) (struct server_ctx_s *sctx, struct connection_s * conn);
} server_ctx_t;

//separate file + init & destroy method
typedef struct connection_s {
  int fd; 
  int bytes_read;
  int (*action) (server_ctx_t *sctx, struct connection_s * conn);
} connection_t;

/* TODO write accessors */
typedef struct task_s {
  connection_t * connection;
  struct task_s * next;
} task_t;

typedef struct workqueue_s {
  pthread_mutex_t mutex;
  pthread_cond_t q_not_empty;
  pthread_cond_t thread_available;
  pthread_t * work_threads;
  int available_threads;

  task_t *t_head;
  task_t *t_tail;
  
  server_ctx_t *sctx;
} workqueue_t;

//ugly tmp macro
#ifdef DEBUG
#define WORKQUEUE_DISPLAY_TASKS(_n) \
  do { \
  printf("task list:\n"); \
  task_t * tmp = (_n)->t_head; \
  int task_count = 0; \
  while(tmp != NULL){ \
    task_count++; \
    printf("task %p\n", tmp); \
    tmp = (tmp)->next; \
  } \
  printf("task count: %d\n", task_count); \
  } while(0) 
#else
#define WORKQUEUE_DISPLAY_TASKS(_n) do {} while(0)
#endif
//connection_t accessors
#define x_fd_connection_t(_n) (deref_connection_t(_n)->fd)
#define fd_connection_t(_n) ((void)0, x_fd_connection_t(_n))

#define x_action_connection_t(_n) (deref_connection_t(_n)->action)
#define action_connection_t(_n) ((void)0, x_action_connection_t(_n))

#define x_bytes_read_connection_t(_n) (deref_connection_t(_n)->bytes_read)
#define bytes_read_connection_t(_n) ((void)0, x_bytes_read_connection_t(_n))


#ifdef DEBUG
#define deref_server_ctx_t(_n) (assert((_n)!=0), (_n))
#define deref_connection_t(_n) (assert((_n)!=0), (_n))
#else
#define deref_server_ctx_t(_n) (_n)
#define deref_connection_t(_n) (_n)
#endif
#define x_queue_server_ctx_t(_n) (deref_server_ctx_t(_n)->queue)
#define queue_server_ctx_t(_n) ((void)0, x_queue_server_ctx_t(_n))

#define x_fd_server_ctx_t(_n) (deref_server_ctx_t(_n)->fd)
#define fd_server_ctx_t(_n) ((void)0, x_fd_server_ctx_t(_n))

#define x_socket_path_server_ctx_t(_n) (deref_server_ctx_t(_n)->socket_path)
#define socket_path_server_ctx_t(_n) ((void)0, x_socket_path_server_ctx_t(_n))

#define x_num_connections_server_ctx_t(_n) (deref_server_ctx_t(_n)->num_connections)
#define num_connections_server_ctx_t(_n) ((void)0, x_num_connections_server_ctx_t(_n))

#define x_avail_connections_server_ctx_t(_n) (deref_server_ctx_t(_n)->avail_connections)
#define avail_connections_server_ctx_t(_n) ((void)0, x_avail_connections_server_ctx_t(_n))

#define x_events_server_ctx_t(_n) (deref_server_ctx_t(_n)->events)
#define events_server_ctx_t(_n) ((void)0, x_events_server_ctx_t(_n))

#define x_default_action_server_ctx_t(_n) (deref_server_ctx_t(_n)->default_action)
#define default_action_server_ctx_t(_n) ((void)0, x_default_action_server_ctx_t(_n))

#define server_error(...) do { \
  fprintf(stderr, __VA_ARGS__); exit(1); \
} while (0)

#define server_debug_print(_n) do { \
  printf("allocated connections: %d\n", avail_connections_server_ctx_t(_n)); \
  printf("active connections: %d\n", num_connections_server_ctx_t(_n)); \
  printf("server fd: %d\n", fd_server_ctx_t(_n)); \
  printf("--\n"); \
} while (0)

//TODO: remove non-public methods
void server_alloc_event(server_ctx_t * server_ctx);
int server_accept(server_ctx_t * sctx, connection_t * conn);
void server_run(server_ctx_t * sctx);
server_ctx_t * server_init(server_ctx_t * server_ctx, char * sock_path);
void server_add_task_to_workqueue(workqueue_t * w_queue, connection_t * conn);
void * server_thread_work(void * arg);

//TODO: improve this interface
void server_connection_enable_read(server_ctx_t * sctx, connection_t * conn);
void server_connection_enable_write(server_ctx_t * sctx, connection_t * conn);
void server_connection_disable_read(server_ctx_t * sctx, connection_t * conn);
void server_connection_delete_write(server_ctx_t * sctx, connection_t * conn);
#endif
