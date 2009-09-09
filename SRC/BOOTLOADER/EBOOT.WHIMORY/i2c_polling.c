
#include <bsp.h>


#define IIC_POWER_ON	(1<<17)	// PCLK_GATE bit 17

#define	IIC_WRDATA		(1)
#define	IIC_POLLACK		(2)
#define	IIC_RDDATA		(3)
#define	IIC_SETRDADDR	(4)
#define	IIC_BUFSIZE		0x20


static volatile S3C6410_IIC_REG *g_pIICReg = NULL;
static volatile S3C6410_GPIO_REG *g_pGPIOReg = NULL;
static volatile S3C6410_SYSCON_REG *g_pSysConReg = NULL;

static UINT8 g_iicData[IIC_BUFSIZE];
static int g_iicDataCount = 0;
static int g_iicStatus = 0;
static int g_iicMode = 0;
static int g_iicPt = 0;


#define MSEC_DELAY_SCALER	(10000)
static void iicDelay(unsigned long dwMSecs)
{
	volatile int dwScaledSecs = (dwMSecs * MSEC_DELAY_SCALER);
	while (dwScaledSecs--);
}
static void iicPolling(void)
{
	unsigned long iicSt, i;

	if (g_pIICReg->IICCON & 0x10)	//Tx/Rx Interrupt Enable
	{
		iicSt = g_pIICReg->IICSTAT;
		if (iicSt & 0x8){}	//When bus arbitration is failed.
		if (iicSt & 0x4){}	//When a slave address is matched with IICADD
		if (iicSt & 0x2){}	//When a slave address is 0000000b
		if (iicSt & 0x1){}	//When ACK isn't received

		switch (g_iicMode)
		{
		case IIC_POLLACK:
			g_iicStatus = iicSt;
			break;
		case IIC_RDDATA:
			if ((g_iicDataCount--) == 0)
			{
				g_iicData[g_iicPt++] = g_pIICReg->IICDS;

				g_pIICReg->IICSTAT = 0x90;	//Stop MasRx condition
				g_pIICReg->IICCON  = 0xaf;	//Resumes IIC operation.
				iicDelay(1);				//Wait until stop condtion is in effect.
				//Too long time...
				//The pending bit will not be set after issuing stop condition.
                break;
            }
			g_iicData[g_iicPt++] = g_pIICReg->IICDS;
			//The last data has to be read with no ack.
			if ((g_iicDataCount) == 0)
				g_pIICReg->IICCON = 0x2f;	//Resumes IIC operation with NOACK.
			else
				g_pIICReg->IICCON = 0xaf;	//Resumes IIC operation with ACK
			break;
        case IIC_WRDATA:
			if ((g_iicDataCount--) == 0)
			{
				g_pIICReg->IICSTAT = 0xd0;	//stop MasTx condition
				g_pIICReg->IICCON  = 0xaf;	//resumes IIC operation.
				iicDelay(1);				//wait until stop condtion is in effect.
				//The pending bit will not be set after issuing stop condition.
				break;
			}
			g_pIICReg->IICDS = g_iicData[g_iicPt++];	//g_iicData[0] has dummy
			for (i=0; i<10; i++);		//for setup time until rising edge of IICSCL
			g_pIICReg->IICCON = 0xaf;	//resumes IIC operation.
			break;
		case IIC_SETRDADDR:
			if ((g_iicDataCount--) == 0)
				break;	//IIC operation is stopped because of IICCON[4]    
			g_pIICReg->IICDS = g_iicData[g_iicPt++];
			for (i=0; i<10; i++);		//for setup time until rising edge of IICSCL
			g_pIICReg->IICCON = 0xaf;	//resumes IIC operation.
			break;
		default:
			break;
		}
	}
}


