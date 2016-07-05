typedef unsigned int (*copy_sd_mmc_to_mem) (unsigned int  channel, unsigned int  start_block, unsigned char block_size, unsigned int  *trg, unsigned int  init);

void copy_code_to_dram(void)
{
	void (*BL2)(void);

	// 函数指针
	copy_sd_mmc_to_mem copy_bl2 = (copy_sd_mmc_to_mem) (*(unsigned int *) (0xD0037F98));


  copy_bl2(0, 49, 32,(unsigned int *)0x23E00000, 0);

	// 跳转到DRAM
  BL2 = (void *)0x23E00000;
  (*BL2)();
}
