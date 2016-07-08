#include "cpu.h"
#include "Hsmmc.h"

#define HSMMC_NUM		0

#if (HSMMC_NUM == 0)
#define HSMMC_BASE	(0xEB000000)
#elif (HSMMC_NUM == 1)
#define HSMMC_BASE	(0xEB100000)
#elif (HSMMC_NUM == 2)
#define HSMMC_BASE	(0xEB200000)
#elif (HSMMC_NUM == 3)
#define HSMMC_BASE	(0xEB300000)
#else
#error "Configure HSMMC: HSMMC0 ~ HSMM3(0 ~ 3)"
#endif

#define MAX_BLOCK  65535

static uint8_t CardType; // ¿¨ÀàÐÍ
static uint32_t RCA; // ¿¨Ïà¶ÔµØÖ·

void Delay_us(uint32_t Count)
{

	//¿¿1us,¿¿¿nCount us

	// ¿icache, cpu clock 1G, ¿¿¿¿¿

	int32_t temp1 = 59;// Arm clock¿1G,¿¿¿¿¿17¿Arm clock

	int32_t temp2 = 0;

	asm volatile (

		"Delay_us_0:\n"

		"mov  %0, %2\n"  

		"Delay_us_1:\n"

		"subs  %0, %0, #1\n" // ¿¿¿ cycle 1

		"mov %1, %1\n"	// ¿¿¿ cycle 1

		"mov %1, %1\n"	// ¿¿¿ cycle 2 

		"mov %1, %1\n"	// ¿¿¿ cycle 3 

		"mov %1, %1\n"	// ¿¿¿ cycle 4 	

		"mov %1, %1\n"	// ¿¿¿ cycle 5 	

		"mov %1, %1\n"	// ¿¿¿ cycle 6 

		"mov %1, %1\n"	// ¿¿¿ cycle 7 

		"mov %1, %1\n"	// ¿¿¿ cycle 8 

		"mov %1, %1\n"	// ¿¿¿ cycle 9 

		"mov %1, %1\n"	// ¿¿¿ cycle 10 

		"mov %1, %1\n"	// ¿¿¿ cycle 11 

		"mov %1, %1\n"	// ¿¿¿ cycle 12 

		"mov %1, %1\n"	// ¿¿¿ cycle 13 

		"mov %1, %1\n"	// ¿¿¿ cycle 14 

		"mov %1, %1\n"	// ¿¿¿ cycle 15 

		"mov %1, %1\n"	// ¿¿¿ cycle 16 

		"mov %0, %0\n"	// ¿¿¿ cycle 16 

		"bne Delay_us_1\n" // ¿¿¿¿¿¿¿¿¿ ,cycle 17 

		"subs  %1, %1, #1\n"   // ¿¿¿¿¿nCount¿¿0

		"bne Delay_us_0\n" 

		: "+r"(temp2), "+r"(Count): "r"(temp1): "cc" 

	);	

}

static void Hsmmc_ClockOn(uint8_t On)
{
	uint32_t Timeout;
	if (On) {
		__REGw(HSMMC_BASE+CLKCON_OFFSET) |= (1<<2); // sdÊ±ÖÓÊ¹ÄÜ
		Timeout = 1000; // Wait max 10 ms
		while (!(__REGw(HSMMC_BASE+CLKCON_OFFSET) & (1<<3))) {
			// µÈ´ýSDÊä³öÊ±ÖÓÎÈ¶¨
			if (Timeout == 0) {
				return;
			}
			Timeout--;
			Delay_us(10);
		}
	} else {
		__REGw(HSMMC_BASE+CLKCON_OFFSET) &= ~(1<<2); // sdÊ±ÖÓ½ûÖ¹
	}
}

static void Hsmmc_SetClock(uint32_t Clock)
{
	uint32_t Temp;
	uint32_t Timeout;
	uint32_t i;
	Hsmmc_ClockOn(0); // ¹Ø±ÕÊ±ÖÓ	
	Temp = __REG(HSMMC_BASE+CONTROL2_OFFSET);
	// Set SCLK_MMC(48M) from SYSCON as a clock source	
	Temp = (Temp & (~(3<<4))) | (2<<4);
	Temp |= (1u<<31) | (1u<<30) | (1<<8);
	if (Clock <= 500000) {
		Temp &= ~((1<<14) | (1<<15));
		__REG(HSMMC_BASE+CONTROL3_OFFSET) = 0;
	} else {
		Temp |= ((1<<14) | (1<<15));
		__REG(HSMMC_BASE+CONTROL3_OFFSET) = (1u<<31) | (1<<23);
	}
	__REG(HSMMC_BASE+CONTROL2_OFFSET) = Temp;
	
	for (i=0; i<=8; i++) {
		if (Clock >= (48000000/(1<<i))) {
			break;
		}
	}
	Temp = ((1<<i) / 2) << 8; // clock div
	Temp |= (1<<0); // Internal Clock Enable
	__REGw(HSMMC_BASE+CLKCON_OFFSET) = Temp;
	Timeout = 1000; // Wait max 10 ms
	while (!(__REGw(HSMMC_BASE+CLKCON_OFFSET) & (1<<1))) {
		// µÈ´ýÄÚ²¿Ê±ÖÓÕñµ´ÎÈ¶¨
		if (Timeout == 0) {
			return;
		}
		Timeout--;
		Delay_us(10);
	}
	
	Hsmmc_ClockOn(1); // Ê¹ÄÜÊ±ÖÓ
}

static int32_t Hsmmc_WaitForBufferReadReady(void)
{	
	int32_t ErrorState;
	while (1) {
		if (__REGw(HSMMC_BASE+NORINTSTS_OFFSET) & (1<<15)) { // ³öÏÖ´íÎó
			break;
		}
		if (__REGw(HSMMC_BASE+NORINTSTS_OFFSET) & (1<<5)) { // ¶Á»º´æ×¼±¸ºÃ
			__REGw(HSMMC_BASE+NORINTSTS_OFFSET) |= (1<<5); // Çå³ý×¼±¸ºÃ±êÖ¾
			return 0;
		}				
	}
			
	ErrorState = __REGw(HSMMC_BASE+ERRINTSTS_OFFSET) & 0x1ff; // ¿ÉÄÜÍ¨ÐÅ´íÎó,CRC¼ìÑé´íÎó,³¬Ê±µÈ
	__REGw(HSMMC_BASE+NORINTSTS_OFFSET) = __REGw(HSMMC_BASE+NORINTSTS_OFFSET); // Çå³ýÖÐ¶Ï±êÖ¾	
	__REGw(HSMMC_BASE+ERRINTSTS_OFFSET) = __REGw(HSMMC_BASE+ERRINTSTS_OFFSET); // Çå³ý´íÎóÖÐ¶Ï±êÖ¾
	Debug("Read buffer error, NORINTSTS: %04x\r\n", ErrorState);
	return ErrorState;
}

