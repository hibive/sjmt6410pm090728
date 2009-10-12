
#ifndef _KEYPAD_H_
#define _KEYPAD_H_

typedef enum
{
	KEY_NONE=0x00,

	KEY_RIGHT,
	KEY_F13,
	KEY_F14,
	KEY_F15,

	KEY_F16,
	KEY_F17,
	KEY_MENU,
	KEY_LEFT,

	KEY_UP,
	KEY_DOWN,
	KEY_VOLDOWN,
	KEY_VOLUP,

	KEY_ENTER,

	KEY_HOLD=0x80,
} EKEY_DATA;

void InitializeKeypad(void);
EKEY_DATA GetKeypad(void);

#endif  _KEYPAD_H_

