.global _start

_start:
	// �رտ��Ź�
	ldr	r0, =0xE2700000			
	mov	r1, #0
	str	r1, [r0]

	// ����ջ���Ա����c����
	ldr	sp, =0xD0037D80

	// ��ʼ��ʱ��
	bl clock_init	
				
	// ��ʼ���ڴ�
	bl mem_init						

	// �ض�λ��������DRAM������				
	bl copy_code_to_dram

halt:
	b halt