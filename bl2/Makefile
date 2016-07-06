bl2.bin: start.o main.o uart.o lib/libc.a
	arm-linux-gnueabi-ld -Tbl2.lds -o bl2.elf $^
	arm-linux-gnueabi-objcopy -O binary bl2.elf bl2.bin

.PHONY : lib/libc.a
lib/libc.a:
	cd lib; make; cd ..

%.o : %.S
	arm-linux-gnueabi-gcc -o $@ $< -c  

%.o : %.c
	arm-linux-gnueabi-gcc -o $@ $< -c   

clean:
	rm *.o *.elf *.bin -f
	cd lib; make clean; cd ..
