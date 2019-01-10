#include "utils.h"
int main(){
  int s = 1302568;
  char *ptr = shared_mem_get("test_shm", s, true);
  char *cl = shared_mem_get("test_shm", s, false);
  cl[0] = 'a';
  assertion(ptr[0] == 'a');
  shared_mem_close("test_shm", ptr, s);
}