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
// THE SAMPLE SOURCE CODE IS PROVIDED "AS IS", WITH NO WARRANTIES OR INDEMNITIES.
//
//
// Copyright (c) Samsung Electronics. Co. LTD.  All rights reserved.
//
/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

Module Name:    s5x532.h

Abstract:       S5K3AA Camera module setting binary microcode sequence

Functions:


Notes:


--*/

#ifndef _S5X532_H_
#define _S5X532_H_

#define MEGAPIXEL_CAMMODE 0
#define SIMPLE_MODULE_SETTING   (TRUE)

//const 
#if (SIMPLE_MODULE_SETTING)
unsigned char S5K3AA_YCbCr8bit[][2] = 
{
    {0xec, 0x00},
    {0x71, 0x00}, // PCLK setting    

    {0xec, 0x00},
    {0x72, 0xC8},
    
    {0xec, 0x07},
    {0x37, 0x00},

    {0xec, 0x00},
    {0x02, 0x01},
    
    {0xec, 0x01},
    {0x6a, 0x01},

    {0xec, 0x00},    // Bright Setting. 
    {0x76, 0xff},    // Brightness
    {0x77, 0xd0},    // Color Level
    {0x79, 0x05},    // White Balance R
    {0x7a, 0x03}    // Write Balance B

};
#else
unsigned char S5K3AA_YCbCr8bit[][2] = 
{

                    //***************************************************
                //  name:     S5K3AAEX EVT2 setfile
                //  ver:        v2.61    
                //  history:     
                //    v0.0    start from 040908 setfile
                //    v1.0    arange register
                //    v1.01    change MCLK(25Mhz) and Frame rate(7fps)
                //    v2.0    adjust register setting for 3AA EVT2
                //        - color correction, RGB shding off, hsync start position, Mirror, fps
                //        - Color Tuning, YGRPDLY 
                //    v2.1    change Frame rate(7.5fps) and Total gain to x4 
                //        (because of reducing visual noise at low illumination)
                //        - change BPRM AGC Max and FrameAE start 
                //        improve AE speed
                //    v2.2    modify AWB G gain and solve 50hz flicker detection fail in 25MHz 7.5fps
                //    v2.3    Adjust gamma, Dark Slice, white point, Hue gain,
                //        White Balance B control, Y gain On, Digital Clamp On
                //        lower AWB G gain
                //    v2.4    Adjust AE window weight, Y Gamma, WhitePoint, Shading and BPR Max Thres.
                //     v2.41    Adjust AE/AWB window and AWB internal window boundary to decrease skin color tracking
                //     v2.411    special version for PSCDS
                //     v2.412    RGB shading off
                //    v2.5    Lens change STW to Sekonix 
                //        adjust White point and Y shading Coef (RGB shading off)
                //    v2.6    New Tuning because of Full YC off and YCbCr Coef change
                //        Gamma, Dark Slice, color matrix (not use), Color suppress
                //        R Gain and DBPR agc MIN/MAX
                //    v2.61    VCK inversion(data rising)
                //***************************************************
                ///////////////////////////////////////////////////
                // page 0
        {0xec,0x00},
        {0x75,0x06},                        // Mirror
        {0x23,0x52},            // RV/H Control
        {0xec,0x06},
        {0x8c,0x0b},            // in case of wrp wcp control for mirror
        {0x8d,0x0c},
        {0x8e,0x0b},
        {0x8f,0x0c},
        
        {0xec,0x00},
        {0x34,0x0e},              // AWB Tracking Boundary Constant  R_Limit High
                                /////////////////////////////
                                // Color Tunning
                                
                                //White Point 
        {0x40,0x18},    //1c    //18    
        {0x41,0x4b},    //4a    //41    //4a    
        {0x42,0x1e},    //23    //1e    
        {0x43,0x3c},    //39    //33    //3b    
        {0x44,0x2a},    //29    //2a    //2e    //2b    
        {0x45,0x2e},    //2b    //2c    //2b    //25    //2d
        
                        //Hue, Gain control                           
        {0x48,0xf8},    //f0    //c0    //fe                          
        {0x49,0xfe},                                 
        {0x4a,0x32},    //28    //03    //01                          
        {0x4b,0x40},                                  
        {0x4c,0xfe},                                  
        {0x4d,0xfe},                                  
        {0x4e,0x50},    //0e                          
        {0x4f,0x08},    //10    //d0   //10    //02                          
                                                
        {0x50,0xf0},    //e0    //a4    //aa    //e0                          
        {0x51,0xfe},   //fe                                  
        {0x52,0x42},    //40   //28    //01                          
        {0x53,0x40},    //3e                          
        {0x54,0xfe},                                  
        {0x55,0xfe},   //fc    //fe                          
        {0x56,0x2a},   //6a    //2e                          
        {0x57,0x20},    //18    //30    //1a    //32                          
                                                
        {0x58,0xda},    //ea    //ca    //a4    //aa    //c6                          
        {0x59,0xfe},   //ec    //fa                          
        {0x5a,0x54},    //44    //4a    //20    //30    //35    //30          
        {0x5b,0x28},    //38    //20   //1a    //32          
        {0x5c,0xfe},                                  
        {0x5d,0xfe},                                 
        {0x5e,0x4a},   //7e    //7a                  
        {0x5f,0x20},    //40    //18    //08    //20    //30    //3a   //2a    //42  
        
        {0x6c,0x10},   // AE Target
        {0x77,0xea},   // Color Gain
        {0x79,0xfe},    // White balance R control
        {0x7a,0x01},    // White balance B control
        {0x83,0xc0},    // Color suppress
                        //s8a04    // unstable diff
        
                        ///////////////////////////////////////////////////
                        // page 1
        {0xec,0x01},
        {0x8b,0x21},    //V Output Sync inversion, Full_YC off,SCK656_INV_R
        {0x6a,0x07},    //Y,Cr first ITU R-601 format
        
        {0xec,0x03},
        {0x56,0x05},    
        
        {0xec,0x04},
//        {0x06,0x07},    
        {0x2e,0x05},    
        
        {0xec,0x01},
        {0x2d,0x15},    //Adjust YGRPDLY 3clk->0clk
        
                        /// Color Matrix
                        //s107e
                        //s12fe
                        //s187e    //a0 
        
        {0x1c,0xd0},   // Highlight Suppress Reference
                        /////////////////////////////
                        //  gamma : 1.800000
                        //Y Gamma
        {0x2e,0x08},    
        {0x2f,0x18},    
        {0x30,0x26},    
        {0x31,0x70},    //60    
        {0x32,0xf0},    //e0    
        {0x33,0x94},    //90    
        {0x34,0x70},    //b0    
        {0x35,0xbc},    //80    
        {0x36,0x00},    
        {0x37,0x1b},    
        
            // Dark Slice
            //s4018   // Black Balance    Red 
            //s4118    //fe   // Black Balance    Blue
            //s4211    //fa   // Black Balance    Green
        
        {0x41,0xfe},   // Black Balance    Blue       
        {0x42,0xfa},   // Black Balance    Green                                
        
        {0x48,0xee},    // YCrCb Coef  
        {0x49,0xf7},                                                  
        
                        //C Gamma  
        {0x55,0x08},    //08        
        {0x56,0x18},    //18        
        {0x57,0x26},    //26        
        {0x58,0x60},    //70        //60        
        {0x59,0xf0},        //e0        
        {0x5a,0xa9},        //94    //90          
        {0x5b,0x60},    //3b    //70    //7b    //b0    
        {0x5c,0x50},    //96    //bc    //5a    //80    
        {0x5d,0x00},    //00        
        {0x5e,0x1b},    //1b    
        
        {0x69,0x0f},    // VCK inversion(data rising)
        {0xec,0x03},
        {0x55,0x0f},
        
        {0xec,0x04},
//        {0x05,0x0f},    
        {0x2d,0x1d},            

        {0xec,0x01},
                        //adjust AE window
        {0x97,0x18},
        {0x9a,0xe0},
        {0x9c,0x20},
        {0x9e,0xe8},
        
        {0xec,0x06},
        {0x00,0x18},
        {0x02,0xe0},
        {0x04,0x20},
        {0x06,0xe8},
        
                            //adjust AWB window
        {0xec,0x01},
        {0xa0,0x5a},
        {0xa2,0xd8},
        {0xa4,0x33},
        {0xa6,0xba},
        
        {0xec,0x06},
        {0x08,0x5a},
        {0x0a,0xd8},
        {0x0c,0x33},
        {0x0e,0xba},
        
        {0xec,0x01},
        {0xaa,0xe0},   // For AWB, High Threshold Value of Y signal
        {0xab,0x30},        // For AWB, Low Threshold Value of Y signal
        
                        /// Pixel Filter
        {0xac,0x80},   // AWB B-G Low Threshold
        {0xad,0x80},    // AWB B-G High Threshold
        {0xae,0x80},    // AWB R-G Low Threshold
        {0xaf,0x80},    // AWB R-G Low Threshold
        {0xb0,0x60},    // AWB R-G,B-G High Threshold
        {0xb1,0x80},    // AWB R-G,B-G Low Threshold
        
                        ///////////////////////////////////////////////////
                        //// page 2
        {0xec,0x02},
        {0x05,0x0b},
        {0x07,0x0b},
                        //s1f13    //Adjust Global Gain for 20Mhz
        {0x26,0x86},   //ADLC Set
        {0x2d,0x00},    //APS Bias Current   6uA  ---> 1uA    for SHBN & HN reduce 
        {0x2e,0x50},    //Ramp Gain
        {0x2f,0x30},    //Ramp Bias Current    for SHBN reduce 
        {0x30,0x05},    //for reducing noise... switch cap
        
                        ///////////////////////////////////////////////////
                            //// page 5
                            // Shading
        {0xec,0x05},
        {0x00,0x0d},      // Shading Old(Y shading) ON & Shading New(RGB Shading) OFF
                            //s000f      // Shading Old(Y shading)  & Shading New(RGB Shading)
        
                        ///RGB Shading
        {0x05,0x05},   // R Shading of RGB Shading
        {0x06,0x05},
        {0x07,0x05},
        {0x08,0x05},
        {0x09,0x00},   // GR Shading of RGB Shading
        {0x0a,0x00},
        {0x0b,0x00},
        {0x0c,0x00},
        {0x0d,0x00},   // GB Shading of RGB Shading
        {0x0e,0x00},
        {0x0f,0x00},
        {0x10,0x00},
        {0x11,0x00},   // B Shading of RGB Shading
        {0x12,0x00},
        {0x13,0x00},
        {0x14,0x00},
        
                        /// Y Shading
        {0x2d,0xd0},    //e0    //d6    //e0    
        {0x2e,0xb0},    //c0    //aa    //c0
        {0x2f,0xa0},        //95    //a0
        {0x30,0x88},        //86    //88    //80
        {0x31,0x88},        //86    //88    //80
        {0x32,0xa0},        //95    //a0
        {0x33,0xb0},    //c0    //aa    //c0
        {0x34,0xd0},    //e0    //d6    //e0
        
        {0x35,0xd0},    //e0
        {0x36,0xb0},    //ba    //a6
        {0x37,0x98},    //94    //90
        {0x38,0x88},    //80
        {0x39,0x88},    //80
        {0x3a,0x98},    //94    //90
        {0x3b,0xb0},    //ba    //a6
        {0x3c,0xd0},    //e0
        
                    ////////////////////////////////////////////////
                    // page 7
        {0xec,0x07},
        {0x11,0xfe},   // GGain_Offset
        {0x17,0x40},    // lower AWB Ggain because G is stronger than R/B
                        //s1742    // modify AWB Ggain to make <1.46> =0x40(x1)
                    
        {0x3c,0x00},    //adjust AWB internal window boundary to decrease skin color tracking
        
                        // to dectect 50Hz flicker in 25Mhz 7.5fps
        {0x60,0x10},    // adjust flicker thres
        {0x62,0x11},    // adjust edge detection boundary
        {0x63,0x04},    
        
        {0x69,0x10},    // for mirror
        {0x70,0x80},   // BLC Off
                    
                        // AE weight
        {0x80,0x10},    //40
        {0x81,0x00},    //10
        {0x82,0x01},    //10
        {0x83,0x02},    //10
        {0x84,0x02},    //30
        
                        ///////////////////////////////
                        // control start position(hsync hblank )
                        // to remove broken pixel width on left side 
        {0xec,0x01},
        {0x78,0x3d},    // HBLK START
        {0x82,0x3d},    // HS656 START

        {0xec,0x03},
        {0x5e,0x3d},
        {0x68,0x3d},
        
        {0xec,0x04},
//        {0x0e,0x3d},
//        {0x18,0x3d},
        {0x36,0x3d},
        {0x40,0x3d},                    
                        ////////////////////////////
                        // Control Gain
        {0xec,0x00},
        {0x78,0x60},    //Total gain x4 (Only Analog x4)
        {0x2d,0x5a},    //Frame AE start
        {0x82,0x5A},    //Color Suppress AGC Min
        
                        ////////////////////////////
                        // Control BPRM
                        // CIS BPR always off
        {0xec,0x00},
        {0x7e,0x87},    //Color Suppress On, Y gain On, Digital clamp On, DBPRM On
        {0x86,0x01},    //DBPR AGC MIN
        {0x87,0x00},    //DBPRM THRES. MIN
                        //s8710    //DBPRM THRES. MIN.. optimize between BPR and Pseudo color
        {0xec,0x07},
        {0x21,0x90},    //50    //DBPRM THRES. MAX
        {0x22,0x20},    //DBPR AGC MAX
                    
                        ////////////////////////////
                        // improve AE speed
        {0xec,0x00},
        {0x92,0x80},    //AE ratio high
        {0xec,0x07},
        {0x10,0x30},    // AGC predict ON
                        
                        //*********************************************************
                        // If you change MCLK or Frame rate, you change below register
                        //*********************************************************
                        /////////////////////////////
                        ///Adjust Global Gain 
        {0xec,0x02},
        {0x1f,0x0f},    // if MCLK is 25Mhz
        
                        /////////////////////////////
                        ///Flicker setting
        {0xec,0x00},
        {0x72,0x7d},    // if MCLK is 25Mhz
        
                        /////////////////////////////
                        // Adjust Vblank depth
                        /// Make 25Mhz 7.5fps 
        {0xec,0x02},
        {0x17,0x00},
        {0x18,0x8c},
        
        {0xec,0x04},
        {0x01,0x00},
        {0x02,0x8c},
                        //*********************************************************
                        
                        /////////////////////////////
                        ///Flicker setting
        {0xec,0x00},
        {0x74,0x18},  // Auto Flicker start 60hz for 7.5fps
        
                    /////////////////////////////
                    // Frame AE 
        {0xec,0x00},
        {0x73,0x11},    // frame 1/2
        
                        /////////////////////// 1213 setting
        {0xec,0x01},  //sdtv
        {0x19,0x4d},
        {0x1a,0x96},
        {0x1b,0x1d},
        {0x4a,0x41},
        {0x48,0xea},
        {0x47,0x41},
        {0x49,0xf5},
        
        {0xec,0x00},
        {0x44,0x2e},
        {0x45,0x2c},
        
        {0x5f,0x18},
        
        {0xec,0x07},
        {0x21,0x9c},
        {0x22,0x58},
        
        {0xec,0x00},   //  bpr
        {0x87,0x00},
        {0x86,0x48},
        
        {0xec,0x01},
        {0x8b,0x23},  // Full YC
        
        {0x35,0xff},  //Y gamma
        {0x5c,0xff},  //C gamma

        
        {0xec,0x00},  //Bally adatpion
        {0x74,0x04},  // Auto Flicker start 60hz for 7.5fps
        {0x73,0x51},    
//        {0x72,0x78},    //MCLK 24Mhz
//        {0x72,0xA0},    //MCLK 32Mhz
        {0x72,0xF0},    //MCLK 48Mhz
        {0xec,0x04},
        {0x01,0x05},        //4fps
        {0x02,0x8c},

/*
        {0xec,0x03},
        {0x57,0x03},
        {0x58,0x09},
        {0x5b,0x02},
        {0x5c,0x84},
        {0x5f,0x02},
        {0x60,0x84},
        {0x69,0x02},
        {0x6a,0x84},
        {0x63,0x02},
        {0x64,0x04},
        {0x6d,0x02},
        {0x6e,0x04},
        {0x65,0x02},
        {0x66,0x10},
        {0x6f,0x02},
        {0x70,0x10},
        {0xec,0x03},
        {0x5e,0x38},
        {0x52,0x48},  //30fps
*/
        {0xec,0x02},
        {0x02,0x0d},  //9bit
        {0x1f,0x07},  //global gain 
        {0xec,0x07},
        {0x63,0x0a},

/*
        {0xec,0x03},
        {0x56,0x04},    // 04->05  05->04
        
        {0xec,0x04},
        {0x06,0x05},    // 04->05
*/        
        {0xec,0x07},    //bpr by pyo
        {0x21,0x9c},
        {0x22,0x58},
        {0xec,0x00},
        {0x87,0x00},
        {0x86,0x48},    //bpr by pyo
        
        {0x02,0x30},
        {0xec,0x01},
        {0x21,0x40},
        {0x22,0x40},
        {0x23,0x00},
        {0x24,0x00},        
        {0xec,0x00},
        {0x6d,0x00},
        {0x6c,0xf0},
        {0x86,0x10},
        {0xec,0x07},
        {0x9d,0x10},
        {0xec,0x00},
        {0x75,0x05},
        {0x96,0x06},
        {0x78,0x68},
        {0x66,0x02},
        {0x94,0x03},
        {0x97,0x03},


#if (MEGAPIXEL_CAMMODE==0)
    /* Only for VGA Mode */
        {0xec,0x07},    //bpr by pyo
        {0x21,0x9c},
        {0x22,0x58},
        {0xec,0x00},
        {0x87,0x00},
        {0x86,0x48},    //bpr by pyo
        
        {0xec,0x02},
        {0x02,0x0d},  //9bit
        {0x1f,0x07},  //global gain 

        {0xec,0x01},
        {0x21,0x40},
        {0x22,0x40},
        {0x23,0x00},
        {0x24,0x00},
                
        {0xec,0x00},
        {0x7b,0x00},
        {0x73,0x51},
        {0x02,0x01},
//        {0x02,0x30},
#else
        {0xec,0x07},    
        {0x21,0x90},    
        {0x22,0x60},    
        {0xec,0x00},    
        {0x87,0x00},    
        {0x86,0x20},    
        
        {0xec,0x02},
        {0x02,0x0f}, 
        {0x1f,0x0f},
         {0xec,0x01},
        {0x21,0x50},
        {0x22,0x50},
        {0x23,0x10},
        {0x24,0x10},

        {0xec,0x00},
//        {0x7b,0xff},
        {0x73,0x00},
        {0x02,0x00},
#endif
};

#endif

#endif // _S5X532_H_
