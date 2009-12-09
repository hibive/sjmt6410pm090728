//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
//
// Use of this sample source code is subject to the terms of the Microsoft
// license agreement under which you licensed this sample source code. If
// you did not accept the terms of the license agreement, you are not
// authorized to use this sample source code. For the terms of the license,
// please see the license agreement between you and Microsoft or, if applicable,
// see the LICENSE.RTF on your install media or the root of your tools installation.
// THE SAMPLE SOURCE CODE IS PROVIDED "AS IS", WITH NO WARRANTIES.
//
#include <windows.h>
#include <LayMgr.h>
#include "InputLang.h"


/***************************************************************************\
* aVkToBits[]  - map Virtual Keys to Modifier Bits
*
* See InputLang.h for a full description.
*
* Korean Keyboard has only three shifter keys:
*     SHIFT (L & R) affects alphanumeric keys,
*     CTRL  (L & R) is used to generate control characters
*     ALT   (L & R) used for generating characters by number with numpad
\***************************************************************************/
static VK_TO_BIT aVkToBits[] = {
    { VK_SHIFT    ,   KBDSHIFT     },
    { VK_CONTROL  ,   KBDCTRL      },
    { VK_MENU     ,   KBDALT       },
    { 0           ,   0            }
};

/***************************************************************************\
* aModification[]  - map character modifier bits to modification number
*
* See InputLang.h for a full description.
*
\***************************************************************************/

static MODIFIERS CharModifiers = {
    &aVkToBits[0],
    3,
    {
    //  Modification# //  Keys Pressed
    //  ============= // =============
        0,            // 
        1,            // Shift 
        2,            // Control 
        3             // Shift + Control 
     }
};

/***************************************************************************\
*
* aVkToWch2[]  - Virtual Key to WCHAR translation for 2 shift states
* aVkToWch3[]  - Virtual Key to WCHAR translation for 3 shift states
* aVkToWch4[]  - Virtual Key to WCHAR translation for 4 shift states
*
* Table attributes: Unordered Scan, null-terminated
*
* Search this table for an entry with a matching Virtual Key to find the
* corresponding unshifted and shifted WCHAR characters.
*
* Special values for VirtualKey (column 1)
*     0xff          - dead chars for the previous entry
*     0             - terminate the list
*
* Special values for Attributes (column 2)
*     CAPLOK bit    - CAPS-LOCK affect this key like SHIFT
*
* Special values for wch[*] (column 3 & 4)
*     WCH_NONE      - No character
*     WCH_DEAD      - Dead Key (diaresis) or invalid (US keyboard has none)
*     WCH_LGTR      - Ligature (generates multiple characters)
*
\***************************************************************************/


#define VK_OEM_3        VK_BACKQUOTE
#define VK_OEM_PLUS     VK_EQUAL
#define VK_OEM_1        VK_SEMICOLON
#define VK_OEM_7        VK_APOSTROPHE
#define VK_OEM_8        VK_OFF
#define VK_OEM_COMMA    VK_COMMA
#define VK_OEM_PERIOD   VK_PERIOD
#define VK_OEM_2        VK_SLASH
#define VK_OEM_4        VK_LBRACKET
#define VK_OEM_6        VK_RBRACKET
#define VK_OEM_5        VK_BACKSLASH
#define VK_OEM_MINUS    VK_HYPHEN


