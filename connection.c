#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#include "connection.h"

connection_t * connection_create(int fd) {
  connection_t * conn;
  conn = malloc(sizeof(connection_t));
  if(conn == NULL){
    //TODO: uniform error handling
    perror("malloc");
    exit(errno);
  }
  x_fd_connection_t(conn) = fd;
  x_bytes_read_connection_t(conn) = 0;
  return conn;
}

connection_t * connection_manager_get_connection(connection_manager_t * m, int fd) {
  bst_t * tree;
  bst_node_t * node;
  connection_t * conn = NULL;

  assert(m != NULL);

  tree = m->store;
  node = bst_find(tree->root, fd);

  if(node == NULL){
    //printf("get connection %d: node is null\n", fd);
    conn = connection_create(fd);
    bst_add(tree, fd, conn);

  } else {
    if (value_bst_node_t(node) == NULL){
      printf("get connection %d: node value is null\n", fd);
      conn = connection_create(fd);
      x_value_bst_node_t(node) = conn;
    } else {
      //printf("get connection %d: retreiving connection\n", fd);
      conn = (connection_t * ) value_bst_node_t(node);
    }
  }

  return conn;
}

connection_manager_t * connection_manager_init() {

  connection_manager_t * manager = malloc(sizeof(connection_manager_t));
  if(manager == NULL){
    perror("malloc");
    exit(errno);
  }
  manager->store = bst_init();
  return manager;
}

//traverse method
void connection_manager_delete_connection_for_node(bst_node_t * node, void * arg) {
  assert(node != NULL);
  connection_t * conn = (connection_t *) value_bst_node_t(node);
  if(conn == NULL){
    //printf("_delete_connection_in_bst: conn was null\n");
    return;
  }
  //printf("destroying connection with fd %d\n", fd_connection_t(conn));
  if (fd_connection_t(conn) > 0 && (fcntl(fd_connection_t(conn), F_GETFD) != -1 || errno != EBADF)){
    //printf("connection %d is open, attempting to close\n", fd_connection_t(conn));
    close(fd_connection_t(conn));
  }
  free(conn);
  x_value_bst_node_t(node) = NULL;
}

void connection_manager_destroy(connection_manager_t * connection_manager){
  assert(connection_manager != NULL);
  assert(connection_manager->store != NULL);

  bst_traverse(connection_manager->store->root, connection_manager_delete_connection_for_node, NULL);
  bst_free(connection_manager->store);
  free(connection_manager);
}

void connection_manager_delete_connection(connection_manager_t * m, connection_t * conn){
  assert(m != NULL);
  assert(conn != NULL);
  assert(m->store != NULL);
  bst_node_t * node = bst_find(m->store->root, fd_connection_t(conn));

  //printf("deleting connection with fd %d\n", fd_connection_t(conn));
  if (fd_connection_t(conn) > 0 && (fcntl(fd_connection_t(conn), F_GETFD) != -1 || errno != EBADF)){
    //printf("connection %d is open, attempting to close\n", fd_connection_t(conn));
    close(fd_connection_t(conn));
  }
  free(conn);
  if(node){
      x_value_bst_node_t(node) = NULL;
  }

}
