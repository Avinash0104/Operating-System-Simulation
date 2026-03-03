#include "logger.h"

// allocates memory and checks for successful allocation
void* checked_malloc(size_t size) {
  void* result = malloc(size);
  if(!result) {
    logger_write("FATAL: Memory allocation failed");
    return NULL; 
  }
  return result;
}

// frees memory and checks for null pointer
void checked_free(void* addr) {
  assert(addr);
  free(addr);
}
