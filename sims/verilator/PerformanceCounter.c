#include <stdio.h>
#include "rocc.h"
#include "spin_lock.h"
#include "gh_sbsys.h"
#include "ghe.h"


void task_PerfCounter(uint64_t core_id);

int uart_lock;

/* Core_0 thread */
int main(void)
{
  //================== Initialisation ==================//
  lock_acquire(&uart_lock);
  printf("C0: Test is now start \r\n");
  lock_release(&uart_lock);
  ght_set_status (0x01); // ght: start

  //===================== Execution =====================//
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

  __asm__(
          ".loop_load2:"
          "li   a5,   0x82000FFF;"
          "lw   t1,   (t2);"
          "sw         t1,   (t0);"
          "addi t0,   t0,   4;"
          "addi t2,   t2,   4;"
          "blt  t0,   a5,  .loop_load;");



  //=================== Post execution ===================//
  ght_set_status (0x02); // ght: start
  uint64_t status;
  while ((status = ght_get_status()) < 0x0F)
  {

  }

  lock_acquire(&uart_lock);
  printf("All tests are done! Status: %x \r\n", status);
  lock_release(&uart_lock);
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

      default:
      break;
  }
  
  idle();
  return 0;
}

void task_PerfCounter(uint64_t core_id) {
  uint64_t Func_Opcode = 0x0;
  uint64_t perfc = 0;

  //================== Initialisation ==================//
  while (ghe_checkght_status() == 0x00){
  };



  //===================== Execution =====================//
  /* New Method */
  /*
  while (ghe_checkght_status() == 0x01) {
    while ((Func_Opcode = ghe_popx_func_opcode()) != 0x3FF)
    {
        perfc = perfc + 1;
    }
  }
  */


  /* Old Method */
  /*
  while (ghe_checkght_status() == 0x01) {
    while (ghe_status() != GHE_EMPTY)
    {
      ROCC_INSTRUCTION_D (1, Func_Opcode, 0x03);
      perfc = perfc + 1;
    }
  }
  */
  while(1)
  {
    if (ghe_status() != GHE_EMPTY)
    {     
      ROCC_INSTRUCTION_D (1, Func_Opcode, 0x03);
      perfc = perfc + 1;
    }

    if ((ghe_status() == GHE_EMPTY) && (ghe_checkght_status() == 0x00)){
      lock_acquire(&uart_lock);
      printf("C%x: PMC = %x \n", core_id, perfc);
      lock_release(&uart_lock);
      ghe_complete();
      while((ghe_checkght_status() == 0x00)) {
      }
      ghe_go();
    } 
  }
  //=================== Post execution ===================//

  
  ghe_complete();  
}

