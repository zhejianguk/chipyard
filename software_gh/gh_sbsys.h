#include <stdint.h>

#define TRUE 0x01
#define FALSE 0x00
#define NUM_CHECKERS 1
#define DEPTH_GHE 256

struct main_core {
  int exec_complete;
  int exec_start;
};

struct checker_core {
  int r_counter;
  int counter_lock;
};

struct gh_sbsys {
  struct main_core big;
  struct checker_core little;
  int uart_lock; 
};

int ghe_ptr_add1 (int ptr)
{
  int p = ptr;

  if (p == DEPTH_GHE - 1) {
    p = 0;
  } else{
    p = p + 1;
  }
  return p;
}

static inline void ght_start ()
{
  uint64_t status;
  ROCC_INSTRUCTION_DSS (1, status, 0x00, 0x00, 0x07);
}

static inline void ght_stop ()
{
  uint64_t status;
  ROCC_INSTRUCTION_DSS (1, status, 0x01, 0, 0x07);
}

void idle()
{
  while(1){};
}