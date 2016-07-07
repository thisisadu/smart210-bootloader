// —” ±∫Ø ˝
void delay(unsigned long count)
{
	volatile unsigned long i = count;
	while (i--) ;
}

void led_loop()
{
#define 	GPJ2CON 	(*(volatile unsigned long *) 0xE0200280)
#define 	GPJ2DAT		(*(volatile unsigned long *) 0xE0200284)

  GPJ2CON = 0x00001111;		// ≈‰÷√“˝Ω≈
  while(1)					// …¡À∏
  {
    GPJ2DAT = 0;			// LED on
    delay(0x100000);
    GPJ2DAT = 0xf;			// LED off
    delay(0x100000);
  }
}


void main(void)
{
  typedef unsigned int (*copy_sd_mmc_to_mem) (unsigned int  channel, unsigned int  start_block, unsigned char block_size, unsigned int  *trg, unsigned int  init);
	copy_sd_mmc_to_mem copy_fun = (copy_sd_mmc_to_mem) (*(unsigned int *) (0xD0037F98));

  uart_init();

  printf("load dtb\n");

//  led_loop();

  //copy dtb to dram
  copy_fun(0, 200, 100,(unsigned int *)0x24000000, 0);

  printf("load zImage\n");
  //copy zImage to dram
  copy_fun(0, 400, 5000,(unsigned int *)0x23F00000, 0);

  printf("prepare\n");

  //prepare args before jump to kernel
  prepare();

	//jump to kernel
	void (*kernel)(void);
  kernel = (void *)0x23F00000;
  (*kernel)();
}