static VK_TO_WCHARS2 aVkToWch2[] = {
    //                     |          |   SHIFT  |
    //                     |==========|==========|
    {'0'          , 0      ,'0'       ,')'       },
    {'1'          , 0      ,'1'       ,'!'       },
    {'3'          , 0      ,'3'       ,'#'       },
    {'4'          , 0      ,'4'       ,'$'       },
    {'5'          , 0      ,'5'       ,'%'       },
    {'7'          , 0      ,'7'       ,'&'       },
    {'8'          , 0      ,'8'       ,'*'       },
    {'9'          , 0      ,'9'       ,'('       },
    {'A'          , CAPLOK ,'a'       ,'A'       },
    {'B'          , CAPLOK ,'b'       ,'B'       },
    {'C'          , CAPLOK ,'c'       ,'C'       },
    {'D'          , CAPLOK ,'d'       ,'D'       },
    {'E'          , CAPLOK ,'e'       ,'E'       },
    {'F'          , CAPLOK ,'f'       ,'F'       },
    {'G'          , CAPLOK ,'g'       ,'G'       },
    {'H'          , CAPLOK ,'h'       ,'H'       },
    {'I'          , CAPLOK ,'i'       ,'I'       },
    {'J'          , CAPLOK ,'j'       ,'J'       },
    {'K'          , CAPLOK ,'k'       ,'K'       },
    {'L'          , CAPLOK ,'l'       ,'L'       },
    {'M'          , CAPLOK ,'m'       ,'M'       },
    {'N'          , CAPLOK ,'n'       ,'N'       },
    {'O'          , CAPLOK ,'o'       ,'O'       },
    {'P'          , CAPLOK ,'p'       ,'P'       },
    {'Q'          , CAPLOK ,'q'       ,'Q'       },
    {'R'          , CAPLOK ,'r'       ,'R'       },
    {'S'          , CAPLOK ,'s'       ,'S'       },
    {'T'          , CAPLOK ,'t'       ,'T'       },
    {'U'          , CAPLOK ,'u'       ,'U'       },
    {'V'          , CAPLOK ,'v'       ,'V'       },
    {'W'          , CAPLOK ,'w'       ,'W'       },
    {'X'          , CAPLOK ,'x'       ,'X'       },
    {'Y'          , CAPLOK ,'y'       ,'Y'       },
    {'Z'          , CAPLOK ,'z'       ,'Z'       },
    {VK_OEM_1     , 0      ,';'       ,':'       },
    {VK_OEM_2     , 0      ,'/'       ,'?'       },
    {VK_OEM_3     , 0      ,'`'       ,'~'       },
    {VK_OEM_7     , 0      ,0x27      ,'"'       },
    {VK_OEM_8     , 0      ,WCH_NONE  ,WCH_NONE  },
#ifdef	OMNIBOOK_VER
    {VK_OEM_COMMA , 0      ,'>'       ,'<'       },
    {VK_OEM_PERIOD, 0      ,'.'       ,','       },
#else	//!OMNIBOOK_VER
    {VK_OEM_COMMA , 0      ,','       ,'<'       },
    {VK_OEM_PERIOD, 0      ,'.'       ,'>'       },
#endif	OMNIBOOK_VER
    {VK_OEM_PLUS  , 0      ,'='       ,'+'       },
    {VK_TAB       , 0      ,'\t'      ,'\t'      },
    {VK_ADD       , 0      ,'+'       ,'+'       },
    {VK_DECIMAL   , 0      ,'.'       ,'.'       },
    {VK_DIVIDE    , 0      ,'/'       ,'/'       },
    {VK_MULTIPLY  , 0      ,'*'       ,'*'       },
    {VK_SUBTRACT  , 0      ,'-'       ,'-'       },
    {0            , 0      ,0         ,0         }
};

static VK_TO_WCHARS3 aVkToWch3[] = {
    //                     |          |   SHIFT  |  CONTROL  |
    //                     |          |==========|===========|
    {VK_BACK      , 0      ,'\b'      ,'\b'      , 0x7f      },
    {VK_CANCEL    , 0      ,0x03      ,0x03      , 0x03      },
    {VK_ESCAPE    , 0      ,0x1b      ,0x1b      , 0x1b      },
    {VK_OEM_4     , 0      ,'['       ,'{'       , 0x1b      },
    {VK_OEM_5     , 0      ,'\\'      ,'|'       , 0x1c      },
    {VK_OEM_102   , 0      ,'\\'      ,'|'       , 0x1c      },
    {VK_OEM_6     , 0      ,']'       ,'}'       , 0x1d      },
    {VK_RETURN    , 0      ,'\r'      ,'\r'      , '\n'      },
    {VK_SPACE     , 0      ,' '       ,' '       , 0x20      },
    {0            , 0      ,0         ,0         , 0         }
};

