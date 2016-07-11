.global _start

_start:
	bl set_svc

	bl disable_watchdog

	bl disable_interrupt

	bl disable_mmu

	bl disable_dcache

	bl invalid_tlb

	// \C9\E8\D6\C3ջ\A3\AC\D2Ա\E3\B5\F7\D3\C3c\BA\AF\CA\FD
	ldr	sp, =0xD0037D80

	// \B3\F5ʼ\BB\AFʱ\D6\D3
	bl clock_init	
				
	// \B3\F5ʼ\BB\AF\C4ڴ\E6
	bl mem_init						

	// \D6ض\A8λ\A3\AC\B2\A2\CC\F8\B5\BDDRAM\D6\D0\D4\CB\D0\D0				
	bl copy_code_to_dram

set_svc:
	mrs r0,cpsr
	bic r0,r0,#0xff				
	orr r0,r0,#0xd3						@\BF\BFirq,fiq,\BF\BF\BFsvc\BF\BF
	msr cpsr_c,r0
	mov pc,lr

#define  pwtcon 0xe2700000	
disable_watchdog:					  @\BF\BF\BF\BF\BF
	ldr r0,=pwtcon
	mov r1,#0x0
	str r1,[r0]
	mov pc,lr
	
disable_interrupt:					@\BF\BF\BF\BF
	mvn r0,#0x0             
	ldr r1,=0xf2000014
	str r0,[r1]
	
	ldr r1,=0xf2100014
	str r0,[r1]
	
	ldr r1,=0xf2200014
	str r0,[r1]
	
	ldr r1,=0xf2300014
	str r0,[r1]
	mov pc,lr

disable_mmu:
	mrc p15,0,r0,c1,c0,0					@\BF\BFMMU\BF\BF\BFdcache(Icache\BF\BF\BF\BF)
	bic r0, r0, #0x1
	mcr p15,0,r0,c1,c0,0
	mov pc, lr
	
disable_dcache:
	mrc p15,0,r0,c1,c0,0					@\BF\BFMMU\BF\BF\BFdcache(Icache\BF\BF\BF\BF)
	bic r0, r0, #(1<<2)
	mcr p15,0,r0,c1,c0,0
	mov pc, lr

disable_icache:
	mrc p15,0,r0,c1,c0,0					@\BF\BFMMU\BF\BF\BFdcache(Icache\BF\BF\BF\BF)
	bic r0, r0, #(1<<12)
	mcr p15,0,r0,c1,c0,0
	mov pc, lr

invalid_tlb:
	mov r0, #0
	mcr p15,0,r0,c8,c7,0
	mov pc, lr


halt:
	b halt
