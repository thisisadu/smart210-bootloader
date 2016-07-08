.global _start
.global prepare

_start:
	ldr pc, =main

prepare:
	ldr r0, =0x0
	ldr r1, =0xFFFFFFFF
	ldr r2, =0x29000000
        mov pc,lr

halt:
	b halt

