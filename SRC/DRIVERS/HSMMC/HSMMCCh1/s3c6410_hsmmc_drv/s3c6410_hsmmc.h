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
#ifndef _S3C6410_HSMMC_H_
#define _S3C6410_HSMMC_H_

#include "../s3c6410_hsmmc_lib/SDHC.h"

typedef class CSDHControllerCh1 : public CSDHCBase {
    public:
        // Constructor
        CSDHControllerCh1() : CSDHCBase() {}

        // Destructor
        virtual ~CSDHControllerCh1() {}

        // Perform basic initialization including initializing the hardware
        // so that the capabilities register can be read.
        virtual BOOL Init(LPCTSTR pszActiveKey);

        virtual VOID PowerUp();

        virtual LPSDHC_DESTRUCTION_PROC GetDestructionProc() {
            return &DestroyHSMMCHCCh1Object;
        }

        static VOID DestroyHSMMCHCCh1Object(PCSDHCBase pSDHC);

    protected:
        BOOL InitClkPwr();
        BOOL InitGPIO();
        BOOL InitHSMMC();

        BOOL InitCh();
} *PCSDHControllerCh1;

#endif // _S3C6410_HSMMC_H_

