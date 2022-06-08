#include <stdint.h>

#define TRUE 0x01
#define FALSE 0x00
#define NUM_CHECKERS 3
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

static inline void ght_cfg_rtable (uint64_t index, uint64_t func, uint64_t opcode, uint64_t sel_d)
{
  uint64_t set_ref;
  set_ref = ((index & 0x1f)<<4) | ((sel_d & 0xf)<<17) | ((opcode & 0x7f)<<21) | ((func & 0xf)<<28) | 0x02;
  ROCC_INSTRUCTION_SS (1, set_ref, 0X02, 0x06);
}

static inline void ght_cfg_stable_checkers (uint64_t se_id, uint64_t end_id, uint64_t policy, uint64_t start_id)
{
  uint64_t set_stable_checkers;
  set_stable_checkers = ((se_id & 0x1f)<<4) | ((start_id & 0xf)<<17) | ((policy & 0x7f)<<21) | ((end_id & 0xf)<<28) | 0x03;
  ROCC_INSTRUCTION_SS (1, set_stable_checkers, 0X02, 0x06);
}

static inline void ght_cfg_stable_sch (uint64_t se_id, uint64_t m_inst_type, uint64_t index_m)
{
  uint64_t set_stable_sch;
  set_stable_sch = ((se_id & 0x1f)<<4) | ((index_m & 0xf)<<17) | ((m_inst_type & 0x7f)<<21) | 0x04;
  ROCC_INSTRUCTION_SS (1, set_stable_sch, 0X02, 0x06);
}


void idle()
{
  while(1){};
}