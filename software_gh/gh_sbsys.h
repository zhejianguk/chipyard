#include <stdint.h>

#define TRUE 0x01
#define FALSE 0x00
#define NUM_CHECKERS 3
#define DEPTH_GHE 256


static inline void ght_start ()
{
  uint64_t set_status = 0x01;
  uint64_t get_status = 0x00;
  ROCC_INSTRUCTION_DSS (1, get_status, set_status, 0x00, 0x06);
}

static inline uint64_t ght_stop ()
{
  uint64_t set_status = 0x02;
  uint64_t get_status = 0x00;

  ROCC_INSTRUCTION_DSS (1, get_status, set_status, 0x00, 0x06);
  return get_status;
}

void idle()
{
  while(1){};
}