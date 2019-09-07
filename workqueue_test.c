#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "test.h"
#include "binary_search_tree.h"
#include "workqueue.h"


pthread_mutex_t test_mutex;
bst_t * test_btree;
int thread_count = 0;
int task_call_count = 0;

void check_number_of_active_threads(bst_node_t * node, void * arg){
  thread_count++;
}

int remaining_tasks = 0;
void count_remaining_tasks(task_t * task, void * arg){
  remaining_tasks++;
}

void do_work(void *arg) {
  int work_usleep = *((int *) arg);

  pthread_mutex_lock(&test_mutex);
  bst_add(test_btree, (int) pthread_self(), NULL);
  task_call_count++;
  pthread_mutex_unlock(&test_mutex);
  usleep(work_usleep);
}

int test_workqueue(test_context_t * tctx){
  int work_usleep = 4000;
  pthread_mutex_init(&(test_mutex), NULL);
  test_btree = bst_init();

  workqueue_t * pool;
  pool = workqueue_init(5);

  for(int i=0; i < 10; i++){
    usleep(10000);
    workqueue_add_task(pool, do_work, &work_usleep);
  }
  
  work_usleep = 0;
  for(int i=0; i < 10; i++){
    usleep(2000);
    workqueue_add_task(pool, do_work, &work_usleep);
  }
  
  work_usleep = 200000;
  for(int i=0; i < 10; i++){
    workqueue_add_task(pool, do_work, &work_usleep);
  }
  sleep(1);
  
  bst_traverse(test_btree->root, check_number_of_active_threads, NULL);
  printf("thread_count: %d\n", thread_count);
  check(thread_count == 5, tctx);
  printf("task_call_count: %d\n", task_call_count);
  check(task_call_count == 30, tctx);
 

  //assert that we remove remaining tasks when destroying:
  work_usleep = 500000;
  for(int i=0; i < 10; i++){
    workqueue_add_task(pool, do_work, &work_usleep);
  }
  workqueue_destroy(pool, count_remaining_tasks, NULL);
  check(remaining_tasks > 0, tctx);
  printf("remaining tasks when calling destroy: %d\n", remaining_tasks);
  pthread_mutex_destroy(&test_mutex);
  bst_free(test_btree);
  return 0;
}

int main() {
  test_context_t context;
  test_context_init(&context);

  test_ctx(test_workqueue, "test_workqueue", &context);

  test_context_show_result(&context);
}
