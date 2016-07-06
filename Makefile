all:
	make -C ./bl1
	make -C ./bl2
burn:
	sudo dd if=./bl1/bl1.bin of=/dev/mmcblk0 bs=512 seek=1
	sudo dd if=./bl2/bl2.bin of=/dev/mmcblk0 bs=512 seek=49
clean:
	make clean -C ./bl1
	make clean -C ./bl2
