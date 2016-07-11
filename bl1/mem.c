#ifndef __raw_readl
#define __raw_readl(a)		(*(volatile unsigned int*)(a))
#endif

#ifndef __raw_writel
#define __raw_writel(v,a)	(*(volatile unsigned int*)(a) = (v))
#endif

#define readl(addr) __raw_readl(addr)
#define writel(b,addr) __raw_writel((b),addr)

#define S5P_DMC0_BASE 0xF0000000
#define S5P_DRAM_WR	3
#define S5P_DRAM_CAS	4
#define DMC_TIMING_AREF	0x00000618
#define DMC_TIMING_ROW	0x2B34438A
#define DMC_TIMING_DATA	0x24240000
#define DMC_TIMING_PWR	0x0BDC0343
#define S5P_DMC_CONCONTROL	0x00
#define S5P_DMC_MEMCONTROL	0x04
#define S5P_DMC_MEMCONFIG0	0x08
#define S5P_DMC_MEMCONFIG1	0x0C
#define S5P_DMC_DIRECTCMD	0x10
#define S5P_DMC_PRECHCONFIG	0x14
#define S5P_DMC_PHYCONTROL0	0x18
#define S5P_DMC_PHYCONTROL1	0x1C
#define S5P_DMC_PWRDNCONFIG	0x28
#define S5P_DMC_TIMINGAREF	0x30
#define S5P_DMC_TIMINGROW	0x34
#define S5P_DMC_TIMINGDATA	0x38
#define S5P_DMC_TIMINGPOWER	0x3C
#define S5P_DMC_PHYSTATUS	0x40
#define S5P_DMC_MRSTATUS	0x54

/* DRAM commands */
#define CMD(x)  ((x) << 24)
#define BANK(x) ((x) << 16)
#define CHIP(x) ((x) << 20)
#define ADDR(x) (x)

#define MRS	CMD(0x0)
#define PALL	CMD(0x1)
#define PRE	CMD(0x2)
#define DPD	CMD(0x3)
#define REFS	CMD(0x4)
#define REFA	CMD(0x5)
#define CKEL	CMD(0x6)
#define NOP	CMD(0x7)
#define REFSX	CMD(0x8)
#define MRR	CMD(0x9)

#define EMRS1 (MRS | BANK(1))
#define EMRS2 (MRS | BANK(2))
#define EMRS3 (MRS | BANK(3))

/* Burst is (1 << S5P_DRAM_BURST), i.e. S5P_DRAM_BURST=2 for burst 4 */
#ifndef S5P_DRAM_BURST
/* (LP)DDR2 supports burst 4 only, make it default */
# define S5P_DRAM_BURST 2
#endif

typedef unsigned int phys_addr_t;
typedef unsigned int uint32_t;

/**
 * Initialization sequences for different kinds of DRAM
 */
#define dcmd(x) writel((x) | CHIP(chip), base + S5P_DMC_DIRECTCMD)

static void s5p_dram_init_seq_ddr2(phys_addr_t base, unsigned chip)
{
	const uint32_t emr = 0x400; /* DQS disable */
	const uint32_t mr = (((S5P_DRAM_WR) - 1) << 9)
			  | ((S5P_DRAM_CAS) << 4)
			  | (S5P_DRAM_BURST);
	dcmd(NOP);
	/* FIXME wait here? JEDEC recommends but nobody does */
	dcmd(PALL); dcmd(EMRS2); dcmd(EMRS3);
	dcmd(EMRS1 | ADDR(emr));         /* DQS disable */
	dcmd(MRS   | ADDR(mr | 0x100));  /* DLL reset */
	dcmd(PALL); dcmd(REFA); dcmd(REFA);
	dcmd(MRS   | ADDR(mr));          /* DLL no reset */
	dcmd(EMRS1 | ADDR(emr | 0x380)); /* OCD defaults */
	dcmd(EMRS1 | ADDR(emr));         /* OCD exit */
}

#undef dcmd

static inline void s5p_dram_start_dll(phys_addr_t base, uint32_t phycon1)
{
	uint32_t pc0 = 0x00101000; /* the only legal initial value */
	uint32_t lv;

	/* Init DLL */
	writel(pc0, base + S5P_DMC_PHYCONTROL0);
	writel(phycon1, base + S5P_DMC_PHYCONTROL1);

	/* DLL on */
	pc0 |= 0x2;
	writel(pc0, base + S5P_DMC_PHYCONTROL0);

	/* DLL start */
	pc0 |= 0x1;
	writel(pc0, base + S5P_DMC_PHYCONTROL0);

	/* Find lock val */
	do {
		lv = readl(base + S5P_DMC_PHYSTATUS);
	} while ((lv & 0x7) != 0x7);

	lv >>= 6;
	lv &= 0xff; /* ctrl_lock_value[9:2] - coarse */
	pc0 |= (lv << 24); /* ctrl_force */
	writel(pc0, base + S5P_DMC_PHYCONTROL0); /* force value locking */
}

static inline void s5p_dram_setup(phys_addr_t base, uint32_t mc0, uint32_t mc1,
				       int bus16, uint32_t mcon)
{
	mcon |= (S5P_DRAM_BURST) << 20;
	/* 16 or 32-bit bus ? */
	mcon |= bus16 ? 0x1000 : 0x2000;
	if (mc1)
		mcon |= 0x10000; /* two chips */

	writel(mcon, base + S5P_DMC_MEMCONTROL);

	/* Set up memory layout */
	writel(mc0, base + S5P_DMC_MEMCONFIG0);
	if (mc1)
		writel(mc1, base + S5P_DMC_MEMCONFIG1);

	/* Open page precharge policy - reasonable defaults */
	writel(0xFF000000, base + S5P_DMC_PRECHCONFIG);

	/* Set up timings */
	writel(DMC_TIMING_AREF, base + S5P_DMC_TIMINGAREF);
	writel(DMC_TIMING_ROW,  base + S5P_DMC_TIMINGROW);
	writel(DMC_TIMING_DATA, base + S5P_DMC_TIMINGDATA);
	writel(DMC_TIMING_PWR,  base + S5P_DMC_TIMINGPOWER);
}

static inline void s5p_dram_start(phys_addr_t base)
{
	/* Reasonable defaults and auto-refresh on */
	writel(0x0FFF1070, base + S5P_DMC_CONCONTROL);
	/* Reasonable defaults */
	writel(0xFFFF00FF, base + S5P_DMC_PWRDNCONFIG);
}

/*
 * Initialize DDR2 memory bank
 */
void s5p_init_dram_bank_ddr2(phys_addr_t base, uint32_t mc0, uint32_t mc1, int bus16)
{
	/* refcount 8, 180 deg. shift */
	s5p_dram_start_dll(base, 0x00000086);
	/* DDR2 type */
	s5p_dram_setup(base, mc0, mc1, bus16, 0x400);

	/* Start-Up Commands */
	s5p_dram_init_seq_ddr2(base, 0);
	if (mc1)
		s5p_dram_init_seq_ddr2(base, 1);

	s5p_dram_start(base);
}

void mem_init()
{
	s5p_init_dram_bank_ddr2(S5P_DMC0_BASE, 0x20E00323, 0, 0);
}

