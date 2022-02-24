#include <stdio.h>
#include "rocc.h"

static inline unsigned long ghe_status ()
{
  unsigned long status;
  ROCC_INSTRUCTION_DSS (1, status, 0, 0, 0);
  return status; 
  // 0b01: empty; 
  // 0b10: full;
  // 0b00: data buffered;
  // 0b11: error
}


static inline uint64_t ghe_pull_packet1 ()
{
  uint64_t packet;
  if (ghe_status() != 0x01) {
    ROCC_INSTRUCTION_DSS (1, packet, 0, 0, 2);
  } else {
    printf ("The ghe is empty!\r\n");
  }
  return packet;
}

static inline uint64_t ghe_pull_packet2 ()
{
  uint64_t packet;
  ROCC_INSTRUCTION_DSS (1, packet, 0, 0, 3);
  return packet;
}



void lock_acquire(int *lock)
{
  int temp0 = 1;
  __asm__(
        "loop: "
        "amoswap.w.aq %1, %1, (%0);"
        "bnez %1,loop;"
        ://no output register
        :"r" (lock), "r" (temp0)
        :/*no clobbered registers*/
  );
}

void lock_release (int *lock)
{
  __asm__(
          "amoswap.w.rl x0, x0, (%0);"// Release lock by storing 0.
          ://no output
	  :"r" (lock)
          ://no clobbered register
	);
}

int uart_lock = 0;
int *uart_lock_p = &uart_lock;


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
  unsigned long hart_id;
  asm volatile ("csrr %0, mhartid"  : "=r"(hart_id));
  uint64_t Packets_Func_Opcode[256] = {0x0};
  uint64_t Packets_data[256] = {0x0};

  for (int i = 0; i<256; i++){
    Packets_Func_Opcode[i] = ghe_pull_packet1();
    Packets_data[i] = ghe_pull_packet2();
  }

  lock_acquire(uart_lock_p);
  printf("Packetets recieved from Hart %d. \n", hart_id);
  for (int i = 0; i<256; i++){
    printf("Packet: %d is Func+Opcode: %lx Data: %lx \r\n", i, Packets_Func_Opcode[i], Packets_data[i]);
  }
  lock_release(uart_lock_p);

  while(1){

  }

  return 0;
}