#include <stdio.h>




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

  return 0;
}