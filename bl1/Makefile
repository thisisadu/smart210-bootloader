CC      = arm-linux-gnueabi-gcc
LD      = arm-linux-gnueabi-ld
AR      = arm-linux-gnueabi-ar
OBJCOPY = arm-linux-gnueabi-objcopy
OBJDUMP = arm-linux-gnueabi-objdump

bl1.bin: start.o mem.o clock.o main.o 
	arm-linux-gnueabi-ld -Tbl1.lds -o bl1.elf $^
	arm-linux-gnueabi-objcopy -O binary bl1.elf bl1_.bin
	gcc mkv210_image.c -o mkmini210
	./mkmini210 bl1_.bin bl1.bin

%.o : %.S
	arm-linux-gnueabi-gcc -o $@ $< -c 

%.o : %.c
	arm-linux-gnueabi-gcc -o $@ $< -c 

clean:
	rm *.o *.elf *.bin mkmini210 -f
