#include <common.h>
#include <asm/io.h>
#include <asm/cache.h>
#include <asm/arch/io.h>
#include <power_firmware.dat>
//#include <../appf/power_firmware.dat>
void init_suspend_firmware(void)
{
	//1. load source code to memory: 64M kernel + 16M reserve space. Suspend firmware 
	// locate at 64M + 15M.
	unsigned * paddr = (unsigned*)0x04F00000;
	unsigned size = sizeof(power_firmware_code)/sizeof(unsigned);
	int i;
	int (*entry)(void) = (int(*)(void))0x04F05800;
	
	for(i = 0; i < size; i++){
		*paddr = power_firmware_code[i];
		paddr++;
	}

	dcache_flush();
	icache_invalid();
	
	i = entry();
	printf("init suspend firmware done. (ret:%d)\n",i);
}