void IICInitialize(void)
{
	if (NULL == g_pIICReg)
	    g_pIICReg = (S3C6410_IIC_REG *)OALPAtoVA(S3C6410_BASE_REG_PA_IICBUS, FALSE);

	if (NULL == g_pGPIOReg)
	    g_pGPIOReg = (S3C6410_GPIO_REG *)OALPAtoVA(S3C6410_BASE_REG_PA_GPIO, FALSE);

	if (NULL == g_pSysConReg)
		g_pSysConReg = (S3C6410_SYSCON_REG *)OALPAtoVA(S3C6410_BASE_REG_PA_SYSCON, FALSE);

	g_pGPIOReg->GPBPUD = (g_pGPIOReg->GPBPUD & ~(0xF<<10)) | (0x0<<10);	// Pull-up/down disable
	g_pGPIOReg->GPBCON = (g_pGPIOReg->GPBCON & ~(0xFF<<20)) | (0x22<<20);	// GPB6:IICSDA , GPB5:IICSCL

	//Enable ACK, Prescaler IICCLK=PCLK/16, Enable interrupt, Transmit clock value Tx clock=IICCLK/16
	g_pIICReg->IICCON  = (1<<7) | (0<<6) | (1<<5) | (0xf);
	g_pIICReg->IICSTAT = 0x10;	//IIC bus data output enable(Rx/Tx)

	g_pSysConReg->PCLK_GATE |= IIC_POWER_ON;
}

void IICWriteByte(unsigned long slvAddr, unsigned long addr, unsigned char data)
{
    g_iicMode		= IIC_WRDATA;
    g_iicPt			= 0;
    g_iicData[0]	= (unsigned char)addr;
    g_iicData[1]	= data;
    g_iicDataCount	= 2;

	g_pIICReg->IICDS	= slvAddr;	//0xa0
	//Master Tx mode, Start(Write), IIC-bus data output enable
	//Bus arbitration sucessful, Address as slave status flag Cleared,
	//Address zero status flag cleared, Last received bit is 0
	g_pIICReg->IICSTAT	= 0xf0;
	//Clearing the pending bit isn't needed because the pending bit has been cleared.
	while (g_iicDataCount != -1)
		iicPolling();

	g_iicMode = IIC_POLLACK;
	while (1)
	{
		g_pIICReg->IICDS	= slvAddr;
		g_iicStatus			= 0x100;	//To check if g_iicStatus is changed
		g_pIICReg->IICSTAT	= 0xf0;		//Master Tx, Start, Output Enable, Sucessful, Cleared, Cleared, 0
		g_pIICReg->IICCON	= 0xaf;		//Resumes IIC operation.
		while (g_iicStatus == 0x100)
			iicPolling();

		if (!(g_iicStatus & 0x1))
			break;	//When ACK is received
	}
	g_pIICReg->IICSTAT	= 0xd0;	//Master Tx condition, Stop(Write), Output Enable
	g_pIICReg->IICCON	= 0xaf;	//Resumes IIC operation.
	iicDelay(10);	//Wait until stop condtion is in effect.
	//Write is completed.
}
        
void IICReadByte(unsigned long slvAddr, unsigned long addr, unsigned char *data)
{
	g_iicMode		= IIC_SETRDADDR;
	g_iicPt			= 0;
	g_iicData[0]	= (unsigned char)addr;
	g_iicDataCount	= 1;

	g_pIICReg->IICDS	= slvAddr;
	g_pIICReg->IICSTAT	= 0xf0;	//MasTx,Start
	//Clearing the pending bit isn't needed because the pending bit has been cleared.
	while (g_iicDataCount != -1)
		iicPolling();
	
	g_iicMode		= IIC_RDDATA;
	g_iicPt			= 0;
	g_iicDataCount	= 1;

	g_pIICReg->IICDS	= slvAddr;
	g_pIICReg->IICSTAT	= 0xb0;	//Master Rx,Start
	g_pIICReg->IICCON	= 0xaf;	//Resumes IIC operation.
	while (g_iicDataCount != -1)
		iicPolling();

	*data = g_iicData[1];
}
