
#ifndef _DISPLAY_EPD_H_
#define _DISPLAY_EPD_H_

typedef enum
{
	IMAGE_NONE=0x00,

	IMAGE_BOOTUP,
	IMAGE_BOOTMENU,
} EIMAGE_TYPE;

void EPDInitialize(void);
void EPDDisplayImage(EIMAGE_TYPE eImageType);
void EPDShowProgress(unsigned long dwCurrent, unsigned long dwTotal);

void EPDOutputString(const char *fmt, ...);
void EPDOutputChar(const unsigned char ch);
void EPDOutputFlush(void);

int EPDSerialFlashWrite(void);

#endif  _DISPLAY_EPD_H_

