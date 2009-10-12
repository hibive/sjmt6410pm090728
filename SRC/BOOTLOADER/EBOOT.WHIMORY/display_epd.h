
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

void EPDWriteEngFont8x16(const char *fmt, ...);
void EPDFlushEngFont8x16(void);

int EPDSerialFlashWrite(void);

#endif  _DISPLAY_EPD_H_

