
#ifndef _DISPLAY_EPD_H_
#define _DISPLAY_EPD_H_

typedef enum
{
	BITMAP_NONE=0x00,

	BITMAP_BOOTUP,
	BITMAP_BOOTMENU,
} EBITMAP_TYPE;

void EPDInitialize(void);
void EPDDisplayBitmap(EBITMAP_TYPE eBitmapType);
void EPDShowProgress(unsigned long dwCurrent, unsigned long dwTotal);

void EPDOutputString(const char *fmt, ...);
void EPDOutputChar(const unsigned char ch);
void EPDOutputFlush(void);

int EPDSerialFlashWrite(void);

#endif  _DISPLAY_EPD_H_