static VK_TO_WCHARS4 aVkToWch4[] = {
    //                     |          |   SHIFT  |  CONTROL  | SHFT+CTRL |
    //                     |          |==========|===========|===========|
    {'2'          , 0      ,'2'       ,'@'       , WCH_NONE  , 0x00      },
    {'6'          , 0      ,'6'       ,'^'       , WCH_NONE  , 0x1e      },
    {VK_OEM_MINUS , 0      ,'-'       ,'_'       , WCH_NONE  , 0x1f      },
    {0            , 0      ,0         ,0         , 0         , 0         }
};

// Put this last so that VkKeyScan interprets number characters
// as coming from the main section of the kbd (aVkToWch2 and
// aVkToWch4) before considering the numpad (aVkToWch1).

static VK_TO_WCHARS1 aVkToWch1[] = {
    { VK_NUMPAD0   , 0      ,  '0'   },
    { VK_NUMPAD1   , 0      ,  '1'   },
    { VK_NUMPAD2   , 0      ,  '2'   },
    { VK_NUMPAD3   , 0      ,  '3'   },
    { VK_NUMPAD4   , 0      ,  '4'   },
    { VK_NUMPAD5   , 0      ,  '5'   },
    { VK_NUMPAD6   , 0      ,  '6'   },
    { VK_NUMPAD7   , 0      ,  '7'   },
    { VK_NUMPAD8   , 0      ,  '8'   },
    { VK_NUMPAD9   , 0      ,  '9'   },
    { 0            , 0      ,  '\0'  }   //null terminator
};


/***************************************************************************\
* aVkToWcharTable: table of pointers to Character Tables
*
* Describes the character tables and the order they should be searched.
\***************************************************************************/

static  VK_TO_WCHAR_TABLE aVkToWcharTable[] = {
    {  (PVK_TO_WCHARS1)aVkToWch3, 3, sizeof(aVkToWch3[0]) },
    {  (PVK_TO_WCHARS1)aVkToWch4, 4, sizeof(aVkToWch4[0]) },
    {  (PVK_TO_WCHARS1)aVkToWch2, 2, sizeof(aVkToWch2[0]) },
    {  (PVK_TO_WCHARS1)aVkToWch1, 1, sizeof(aVkToWch1[0]) },
    {                       NULL, 0, 0                    },
};


