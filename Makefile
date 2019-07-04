-include config.mk
override CFLAGS+=-std=c99 -D_GNU_SOURCE
LIBS= -lpthread -lbtree -lthreadpool

DEBUG_FLAGS=-g -DDEBUG
BUILD_DIR=release
LDFLAGS = $(REL_LDFLAGS)

.PHONY: clean test all debug_all debug mkdirs

mkdirs:
	mkdir -p debug
	mkdir -p release 

debug: clean
debug: CFLAGS += $(DEBUG_FLAGS)
debug: LDFLAGS = $(DBG_LDFLAGS)
debug: test

debug_all: clean
debug_all: CFLAGS += $(DEBUG_FLAGS)
debug_all: LDFLAGS = $(DBG_LDFLAGS)
debug_all: mkdirs
debug_all: BUILD_DIR=debug
debug_all: all 

all: clean
all: mkdirs
all: image_server 

%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

test: mem_buf_test
	./mem_buf_test

image_server: server.o request.o image.o mem_buf.o
	$(CC) $(CFLAGS) $^ -o $(BUILD_DIR)/$@ $(LDFLAGS) $(LIBS)

mem_buf_test: test.o mem_buf.o mem_buf_test.o
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS) $(LIBS)

clean:
	rm -rf *.o *.a *_test 

.PHONY: all clean test
