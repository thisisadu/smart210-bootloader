CC      = arm-linux-gnueabi-gcc
LD      = arm-linux-gnueabi-ld
AR      = arm-linux-gnueabi-ar
OBJCOPY = arm-linux-gnueabi-objcopy
OBJDUMP = arm-linux-gnueabi-objdump

INCLUDEDIR 	:= $(shell pwd)/include
CFLAGS 		:= -Wall -O2 -fno-builtin
CPPFLAGS   	:= -nostdinc -I$(INCLUDEDIR)

sdram.bin: start.o memory.o clock.o Hsmmc.o copy_bl2.o uart.o stdio.o lib/libc.a
	arm-linux-gnueabi-ld -Tsdram.lds -o sdram.elf $^
	arm-linux-gnueabi-objcopy -O binary sdram.elf sdram.bin
	arm-linux-gnueabi-objdump -D sdram.elf > sdram_elf.dis
	gcc mkv210_image.c -o mkmini210
	./mkmini210 sdram.bin BL1.bin

.PHONY : lib/libc.a
lib/libc.a:
	cd lib; make; cd ..
	
%.o : %.S
	arm-linux-gnueabi-gcc -o $@ $< -c 

%.o : %.c
	arm-linux-gnueabi-gcc -o $@ $< -c 

clean:
	rm *.o *.elf *.bin *.dis mkmini210 -f
