.global _start
.global prepare

_start:
	ldr pc, =main

prepare:
  mov pc,lr

halt:
	b halt