static int32_t Hsmmc_WaitForBufferWriteReady(void)
{
	int32_t ErrorState;
	while (1) {
		if (__REGw(HSMMC_BASE+NORINTSTS_OFFSET) & (1<<15)) { // ³öÏÖ´íÎó
			break;
		}
		if (__REGw(HSMMC_BASE+NORINTSTS_OFFSET) & (1<<4)) { // Ð´»º´æ×¼±¸ºÃ
			__REGw(HSMMC_BASE+NORINTSTS_OFFSET) |= (1<<4); // Çå³ý×¼±¸ºÃ±êÖ¾
			return 0;
		}				
	}
			
	ErrorState = __REGw(HSMMC_BASE+ERRINTSTS_OFFSET) & 0x1ff; // ¿ÉÄÜÍ¨ÐÅ´íÎó,CRC¼ìÑé´íÎó,³¬Ê±µÈ
	__REGw(HSMMC_BASE+NORINTSTS_OFFSET) = __REGw(HSMMC_BASE+NORINTSTS_OFFSET); // Çå³ýÖÐ¶Ï±êÖ¾
	__REGw(HSMMC_BASE+ERRINTSTS_OFFSET) = __REGw(HSMMC_BASE+ERRINTSTS_OFFSET); // Çå³ý´íÎóÖÐ¶Ï±êÖ¾
	Debug("Write buffer error, NORINTSTS: %04x\r\n", ErrorState);
	return ErrorState;
}
	
static int32_t Hsmmc_WaitForCommandDone(void)
{
	uint32_t i;	
	int32_t ErrorState;
	// µÈ´ýÃüÁî·¢ËÍÍê³É
	for (i=0; i<0x20000000; i++) {
		if (__REGw(HSMMC_BASE+NORINTSTS_OFFSET) & (1<<15)) { // ³öÏÖ´íÎó
			break;
		}
		if (__REGw(HSMMC_BASE+NORINTSTS_OFFSET) & (1<<0)) {
			do {
				__REGw(HSMMC_BASE+NORINTSTS_OFFSET) = (1<<0); // Çå³ýÃüÁîÍê³ÉÎ»
			} while (__REGw(HSMMC_BASE+NORINTSTS_OFFSET) & (1<<0));				
			return 0; // ÃüÁî·¢ËÍ³É¹¦
		}
	}
	
	ErrorState = __REGw(HSMMC_BASE+ERRINTSTS_OFFSET) & 0x1ff; // ¿ÉÄÜÍ¨ÐÅ´íÎó,CRC¼ìÑé´íÎó,³¬Ê±µÈ
	__REGw(HSMMC_BASE+NORINTSTS_OFFSET) = __REGw(HSMMC_BASE+NORINTSTS_OFFSET); // Çå³ýÖÐ¶Ï±êÖ¾
	__REGw(HSMMC_BASE+ERRINTSTS_OFFSET) = __REGw(HSMMC_BASE+ERRINTSTS_OFFSET); // Çå³ý´íÎóÖÐ¶Ï±êÖ¾	
	do {
		__REGw(HSMMC_BASE+NORINTSTS_OFFSET) = (1<<0); // Çå³ýÃüÁîÍê³ÉÎ»
	} while (__REGw(HSMMC_BASE+NORINTSTS_OFFSET) & (1<<0));
	
	Debug("Command error, ERRINTSTS = 0x%x ", ErrorState);	
	return ErrorState; // ÃüÁî·¢ËÍ³ö´í	
}

static int32_t Hsmmc_WaitForTransferDone(void)
{
	int32_t ErrorState;
	uint32_t i;
	// µÈ´ýÊý¾Ý´«ÊäÍê³É
	for (i=0; i<0x20000000; i++) {
		if (__REGw(HSMMC_BASE+NORINTSTS_OFFSET) & (1<<15)) { // ³öÏÖ´íÎó
			break;
		}											
		if (__REGw(HSMMC_BASE+NORINTSTS_OFFSET) & (1<<1)) { // Êý¾Ý´«ÊäÍê								
			do {
				__REGw(HSMMC_BASE+NORINTSTS_OFFSET) |= (1<<1); // Çå³ý´«ÊäÍê³ÉÎ»
			} while (__REGw(HSMMC_BASE+NORINTSTS_OFFSET) & (1<<1));									
			return 0;
		}
	}

	ErrorState = __REGw(HSMMC_BASE+ERRINTSTS_OFFSET) & 0x1ff; // ¿ÉÄÜÍ¨ÐÅ´íÎó,CRC¼ìÑé´íÎó,³¬Ê±µÈ
	__REGw(HSMMC_BASE+NORINTSTS_OFFSET) = __REGw(HSMMC_BASE+NORINTSTS_OFFSET); // Çå³ýÖÐ¶Ï±êÖ¾
	__REGw(HSMMC_BASE+ERRINTSTS_OFFSET) = __REGw(HSMMC_BASE+ERRINTSTS_OFFSET); // Çå³ý´íÎóÖÐ¶Ï±êÖ¾
	Debug("Transfer error, rHM1_ERRINTSTS = 0x%04x\r\n", ErrorState);	
	do {
		__REGw(HSMMC_BASE+NORINTSTS_OFFSET) = (1<<1); // ³ö´íºóÇå³ýÊý¾ÝÍê³ÉÎ»
	} while (__REGw(HSMMC_BASE+NORINTSTS_OFFSET) & (1<<1));
	
	return ErrorState; // Êý¾Ý´«Êä³ö´í		
}

