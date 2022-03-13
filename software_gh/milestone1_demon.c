#include <stdio.h>
#include "rocc.h"
#include "spin_lock.h"
#include "gh_sbsys.h"
#include "ghe.h"





struct gh_sbsys gh = {.big.exec_complete = FALSE,
                      .big.exec_start = TRUE,
                      .little.r_counter = NUM_CHECKERS,
                      .little.counter_lock = FALSE,
                      .uart_lock = FALSE, 
                      };

void task_PerfCounter(uint64_t core_id);


/* Core_0 thread */
int main(void)
{
  lock_release(&gh.uart_lock);
  // printf("Big0: Test is now start \r\n");
  lock_release(&gh.little.counter_lock);
  gh.big.exec_start = TRUE;
  ght_start ();


__asm__(
        "li   t0,   0x81000000;"         // write pointer
        "li   t1,   0x55555000;"         // data
        "li   t2,   0x81000000;"         // Read pointer
        "j    .loop_store;");

__asm__(
        ".loop_store:"
        "li   a5,   0x81000FFF;"
        "sw         t1,   (t0);"
        "addi t1,   t1,   1;"            // data + 1
        "addi t0,   t0,   4;"            // write address + 4
        "blt  t0,   a5,  .loop_store;"
        "li   t0,   0x82000000;"
        "li   t2,   0x81000000;"
        "j    .loop_load;");

__asm__(
        ".loop_load:"
        "li   a5,   0x82000FFF;"
        "lw   t1,   (t2);"
        "sw         t1,   (t0);"
        "addi t0,   t0,   4;"
        "addi t2,   t2,   4;"
        "blt  t0,   a5,  .loop_load;");


  /* Post execution */
  ght_stop();
  gh.big.exec_complete = TRUE;
  while(gh.little.r_counter != 0){
    // Idle;
  }
  lock_release(&gh.uart_lock);
  // printf("Big0: All test is done! \r\n");
  lock_release(&gh.little.counter_lock);
  return 0;
}

/* Core_1 & 2 thread */
int __main(void)
{
  uint64_t Hart_id = 0;
  asm volatile ("csrr %0, mhartid"  : "=r"(Hart_id));

  switch (Hart_id){
      case 0x01:
        task_PerfCounter(Hart_id);
      break;

      case 0x02:
        task_PerfCounter(Hart_id);
      break;

      case 0x03:
        task_PerfCounter(Hart_id);
      break;

      case 0x04:
        task_PerfCounter(Hart_id);
      break;

      case 0x05:
        task_PerfCounter(Hart_id);
      break;

      case 0x06:
        task_PerfCounter(Hart_id);
      break;

      case 0x07:
        task_PerfCounter(Hart_id);
      break;

      default:
      break;
  }
  
  idle();
  return 0;
}

void task_PerfCounter(uint64_t core_id) {
  uint64_t Func_Opcode = 0x0;
  int GHE_ptr = 0;
  int perfc = 0;

  while (gh.big.exec_start != TRUE) {
    // idle;
  }

  /* Operating commits */
  while (gh.big.exec_complete != TRUE) {
    while (ghe_status() != GHE_EMPTY)
    {
      ROCC_INSTRUCTION_DSS (1, Func_Opcode, 0, 0, 0x03);
      if ((Func_Opcode & 0x7F) == 0x03) {
        perfc = perfc + 1;
      }
    }
  }

  /* Report results  */
  lock_acquire(&gh.uart_lock);
  // printf("C%x: Performance Counter = %x. \n", core_id, perfc);
  lock_release(&gh.uart_lock);

  lock_acquire(&gh.little.counter_lock);
  gh.little.r_counter = gh.little.r_counter - 1;
  lock_release(&gh.little.counter_lock);
}


