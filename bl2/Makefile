BL2.bin: start.o main.o
	arm-linux-gnueabi-ld -Tsdram.lds -o BL2.elf $^
	arm-linux-gnueabi-objcopy -O binary BL2.elf BL2.bin
	arm-linux-gnueabi-objdump -D BL2.elf > BL2_elf.dis
	
%.o : %.S
	arm-linux-gnueabi-gcc -o $@ $< -c  

%.o : %.c
	arm-linux-gnueabi-gcc -o $@ $< -c   

clean:
	rm *.o *.elf *.bin *.dis -f