static const VKEY_TO_SCANCODE VKeyToXTScanCodeTable[] =
{
{ /* 00                                */    0,               0 },
{ /* 01 VK_LBUTTON                     */    0,               0 },
{ /* 02 VK_RBUTTON                     */    0,               0 },
{ /* 03 VK_CANCEL                      */    SC_E0,        0x46 },
{ /* 04 VK_MBUTTON                     */    0,               0 },
{ /* 05                                */    0,               0 },
{ /* 06                                */    0,               0 },
{ /* 07                                */    0,               0 },
{ /* 08 VK_BACK                        */    0,            0x0e },
{ /* 09 VK_TAB                         */    0,            0x0f },
{ /* 0A                                */    0,               0 },
{ /* 0B                                */    0,               0 },
{ /* 0C VK_CLEAR                       */    0,            0x4c },
{ /* 0D VK_RETURN                      */    0,            0x1c },
{ /* 0E                                */    0,               0 },
{ /* 0F                                */    0,               0 },

{ /* 10 VK_SHIFT                       */    0,            0x2a },
{ /* 11 VK_CONTROL                     */    0,            0x1d },
{ /* 12 VK_MENU                        */    0,            0x38 },
{ /* 13 VK_PAUSE                       */    0,               0 },
{ /* 14 VK_CAPITAL                     */    0,            0x3a },
{ /* 15 VK_HANGUL                      */    0,            0x72 }, // 72 is what desktop Windows expects
{ /* 16                                */    0,               0 },
{ /* 17 VK_JUNJA                       */    0,               0 },
{ /* 18 VK_FINAL                       */    0,               0 },
{ /* 19 VK_HANJA                       */    0,            0x71 },
{ /* 1A                                */    0,               0 },
{ /* 1B VK_ESCAPE                      */    0,            0x01 },
{ /* 1C VK_CONVERT                     */    0,               0 },
{ /* 1D VK_NOCONVERT                   */    0,               0 },
{ /* 1E                                */    0,               0 },
{ /* 1F                                */    0,               0 },

{ /* 20 VK_SPACE                       */    0,            0x39 },
{ /* 21 VK_PRIOR                       */    SC_E0,        0x49 },
{ /* 22 VK_NEXT                        */    SC_E0,        0x51 },
{ /* 23 VK_END                         */    SC_E0,        0x4f },
{ /* 24 VK_HOME                        */    SC_E0,        0x47 },
{ /* 25 VK_LEFT                        */    SC_E0,        0x4b },
{ /* 26 VK_UP                          */    SC_E0,        0x48 },
{ /* 27 VK_RIGHT                       */    SC_E0,        0x4d },
{ /* 28 VK_DOWN                        */    SC_E0,        0x50 },
{ /* 29 VK_SELECT                      */    0,               0 },
{ /* 2A VK_PRINT                       */    0,               0 },
{ /* 2B VK_EXECUTE                     */    0,               0 },
{ /* 2C VK_SNAPSHOT                    */    SC_E0,        0x37 },
{ /* 2D VK_INSERT                      */    SC_E0,        0x52 },
{ /* 2E VK_DELETE                      */    SC_E0,        0x53 },
{ /* 2F VK_HELP                        */    0,            0x63 },

{ /* 30 '0'                            */    0,            0x0b },
{ /* 31 '1'                            */    0,            0x02 },
{ /* 32 '2'                            */    0,            0x03 },
{ /* 33 '3'                            */    0,            0x04 },
{ /* 34 '4'                            */    0,            0x05 },
{ /* 35 '5'                            */    0,            0x06 },
{ /* 36 '6'                            */    0,            0x07 },
{ /* 37 '7'                            */    0,            0x08 },
{ /* 38 '8'                            */    0,            0x09 },
{ /* 39 '9'                            */    0,            0x0a },
{ /* 3A                                */    0,               0 },
{ /* 3B                                */    0,               0 },
{ /* 3C                                */    0,               0 },
{ /* 3D                                */    0,               0 },
{ /* 3E                                */    0,               0 },
{ /* 3F                                */    0,               0 },

{ /* 40                                */    0,               0 },
{ /* 41 'A'                            */    0,            0x1e },
{ /* 42 'B'                            */    0,            0x30 },
{ /* 43 'C'                            */    0,            0x2e },
{ /* 44 'D'                            */    0,            0x20 },
{ /* 45 'E'                            */    0,            0x12 },
{ /* 46 'F'                            */    0,            0x21 },
{ /* 47 'G'                            */    0,            0x22 },
{ /* 48 'H'                            */    0,            0x23 },
{ /* 49 'I'                            */    0,            0x17 },
{ /* 4A 'J'                            */    0,            0x24 },
{ /* 4B 'K'                            */    0,            0x25 },
{ /* 4C 'L'                            */    0,            0x26 },
{ /* 4D 'M'                            */    0,            0x32 },
{ /* 4E 'N'                            */    0,            0x31 },
{ /* 4F 'O'                            */    0,            0x18 },

{ /* 50 'P'                            */    0,            0x19 },
{ /* 51 'Q'                            */    0,            0x10 },
{ /* 52 'R'                            */    0,            0x13 },
{ /* 53 'S'                            */    0,            0x1f },
{ /* 54 'T'                            */    0,            0x14 },
{ /* 55 'U'                            */    0,            0x16 },
{ /* 56 'V'                            */    0,            0x2f },
{ /* 57 'W'                            */    0,            0x11 },
{ /* 58 'X'                            */    0,            0x2d },
{ /* 59 'Y'                            */    0,            0x15 },
{ /* 5A 'Z'                            */    0,            0x2c },
{ /* 5B VK_LWIN                        */    SC_E0,        0x5b },
{ /* 5C VK_RWIN                        */    SC_E0,        0x5c },
{ /* 5D VK_APPS                        */    SC_E0,        0x5d },
{ /* 5E                                */    0,               0 },
{ /* 5F VK_SLEEP                       */    0,               0 },

{ /* 60 VK_NUMPAD0                     */    0,            0x52 },
{ /* 61 VK_NUMPAD1                     */    0,            0x4f },
{ /* 62 VK_NUMPAD2                     */    0,            0x50 },
{ /* 63 VK_NUMPAD3                     */    0,            0x51 },
{ /* 64 VK_NUMPAD4                     */    0,            0x4b },
{ /* 65 VK_NUMPAD5                     */    0,            0x4c },
{ /* 66 VK_NUMPAD6                     */    0,            0x4d },
{ /* 67 VK_NUMPAD7                     */    0,            0x47 },
{ /* 68 VK_NUMPAD8                     */    0,            0x48 },
{ /* 69 VK_NUMPAD9                     */    0,            0x49 },
{ /* 6A VK_MULTIPLY                    */    0,            0x37 },
{ /* 6B VK_ADD                         */    0,            0x4e },
{ /* 6C VK_SEPARATOR                   */    0,               0 },
{ /* 6D VK_SUBTRACT                    */    0,            0x4a },
{ /* 6E VK_DECIMAL                     */    0,            0x53 },
{ /* 6F VK_DIVIDE                      */    SC_E0,        0x35 },

{ /* 70 VK_F1                          */    0,            0x3b },
{ /* 71 VK_F2                          */    0,            0x3c },
{ /* 72 VK_F3                          */    0,            0x3d },
{ /* 73 VK_F4                          */    0,            0x3e },
{ /* 74 VK_F5                          */    0,            0x3f },
{ /* 75 VK_F6                          */    0,            0x40 },
{ /* 76 VK_F7                          */    0,            0x41 },
{ /* 77 VK_F8                          */    0,            0x42 },
{ /* 78 VK_F9                          */    0,            0x43 },
{ /* 79 VK_F10                         */    0,            0x44 },
{ /* 7A VK_F11                         */    0,            0x57 },
{ /* 7B VK_F12                         */    0,            0x58 },
{ /* 7C VK_F13                         */    0,            0x64 },
{ /* 7D VK_F14                         */    0,            0x65 },
{ /* 7E VK_F15                         */    0,            0x66 },
{ /* 7F VK_F16                         */    0,            0x67 },

{ /* 80 VK_F17                         */    0,            0x68 },
{ /* 81 VK_F18                         */    0,            0x69 },
{ /* 82 VK_F19                         */    0,            0x6a },
{ /* 83 VK_F20                         */    0,            0x6b },
{ /* 84 VK_F21                         */    0,            0x6c },
{ /* 85 VK_F22                         */    0,            0x6d },
{ /* 86 VK_F23                         */    0,            0x6e },
{ /* 87 VK_F24                         */    0,            0x76 },
{ /* 88                                */    0,               0 },
{ /* 89                                */    0,               0 },
{ /* 8A                                */    0,               0 },
{ /* 8B                                */    0,               0 },
{ /* 8C                                */    0,               0 },
{ /* 8D                                */    0,               0 },
{ /* 8E                                */    0,               0 },
{ /* 8F                                */    0,               0 },

{ /* 90 VK_NUMLOCK                     */    0,            0x45 },
{ /* 91 VK_SCROLL                      */    0,            0x46 },
{ /* 92                                */    0,               0 },
{ /* 93                                */    0,               0 },
{ /* 94                                */    0,               0 },
{ /* 95                                */    0,               0 },
{ /* 96                                */    0,               0 },
{ /* 97                                */    0,               0 },
{ /* 98                                */    0,               0 },
{ /* 99                                */    0,               0 },
{ /* 9A                                */    0,               0 },
{ /* 9B                                */    0,               0 },
{ /* 9C                                */    0,               0 },
{ /* 9D                                */    0,               0 },
{ /* 9E                                */    0,               0 },
{ /* 9F                                */    0,               0 },

{ /* A0 VK_LSHIFT                      */    0,            0x2a },
{ /* A1 VK_RSHIFT                      */    0,            0x36 },
{ /* A2 VK_LCONTROL                    */    0,            0x1d },
{ /* A3 VK_RCONTROL                    */    SC_E0,        0x1d },
{ /* A4 VK_LMENU                       */    0,            0x38 },
{ /* A5 VK_RMENU                       */    SC_E0,        0x38 },
{ /* A6 VK_BROWSER_BACK                */    SC_E0,        0x6a },
{ /* A7 VK_BROWSER_FORWARD             */    SC_E0,        0x69 },
{ /* A8 VK_BROWSER_REFRESH             */    SC_E0,        0x67 },
{ /* A9 VK_BROWSER_STOP                */    SC_E0,        0x68 },
{ /* AA VK_BROWSER_SEARCH              */    SC_E0,        0x65 },
{ /* AB VK_BROWSER_FAVORITES           */    SC_E0,        0x66 },
{ /* AC VK_BROWSER_HOME                */    SC_E0,        0x32 },
{ /* AD VK_VOLUME_MUTE                 */    SC_E0,        0x20 },
{ /* AE VK_VOLUME_DOWN                 */    SC_E0,        0x2e },
{ /* AF VK_VOLUME_UP                   */    SC_E0,        0x30 },

{ /* B0 VK_MEDIA_NEXT_TRACK            */    SC_E0,        0x19 },
{ /* B1 VK_MEDIA_PREV_TRACK            */    SC_E0,        0x10 },
{ /* B2 VK_MEDIA_STOP                  */    SC_E0,        0x24 },
{ /* B3 VK_MEDIA_PLAY_PAUSE            */    SC_E0,        0x22 },
{ /* B4 VK_LAUNCH_MAIL                 */    SC_E0,        0x6c },
{ /* B5 VK_LAUNCH_MEDIA_SELECT         */    SC_E0,        0x6d },
{ /* B6 VK_LAUNCH_APP1                 */    SC_E0,        0x6b },
{ /* B7 VK_LAUNCH_APP2                 */    SC_E0,        0x21 },
{ /* B8                                */    0,               0 },
{ /* B9                                */    0,               0 },
{ /* BA VK_SEMICOLON                   */    0,            0x27 },
{ /* BB VK_EQUAL                       */    0,            0x0d },
{ /* BC VK_COMMA                       */    0,            0x33 },
{ /* BD VK_HYPHEN                      */    0,            0x0c },
{ /* BE VK_PERIOD                      */    0,            0x34 },
{ /* BF VK_SLASH                       */    0,            0x35 },

{ /* C0 VK_BACKQUOTE                   */    0,            0x29 },
{ /* C1                                */    0,               0 },
{ /* C2                                */    0,               0 },
{ /* C3                                */    0,               0 },
{ /* C4                                */    0,               0 },
{ /* C5                                */    0,               0 },
{ /* C6                                */    0,               0 },
{ /* C7                                */    0,               0 },
{ /* C8                                */    0,               0 },
{ /* C9                                */    0,               0 },
{ /* CA                                */    0,               0 },
{ /* CB                                */    0,               0 },
{ /* CC                                */    0,               0 },
{ /* CD                                */    0,               0 },
{ /* CE                                */    0,               0 },
{ /* CF                                */    0,               0 },

{ /* D0                                */    0,               0 },
{ /* D1                                */    0,               0 },
{ /* D2                                */    0,               0 },
{ /* D3                                */    0,               0 },
{ /* D4                                */    0,               0 },
{ /* D5                                */    0,               0 },
{ /* D6                                */    0,               0 },
{ /* D7                                */    0,               0 },
{ /* D8                                */    0,               0 },
{ /* D9                                */    0,               0 },
{ /* DA                                */    0,               0 },
{ /* DB VK_LBRACKET                    */    0,            0x1a },
{ /* DC VK_BACKSLASH                   */    0,            0x2b },
{ /* DD VK_RBRACKET                    */    0,            0x1b },
{ /* DE VK_APOSTROPHE                  */    0,            0x28 },
{ /* DF VK_OFF                         */    SC_E0,        0x5f },

{ /* E0                                */    0,               0 },
{ /* E1                                */    0,               0 },
{ /* E2 VK_OEM_102                     */    0,               0 },
{ /* E3                                */    0,               0 },
{ /* E4                                */    0,               0 },
{ /* E5 VK_PROCESSKEY                  */    0,               0 },
{ /* E6                                */    0,               0 },
{ /* E7                                */    0,               0 },
{ /* E8                                */    0,               0 },
{ /* E9                                */    0,               0 },
{ /* EA                                */    0,               0 },
{ /* EB                                */    0,               0 },
{ /* EC                                */    0,               0 },
{ /* ED                                */    0,               0 },
{ /* EE                                */    0,               0 },
{ /* EF                                */    0,               0 },

{ /* F0 VK_DBE_ALPHANUMERIC            */    0,               0 },
{ /* F1 VK_DBE_KATAKANA                */    0,               0 },
{ /* F2 VK_DBE_HIRAGANA                */    0,               0 },
{ /* F3 VK_DBE_SBCSCHAR                */    0,               0 },
{ /* F4 VK_DBE_DBCSCHAR                */    0,               0 },
{ /* F5 VK_DBE_ROMAN                   */    0,               0 },
{ /* F6 VK_DBE_NOROMAN                 */    0,               0 },
{ /* F7 VK_DBE_ENTERWORDREGISTERMODE   */    0,               0 },
{ /* F8 VK_EXSEL                       */    0,               0 },
{ /* F9 VK_DBE_FLUSHSTRING             */    0,               0 },
{ /* FA VK_DBE_CODEINPUT               */    0,               0 },
{ /* FB VK_ZOOM                        */    0,               0 },
{ /* FC VK_DBE_DETERMINESTRING         */    0,               0 },
{ /* FD VK_DBE_ENTERDLGCONVERSIONMODE  */    0,               0 },
{ /* FE VK_OEM_CLEAR                   */    0,               0 },
{ /* FF                                */    0,               0 },
};



