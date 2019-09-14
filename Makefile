-include config.mk
override CFLAGS+=-std=c99 -D_GNU_SOURCE -Wall

#TODO: use -lthreadpool instead as before?
LIBS= -lpthread -lbtree #-ltoken_generator

DEBUG_FLAGS=-g -DDEBUG
BUILD_DIR=release
LDFLAGS = $(REL_LDFLAGS)

.PHONY: lib clean test all_tests lib debug_lib mkdirs dbg_image_server \
	rel_image_server

default: lib

mkdirs:
	mkdir -p debug
	mkdir -p release 

test: clean
test: CFLAGS += $(DEBUG_FLAGS)
test: LDFLAGS = $(DBG_LDFLAGS)
test: all_tests

debug_lib: CFLAGS += $(DEBUG_FLAGS)
debug_lib: LDFLAGS = $(DBG_LDFLAGS)
debug_lib: mkdirs
debug_lib: BUILD_DIR=debug
debug_lib: libserver.a

lib: mkdirs
lib: libserver.a 

dbg_image_server: debug_lib 
dbg_image_server: BUILD_DIR=debug
dbg_image_server: CFLAGS += $(DEBUG_FLAGS)
dbg_image_server: LDFLAGS = $(DBG_LDFLAGS)
dbg_image_server: image_server

rel_image_server: lib 
rel_image_server: image_server

%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

libserver.a: server.o workqueue.o connection.o
	$(AR) rc $(BUILD_DIR)/$@ $^

# TODO: this target should be 'private' in some way
all_tests: mem_buf_test workqueue_test
	./mem_buf_test
	./workqueue_test

# TODO: this target should be 'private' in some way
image_server: image.o mem_buf.o
	$(CC) $(CFLAGS) $^ -o $(BUILD_DIR)/$@ $(LDFLAGS) -L./$(BUILD_DIR) $(LIBS) -lserver
	ln -f -s $(BUILD_DIR)/image_server image_server

mem_buf_test: test.o mem_buf.o mem_buf_test.o
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS) $(LIBS)

workqueue_test: test.o workqueue.o workqueue_test.o
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS) $(LIBS)

clean:
	rm -rf *.o *_test {debug,release}/{image_server,*.a} 
