
#include <windows.h>
#include <iic.h>
#include <bsp.h>


#define MYMSG(x)	RETAILMSG(0, x)
#define MYERR(x)	RETAILMSG(1, x)


#define	IIC_PMIC_ADDR	0xCC
#define IIC_POWER_ON	(1<<17)	// PCLK_GATE bit 17
#define	IIC_WRDATA		(1)
#define	IIC_POLLACK		(2)
#define	IIC_RDDATA		(3)
#define	IIC_SETRDADDR	(4)
#define	IIC_BUFSIZE		0x20


static volatile S3C6410_IIC_REG *g_pIICReg = NULL;
static volatile BSP_ARGS *g_pBspArgs = NULL;
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

void IIC_Initialize(volatile BSP_ARGS *pBspArgs)
{
	PHYSICAL_ADDRESS ioPhysicalBase = {0,0};

	ioPhysicalBase.LowPart = S3C6410_BASE_REG_PA_IICBUS;
	g_pIICReg = (S3C6410_IIC_REG *)MmMapIoSpace(ioPhysicalBase, sizeof(S3C6410_IIC_REG), FALSE);

	g_pBspArgs = pBspArgs;
}
void IIC_WriteRegister(UCHAR Reg, UCHAR Val)
{
    g_iicMode		= IIC_WRDATA;
    g_iicPt			= 0;
    g_iicData[0]	= Reg;
    g_iicData[1]	= Val;
    g_iicDataCount	= 2;

	g_pIICReg->IICDS	= IIC_PMIC_ADDR;
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
		g_pIICReg->IICDS	= IIC_PMIC_ADDR;
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

	if (0x00 == Reg)
		g_pBspArgs->bPMICRegister_00 = Val;
	else if (0x01 == Reg)
		g_pBspArgs->bPMICRegister_01 = Val;
}
UCHAR IIC_ReadRegister(UCHAR Reg)
{
	UCHAR bRet = 0;

	if (0x00 == Reg)
		bRet = g_pBspArgs->bPMICRegister_00;
	else if (0x01 == Reg)
		bRet = g_pBspArgs->bPMICRegister_01;
	else
	{
		g_iicMode		= IIC_SETRDADDR;
		g_iicPt			= 0;
		g_iicData[0]	= Reg;
		g_iicDataCount	= 1;

		g_pIICReg->IICDS	= IIC_PMIC_ADDR;
		g_pIICReg->IICSTAT	= 0xf0;	//MasTx,Start
		//Clearing the pending bit isn't needed because the pending bit has been cleared.
		while (g_iicDataCount != -1)
			iicPolling();

		g_iicMode		= IIC_RDDATA;
		g_iicPt			= 0;
		g_iicDataCount	= 1;

		g_pIICReg->IICDS	= IIC_PMIC_ADDR;
		g_pIICReg->IICSTAT	= 0xb0;	//Master Rx,Start
		g_pIICReg->IICCON	= 0xaf;	//Resumes IIC operation.
		while (g_iicDataCount != -1)
			iicPolling();

		bRet = g_iicData[1];
	}

	return bRet;
}

void IIC_OtgPower(BOOL bOn)
{
	UCHAR Val;

	if (NULL == g_pIICReg)	// 왜냐면 iic 드라이버가 먼저 초기화를 해야 한다.
		return;

	if (bOn)
	{
		g_pIICReg->IICCON  = (1<<7) | (0<<6) | (1<<5) | (0xf);
		g_pIICReg->IICSTAT = 0x10;	//IIC bus data output enable(Rx/Tx)
	}

	// ELDO3 - VDD_OTGI
	Val = IIC_ReadRegister(0x00);
	Val = (Val & ~(1<<3)) | (bOn ? (1<<3) : (0<<3));
	IIC_WriteRegister(0x00, Val);

	// ELDO8 - VDD_OTG
	Val = IIC_ReadRegister(0x01);
	Val = (Val & ~(1<<5)) | (bOn ? (1<<5) : (0<<5));
	IIC_WriteRegister(0x01, Val);
}
