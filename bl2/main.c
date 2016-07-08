
void main(void)
{
  int ret;

  uart_init();

  Hsmmc_Init();

  //copy dtb to dram
  ret = Hsmmc_ReadBlock((unsigned int *)0x29000000,200,300);
  printf("load dtb\n");

  //copy zImage to dram
  ret = Hsmmc_ReadBlock((unsigned int *)0x24000000,400,5000);
  printf("load zImage\n");

  //prepare args before jump to kernel
  prepare();

  //jump to kernel
  void (*kernel)(void);
  kernel = (void *)0x24000000;
  (*kernel)();
}