static int32_t Hsmmc_IssueCommand(uint8_t Cmd, uint32_t Arg, uint8_t Data, uint8_t Response)
{
	uint32_t i;
	uint32_t Value;
	uint32_t ErrorState;
	// ¼ì²éCMDÏßÊÇ·ñ×¼±¸ºÃ·¢ËÍÃüÁî
	for (i=0; i<0x1000000; i++) {
		if (!(__REG(HSMMC_BASE+PRNSTS_OFFSET) & (1<<0))) {
			break;
		}
	}
	if (i == 0x1000000) {
		Debug("CMD line time out, PRNSTS: %04x\r\n", __REG(HSMMC_BASE+PRNSTS_OFFSET));
		return -1; // ÃüÁî³¬Ê±
	}
	
	// ¼ì²éDATÏßÊÇ·ñ×¼±¸ºÃ
	if (Response == CMD_RESP_R1B) { // R1b»Ø¸´Í¨¹ýDAT0·´À¡Ã¦ÐÅºÅ
		for (i=0; i<0x10000000; i++) {
			if (!(__REG(HSMMC_BASE+PRNSTS_OFFSET) & (1<<1))) {
				break;
			}		
		}
		if (i == 0x10000000) {
			Debug("Data line time out, PRNSTS: %04x\r\n", __REG(HSMMC_BASE+PRNSTS_OFFSET));			
			return -2;
		}		
	}
	
	__REG(HSMMC_BASE+ARGUMENT_OFFSET) = Arg; // Ð´ÈëÃüÁî²ÎÊý
	
	Value = (Cmd << 8); // command index
	// CMD12¿ÉÖÕÖ¹´«Êä
	if (Cmd == 0x12) {
		Value |= (0x3 << 6); // command type
	}
	if (Data) {
		Value |= (1 << 5); // ÐèÊ¹ÓÃDATÏß×÷Îª´«ÊäµÈ
	}	
	
	switch (Response) {
	case CMD_RESP_NONE:
		Value |= (0<<4) | (0<<3) | 0x0; // Ã»ÓÐ»Ø¸´,²»¼ì²éÃüÁî¼°CRC
		break;
	case CMD_RESP_R1:
	case CMD_RESP_R5:
	case CMD_RESP_R6:
	case CMD_RESP_R7:		
		Value |= (1<<4) | (1<<3) | 0x2; // ¼ì²é»Ø¸´ÖÐµÄÃüÁî,CRC
		break;
	case CMD_RESP_R2:
		Value |= (0<<4) | (1<<3) | 0x1; // »Ø¸´³¤¶ÈÎª136Î»,°üº¬CRC
		break;
	case CMD_RESP_R3:
	case CMD_RESP_R4:
		Value |= (0<<4) | (0<<3) | 0x2; // »Ø¸´³¤¶È48Î»,²»°üº¬ÃüÁî¼°CRC
		break;
	case CMD_RESP_R1B:
		Value |= (1<<4) | (1<<3) | 0x3; // »Ø¸´´øÃ¦ÐÅºÅ,»áÕ¼ÓÃData[0]Ïß
		break;
	default:
		break;	
	}
	
	__REGw(HSMMC_BASE+CMDREG_OFFSET) = Value;
	
	ErrorState = Hsmmc_WaitForCommandDone();
	if (ErrorState) {
		Debug("Command = %d\r\n", Cmd);
	}	
	return ErrorState; // ÃüÁî·¢ËÍ³ö´í
}

int32_t Hsmmc_Switch(uint8_t Mode, int32_t Group, int32_t Function, uint8_t *pStatus)
{
	int32_t ErrorState;
	int32_t Temp;
	uint32_t i;
	
	__REGw(HSMMC_BASE+BLKSIZE_OFFSET) = (7<<12) | (64<<0); // ×î´óDMA»º´æ´óÐ¡,blockÎª512Î»64×Ö½Ú			
	__REGw(HSMMC_BASE+BLKCNT_OFFSET) = 1; // Ð´ÈëÕâ´Î¶Á1 blockµÄsd×´Ì¬Êý¾Ý
	Temp = (Mode << 31U) | 0xffffff;
	Temp &= ~(0xf<<(Group * 4));
	Temp |= Function << (Group * 4);
	__REG(HSMMC_BASE+ARGUMENT_OFFSET) = Temp; // Ð´ÈëÃüÁî²ÎÊý	

	// DMA½ûÄÜ,¶Áµ¥¿é		
	__REGw(HSMMC_BASE+TRNMOD_OFFSET) = (0<<5) | (1<<4) | (0<<2) | (1<<1) | (0<<0);	
	// ÉèÖÃÃüÁî¼Ä´æÆ÷,SWITCH_FUNC CMD6,R1»Ø¸´
	__REGw(HSMMC_BASE+CMDREG_OFFSET) = (CMD6<<8)|(1<<5)|(1<<4)|(1<<3)|0x2;
	ErrorState = Hsmmc_WaitForCommandDone();
	if (ErrorState) {
		Debug("CMD6 error\r\n");
		return ErrorState;
	}

	ErrorState = Hsmmc_WaitForBufferReadReady();
	if (ErrorState) {
		return ErrorState;
	}
	
	pStatus += 64 - 1;
	for (i=0; i<64/4; i++) {
		Temp = __REG(HSMMC_BASE+BDATA_OFFSET);
		*pStatus-- = (uint8_t)Temp;
		*pStatus-- = (uint8_t)(Temp>>8);
		*pStatus-- = (uint8_t)(Temp>>16);
		*pStatus-- = (uint8_t)(Temp>>24);			
	}
		
	ErrorState = Hsmmc_WaitForTransferDone();
	if (ErrorState) {
		Debug("Get sd status error\r\n");
		return ErrorState;
	}	
	return 0;	
}

// 512Î»µÄsd¿¨À©Õ¹×´Ì¬Î»
int32_t Hsmmc_GetSdState(uint8_t *pStatus)
{
	int32_t ErrorState;
	uint32_t Temp;
	uint32_t i;
	if (CardType == SD_HC || CardType == SD_V2 || CardType == SD_V1) {
		if (Hsmmc_GetCardState() != CARD_TRAN) { // ±ØÐèÔÚtransfer status
			return -1; // ¿¨×´Ì¬´íÎó
		}		
		Hsmmc_IssueCommand(CMD55, RCA<<16, 0, CMD_RESP_R1);
		
		__REGw(HSMMC_BASE+BLKSIZE_OFFSET) = (7<<12) | (64<<0); // ×î´óDMA»º´æ´óÐ¡,blockÎª512Î»64×Ö½Ú			
		__REGw(HSMMC_BASE+BLKCNT_OFFSET) = 1; // Ð´ÈëÕâ´Î¶Á1 blockµÄsd×´Ì¬Êý¾Ý
		__REG(HSMMC_BASE+ARGUMENT_OFFSET) = 0; // Ð´ÈëÃüÁî²ÎÊý	

		// DMA½ûÄÜ,¶Áµ¥¿é		
		__REGw(HSMMC_BASE+TRNMOD_OFFSET) = (0<<5) | (1<<4) | (0<<2) | (1<<1) | (0<<0);	
		// ÉèÖÃÃüÁî¼Ä´æÆ÷,¶Á×´Ì¬ÃüÁîCMD13,R1»Ø¸´
		__REGw(HSMMC_BASE+CMDREG_OFFSET) = (CMD13<<8)|(1<<5)|(1<<4)|(1<<3)|0x2;
		ErrorState = Hsmmc_WaitForCommandDone();
		if (ErrorState) {
			Debug("CMD13 error\r\n");
			return ErrorState;
		}
		
		ErrorState = Hsmmc_WaitForBufferReadReady();
		if (ErrorState) {
			return ErrorState;
		}
		
		pStatus += 64 - 1;
		for (i=0; i<64/4; i++) {
			Temp = __REG(HSMMC_BASE+BDATA_OFFSET);
			*pStatus-- = (uint8_t)Temp;
			*pStatus-- = (uint8_t)(Temp>>8);
			*pStatus-- = (uint8_t)(Temp>>16);
			*pStatus-- = (uint8_t)(Temp>>24);			
		}
		
		ErrorState = Hsmmc_WaitForTransferDone();
		if (ErrorState) {
			Debug("Get sd status error\r\n");
			return ErrorState;
		}
							
		return 0;
	}
	return -1; // ·Çsd¿¨
}

