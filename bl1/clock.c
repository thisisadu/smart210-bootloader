// ʱ����ؼĴ���
#define APLL_LOCK 			( *((volatile unsigned long *)0xE0100000) )		
#define MPLL_LOCK 			( *((volatile unsigned long *)0xE0100008) )
#define EPLL_LOCK 			( *((volatile unsigned long *)0xE0100010) )
#define VPLL_LOCK 			( *((volatile unsigned long *)0xE0100020) )

#define APLL_CON0 			( *((volatile unsigned long *)0xE0100100) )
#define APLL_CON1 			( *((volatile unsigned long *)0xE0100104) )
#define MPLL_CON 			( *((volatile unsigned long *)0xE0100108) )
#define EPLL_CON0 			( *((volatile unsigned long *)0xE0100110) )
#define VPLL_CON 			( *((volatile unsigned long *)0xE0100120) )

#define CLK_SRC0 			( *((volatile unsigned long *)0xE0100200) )
#define CLK_SRC1 			( *((volatile unsigned long *)0xE0100204) )
#define CLK_SRC2 			( *((volatile unsigned long *)0xE0100208) )
#define CLK_SRC3 			( *((volatile unsigned long *)0xE010020c) )
#define CLK_SRC4 			( *((volatile unsigned long *)0xE0100210) )
#define CLK_SRC5 			( *((volatile unsigned long *)0xE0100214) )
#define CLK_SRC6 			( *((volatile unsigned long *)0xE0100218) )
#define CLK_SRC_MASK0 		( *((volatile unsigned long *)0xE0100280) )
#define CLK_SRC_MASK1 		( *((volatile unsigned long *)0xE0100284) )

#define CLK_DIV0 			( *((volatile unsigned long *)0xE0100300) )
#define CLK_DIV1 			( *((volatile unsigned long *)0xE0100304) )
#define CLK_DIV2 			( *((volatile unsigned long *)0xE0100308) )
#define CLK_DIV3 			( *((volatile unsigned long *)0xE010030c) )
#define CLK_DIV4 			( *((volatile unsigned long *)0xE0100310) )
#define CLK_DIV5 			( *((volatile unsigned long *)0xE0100314) )
#define CLK_DIV6 			( *((volatile unsigned long *)0xE0100318) )
#define CLK_DIV7 			( *((volatile unsigned long *)0xE010031c) )

#define CLK_DIV0_MASK		0x7fffffff

#define APLL_MDIV       	0x7d
#define APLL_PDIV       	0x3
#define APLL_SDIV      	 	0x1
#define MPLL_MDIV			0x29b
#define MPLL_PDIV			0xc
#define MPLL_SDIV			0x1
#define EPLL_MDIV			48
#define EPLL_PDIV			3
#define EPLL_SDIV			2
#define VPLL_MDIV			108
#define VPLL_PDIV			6
#define VPLL_SDIV			3

#define set_pll(mdiv, pdiv, sdiv)	(1<<31 | mdiv<<16 | pdiv<<8 | sdiv)
#define APLL_VAL		set_pll(APLL_MDIV,APLL_PDIV,APLL_SDIV)
#define MPLL_VAL		set_pll(MPLL_MDIV,MPLL_PDIV,MPLL_SDIV)
#define EPLL_VAL		set_pll(EPLL_MDIV,EPLL_PDIV,EPLL_SDIV)
#define VPLL_VAL		set_pll(VPLL_MDIV,VPLL_PDIV,VPLL_SDIV)


void clock_init()
{
	// 1 ���ø���ʱ�ӿ��أ���ʱ��ʹ��PLL
	CLK_SRC0 = 0x0;

	
	// 2 ��������ʱ�䣬ʹ��Ĭ��ֵ����
	// ����PLL��ʱ�Ӵ�Fin������Ŀ��Ƶ��ʱ����Ҫһ����ʱ�䣬������ʱ�� 			
	APLL_LOCK = 0x0000FFFF;          			
	MPLL_LOCK = 0x0000FFFF;					
	EPLL_LOCK = 0x0000FFFF;          			
	VPLL_LOCK = 0x0000FFFF;					

	// 3 ���÷�Ƶ
	CLK_DIV0 = 0x14131440;
	//CLK_DIV0 = (0<<0)|(4<<8)|(1<<12)|(3<<16)|(1<<20)|(4<<24)|(1<<28);

	// 4 ����PLL
	// FOUT= MDIV * FIN / (PDIV*2^(SDIV-1)) = 0x7d*24/(0x3*2^(1-1))=1000 MHz
	APLL_CON0 = APLL_VAL;
	// FOUT = MDIV*FIN/(PDIV*2^SDIV)=0x29b*24/(0xc*2^1)= 667 MHz
	MPLL_CON  = MPLL_VAL;					

	EPLL_CON0  = EPLL_VAL;					

	VPLL_CON  = VPLL_VAL;					

	// 5 ���ø���ʱ�ӿ��أ�ʹ��PLL
	CLK_SRC0 = 0x00001111;
	//CLK_SRC0 = (1<<0)|(1<<4)|(1<<8)|(1<<12);
}
