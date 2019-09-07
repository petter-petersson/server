#ifndef __WORKQUEUE_H
#define __WORKQUEUE_H

#include <pthread.h>

struct workqueue_s;

typedef struct task_s {
  void (*fn) (void *); //or return void * ?
  void * fn_arg;
  struct task_s * next;
} task_t;

typedef struct workqueue_s {
  pthread_mutex_t mutex;
  pthread_cond_t q_not_empty;
  pthread_t * work_threads;
  int num_threads;
  int _running;

  task_t *t_head;
  task_t *t_tail;
  
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

workqueue_t * workqueue_init(int num_threads);
void * workqueue_thread_work(void * arg);
void workqueue_add_task(workqueue_t * w_queue, void (*work_method)(void *), void *arg);
void workqueue_destroy(workqueue_t * w_queue, void (*destroy_each_method)(task_t *, void *), void *arg);
#endif