// Reads the SD Configuration Register (SCR).
int32_t Hsmmc_Get_SCR(SD_SCR *pSCR)
{
	uint8_t *pBuffer;
	int32_t ErrorState;
	uint32_t Temp;
	uint32_t i;
	Hsmmc_IssueCommand(CMD55, RCA<<16, 0, CMD_RESP_R1); 
	__REGw(HSMMC_BASE+BLKSIZE_OFFSET) = (7<<12) | (8<<0); // ×î´óDMA»º´æ´óÐ¡,blockÎª64Î»8×Ö½Ú			
	__REGw(HSMMC_BASE+BLKCNT_OFFSET) = 1; // Ð´ÈëÕâ´Î¶Á1 blockµÄsd×´Ì¬Êý¾Ý
	__REG(HSMMC_BASE+ARGUMENT_OFFSET) = 0; // Ð´ÈëÃüÁî²ÎÊý	

	// DMA½ûÄÜ,¶Áµ¥¿é		
	__REGw(HSMMC_BASE+TRNMOD_OFFSET) = (0<<5) | (1<<4) | (0<<2) | (1<<1) | (0<<0);	
	// ÉèÖÃÃüÁî¼Ä´æÆ÷,read SD Configuration CMD51,R1»Ø¸´
	__REGw(HSMMC_BASE+CMDREG_OFFSET) = (CMD51<<8)|(1<<5)|(1<<4)|(1<<3)|0x2;
	ErrorState = Hsmmc_WaitForCommandDone();
	if (ErrorState) {
		Debug("CMD51 error\r\n");
		return ErrorState;
	}

	ErrorState = Hsmmc_WaitForBufferReadReady();
	if (ErrorState) {
		return ErrorState;
	}
	
	// Wide width data (SD Memory Register)
	pBuffer = (uint8_t *)pSCR + sizeof(SD_SCR) - 1;
	for (i=0; i<8/4; i++) {
		Temp = __REG(HSMMC_BASE+BDATA_OFFSET);
		*pBuffer-- = (uint8_t)Temp;
		*pBuffer-- = (uint8_t)(Temp>>8);
		*pBuffer-- = (uint8_t)(Temp>>16);
		*pBuffer-- = (uint8_t)(Temp>>24);		
	}
	
	ErrorState = Hsmmc_WaitForTransferDone();
	if (ErrorState) {
		Debug("Get SCR register error\r\n");
		return ErrorState;
	}

	return 0;
}

// Asks the selected card to send its cardspecific data
int32_t Hsmmc_Get_CSD(uint8_t *pCSD)
{
	uint32_t i;
	uint32_t Response[4];
	int32_t State = -1;

	if (CardType != SD_HC && CardType != SD_V1 && CardType != SD_V2) {
		return State; // Î´Ê¶±ðµÄ¿¨
	}
	// È¡Ïû¿¨Ñ¡Ôñ,ÈÎºÎ¿¨¾ù²»»Ø¸´,ÒÑÑ¡ÔñµÄ¿¨Í¨¹ýRCA=0È¡ÏûÑ¡Ôñ,
	// ¿¨»Øµ½stand-by×´Ì¬
	Hsmmc_IssueCommand(CMD7, 0, 0, CMD_RESP_NONE);
	for (i=0; i<1000; i++) {
		if (Hsmmc_GetCardState() == CARD_STBY) { // CMD9ÃüÁîÐèÔÚstandy-by status				
			break; // ×´Ì¬ÕýÈ·
		}
		Delay_us(100);
	}
	if (i == 1000) {
		return State; // ×´Ì¬´íÎó
	}	
	
	// ÇëÇóÒÑ±ê¼Ç¿¨·¢ËÍ¿¨ÌØ¶¨Êý¾Ý(CSD),»ñµÃ¿¨ÐÅÏ¢
	if (!Hsmmc_IssueCommand(CMD9, RCA<<16, 0, CMD_RESP_R2)) {
		pCSD++; // Ìø¹ýµÚÒ»×Ö½Ú,CSDÖÐ[127:8]Î»¶ÔÎ»¼Ä´æÆ÷ÖÐµÄ[119:0]
		Response[0] = __REG(HSMMC_BASE+RSPREG0_OFFSET);
		Response[1] = __REG(HSMMC_BASE+RSPREG1_OFFSET);
		Response[2] = __REG(HSMMC_BASE+RSPREG2_OFFSET);
		Response[3] = __REG(HSMMC_BASE+RSPREG3_OFFSET);	
		for (i=0; i<15; i++) { // ¿½±´»Ø¸´¼Ä´æÆ÷ÖÐµÄ[119:0]µ½pCSDÖÐ
			*pCSD++ = ((uint8_t *)Response)[i];
		}	
		State = 0; // CSD»ñÈ¡³É¹¦
	}

	Hsmmc_IssueCommand(CMD7, RCA<<16, 0, CMD_RESP_R1); // Ñ¡Ôñ¿¨,¿¨»Øµ½transfer×´Ì¬
	return State;
}

// R1»Ø¸´ÖÐ°üº¬ÁË32Î»µÄcard state,¿¨Ê¶±ðºó,¿ÉÔÚÈÎÒ»×´Ì¬Í¨¹ýCMD13»ñµÃ¿¨×´Ì¬
int32_t Hsmmc_GetCardState(void)
{
	if (Hsmmc_IssueCommand(CMD13, RCA<<16, 0, CMD_RESP_R1)) {
		return -1; // ¿¨³ö´í
	} else {
		return ((__REG(HSMMC_BASE+RSPREG0_OFFSET)>>9) & 0xf); // ·µ»ØR1»Ø¸´ÖÐµÄ[12:9]¿¨×´Ì¬
	}
}

