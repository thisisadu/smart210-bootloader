
void copy_code_to_dram(void)
{
	void (*BL2)(void);

	uart_init();

	printf("uart init success!\n");

	Hsmmc_Init();

	Hsmmc_ReadBlock((unsigned char *)0x23E00000,49,32);
	
	// Ìø×ªµ½DRAM
    	BL2 = (void *)0x23E00000;
    	(*BL2)();
}
