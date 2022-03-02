#include <stdio.h>
#include "rocc.h"
#include "spin_lock.h"
#include "ghe.h"


#define TRUE 0x01
#define FALSE 0x00

int uart_lock = 0;
int *uart_lock_p = &uart_lock;

void task_core1(uint64_t core_id);
void task_core2(uint64_t core_id);

/* Core_0 thread */
int main(void)
{

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

  while(1){}
  return 0;
}

/* Core_1 & 2 thread */
int __main(void)
{
  
  uint64_t Hart_id = 0;
  asm volatile ("csrr %0, mhartid"  : "=r"(Hart_id));

  if (Hart_id == 0x01){
    task_core1(Hart_id);
  }

  if (Hart_id == 0x02){
    task_core2(Hart_id);
  }

  while(1){

  }

  return 0;
}

void task_core1(uint64_t core_id) {
  uint64_t EOB = FALSE;
  uint64_t Buffer_Func_Opcode[64] = {0x0};
  uint64_t Buffer_data[64] = {0x0}; 
  int GHE_ptr = 0;

  while (EOB != TRUE) {
    while (ghe_status() == GHE_EMPTY){
    }
    Buffer_Func_Opcode[GHE_ptr] = ghe_top_func_opcode();
    Buffer_data[GHE_ptr] = ghe_pop_data();
    GHE_ptr = GHE_ptr + 1;
    // Check if end of monitoring 
    if (GHE_ptr == 64){
      EOB = TRUE;
    }
  }

  lock_acquire(uart_lock_p);
  printf("Packetets recieved from Hart %x. \n", core_id);
  for (int j = 0; j < 64; j++) {
    printf("Packet: %d is Func+Opcode: %lx Data: %lx \r\n", j, Buffer_Func_Opcode[j], Buffer_data[j]);
  }
  lock_release(uart_lock_p);

}


void task_core2(uint64_t core_id) {
  uint64_t EOB = FALSE;
  uint64_t Buffer_Func_Opcode[64] = {0x0};
  uint64_t Buffer_data[64] = {0x0}; 
  int GHE_ptr = 0;

  while (EOB != TRUE) {
    while (ghe_status() == GHE_EMPTY){
    }
    Buffer_data[GHE_ptr] = ghe_top_data();
    Buffer_Func_Opcode[GHE_ptr] = ghe_pop_func_opcode();
    GHE_ptr = GHE_ptr + 1;
    // Check if end of monitoring 
    if (GHE_ptr == 64){
      EOB = TRUE;
    }
  }

  lock_acquire(uart_lock_p);
  printf("Packetets recieved from Hart %x. \n", core_id);
  for (int j = 0; j < 64; j++) {
    printf("Packet: %d is Func+Opcode: %lx Data: %lx \r\n", j, Buffer_Func_Opcode[j], Buffer_data[j]);
  }
  lock_release(uart_lock_p);
  
}