static int32_t Hsmmc_SetBusWidth(uint8_t Width)
{
	int32_t State;
	if ((Width != 1) || (Width != 4)) {
		return -1;
	}
	State = -1; // ÉèÖÃ³õÊ¼ÎªÎ´³É¹¦
	__REGw(HSMMC_BASE+NORINTSTSEN_OFFSET) &= ~(1<<8); // ¹Ø±Õ¿¨ÖÐ¶Ï
	Hsmmc_IssueCommand(CMD55, RCA<<16, 0, CMD_RESP_R1);
	if (Width == 1) {
		if (!Hsmmc_IssueCommand(CMD6, 0, 0, CMD_RESP_R1)) { // 1Î»¿í
			__REGb(HSMMC_BASE+HOSTCTL_OFFSET) &= ~(1<<1);
			State = 0; // ÃüÁî³É¹¦
		}
	} else {
		if (!Hsmmc_IssueCommand(CMD6, 2, 0, CMD_RESP_R1)) { // 4Î»¿í
			__REGb(HSMMC_BASE+HOSTCTL_OFFSET) |= (1<<1);
			State = 0; // ÃüÁî³É¹¦
		}
	}
	__REGw(HSMMC_BASE+NORINTSTSEN_OFFSET) |= (1<<8); // ´ò¿ª¿¨ÖÐ¶Ï	
	return State; // ·µ»Ø0Îª³É¹¦
}

int32_t Hsmmc_EraseBlock(uint32_t StartBlock, uint32_t EndBlock)
{
	uint32_t i;

	if (CardType == SD_V1 || CardType == SD_V2) {
		StartBlock <<= 9; // ±ê×¼¿¨Îª×Ö½ÚµØÖ·
		EndBlock <<= 9;
	} else if (CardType != SD_HC) {
		return -1; // Î´Ê¶±ðµÄ¿¨
	}	
	Hsmmc_IssueCommand(CMD32, StartBlock, 0, CMD_RESP_R1);
	Hsmmc_IssueCommand(CMD33, EndBlock, 0, CMD_RESP_R1);
	if (!Hsmmc_IssueCommand(CMD38, 0, 0, CMD_RESP_R1B)) {
		for (i=0; i<0x10000; i++) {
			if (Hsmmc_GetCardState() == CARD_TRAN) { // ²Á³ýÍê³Éºó·µ»Øµ½transfer×´Ì¬
				Debug("Erasing complete!\r\n");
				return 0; // ²Á³ý³É¹¦				
			}
			Delay_us(1000);			
		}		
	}		

	Debug("Erase block failed\r\n");
	return -1; // ²Á³ýÊ§°Ü
}

int32_t Hsmmc_ReadBlock(uint8_t *pBuffer, uint32_t BlockAddr, uint32_t BlockNumber)
{
	uint32_t Address = 0;
	uint32_t ReadBlock;
	uint32_t i;
	uint32_t j;
	int32_t ErrorState;
	uint32_t Temp;
	
	if (pBuffer == 0 || BlockNumber == 0) {
		return -1;
	}

	__REGw(HSMMC_BASE+NORINTSTS_OFFSET) = __REGw(HSMMC_BASE+NORINTSTS_OFFSET); // Çå³ýÖÐ¶Ï±êÖ¾
	__REGw(HSMMC_BASE+ERRINTSTS_OFFSET) = __REGw(HSMMC_BASE+ERRINTSTS_OFFSET); // Çå³ý´íÎóÖÐ¶Ï±êÖ¾	
	
	while (BlockNumber > 0) {
		if (BlockNumber <= MAX_BLOCK) {
			ReadBlock = BlockNumber; // ¶ÁÈ¡µÄ¿éÊýÐ¡ÓÚ65536 Block
			BlockNumber = 0; // Ê£Óà¶ÁÈ¡¿éÊýÎª0
		} else {
			ReadBlock = MAX_BLOCK; // ¶ÁÈ¡µÄ¿éÊý´óÓÚ65536 Block,·Ö¶à´Î¶Á
			BlockNumber -= ReadBlock;
		}
		// ¸ù¾ÝsdÖ÷»ú¿ØÖÆÆ÷±ê×¼,°´Ë³ÐòÐ´ÈëÖ÷»ú¿ØÖÆÆ÷ÏàÓ¦µÄ¼Ä´æÆ÷		
		__REGw(HSMMC_BASE+BLKSIZE_OFFSET) = (7<<12) | (512<<0); // ×î´óDMA»º´æ´óÐ¡,blockÎª512×Ö½Ú			
		__REGw(HSMMC_BASE+BLKCNT_OFFSET) = ReadBlock; // Ð´ÈëÕâ´Î¶ÁblockÊýÄ¿
		
		if (CardType == SD_HC) {
			Address = BlockAddr; // SDHC¿¨Ð´ÈëµØÖ·ÎªblockµØÖ·
		} else if (CardType == SD_V1 || CardType == SD_V2) {
			Address = BlockAddr << 9; // ±ê×¼¿¨Ð´ÈëµØÖ·Îª×Ö½ÚµØÖ·
		}	
		BlockAddr += ReadBlock; // ÏÂÒ»´Î¶Á¿éµÄµØÖ·
		__REG(HSMMC_BASE+ARGUMENT_OFFSET) = Address; // Ð´ÈëÃüÁî²ÎÊý		
		
		if (ReadBlock == 1) {
			// ÉèÖÃ´«ÊäÄ£Ê½,DMA½ûÄÜ,¶Áµ¥¿é
			__REGw(HSMMC_BASE+TRNMOD_OFFSET) = (0<<5) | (1<<4) | (0<<2) | (1<<1) | (0<<0);
			// ÉèÖÃÃüÁî¼Ä´æÆ÷,µ¥¿é¶ÁCMD17,R1»Ø¸´
			__REGw(HSMMC_BASE+CMDREG_OFFSET) = (CMD17<<8)|(1<<5)|(1<<4)|(1<<3)|0x2;
		} else {
			// ÉèÖÃ´«ÊäÄ£Ê½,DMA½ûÄÜ,¶Á¶à¿é			
			__REGw(HSMMC_BASE+TRNMOD_OFFSET) = (1<<5) | (1<<4) | (1<<2) | (1<<1) | (0<<0);
			// ÉèÖÃÃüÁî¼Ä´æÆ÷,¶à¿é¶ÁCMD18,R1»Ø¸´	
			__REGw(HSMMC_BASE+CMDREG_OFFSET) = (CMD18<<8)|(1<<5)|(1<<4)|(1<<3)|0x2;			
		}	
		ErrorState = Hsmmc_WaitForCommandDone();
		if (ErrorState) {
			Debug("Read Command error\r\n");
			return ErrorState;
		}	
		for (i=0; i<ReadBlock; i++) {
			ErrorState = Hsmmc_WaitForBufferReadReady();
			if (ErrorState) {
				return ErrorState;
			}
			if (((uint32_t)pBuffer & 0x3) == 0) {	
				for (j=0; j<512/4; j++) {
					*(uint32_t *)pBuffer = __REG(HSMMC_BASE+BDATA_OFFSET);
					pBuffer += 4;
				}
			} else {
				for (j=0; j<512/4; j++) {
					Temp = __REG(HSMMC_BASE+BDATA_OFFSET);
					*pBuffer++ = (uint8_t)Temp;
					*pBuffer++ = (uint8_t)(Temp>>8);
					*pBuffer++ = (uint8_t)(Temp>>16);
					*pBuffer++ = (uint8_t)(Temp>>24);
				}
			}						
		}
		
		ErrorState = Hsmmc_WaitForTransferDone();
		if (ErrorState) {
			Debug("Read block error\r\n");
			return ErrorState;
		}	
	}
	return 0; // ËùÓÐ¿é¶ÁÍê
}

