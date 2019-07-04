#ifndef __MEM_BUF_H
#define __MEM_BUF_H

#include <stdint.h>

struct mem_buf_s {
  uint8_t * buf;
  uint32_t pos;
  uint32_t buf_size;
  uint32_t mem_chunk_size;
};

typedef struct mem_buf_s mem_buf_t;

mem_buf_t * mem_buf_create(uint32_t mem_chunk_size);
void mem_buf_destroy(mem_buf_t * buf);
int mem_buf_append(mem_buf_t * buf, uint8_t * append, uint32_t append_size);

#endif
