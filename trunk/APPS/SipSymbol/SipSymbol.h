
#pragma once

//================================================================
// Returns number of elements
#define dim(x)	(sizeof(x) / sizeof(x[0])) 
//----------------------------------------------------------------------
// Generic defines and data types
struct decodeUINT {								// Structure associates
	UINT Code;									// messages
	LRESULT (*Fxn)(HWND, UINT, WPARAM, LPARAM);	// with a function.
};
struct decodeCMD {								// Structure associates
	UINT Code;									// menu IDs with a
	LRESULT (*Fxn)(HWND, WORD, HWND, WORD);		// function
};
//----------------------------------------------------------------------


//----------------------------------------------------------------------
#define CLASS_NAME		_T("_SIPSYMBOL_")
//----------------------------------------------------------------------
// Function prototypes
BOOL IsAlreadyExist(LPWSTR lpCmdLine);
HWND InitInstance(HINSTANCE, LPWSTR, int);
int TermInstance(HINSTANCE, int);
//----------------------------------------------------------------------
// Window procedures
LRESULT CALLBACK MainWndProc(HWND, UINT, WPARAM, LPARAM);
//----------------------------------------------------------------------


//----------------------------------------------------------------------
#define HC_ACTION		0
#define WH_KEYBOARD_LL	20
//----------------------------------------------------------------------
typedef LRESULT (WINAPI* HOOKPROC)(int, WPARAM, LPARAM);
typedef HHOOK (WINAPI* SETWINDOWSHOOKEX)(int, HOOKPROC, HINSTANCE, DWORD);
typedef BOOL (WINAPI* UNHOOKWINDOWSHOOKEX)(HHOOK);
typedef LRESULT (WINAPI* CALLNEXTHOOKEX)(HHOOK, int , WPARAM, LPARAM);
//----------------------------------------------------------------------
/*typedef struct {
	DWORD vkCode;
	DWORD scanCode;
	DWORD flags;
	DWORD time;
	ULONG_PTR dwExtraInfo;
} KBDLLHOOKSTRUCT, *PKBDLLHOOKSTRUCT;*/
//----------------------------------------------------------------------
LRESULT CALLBACK KeyHookProc(int nCode, WPARAM wParam, LPARAM lParam);
//----------------------------------------------------------------------
