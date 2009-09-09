//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
//
// Use of this source code is subject to the terms of the Microsoft end-user
// license agreement (EULA) under which you licensed this SOFTWARE PRODUCT.
// If you did not accept the terms of the EULA, you are not authorized to use
// this source code. For a copy of the EULA, please see the LICENSE.RTF on your
// install media.
//

#ifndef _SMDK6410_BUTTON_H_
#define _SMDK6410_BUTTON_H_

BOOL Button_initialize_register_address(void *pGPIOReg);
void Button_port_initialize(void);

BOOL Button_pwrbtn_set_interrupt_method(EINT_SIGNAL_METHOD eMethod);
BOOL Button_pwrbtn_set_filter_method(EINT_FILTER_METHOD eMethod, unsigned int uiFilterWidth);
void Button_pwrbtn_enable_interrupt(void);
void Button_pwrbtn_disable_interrupt(void);
void Button_pwrbtn_clear_interrupt_pending(void);
BOOL Button_pwrbtn_is_pushed(void);

BOOL Button_rstbtn_set_interrupt_method(EINT_SIGNAL_METHOD eMethod);
BOOL Button_rstbtn_set_filter_method(EINT_FILTER_METHOD eMethod, unsigned int uiFilterWidth);
void Button_rstbtn_enable_interrupt(void);
void Button_rstbtn_disable_interrupt(void);
void Button_rstbtn_clear_interrupt_pending(void);
BOOL Button_rstbtn_is_pushed(void);

#endif    // _SMDK6410_BUTTON_H_

