#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "test.h"
#include "connection.h"

//we do not want to close dummy fd's
extern int close(int fildes){
  printf("stub close()\n");
  return 0;
}

int test_add_and_get_connection(test_context_t * tctx) {
  connection_t * conn1, * conn2, * conn3;
  connection_manager_t * m = connection_manager_init();

  conn1 = connection_manager_get_connection(m, 1);
  check(conn1 != NULL, tctx);
  conn2 = connection_manager_get_connection(m, 2);
  check(conn2 != NULL, tctx);
  conn3 = connection_manager_get_connection(m, 3);
  check(conn3 != NULL, tctx);

  check(connection_manager_count(m) == 3, tctx);

  conn3 = connection_manager_get_connection(m, 3);
  check(connection_manager_count(m) == 3, tctx);

  connection_manager_destroy(m);
  return 0;
}

int test_delete_connection(test_context_t * tctx) {
  connection_t * conn;
  int count;
  connection_manager_t * m = connection_manager_init();

  conn = connection_manager_get_connection(m, 1);
  check(conn != NULL, tctx);

  count = connection_manager_count(m);
  printf("1. count: %d\n", count);
  check(connection_manager_count(m) == 1, tctx);

  connection_manager_delete_connection(m, conn);
  count = connection_manager_count(m);
  printf("2. count: %d\n", count);
  check(connection_manager_count(m) == 1, tctx);


  connection_manager_destroy(m);
  return 0;
}

int main() {
  test_context_t context;
  test_context_init(&context);

  test_ctx(test_add_and_get_connection,
           "test_add_and_get_connection",
           &context);
  printf("-\n");
  test_ctx(test_delete_connection,
           "test_delete_connection",
           &context);

  test_context_show_result(&context);
}
