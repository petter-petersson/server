#ifndef __TEST_H
#define __TEST_H

#include <stdint.h>

typedef struct test_context_s {
  uint16_t test_passed;
  uint16_t test_failed;
  uint16_t checks_run;
} test_context_t;

void test (int (*func)(void), const char *name, test_context_t *tc);
void test_ctx (int (*func)(test_context_t *), const char *name, test_context_t *tc);

void test_context_init(test_context_t * context);
void test_context_show_result(test_context_t * context);

#define fail()  return __LINE__

#define check_s(cond) do { if (!(cond)) fail(); } while(0)
#define check(cond, context) do { if (!(cond)) fail(); context->checks_run++; } while(0)

#endif
