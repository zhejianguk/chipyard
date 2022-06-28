#include <stdint.h>

#define TRUE 0x01
#define FALSE 0x00
#define NUM_CORES 8
#define DEPTH_GHE 256

static inline void ght_set_status (uint64_t status)
{
  ROCC_INSTRUCTION_SS (1, status, 0X01, 0x06);
}

static inline uint64_t ght_get_status ()
{
  uint64_t get_status;
  ROCC_INSTRUCTION_DSS (1, get_status, 0X00, 0X00, 0x06);
  return get_status;
}

static inline void ght_cfg_filter (uint64_t index, uint64_t func, uint64_t opcode, uint64_t sel_d)
{
  uint64_t set_ref;
  set_ref = ((index & 0x1f)<<4) | ((sel_d & 0xf)<<17) | ((opcode & 0x7f)<<21) | ((func & 0xf)<<28) | 0x02;
  ROCC_INSTRUCTION_SS (1, set_ref, 0X02, 0x06);
}

static inline void ght_cfg_se (uint64_t se_id, uint64_t end_id, uint64_t policy, uint64_t start_id)
{
  uint64_t set_se;
  set_se = ((se_id & 0x1f)<<4) | ((start_id & 0xf)<<17) | ((policy & 0x7f)<<21) | ((end_id & 0xf)<<28) | 0x04;
  ROCC_INSTRUCTION_SS (1, set_se, 0X02, 0x06);
}

static inline void ght_cfg_mapper (uint64_t inst_type, uint64_t ses_receiving_inst)
{
  uint64_t set_mapper;
  set_mapper = ((inst_type & 0x1f)<<4) | ((ses_receiving_inst & 0xFFFF)<<16) | 0x03;
  ROCC_INSTRUCTION_SS (1, set_mapper, 0X02, 0x06);
}


void idle()
{
  while(1){};
}