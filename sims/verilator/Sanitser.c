#include <stdio.h>
#include <stdlib.h>
#include "rocc.h"
#include "spin_lock.h"
#include "gh_sbsys.h"
#include "ghe.h"
#include "malloc.h"

int uart_lock;
void task_Sanitiser(uint64_t core_id);


/* Core_0 thread */
int main(void)
{
  lock_acquire(&uart_lock);
  printf("C0: Test is now start!\r\n");
  lock_release(&uart_lock);
  ght_set_status (0x01);

  int n = 100;
  int *ptr = NULL;
  int sum = 0;

  ptr = (int*) malloc(n * sizeof(int));
 
  // if memory cannot be allocated
  if(ptr == NULL) {
    printf("C0: Error! memory not allocated.");
    exit(0);
  }

  for (int i = 0; i < n; i++)
  {
    *(ptr + i) = i;
  }

  for (int i = 0; i < n; i++)
  {
    sum = sum + *(ptr + i);
  }


    

  printf("C0: Sum = %d \r\n", sum);
  
  // deallocating the memory
  free(ptr);

  /* Post execution */
  ght_set_status (0x02); // ght: start
  uint64_t status;
  while ((status = ght_get_status()) < 0x0F)
  {

  }

  lock_acquire(&uart_lock);
  printf("All tests are done!\n", status);
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
        task_Sanitiser(Hart_id);
      break;

      default:
      break;
  }
  
  idle();
  return 0;
}

void task_Sanitiser(uint64_t core_id) {
  uint64_t Func_Opcode = 0x0;
  uint64_t Packet = 0x0;
  uint64_t perfc = 0;

  /* Wait for start */
  while (ghe_checkght_status() == 0x00){
  };


  /* Old Method */
  while (ghe_checkght_status() == 0x01) {
    while (ghe_status() != GHE_EMPTY)
    {
      // Top func & opcode
      ROCC_INSTRUCTION_D (1, Func_Opcode, 0x02);

      if ((Func_Opcode & 0x7F) == 0X03)
      {
        ROCC_INSTRUCTION_D (1, Packet, 0x05);
        // printf("The loaded address is: %x \r\n ", Packet);
      }

      if ((Func_Opcode & 0x7F) == 0X23)
      {
        ROCC_INSTRUCTION_D (1, Packet, 0x05);
        // printf("The stored address is: %x \r\n ", Packet);
      }
      perfc = perfc + 1;
    }
  }



  /* Report results  */
  lock_acquire(&uart_lock);
  printf("C%x: PMC = %x \n", core_id, perfc);
  lock_release(&uart_lock);
  
  ghe_complete();  
}