static INPUT_LANGUAGE il_0412 = {
    sizeof(INPUT_LANGUAGE),

    // Type and subtype.
    8,
    3,

    // Modifier keys
    &CharModifiers,
    NULL,
    NULL,

    // Character tables
    aVkToWcharTable,

    // Diacritics
    NULL,

    // Virtual Keys to XT scan codes
    VKeyToXTScanCodeTable,

    // Locale-specific processing
    0,

    // No ligatures
    0, 0, NULL,

    // Function keys
    12,
};

extern "C"
BOOL
WINAPI
IL_00000412(
    PINPUT_LANGUAGE pInputLanguage
    )
{
    PREFAST_ASSERT(pInputLanguage != NULL);

    BOOL fRet = FALSE;

    if (pInputLanguage->dwSize != sizeof(INPUT_LANGUAGE)) {
        RETAILMSG(1, (_T("IL_00000412: data structure size mismatch\r\n")));
        goto leave;
    }

    ASSERT(dim(VKeyToXTScanCodeTable) == COUNT_VKEYS);

    *pInputLanguage = il_0412;
    
    fRet = TRUE;

leave:
    return fRet;
}
#ifdef DEBUG
// Verify function declaration against the typedef.
static PFN_INPUT_LANGUAGE_ENTRY v_pfnILEntry = IL_00000412;
#endif
