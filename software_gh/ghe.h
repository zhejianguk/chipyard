#include <stdint.h>

#define GHE_FULL 0x10
#define GHE_EMPTY 0x01


static inline uint64_t ghe_status ()
{
  uint64_t status;
  ROCC_INSTRUCTION_DSS (1, status, 0, 0, 0);
  return status; 
  // 0b01: empty; 
  // 0b10: full;
  // 0b00: data buffered;
  // 0b11: error
}

static inline uint64_t ghe_top_func_opcode ()
{
  uint64_t packet;
  if (ghe_status() != 0x01) {
    ROCC_INSTRUCTION_DSS (1, packet, 0, 0, 0x02);
  }
  return packet;
}

static inline uint64_t ghe_pop_func_opcode ()
{
  uint64_t packet;
  if (ghe_status() != 0x01) {
    ROCC_INSTRUCTION_DSS (1, packet, 0, 0, 0x03);
  }
  return packet;
}

static inline uint64_t ghe_top_data ()
{
  uint64_t packet;
  if (ghe_status() != 0x01) {
    ROCC_INSTRUCTION_DSS (1, packet, 0, 0, 0x04);
  }
  return packet;
}

static inline uint64_t ghe_pop_data ()
{
  uint64_t packet;
  if (ghe_status() != 0x01) {
    ROCC_INSTRUCTION_DSS (1, packet, 0, 0, 0x05);
  }
  return packet;
}