int32_t Hsmmc_WriteBlock(uint8_t *pBuffer, uint32_t BlockAddr, uint32_t BlockNumber)
{
	uint32_t Address = 0;
	uint32_t WriteBlock;
	uint32_t i;
	uint32_t j;
	int32_t ErrorState;
	
	if (pBuffer == 0 || BlockNumber == 0) {
		return -1; // ²ÎÊý´íÎó
	}	
	
	__REGw(HSMMC_BASE+NORINTSTS_OFFSET) = __REGw(HSMMC_BASE+NORINTSTS_OFFSET); // Çå³ýÖÐ¶Ï±êÖ¾
	__REGw(HSMMC_BASE+ERRINTSTS_OFFSET) = __REGw(HSMMC_BASE+ERRINTSTS_OFFSET); // Çå³ý´íÎóÖÐ¶Ï±êÖ¾
	
	while (BlockNumber > 0) {	
		if (BlockNumber <= MAX_BLOCK) {
			WriteBlock = BlockNumber;// Ð´ÈëµÄ¿éÊýÐ¡ÓÚ65536 Block
			BlockNumber = 0; // Ê£ÓàÐ´Èë¿éÊýÎª0
		} else {
			WriteBlock = MAX_BLOCK; // Ð´ÈëµÄ¿éÊý´óÓÚ65536 Block,·Ö¶à´ÎÐ´
			BlockNumber -= WriteBlock;
		}
		if (WriteBlock > 1) { // ¶à¿éÐ´,·¢ËÍACMD23ÏÈÉèÖÃÔ¤²Á³ý¿éÊý
			Hsmmc_IssueCommand(CMD55, RCA<<16, 0, CMD_RESP_R1);
			Hsmmc_IssueCommand(CMD23, WriteBlock, 0, CMD_RESP_R1);
		}
		
		// ¸ù¾ÝsdÖ÷»ú¿ØÖÆÆ÷±ê×¼,°´Ë³ÐòÐ´ÈëÖ÷»ú¿ØÖÆÆ÷ÏàÓ¦µÄ¼Ä´æÆ÷			
		__REGw(HSMMC_BASE+BLKSIZE_OFFSET) = (7<<12) | (512<<0); // ×î´óDMA»º´æ´óÐ¡,blockÎª512×Ö½Ú		
		__REGw(HSMMC_BASE+BLKCNT_OFFSET) = WriteBlock; // Ð´ÈëblockÊýÄ¿	
		
		if (CardType == SD_HC) {
			Address = BlockAddr; // SDHC¿¨Ð´ÈëµØÖ·ÎªblockµØÖ·
		} else if (CardType == SD_V1 || CardType == SD_V2) {
			Address = BlockAddr << 9; // ±ê×¼¿¨Ð´ÈëµØÖ·Îª×Ö½ÚµØÖ·
		}
		BlockAddr += WriteBlock; // ÏÂÒ»´ÎÐ´µØÖ·
		__REG(HSMMC_BASE+ARGUMENT_OFFSET) = Address; // Ð´ÈëÃüÁî²ÎÊý			
		
		if (WriteBlock == 1) {
			// ÉèÖÃ´«ÊäÄ£Ê½,DMA½ûÄÜÐ´µ¥¿é
			__REGw(HSMMC_BASE+TRNMOD_OFFSET) = (0<<5) | (0<<4) | (0<<2) | (1<<1) | (0<<0);
			// ÉèÖÃÃüÁî¼Ä´æÆ÷,µ¥¿éÐ´CMD24,R1»Ø¸´
			__REGw(HSMMC_BASE+CMDREG_OFFSET) = (CMD24<<8)|(1<<5)|(1<<4)|(1<<3)|0x2;			
		} else {
			// ÉèÖÃ´«ÊäÄ£Ê½,DMA½ûÄÜÐ´¶à¿é
			__REGw(HSMMC_BASE+TRNMOD_OFFSET) = (1<<5) | (0<<4) | (1<<2) | (1<<1) | (0<<0);
			// ÉèÖÃÃüÁî¼Ä´æÆ÷,¶à¿éÐ´CMD25,R1»Ø¸´
			__REGw(HSMMC_BASE+CMDREG_OFFSET) = (CMD25<<8)|(1<<5)|(1<<4)|(1<<3)|0x2;					
		}
		ErrorState = Hsmmc_WaitForCommandDone();
		if (ErrorState) {
			Debug("Write Command error\r\n");
			return ErrorState;
		}	
		
		for (i=0; i<WriteBlock; i++) {
			ErrorState = Hsmmc_WaitForBufferWriteReady();
			if (ErrorState) {
				return ErrorState;
			}
			if (((uint32_t)pBuffer & 0x3) == 0) {
				for (j=0; j<512/4; j++) {
					__REG(HSMMC_BASE+BDATA_OFFSET) = *(uint32_t *)pBuffer;
					pBuffer += 4;
				}
			} else {
				for (j=0; j<512/4; j++) {
					__REG(HSMMC_BASE+BDATA_OFFSET) = pBuffer[0] + ((uint32_t)pBuffer[1]<<8) +
					((uint32_t)pBuffer[2]<<16) + ((uint32_t)pBuffer[3]<<24);
					pBuffer += 4;
				}
			}	
		}
		
		ErrorState = Hsmmc_WaitForTransferDone();
		if (ErrorState) {
			Debug("Write block error\r\n");
			return ErrorState;
		}
		for (i=0; i<0x10000000; i++) {
			if (Hsmmc_GetCardState() == CARD_TRAN) { // ÐèÔÚtransfer status
				break; // ×´Ì¬ÕýÈ·
			}
		}
		if (i == 0x10000000) {
			return -3; // ×´Ì¬´íÎó»òProgramming³¬Ê±
		}		
	}
	return 0; // Ð´ÍêËùÓÐÊý¾Ý
}

