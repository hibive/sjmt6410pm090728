#ifndef __LTS222QV_RGB_DATASET_H__
#define __LTS222QV_RGB_DATASET_H__

#if __cplusplus
extern "C"
{
#endif

#define TIGHT_LATENCY        (1)

const unsigned int LTS222QV_RGB_initialize[][3] =
{
#if TIGHT_LATENCY       // as Module spec, If this not work change TIGHT_LATENCY as 0 
    {0x22, 0x01, 0},    // PARTIAL 2 DISPLAY AREA RASTER-ROW NUMBER REGISTER 1
    {0x03, 0x01, 0},    // RESET REGISTER

    ///////////////////////////////////////////////////////////////////
    // Initializing Function 1
    ///////////////////////////////////////////////////////////////////
    {0x00, 0xa0, 1},    // CONTROL REGISTER 1, delay about 300ns
    {0x01, 0x10, 1},    // CONTROL REGISTER 2, delay about 300ns
    {0x02, 0x00, 1},    // RGB INTERFACE REGISTER
    {0x05, 0x00, 1},    // DATA ACCESS CONTROL REGISTER
    {0x0D, 0x00, 40},   // delay about 40ms

    ///////////////////////////////////////////////////////////////////
    // Initializing Function 2
    ///////////////////////////////////////////////////////////////////
    {0x0E, 0x00, 1},    // delay about 300ns
    {0x0F, 0x00, 1},    // delay about 300ns
    {0x10, 0x00, 1},    // delay about 300ns
    {0x11, 0x00 ,1},    // delay about 300ns
    {0x12, 0x00 ,1},    // delay about 300ns
    {0x13, 0x00 ,1},    // DISPLAY SIZE CONTROL REGISTER
    {0x14, 0x00 ,1},    // PARTIAL-OFF AREA COLOR REGISTER 1
    {0x15, 0x00 ,1},    // PARTIAL-OFF AREA COLOR REGISTER 2
    {0x16, 0x00 ,1},    // PARTIAL 1 DISPLAY AREA STARTING REGISTER 1
    {0x17, 0x00 ,1},    // PARTIAL 1 DISPLAY AREA STARTING REGISTER 2
    {0x34, 0x01 ,1},    // POWER SUPPLY SYSTEM CONTROL REGISTER 14
    {0x35, 0x00 ,40},   // POWER SUPPLY SYSTEM CONTROL REGISTER 7

    ////////////////////////////////////////////////////////////////////
    // Initializing Function 3
    ////////////////////////////////////////////////////////////////////
    {0x8D, 0x01 ,1},    // delay about 300ns
    {0x8B, 0x28 ,1},    // delay about 300ns
    {0x4B, 0x00 ,1},    // delay about 300ns
    {0x4C, 0x00 ,1},    // delay about 300ns
    {0x4D, 0x00 ,1},    // delay about 300ns
    {0x4E, 0x00 ,1},    // delay about 300ns
    {0x4F, 0x00 ,1},    // delay about 300ns
    {0x50, 0x00 ,50},   //  ID CODE REGISTER 2, Check it out, delay about 50 ms
    {0x86, 0x00 ,1},    // delay about 300ns
    {0x87, 0x26 ,1},    // delay about 300ns
    {0x88, 0x02 ,1},    // delay about 300ns
    {0x89, 0x05 ,1},    // delay about 300ns
    {0x33, 0x01 ,1},    //  POWER SUPPLY SYSTEM CONTROL REGISTER 13
    {0x37, 0x06 ,50},   //  POWER SUPPLY SYSTEM CONTROL REGISTER 12, Check it out
    {0x76, 0x00 ,40},   //  SCROLL AREA START REGISTER 2, delay about 30ms

    /////////////////////////////////////////////////////////////////////
    // Initializing Function 4
    /////////////////////////////////////////////////////////////////////
    {0x42, 0x00 ,1},    // delay about 300ns
    {0x43, 0x00 ,1},    // delay about 300ns
    {0x44, 0x00 ,1},    // delay about 300ns
    {0x45, 0x00 ,1},    //  CALIBRATION REGISTER
    {0x46, 0xef ,1},
    {0x47, 0x00 ,1},
    {0x48, 0x00 ,1},
    {0x49, 0x01 ,1},    //  ID CODE REGISTER 1                            check it out
    {0x4A, 0x3f ,1},    // delay about 300ns
    {0x3C, 0x00 ,1},    // delay about 300ns
    {0x3D, 0x00 ,1},    // delay about 300ns
    {0x3E, 0x01 ,1},    // delay about 300ns
    {0x3F, 0x3f ,1},    // delay about 300ns
    {0x40, 0x01 ,1},    // delay about 300ns, horizontal back porch, 050105 Boaz.Kim

    {0x41, 0x0a ,1},    //    vertical back porch

    {0x8F, 0x3f ,40},   // this value is more comfortable to look


    /////////////////////////////////////////////////////////////////////
    // Initializing Function 5
    /////////////////////////////////////////////////////////////////////
    {0x90, 0x3f ,1},    // delay about 300ns
    {0x91, 0x33 ,1},    // delay about 300ns
    {0x92, 0x77 ,1},    // delay about 300ns
    {0x93, 0x77 ,1},    // delay about 300ns
    {0x94, 0x17 ,1},    // delay about 300ns
    {0x95, 0x3f ,1},    // delay about 300ns
    {0x96, 0x00 ,1},    // delay about 300ns
    {0x97, 0x33 ,1},    // delay about 300ns
    {0x98, 0x77 ,1},    // delay about 300ns
    {0x99, 0x77 ,1},    // delay about 300ns
    {0x9A, 0x17 ,1},    // delay about 300ns
    {0x9B, 0x07 ,1},    // delay about 300ns
    {0x9C, 0x07 ,1},    // delay about 300ns

    //{0x9D, 0x80 ,40}, //    16 or 18bit RGB (BWS2="H": 16bit, BWS2="L": 18bit[default config in DualLcd b'd])
    {0x9D, 0x81 ,40},   //    Serial RGB18

    /////////////////////////////////////////////////////////////////////
    // Power Setting 2
    /////////////////////////////////////////////////////////////////////
    {0x1D, 0x08 ,1},    // delay about 50 us
    {0x23, 0x00 ,1},    //  PARTIAL 2 DISPLAY AREA RASTER-ROW NUMBER REGISTER 2
    {0x24, 0x94 ,1},    //  POWER SUPPLY SYSTEM CONTROL REGISTER 1
    {0x25, 0x6f ,1},    //  POWER SUPPLY SYSTEM CONTROL REGISTER 2

    /////////////////////////////////////////////////////////////////////
    // Power Setting 3
    /////////////////////////////////////////////////////////////////////
    {0x28, 0x1e, 0},    //
    {0x1A, 0x00, 0},    //
    {0x21, 0x10, 0},    //  PARTIAL 1 DISPLAY AREA RASTER-ROW NUMBER REGISTER 2
    {0x18, 0x25, 40},   //  PARTIAL 2 DISPLAY AREA STARTING REGISTER 1

    // delay about 40ms

    {0x19, 0x48, 0},    //  PARTIAL 2 DISPLAY AREA STARTING REGISTER 2
    {0x18, 0xe5, 10},   //  PARTIAL 2 DISPLAY AREA STARTING REGISTER 1

    // delay about 10ms

    {0x18, 0xF7, 40},   //  PARTIAL 2 DISPLAY AREA STARTING REGISTER 1

    // delay about 40ms

    {0x1B, 0x07, 40},   // VS regulator ON at 4.5V

    // delay about 40ms

    {0x1F, 0x5a, 0},
    {0x20, 0x54, 0},
    {0x1E, 0xc1, 10},

    // delay about 10ms

    {0x21, 0x00, 0},    //  PARTIAL 1 DISPLAY AREA RASTER-ROW NUMBER REGISTER 2
    {0x3B, 0x01, 20},   //

    // delay about 20ms

    {0x00, 0x20, 0},    //  CONTROL REGISTER 1

    {0x02, 0x01, 10},   //  RGB INTERFACE REGISTER

    // delay about 10ms

    {0, 0, 0}
#else    // Serial Safe
    {0x22, 0x01, 0},    // PARTIAL 2 DISPLAY AREA RASTER-ROW NUMBER REGISTER 1
    {0x03, 0x01, 0},    // RESET REGISTER

    ///////////////////////////////////////////////////////////////////
    // Initializing Function 1
    ///////////////////////////////////////////////////////////////////
    {0x00, 0xa0, 1},    // CONTROL REGISTER 1, delay about 300ns
    {0x01, 0x10, 1},    // CONTROL REGISTER 2, delay about 300ns
    {0x02, 0x00, 1},    // RGB INTERFACE REGISTER
    {0x05, 0x00, 1},    // DATA ACCESS CONTROL REGISTER
    {0x0D, 0x00, 400},  // delay about 40ms

    ///////////////////////////////////////////////////////////////////
    // Initializing Function 2
    ///////////////////////////////////////////////////////////////////
    {0x0E, 0x00, 5},    // delay about 300ns
    {0x0F, 0x00, 5},    // delay about 300ns
    {0x10, 0x00, 5},    // delay about 300ns
    {0x11, 0x00 ,5},    // delay about 300ns
    {0x12, 0x00 ,5},    // delay about 300ns
    {0x13, 0x00 ,5},    // DISPLAY SIZE CONTROL REGISTER
    {0x14, 0x00 ,5},    // PARTIAL-OFF AREA COLOR REGISTER 1
    {0x15, 0x00 ,5},    // PARTIAL-OFF AREA COLOR REGISTER 2
    {0x16, 0x00 ,5},    // PARTIAL 1 DISPLAY AREA STARTING REGISTER 1
    {0x17, 0x00 ,5},    // PARTIAL 1 DISPLAY AREA STARTING REGISTER 2
    {0x34, 0x01 ,5},    // POWER SUPPLY SYSTEM CONTROL REGISTER 14
    {0x35, 0x00 ,400},  // POWER SUPPLY SYSTEM CONTROL REGISTER 7

    ////////////////////////////////////////////////////////////////////
    // Initializing Function 3
    ////////////////////////////////////////////////////////////////////
    {0x8D, 0x01 ,5},    // delay about 300ns
    {0x8B, 0x28 ,5},    // delay about 300ns
    {0x4B, 0x00 ,5},    // delay about 300ns
    {0x4C, 0x00 ,5},    // delay about 300ns
    {0x4D, 0x00 ,5},    // delay about 300ns
    {0x4E, 0x00 ,5},    // delay about 300ns
    {0x4F, 0x00 ,5},    // delay about 300ns
    {0x50, 0x00 ,500},  //  ID CODE REGISTER 2, Check it out, delay about 50 ms
    {0x86, 0x00 ,5},    // delay about 300ns
    {0x87, 0x26 ,5},    // delay about 300ns
    {0x88, 0x02 ,5},    // delay about 300ns
    {0x89, 0x05 ,5},    // delay about 300ns
    {0x33, 0x01 ,5},    //  POWER SUPPLY SYSTEM CONTROL REGISTER 13
    {0x37, 0x06 ,500},  //  POWER SUPPLY SYSTEM CONTROL REGISTER 12, Check it out
    {0x76, 0x00 ,400},  //  SCROLL AREA START REGISTER 2, delay about 30ms

    /////////////////////////////////////////////////////////////////////
    // Initializing Function 4
    /////////////////////////////////////////////////////////////////////
    {0x42, 0x00 ,5},    // delay about 300ns
    {0x43, 0x00 ,5},    // delay about 300ns
    {0x44, 0x00 ,5},    // delay about 300ns
    {0x45, 0x00 ,5},    //  CALIBRATION REGISTER
    {0x46, 0xef ,5},
    {0x47, 0x00 ,5},
    {0x48, 0x00 ,5},
    {0x49, 0x01 ,5},    //  ID CODE REGISTER 1                            check it out
    {0x4A, 0x3f ,5},    // delay about 300ns
    {0x3C, 0x00 ,5},    // delay about 300ns
    {0x3D, 0x00 ,5},    // delay about 300ns
    {0x3E, 0x01 ,5},    // delay about 300ns
    {0x3F, 0x3f ,5},    // delay about 300ns
    {0x40, 0x01 ,5},    // delay about 300ns, horizontal back porch, 050105 Boaz.Kim

    {0x41, 0x0a ,5},    //    vertical back porch

    {0x8F, 0x3f ,400},  // this value is more comfortable to look


    /////////////////////////////////////////////////////////////////////
    // Initializing Function 5
    /////////////////////////////////////////////////////////////////////
    {0x90, 0x3f ,5},  // delay about 300ns
    {0x91, 0x33 ,5},  // delay about 300ns
    {0x92, 0x77 ,5},  // delay about 300ns
    {0x93, 0x77 ,5},  // delay about 300ns
    {0x94, 0x17 ,5},  // delay about 300ns
    {0x95, 0x3f ,5},  // delay about 300ns
    {0x96, 0x00 ,5},  // delay about 300ns
    {0x97, 0x33 ,5},  // delay about 300ns
    {0x98, 0x77 ,5},  // delay about 300ns
    {0x99, 0x77 ,5},  // delay about 300ns
    {0x9A, 0x17 ,5},  // delay about 300ns
    {0x9B, 0x07 ,5},  // delay about 300ns
    {0x9C, 0x07 ,5},  // delay about 300ns

    {0x9D, 0x80 ,400}, //    16 or 18bit RGB (BWS2="H": 16bit, BWS2="L": 18bit[default config in DualLcd b'd])

    /////////////////////////////////////////////////////////////////////
    // Power Setting 2
    /////////////////////////////////////////////////////////////////////
    {0x1D, 0x08 ,400},  // delay about 50 us
    {0x23, 0x00 ,400},  //  PARTIAL 2 DISPLAY AREA RASTER-ROW NUMBER REGISTER 2
    {0x24, 0x94 ,400},  //  POWER SUPPLY SYSTEM CONTROL REGISTER 1
    {0x25, 0x6f ,400},  //  POWER SUPPLY SYSTEM CONTROL REGISTER 2

    /////////////////////////////////////////////////////////////////////
    // Power Setting 3
    /////////////////////////////////////////////////////////////////////
    {0x28, 0x1e, 0},    //
    {0x1A, 0x00, 0},    //
    {0x21, 0x10, 0},    //  PARTIAL 1 DISPLAY AREA RASTER-ROW NUMBER REGISTER 2
    {0x18, 0x25, 400},  //  PARTIAL 2 DISPLAY AREA STARTING REGISTER 1

    // delay about 40ms

    {0x19, 0x48, 0},    //  PARTIAL 2 DISPLAY AREA STARTING REGISTER 2
    {0x18, 0xe5, 400},  //  PARTIAL 2 DISPLAY AREA STARTING REGISTER 1

    // delay about 10ms

    {0x18, 0xF7, 400},  //  PARTIAL 2 DISPLAY AREA STARTING REGISTER 1

    // delay about 40ms

    {0x1B, 0x07, 400},  // VS regulator ON at 4.5V

    // delay about 40ms

    {0x1F, 0x5a, 0},
    {0x20, 0x54, 0},
    {0x1E, 0xc1, 400},

    // delay about 10ms

    {0x21, 0x00, 0},    //  PARTIAL 1 DISPLAY AREA RASTER-ROW NUMBER REGISTER 2
    {0x3B, 0x01, 400},  //

    // delay about 20ms

    {0x00, 0x20, 0},    //  CONTROL REGISTER 1

    {0x02, 0x01, 400},  //  RGB INTERFACE REGISTER

    // delay about 10ms

    {0x44, 0x00, 0},
    {0x42, 0x00, 0},
    {0x43, 0x00, 0},

    {0, 0, 0}
#endif
};

#if __cplusplus
}
#endif

#endif    // __LTS222QV_RGB_DATASET_H__
