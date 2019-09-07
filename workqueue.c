#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "workqueue.h"

workqueue_t * workqueue_init(int num_threads) {
  workqueue_t * queue = (workqueue_t *) malloc(sizeof(workqueue_t));
  if(queue == NULL){
    fprintf(stderr, "Unable to create workqueue\n");
    abort();
  }
  queue->_running = 1;
  queue->t_head = NULL;
  queue->t_tail = NULL;
  pthread_mutex_init(&(queue->mutex), NULL);
  pthread_cond_init(&(queue->q_not_empty), NULL);

  queue->num_threads = num_threads;

  queue->work_threads = (pthread_t *)malloc(queue->num_threads * sizeof(pthread_t));
  if(queue->work_threads == NULL) {
    perror("malloc");
    abort();
  }
  for(int i = 0; i < queue->num_threads; i++) {
    if (pthread_create(&(queue->work_threads[i]), NULL, workqueue_thread_work, queue) != 0) {
      perror("pthread_created");
      abort();
    }
  } 
  return queue;
}

void * workqueue_thread_work(void * arg){
  task_t * current;

  workqueue_t * wq = (workqueue_t *) arg;
  assert(wq != NULL);

  while(1) {

    pthread_mutex_lock(&(wq->mutex));
    while(wq->t_head == NULL && wq->_running){
      pthread_cond_wait(&(wq->q_not_empty), &(wq->mutex));
    }

    if(!wq->_running){
      printf("breaking\n");
      pthread_mutex_unlock(&(wq->mutex));
      break;
    }

    current = wq->t_head;
    wq->t_head = current->next;

    if (wq->t_head == NULL) {
      wq->t_tail = NULL;
    }

    pthread_mutex_unlock(&(wq->mutex));

    assert(current->fn != NULL);
    if(current->fn != NULL){
      (current->fn)(current->fn_arg);
    }

    free(current);
    current = NULL;
  }
  return 0;
}

void workqueue_add_task(workqueue_t * w_queue, void (*work_method)(void *), void *arg) {
  assert(w_queue != NULL);

  task_t *current_task;
  current_task = (task_t *) malloc(sizeof(task_t));
  if(current_task == NULL) {
    fprintf(stderr, "Failed to create task\n");
    return;
  }
  current_task->fn = work_method;
  current_task->fn_arg = arg;
  current_task->next = NULL;

  pthread_mutex_lock(&(w_queue->mutex));

  if (w_queue->t_head == NULL) {
    w_queue->t_head = current_task;
  } else {
    w_queue->t_tail->next = current_task;
  }
  w_queue->t_tail = current_task;

  pthread_cond_signal(&(w_queue->q_not_empty));
  pthread_mutex_unlock(&(w_queue->mutex));
}

void workqueue_destroy(workqueue_t * w_queue, void (*destroy_each_method)(task_t *, void *), void *arg) {
  pthread_mutex_lock(&(w_queue->mutex));
  task_t * tmp = w_queue->t_head; //does it matter
  w_queue->t_head = NULL;
  w_queue->_running = 0;
  pthread_cond_broadcast(&(w_queue->q_not_empty));
  pthread_mutex_unlock(&(w_queue->mutex));
  for(int i = 0; i < w_queue->num_threads; i++) {
    pthread_join(w_queue->work_threads[i], NULL);
  }

  task_t * next;
  int task_count = 0;
  while(tmp != NULL){
    task_count++;
    next = tmp->next;
    if(destroy_each_method){
      (destroy_each_method)(tmp, arg);
    }
    free(tmp);
    tmp = next;
  }
  printf("task count: %d\n", task_count);

  pthread_mutex_destroy(&(w_queue->mutex));
  pthread_cond_destroy(&(w_queue->q_not_empty));
  free(w_queue->work_threads);
  free(w_queue);
}
