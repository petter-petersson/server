#ifndef __IMAGE_H
#define __IMAGE_H

#include "mem_buf.h"

#define DEFAULT_SOCK_PATH "/tmp/jamboree.sock"

// file:///usr/local/share/doc/ImageMagick-6.5.7/www/api/blob.html
// file:///usr/local/share/doc/ImageMagick-6.5.7/www/magick-core.html

//TODO data types
void image_process(mem_buf_t * data);

#endif
