all:
	make -C ./BL1
	make -C ./BL2
	mv BL1/BL1.bin .
	mv BL2/BL2.bin .
burn:
	sudo dd if=BL1.bin of=/dev/mmcblk0 bs=512 seek=1
	sudo dd if=BL2.bin of=/dev/mmcblk0 bs=512 seek=49
clean:
	make clean -C ./BL1
	make clean -C ./BL2
	rm *.bin
