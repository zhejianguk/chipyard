#include <stdint.h>

#define GHE_FULL 0x10
#define GHE_EMPTY 0x01


static inline uint64_t ghe_status ()
{
  uint64_t status;
  ROCC_INSTRUCTION_D (1, status, 0x00);
  return status; 
  // 0b01: empty; 
  // 0b10: full;
  // 0b00: data buffered;
  // 0b11: error
}


static inline uint64_t ghe_top_func_opcode ()
{
  uint64_t packet = 0x00;
  if (ghe_status() != 0x01) {
    ROCC_INSTRUCTION_D (1, packet, 0x02);
  }
  return packet;
}

static inline uint64_t ghe_pop_func_opcode ()
{
  uint64_t packet = 0x00;
  if (ghe_status() != 0x01) {
    ROCC_INSTRUCTION_D (1, packet, 0x03);
  }
  return packet;
}

static inline uint64_t ghe_topx_func_opcode ()
{
  uint64_t packet;
  ROCC_INSTRUCTION_D (1, packet, 0x02);
  return packet;
}

static inline uint64_t ghe_popx_func_opcode ()
{
  uint64_t packet;
  ROCC_INSTRUCTION_D (1, packet, 0x03);
  return packet;
}


static inline uint64_t ghe_top_data ()
{
  uint64_t packet = 0x00;
  if (ghe_status() != 0x01) {
    ROCC_INSTRUCTION_D (1, packet, 0x04);
  }
  return packet;
}


static inline uint64_t ghe_pop_data ()
{
  uint64_t packet = 0x00;
  if (ghe_status() != 0x01) {
    ROCC_INSTRUCTION_D (1, packet, 0x05);
  }
  return packet;
}


static inline uint64_t ghe_checkght_status ()
{
  uint64_t status;
  ROCC_INSTRUCTION_D (1, status, 0x07);
  return status; 
}


static inline uint64_t ghe_complete ()
{
  uint64_t get_status = 0;
  uint64_t set_status = 0x01;
  ROCC_INSTRUCTION_DS (1, get_status, set_status, 0x01);
  return get_status; 
}

static inline uint64_t ghe_release ()
{
  uint64_t get_status = 0;
  uint64_t set_status = 0xFF;
  ROCC_INSTRUCTION_DS (1, get_status, set_status, 0x01);
  return get_status; 
}

static inline uint64_t ghe_go ()
{
  uint64_t get_status = 0;
  uint64_t set_status = 0;
  ROCC_INSTRUCTION_DS (1, get_status, set_status, 0x01);
  return get_status; 
}
