#include <stdint.h>

#define TRUE 0x01
#define FALSE 0x00
#define NUM_CHECKERS 3
#define DEPTH_GHE 256

static inline void ght_set_status (uint64_t status)
{
  ROCC_INSTRUCTION_S (1, status, 0x06);
}

static inline uint64_t ght_get_status ()
{
  uint64_t get_status;
  ROCC_INSTRUCTION_D (1, get_status, 0x06);
  return get_status;
}


void idle()
{
  while(1){};
}