int Hsmmc_Init(void)
{
	int32_t Timeout;
	uint32_t Capacity;
	uint32_t i;
	uint32_t OCR;
	uint32_t Temp;
	uint8_t SwitchStatus[64];
	SD_SCR SCR;
	uint8_t CSD[16];
	uint32_t c_size, c_size_multi, read_bl_len;	

	// ÉèÖÃHSMMCµÄ½Ó¿ÚÒý½ÅÅäÖÃ
#if (HSMMC_NUM == 0)
	// channel 0,GPG0[0:6] = CLK, CMD, CDn, DAT[0:3]
	GPG0CON_REG = 0x2222222;
	// pull up enable
	GPG0PUD_REG = 0x2aaa;
	GPG0DRV_REG = 0x3fff;
	// channel 0 clock src = SCLKEPLL = 96M
	CLK_SRC4_REG = (CLK_SRC4_REG & (~(0xf<<0))) | (0x7<<0);
	// channel 0 clock = SCLKEPLL/2 = 48M
	CLK_DIV4_REG = (CLK_DIV4_REG & (~(0xf<<0))) | (0x1<<0);	
	
#elif (HSMMC_NUM == 1)
	// channel 1,GPG1[0:6] = CLK, CMD, CDn, DAT[0:3]
	GPG1CON_REG = 0x2222222;
	// pull up enable
	GPG1PUD_REG = 0x2aaa;
	GPG1DRV_REG = 0x3fff;
	// channel 1 clock src = SCLKEPLL = 96M
	CLK_SRC4_REG = (CLK_SRC4_REG & (~(0xf<<4))) | (0x7<<4);
	// channel 1 clock = SCLKEPLL/2 = 48M
	CLK_DIV4_REG = (CLK_DIV4_REG & (~(0xf<<4))) | (0x1<<4);
	
#elif (HSMMC_NUM == 2)
	// channel 2,GPG2[0:6] = CLK, CMD, CDn, DAT[0:3]
	GPG2CON_REG = 0x2222222;
	// pull up enable
	GPG2PUD_REG = 0x2aaa;
	GPG2DRV_REG = 0x3fff;
	// channel 2 clock src = SCLKEPLL = 96M
	CLK_SRC4_REG = (CLK_SRC4_REG & (~(0xf<<8))) | (0x7<<8);
	// channel 2 clock = SCLKEPLL/2 = 48M
	CLK_DIV4_REG = (CLK_DIV4_REG & (~(0xf<<8))) | (0x1<<8);	
	
#elif (HSMMC_NUM == 3)
	// channel 3,GPG3[0:6] = CLK, CMD, CDn, DAT[0:3]
	GPG3CON_REG = 0x2222222;
	// pull up enable
	GPG3PUD_REG = 0x2aaa;
	GPG3DRV_REG = 0x3fff;
	// channel 3 clock src = SCLKEPLL = 96M
	CLK_SRC4_REG = (CLK_SRC4_REG & (~(0xf<<12))) | (0x7<<12);
	// channel 3 clock = SCLKEPLL/2 = 48M
	CLK_DIV4_REG = (CLK_DIV4_REG & (~(0xf<<12))) | (0x1<<12);	
	
#endif
	// software reset for all
	__REGb(HSMMC_BASE+SWRST_OFFSET) = 0x1;
	Timeout = 1000; // Wait max 10 ms
	while (__REGb(HSMMC_BASE+SWRST_OFFSET) & (1<<0)) {
		if (Timeout == 0) {
			return -1; // reset timeout
		}
		Timeout--;
		Delay_us(10);
	}	
	
	Hsmmc_SetClock(400000); // 400k
	__REGb(HSMMC_BASE+TIMEOUTCON_OFFSET) = 0xe; // ×î´ó³¬Ê±Ê±¼ä
	__REGb(HSMMC_BASE+HOSTCTL_OFFSET) &= ~(1<<2); // Õý³£ËÙ¶ÈÄ£Ê½
	// Çå³ýÕý³£ÖÐ¶Ï×´Ì¬±êÖ¾
	__REGw(HSMMC_BASE+NORINTSTS_OFFSET) = __REGw(HSMMC_BASE+NORINTSTS_OFFSET);
	// Çå³ý´íÎóÖÐ¶Ï×´Ì¬±êÖ¾
	__REGw(HSMMC_BASE+ERRINTSTS_OFFSET) = __REGw(HSMMC_BASE+ERRINTSTS_OFFSET);
	__REGw(HSMMC_BASE+NORINTSTSEN_OFFSET) = 0x7fff; // [14:0]ÖÐ¶Ï×´Ì¬Ê¹ÄÜ
	__REGw(HSMMC_BASE+ERRINTSTSEN_OFFSET) = 0x3ff; // [9:0]´íÎóÖÐ¶Ï×´Ì¬Ê¹ÄÜ
	__REGw(HSMMC_BASE+NORINTSIGEN_OFFSET) = 0x7fff; // [14:0]ÖÐ¶ÏÐÅºÅÊ¹ÄÜ	
	__REGw(HSMMC_BASE+ERRINTSIGEN_OFFSET) = 0x3ff; // [9:0]´íÎóÖÐ¶ÏÐÅºÅÊ¹ÄÜ
	
	Hsmmc_IssueCommand(CMD0, 0, 0, CMD_RESP_NONE); // ¸´Î»ËùÓÐ¿¨µ½¿ÕÏÐ×´Ì¬	
		
	CardType = UNUSABLE; // ¿¨ÀàÐÍ³õÊ¼»¯²»¿ÉÓÃ
	if (Hsmmc_IssueCommand(CMD8, 0x1aa, 0, CMD_RESP_R7)) { // Ã»ÓÐ»Ø¸´,MMC/sd v1.x/not card
			for (i=0; i<100; i++) {
				Hsmmc_IssueCommand(CMD55, 0, 0, CMD_RESP_R1);
				if (!Hsmmc_IssueCommand(CMD41, 0, 0, CMD_RESP_R3)) { // CMD41ÓÐ»Ø¸´ËµÃ÷Îªsd¿¨
					OCR = __REG(HSMMC_BASE+RSPREG0_OFFSET); // »ñµÃ»Ø¸´µÄOCR(²Ù×÷Ìõ¼þ¼Ä´æÆ÷)Öµ
					if (OCR & 0x80000000) { // ¿¨ÉÏµçÊÇ·ñÍê³ÉÉÏµçÁ÷³Ì,ÊÇ·ñbusy
						CardType = SD_V1; // ÕýÈ·Ê¶±ð³ösd v1.x¿¨
						Debug("SD card version 1.x is detected\r\n");
						break;
					}
				} else {
					// MMC¿¨Ê¶±ð
					Debug("MMC card is not supported\r\n");
					return -1;
				}
				Delay_us(1000);				
			}
	} else { // sd v2.0
		Temp = __REG(HSMMC_BASE+RSPREG0_OFFSET);
		if (((Temp&0xff) == 0xaa) && (((Temp>>8)&0xf) == 0x1)) { // ÅÐ¶Ï¿¨ÊÇ·ñÖ§³Ö2.7~3.3vµçÑ¹
			OCR = 0;
			for (i=0; i<100; i++) {
				OCR |= (1<<30);
				Hsmmc_IssueCommand(CMD55, 0, 0, CMD_RESP_R1);
				Hsmmc_IssueCommand(CMD41, OCR, 0, CMD_RESP_R3); // redayÌ¬
				OCR = __REG(HSMMC_BASE+RSPREG0_OFFSET);
				if (OCR & 0x80000000) { // ¿¨ÉÏµçÊÇ·ñÍê³ÉÉÏµçÁ÷³Ì,ÊÇ·ñbusy
					if (OCR & (1<<30)) { // ÅÐ¶Ï¿¨Îª±ê×¼¿¨»¹ÊÇ¸ßÈÝÁ¿¿¨
						CardType = SD_HC; // ¸ßÈÝÁ¿¿¨
						Debug("SDHC card is detected\r\n");
					} else {
						CardType = SD_V2; // ±ê×¼¿¨
						Debug("SD version 2.0 standard card is detected\r\n");
					}
					break;
				}
				Delay_us(1000);
			}
		}
	}
	if (CardType == SD_HC || CardType == SD_V1 || CardType == SD_V2) {
		Hsmmc_IssueCommand(CMD2, 0, 0, CMD_RESP_R2); // ÇëÇó¿¨·¢ËÍCID(¿¨ID¼Ä´æÆ÷)ºÅ,½øÈëident
		Hsmmc_IssueCommand(CMD3, 0, 0, CMD_RESP_R6); // ÇëÇó¿¨·¢²¼ÐÂµÄRCA(¿¨Ïà¶ÔµØÖ·),Stand-by×´Ì¬
		RCA = (__REG(HSMMC_BASE+RSPREG0_OFFSET) >> 16) & 0xffff; // ´Ó¿¨»Ø¸´ÖÐµÃµ½¿¨Ïà¶ÔµØÖ·
		Hsmmc_IssueCommand(CMD7, RCA<<16, 0, CMD_RESP_R1); // Ñ¡ÔñÒÑ±ê¼ÇµÄ¿¨,transfer×´Ì¬
		
		Hsmmc_Get_SCR(&SCR);
		if (SCR.SD_SPEC == 0) { // sd 1.0 - sd 1.01
			// Version 1.0 doesn't support switching
			Hsmmc_SetClock(24000000); // ÉèÖÃSDCLK = 48M/2 = 24M			
		} else { // sd 1.10 / sd 2.0
			Temp = 0;
			for (i=0; i<4; i++) {
				if (Hsmmc_Switch(0, 0, 1, SwitchStatus) == 0) { // switch check
					if (!(SwitchStatus[34] & (1<<1))) { // Group 1, function 1 high-speed bit 273					
						// The high-speed function is ready
						if (SwitchStatus[50] & (1<<1)) { // Group, function 1 high-speed support bit 401
							// high-speed is supported
							if (Hsmmc_Switch(1, 0, 1, SwitchStatus) == 0) { // switch
								if ((SwitchStatus[47] & 0xf) == 1) { // function switch in group 1 is ok?
									// result of the switch high-speed in function group 1
									Debug("Switch to high speed mode: CLK @ 50M\r\n");
									Hsmmc_SetClock(48000000); // ÉèÖÃSDCLK = 48M	
									Temp = 1;									
								}
							}
						}
						break;
					}
				} 
			}
			if (Temp == 0) {
				Hsmmc_SetClock(24000000); // ÉèÖÃSDCLK = 48M/2 = 24M
			}
		}
			
		if (!Hsmmc_SetBusWidth(4)) {
			Debug("Set bus width error\r\n");
			return -1; // Î»¿íÉèÖÃ³ö´í
		}
		if (Hsmmc_GetCardState() == CARD_TRAN) { // ´ËÊ±¿¨Ó¦ÔÚtransferÌ¬
			if (!Hsmmc_IssueCommand(CMD16, 512, 0, CMD_RESP_R1)) { // ÉèÖÃ¿é³¤¶ÈÎª512×Ö½Ú
				__REGw(HSMMC_BASE+NORINTSTS_OFFSET) = 0xffff; // Çå³ýÖÐ¶Ï±êÖ¾
				Hsmmc_Get_CSD(CSD);
				if ((CSD[15]>>6) == 0) { // CSD v1.0->sd V1.x, sd v2.00 standar
					read_bl_len = CSD[10] & 0xf; // [83:80]
					c_size_multi = ((CSD[6] & 0x3) << 1) + ((CSD[5] & 0x80) >> 7); // [49:47]
					c_size = ((int32_t)(CSD[9]&0x3) << 10) + ((int32_t)CSD[8]<<2) + (CSD[7]>>6); // [73:62]				
					Capacity = (c_size + 1) << ((c_size_multi + 2) + (read_bl_len-9)); // block(512 byte)
				} else {
					c_size = ((CSD[8]&0x3f) << 16) + (CSD[7]<<8) + CSD[6]; // [69:48]
					// ¿¨ÈÝÁ¿Îª×Ö½Ú(c_size+1)*512K byte,ÒÔ1ÉÈÇø512 byte×Ö,¿¨µÄÉÈÇøÊýÎª		
					Capacity = (c_size+1) << 10;// block (512 byte)
				}
				Debug("Card Initialization succeed\r\n");				
				Debug("Capacity: %ldMB\r\n", Capacity / (1024*1024 / 512));
				return 0; // ³õÊ¼»¯³É¹¦							
			}
		}
	}
	Debug("Card Initialization failed\r\n");
	return -1; // ¿¨¹¤×÷Òì³£
}
