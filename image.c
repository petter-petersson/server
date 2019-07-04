#include <stdio.h>
#include <stdlib.h>

#include "image.h"

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
