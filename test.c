#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "test.h"

void test (int (*func)(void), const char *name, test_context_t *tc){
  int r = func();
  if(r == 0){
    tc->test_passed++;
  } else {
    tc->test_failed++;
    printf("FAILED: %s (at line %d)\n", name ,r);
  }
}
void test_ctx (int (*func)(test_context_t *), const char *name, test_context_t *tc){
  int r = func(tc);
  if(r == 0){
    tc->test_passed++;
  } else {
    tc->test_failed++;
    printf("FAILED: %s (at line %d)\n", name ,r);
  }
}

void test_context_init(test_context_t * context){
  context->test_passed = 0;
  context->test_failed = 0;
  context->checks_run = 0;
}

void test_context_show_result(test_context_t * context){
  printf("\n");
  printf("--------------------------------------------\n");
  printf("passed tests: %d\n", context->test_passed);
  printf("failed tests: %d\n", context->test_failed);
  printf("checks run:   %d\n", context->checks_run);
}
