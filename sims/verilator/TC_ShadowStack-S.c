#include <stdio.h>
#include <stdlib.h>
#include "rocc.h"
#include "spin_lock.h"
#include "ght.h"
#include "ghe.h"
#include "tasks.h"
#include "malloc.h"

int uart_lock;
char* shadow;

/* Core_0 thread */
int main(void)
{
  int *ptr = NULL;
  int ptr_size = 128;
  int sum = 0;
  //================== Initialisation ==================//
  // shadow memory
  shadow = shadow_malloc(32*1024*1024*sizeof(char));
  if(shadow == NULL) {
    printf("C0: Error! memory not allocated.");
    exit(0);
  }

  // Insepct JAL 
  // inst_index: 0x03 
  // Func: 0x00 - 0x0F
  // Data path: ALU + RD + RS1
  ght_cfg_filter(0x03, 0x00, 0x6F, 0x05);
  ght_cfg_filter(0x03, 0x01, 0x6F, 0x05);
  ght_cfg_filter(0x03, 0x02, 0x6F, 0x05);
  ght_cfg_filter(0x03, 0x03, 0x6F, 0x05);
  ght_cfg_filter(0x03, 0x04, 0x6F, 0x05);
  ght_cfg_filter(0x03, 0x05, 0x6F, 0x05);
  ght_cfg_filter(0x03, 0x06, 0x6F, 0x05);
  ght_cfg_filter(0x03, 0x07, 0x6F, 0x05);

  // Insepct JALR 
  // inst_index: 0x03 
  // Func: 0x00
  // Data path: ALU + RD + RS1
  ght_cfg_filter(0x03, 0x00, 0x67, 0x05);
  

  // se: 0x03, end_id: 0x02, scheduling: rr, start_id: 0x01
  ght_cfg_se (0x03, 0x01, 0x01, 0x01);

  // inst_index: 0x03 se: 0x03
  ght_cfg_mapper (0x03, 0b1000);


  lock_acquire(&uart_lock);
  printf("C0: Test is now start \r\n");
  lock_release(&uart_lock);
  ght_set_status (0x01); // ght: start

  //===================== Execution =====================//
  /*
  for (int i = 0; i < 3; i++)
  {
    task_synthetic();
  }

  ptr = (int*) malloc(ptr_size * sizeof(int));
 
  // if memory cannot be allocated
  if(ptr == NULL) {
    printf("C0: Error! memory not allocated.");
    exit(0);
  }

  for (int i = 0; i < ptr_size; i++)
  {
    *(ptr + i) = i;
  }

  for (int i = 0; i < ptr_size; i++)
  {
    sum = sum + *(ptr+i);
  }

  free(ptr);
  */ 
 
  printf("   \r\n");



  //=================== Post execution ===================//
  ght_set_status (0x02);
  uint64_t status;
  while ((status = ght_get_status()) < 0x1FFFF)
  {
    ght_set_status (0x02);
  }

  lock_acquire(&uart_lock);
  printf("All tests are done! Status: %x, sum = %x \r\n", status, sum);
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
        task_ShadowStack_S(Hart_id);
      break;

      case 0x02:
        task_ShadowStack_S(Hart_id);
      break;

      case 0x03:
        task_ShadowStack_S(Hart_id);
      break;

      case 0x04:
        task_ShadowStack_S(Hart_id);
      break;

      case 0x05:
        task_ShadowStack_S(Hart_id);
      break;

      case 0x06:
        task_ShadowStack_S(Hart_id);
      break;

      case 0x07:
        task_ShadowStack_S(Hart_id);
      break;



      default:
      break;
  }
  
  idle();
  return 0;
}