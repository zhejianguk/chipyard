#include "rocc.h"
#include <stdio.h>

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

static inline void ghe_push (uint64_t data_pushed)
{
	if (ghe_status() != 0x02) {
		ROCC_INSTRUCTION_SS (1, data_pushed, 0, 1);
	} else {
		printf ("The ghe is full!\r\n");
	}
}

static inline uint64_t  ghe_pull ()
{
	uint64_t data_pulled;
	if (ghe_status() != 0x01) {
		ROCC_INSTRUCTION_DSS (1, data_pulled, 0, 0, 2);
	} else {
		printf ("The ghe is empty!\r\n");
	}
	return data_pulled;
}


int main(void)
{
	printf("GHE test is now started: \r\n");
	unsigned long channel_status;
	channel_status = ghe_status();
	printf ("The channel's current status is %x\n\r", channel_status);
	// Expected: 0x01

	uint64_t data_in[64];
	uint64_t data_out[64] = {0x0};
	for (int i = 0; i<64; i++)
	{
		data_in[i] = 0x777744445555FFFF+i;
	}
	
	/* Simple test */
	ghe_push (data_in[0]);
	channel_status = ghe_status();
	printf ("The channel's current status is %x\n\r", channel_status);
	// Expected: 0x00

	data_out[0] = ghe_pull();
	channel_status = ghe_status();
	printf ("The fetched data is %lx, and the channel's current status is %x\n\r", data_out[0], channel_status);
	// Expected: 0x5555FFFF, 0x01


	/* Full test */
	// Pull
	for (int i = 0; i<64; i++)
	{
		ghe_push (data_in[i]);
	}
	channel_status = ghe_status();
	printf ("The channel's current status is %x\n\r", channel_status);
	// Expected: 0x10 (2)


	for (int i = 0; i<64; i++)
	{
		data_out[i] = ghe_pull ();
		channel_status = ghe_status();
		printf ("The fetched data is %lx, and the channel's current status is %x\n\r", data_out[i], channel_status);
	}

	return 0;
}
