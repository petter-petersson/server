#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "test.h"
#include "mem_buf.h"

#define TEST_CHUNK_SIZE 3
#define LONG_STR "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum."

int test_append_to_empty(test_context_t * tctx){
  char * strs[] = {"apa", "gnu-gnu-gnu", "gitarr"};

  mem_buf_t * target = mem_buf_create(20);

  int res;
  //TODO macro for strings
  res = mem_buf_append(target, (uint8_t *)strs[0], strlen(strs[0]));

  check(res == 0, tctx);
  printf("%s\n", target->buf);
  check( strcmp((char *)target->buf, "apa")==0, tctx);
  check( target->pos == 3, tctx);
  check( target->buf_size == 20, tctx);

  mem_buf_destroy(target);

  target = mem_buf_create(TEST_CHUNK_SIZE);
  res = mem_buf_append(target, (uint8_t *)strs[1], strlen(strs[1]));
  check(res == 0, tctx);
  printf("%s\n", target->buf);
  check( strcmp((char *)target->buf, "gnu-gnu-gnu")==0, tctx);
  check( target->pos == 11, tctx);
  check( target->buf_size == 11, tctx);
  mem_buf_destroy(target);

  return 0;
}

int test_many(test_context_t * tctx){
  const char * strs[] = {
    "Lorem", "ipsum", "dolor", "sit", "amet,", "consectetur", "adipiscing", "elit,", "sed", "do", "eiusmod", "tempor", "incididunt", "ut", "labore", "et", "dolore", "magna", "aliqua.", "Ut", "enim", "ad", "minim", "veniam,", "quis", "nostrud", "exercitation", "ullamco", "laboris", "nisi", "ut", "aliquip", "ex", "ea", "commodo", "consequat.", "Duis", "aute", "irure", "dolor", "in", "reprehenderit", "in", "voluptate", "velit", "esse", "cillum", "dolore", "eu", "fugiat", "nulla", "pariatur.", "Excepteur", "sint", "occaecat", "cupidatat", "non", "proident,", "sunt", "in", "culpa", "qui", "officia", "deserunt", "mollit", "anim", "id", "est", "laborum.", "Sed", "ut", "perspiciatis", "unde", "omnis", "iste", "natus", "error", "sit", "voluptatem", "accusantium", "doloremque", "laudantium,", "totam", "rem", "aperiam,", "eaque", "ipsa", "quae", "ab", "illo", "inventore", "veritatis", "et", "quasi", "architecto", "beatae", "vitae", "dicta", "sunt", "explicabo.", "Nemo", "enim", "ipsam", "voluptatem", "quia", "voluptas", "sit", "aspernatur", "aut", "odit", "aut", "fugit,", "sed", "quia", "consequuntur", "magni", "dolores", "eos", "qui", "ratione", "voluptatem", "sequi", "nesciunt.", "Neque", "porro", "quisquam", "est,", "qui", "dolorem", "ipsum", "quia", "dolor", "sit", "amet,", "consectetur,", "adipisci", "velit,", "sed", "quia", "non", "numquam", "eius", "modi", "tempora", "incidunt", "ut", "labore", "et", "dolore", "magnam", "aliquam", "quaerat", "voluptatem.", "Ut", "enim", "ad", "minima", "veniam,", "quis", "nostrum", "exercitationem", "ullam", "corporis", "suscipit", "laboriosam,", "nisi", "ut", "aliquid", "ex", "ea", "commodi", "consequatur?", "Quis", "autem", "vel", "eum", "iure", "reprehenderit", "qui", "in", "ea", "voluptate", "velit", "esse", "quam", "nihil", "molestiae", "consequatur,", "vel", "illum", "qui", "dolorem", "eum", "fugiat", "quo", "voluptas", "nulla", "pariatur?", NULL
  };
  mem_buf_t * target = mem_buf_create(4);
  int res, i;

  for(i = 0; i < 198; i++){
    res = mem_buf_append(target, (uint8_t *)strs[i], strlen(strs[i]));
  }
  printf("%d, %lu, %d\n", target->pos, strlen((char *)target->buf), target->buf_size);
  printf("%s\n", (char *)target->buf);
  mem_buf_destroy(target);
  return 0;
}

int test_append_long(test_context_t * tctx){

  mem_buf_t * target = mem_buf_create(64);

  printf("strlen init: %lu\n", strlen(LONG_STR));
  printf("sizeof(uint8_t): %lu\n", sizeof(uint8_t));
  printf("sizeof(char): %lu\n", sizeof(char));
  mem_buf_append(target, (uint8_t *)LONG_STR, strlen(LONG_STR));
  mem_buf_append(target, (uint8_t *)"\r\n", strlen("\r\n"));

  //447, 506, 509
  printf("%d, %lu, %d\n", target->pos, strlen((char *)target->buf), target->buf_size);
  printf("%s\n", (char *)target->buf);
  
  check( strlen((char *)target->buf) == 447, tctx); //depends on null termination or not in impl.
  check( target->pos == 447, tctx);
  check( target->buf_size == 509, tctx);

  mem_buf_destroy(target);
  return 0;
}

int main() {
  test_context_t context;
  test_context_init(&context);

  test_ctx(test_append_to_empty, "test_append_to_empty", &context);
  test_ctx(test_many, "test_many", &context);
  test_ctx(test_append_long, "test_append_long", &context);

  test_context_show_result(&context);
}
