#include <stdio.h>
#include "rocc.h"
#include "spin_lock.h"
#include "ght.h"
#include "ghe.h"
#include "tasks.h"

int uart_lock;
char* shadow;

/* Core_0 thread */
int main(void)
{
  //================== Initialisation ==================//
  // Insepct JAL 
  // inst_index: 0x03 
  // Func: 0x00 - 0x0F
  // Data path: ALU + RD + RS1
  ght_cfg_filter(0x03, 0x00, 0x6F, 0x04);
  ght_cfg_filter(0x03, 0x01, 0x6F, 0x04);
  ght_cfg_filter(0x03, 0x02, 0x6F, 0x04);
  ght_cfg_filter(0x03, 0x03, 0x6F, 0x04);
  ght_cfg_filter(0x03, 0x04, 0x6F, 0x04);
  ght_cfg_filter(0x03, 0x05, 0x6F, 0x04);
  ght_cfg_filter(0x03, 0x06, 0x6F, 0x04);
  ght_cfg_filter(0x03, 0x07, 0x6F, 0x04);

  // Insepct JALR 
  // inst_index: 0x03 
  // Func: 0x00
  // Data path: ALU + RD + RS1
  ght_cfg_filter(0x03, 0x00, 0x67, 0x04);
  

  // se: 0x03, end_id: 0x02, scheduling: rr, start_id: 0x01
  ght_cfg_se (0x03, 0x01, 0x01, 0x01);

  // inst_index: 0x03 se: 0x03
  ght_cfg_mapper (0x03, 0b1000);


  lock_acquire(&uart_lock);
  printf("C0: Test is now start \r\n");
  lock_release(&uart_lock);
  ght_set_status (0x01); // ght: start

  uint64_t Hart_id = 0;
  asm volatile ("csrr %0, mhartid"  : "=r"(Hart_id));
  uint64_t loop = task_synthetic2(Hart_id);

  for (int i; i < loop; i ++){
    task_synthetic();
  }




  //=================== Post execution ===================//
  ght_set_status (0x02);
  uint64_t status;
  while ((status = ght_get_status()) < 0x1FFFF)
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
        task_ShadowStack(Hart_id);
      break;

      case 0x02:
        task_ShadowStack(Hart_id);
      break;

      case 0x03:
        task_ShadowStack(Hart_id);
      break;

      case 0x04:
        task_ShadowStack(Hart_id);
      break;

      default:
      break;
  }
  
  idle();
  return 0;
}