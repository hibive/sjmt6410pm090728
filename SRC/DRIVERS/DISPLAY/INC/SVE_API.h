
#ifndef _SVE_API_H_
#define _SVE_API_H_

typedef enum
{
    SVE_RESOURCE_API_BASE = 100,

    // Resource Request/Release IOCTL(FIMD)
    SVE_RSC_REQUEST_FIMD_INTERFACE,
    SVE_RSC_RELEASE_FIMD_INTERFACE,
    SVE_RSC_REQUEST_FIMD_WIN0,
    SVE_RSC_RELEASE_FIMD_WIN0,
    SVE_RSC_REQUEST_FIMD_WIN1,
    SVE_RSC_RELEASE_FIMD_WIN1,
    SVE_RSC_REQUEST_FIMD_WIN2,
    SVE_RSC_RELEASE_FIMD_WIN2,
    SVE_RSC_REQUEST_FIMD_WIN3,
    SVE_RSC_RELEASE_FIMD_WIN3,
    SVE_RSC_REQUEST_FIMD_WIN4,
    SVE_RSC_RELEASE_FIMD_WIN4,

    // Resource Request/Release IOCTL(Post Processor)
    SVE_RSC_REQUEST_POST,
    SVE_RSC_RELEASE_POST,

    // Resource Request/Release IOCTL(Image Rotator)
    SVE_RSC_REQUEST_ROTATOR,
    SVE_RSC_RELEASE_ROTATOR,

    // Resource Request/Release IOCTL(TV Scaler & TV Encoder)
    SVE_RSC_REQUEST_TVSCALER_TVENCODER,
    SVE_RSC_RELEASE_TVSCALER_TVENCODER,

    SVE_RESOURCE_API_END,

    // FIMD Function IOCTL
    SVE_FIMD_FUNCTION_API_BASE = 200,
    SVE_FIMD_SET_INTERFACE_PARAM,
    SVE_FIMD_SET_OUTPUT_RGBIF,
    SVE_FIMD_SET_OUTPUT_TV,
    SVE_FIMD_SET_OUTPUT_ENABLE,
    SVE_FIMD_SET_OUTPUT_DISABLE,
    SVE_FIMD_SET_WINDOW_MODE,
    SVE_FIMD_SET_WINDOW_POSITION,
    SVE_FIMD_SET_WINDOW_FRAMEBUFFER,
    SVE_FIMD_SET_WINDOW_COLORMAP,
    SVE_FIMD_SET_WINDOW_ENABLE,
    SVE_FIMD_SET_WINDOW_DISABLE,
    SVE_FIMD_SET_WINDOW_BLEND_DISABLE,
    SVE_FIMD_SET_WINDOW_BLEND_COLORKEY,
    SVE_FIMD_SET_WINDOW_BLEND_ALPHA,
    SVE_FIMD_WAIT_FRAME_INTERRUPT,
    SVE_FIMD_GET_OUTPUT_STATUS,
    SVE_FIMD_GET_WINDOW_STATUS,
    SVE_FIMD_FUNCTION_API_END,

    // Post Processor Function IOCTL
    SVE_POST_FUNCTION_API_BASE = 300,
    SVE_POST_SET_PROCESSING_PARAM,
    SVE_POST_SET_SOURCE_BUFFER,
    SVE_POST_SET_NEXT_SOURCE_BUFFER,
    SVE_POST_SET_DESTINATION_BUFFER,
    SVE_POST_SET_NEXT_DESTINATION_BUFFER,
    SVE_POST_SET_PROCESSING_START,
    SVE_POST_SET_PROCESSING_STOP,
    SVE_POST_WAIT_PROCESSING_DONE,
    SVE_POST_GET_PROCESSING_STATUS,
    SVE_POST_FUNCTION_API_END,

    // Local Path (FIMD+Post) Function IOCTL
    SVE_LOCALPATH_FUNCTION_API_BASE = 400,
    SVE_LOCALPATH_SET_WIN0_START,
    SVE_LOCALPATH_SET_WIN0_STOP,
    SVE_LOCALPATH_SET_WIN1_START,
    SVE_LOCALPATH_SET_WIN1_STOP,
    SVE_LOCALPATH_SET_WIN2_START,
    SVE_LOCALPATH_SET_WIN2_STOP,
    SVE_LOCALPATH_FUNCTION_API_END,

    // Image Rotator Function IOCTL
    SVE_ROTATOR_FUNCTION_API_BASE = 500,
    SVE_ROTATOR_SET_OPERATION_PARAM,
    SVE_ROTATOR_SET_SOURCE_BUFFER,
    SVE_ROTATOR_SET_DESTINATION_BUFFER,
    SVE_ROTATOR_SET_OPERATION_START,
    SVE_ROTATOR_SET_OPERATION_STOP,
    SVE_ROTATOR_WAIT_OPERATION_DONE,
    SVE_ROTATOR_GET_STATUS,
    SVE_ROTATOR_FUNCTION_API_END,

    // TV Scaler Function IOCTL
    SVE_TVSC_FUNCTION_API_BASE = 600,
    SVE_TVSC_SET_PROCESSING_PARAM,
    SVE_TVSC_SET_SOURCE_BUFFER,
    SVE_TVSC_SET_NEXT_SOURCE_BUFFER,
    SVE_TVSC_SET_DESTINATION_BUFFER,
    SVE_TVSC_SET_NEXT_DESTINATION_BUFFER,
    SVE_TVSC_SET_PROCESSING_START,
    SVE_TVSC_SET_PROCESSING_STOP,
    SVE_TVSC_WAIT_PROCESSING_DONE,
    SVE_TVSC_GET_PROCESSING_STATUS,
    SVE_TVSC_FUNCTION_API_END,

    // TV Encoder Function IOCTL
    SVE_TVENC_FUNCTION_API_BASE = 700,
    SVE_TVENC_SET_INTERFACE_PARAM,
    SVE_TVENC_SET_ENCODER_ON,
    SVE_TVENC_SET_ENCODER_OFF,
    SVE_TVENC_GET_INTERFACE_STATUS,
    SVE_TVENC_FUNCTION_API_END,

    // Power Management IOCTL
    SVE_PM_POWER_API_BASE = 1000,
    SVE_PM_SET_POWER_ON,
    SVE_PM_SET_POWER_OFF,
    SVE_PM_GET_POWER_STATUS,
    SVE_PM_POWER_API_END,

    SVE_API_ENUM_END
} SVE_API_FUNCTION_CODE;    // 0x0 ~ 0xFFF

#define    SVE_DEVICE_TYPE    (0xA000)        // SVE-unique device type

// Resource Request/Release IOCTL
#define    IOCTL_SVE_RSC_REQUEST_FIMD_INTERFACE    \
        CTL_CODE(SVE_DEVICE_TYPE, SVE_RSC_REQUEST_FIMD_INTERFACE, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define    IOCTL_SVE_RSC_RELEASE_FIMD_INTERFACE    \
        CTL_CODE(SVE_DEVICE_TYPE, SVE_RSC_RELEASE_FIMD_INTERFACE, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define    IOCTL_SVE_RSC_REQUEST_FIMD_WIN0    \
        CTL_CODE(SVE_DEVICE_TYPE, SVE_RSC_REQUEST_FIMD_WIN0, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define    IOCTL_SVE_RSC_RELEASE_FIMD_WIN0        \
        CTL_CODE(SVE_DEVICE_TYPE, SVE_RSC_RELEASE_FIMD_WIN0, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define    IOCTL_SVE_RSC_REQUEST_FIMD_WIN1    \
        CTL_CODE(SVE_DEVICE_TYPE, SVE_RSC_REQUEST_FIMD_WIN1, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define    IOCTL_SVE_RSC_RELEASE_FIMD_WIN1        \
        CTL_CODE(SVE_DEVICE_TYPE, SVE_RSC_RELEASE_FIMD_WIN1, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define    IOCTL_SVE_RSC_REQUEST_FIMD_WIN2    \
        CTL_CODE(SVE_DEVICE_TYPE, SVE_RSC_REQUEST_FIMD_WIN2, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define    IOCTL_SVE_RSC_RELEASE_FIMD_WIN2        \
        CTL_CODE(SVE_DEVICE_TYPE, SVE_RSC_RELEASE_FIMD_WIN2, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define    IOCTL_SVE_RSC_REQUEST_FIMD_WIN3    \
        CTL_CODE(SVE_DEVICE_TYPE, SVE_RSC_REQUEST_FIMD_WIN3, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define    IOCTL_SVE_RSC_RELEASE_FIMD_WIN3        \
        CTL_CODE(SVE_DEVICE_TYPE, SVE_RSC_RELEASE_FIMD_WIN3, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define    IOCTL_SVE_RSC_REQUEST_FIMD_WIN4    \
        CTL_CODE(SVE_DEVICE_TYPE, SVE_RSC_REQUEST_FIMD_WIN4, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define    IOCTL_SVE_RSC_RELEASE_FIMD_WIN4        \
        CTL_CODE(SVE_DEVICE_TYPE, SVE_RSC_RELEASE_FIMD_WIN4, METHOD_BUFFERED, FILE_ANY_ACCESS)

// Resource Request/Release IOCTL(Post Processor)
#define    IOCTL_SVE_RSC_REQUEST_POST    \
        CTL_CODE(SVE_DEVICE_TYPE, SVE_RSC_REQUEST_POST, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define    IOCTL_SVE_RSC_RELEASE_POST    \
        CTL_CODE(SVE_DEVICE_TYPE, SVE_RSC_RELEASE_POST, METHOD_BUFFERED, FILE_ANY_ACCESS)

// Resource Request/Release IOCTL(Image Rotator)
#define    IOCTL_SVE_RSC_REQUEST_ROTATOR    \
        CTL_CODE(SVE_DEVICE_TYPE, SVE_RSC_REQUEST_ROTATOR, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define    IOCTL_SVE_RSC_RELEASE_ROTATOR    \
        CTL_CODE(SVE_DEVICE_TYPE, SVE_RSC_RELEASE_ROTATOR, METHOD_BUFFERED, FILE_ANY_ACCESS)

// Resource Request/Release IOCTL(TV Scaler & TV Encoder)
#define    IOCTL_SVE_RSC_REQUEST_TVSCALER_TVENCODER    \
        CTL_CODE(SVE_DEVICE_TYPE, SVE_RSC_REQUEST_TVSCALER_TVENCODER, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define    IOCTL_SVE_RSC_RELEASE_TVSCALER_TVENCODER    \
        CTL_CODE(SVE_DEVICE_TYPE, SVE_RSC_RELEASE_TVSCALER_TVENCODER, METHOD_BUFFERED, FILE_ANY_ACCESS)

// FIMD IOCTL
#define    IOCTL_SVE_FIMD_SET_INTERFACE_PARAM    \
        CTL_CODE(SVE_DEVICE_TYPE, SVE_FIMD_SET_INTERFACE_PARAM, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define    IOCTL_SVE_FIMD_SET_OUTPUT_RGBIF    \
        CTL_CODE(SVE_DEVICE_TYPE, SVE_FIMD_SET_OUTPUT_RGBIF, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define    IOCTL_SVE_FIMD_SET_OUTPUT_TV    \
        CTL_CODE(SVE_DEVICE_TYPE, SVE_FIMD_SET_OUTPUT_TV, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define    IOCTL_SVE_FIMD_SET_OUTPUT_ENABLE    \
        CTL_CODE(SVE_DEVICE_TYPE, SVE_FIMD_SET_OUTPUT_ENABLE, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define    IOCTL_SVE_FIMD_SET_OUTPUT_DISABLE    \
        CTL_CODE(SVE_DEVICE_TYPE, SVE_FIMD_SET_OUTPUT_DISABLE, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define    IOCTL_SVE_FIMD_SET_WINDOW_MODE    \
        CTL_CODE(SVE_DEVICE_TYPE, SVE_FIMD_SET_WINDOW_MODE, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define    IOCTL_SVE_FIMD_SET_WINDOW_POSITION        \
        CTL_CODE(SVE_DEVICE_TYPE, SVE_FIMD_SET_WINDOW_POSITION, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define    IOCTL_SVE_FIMD_SET_WINDOW_FRAMEBUFFER    \
        CTL_CODE(SVE_DEVICE_TYPE, SVE_FIMD_SET_WINDOW_FRAMEBUFFER, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define    IOCTL_SVE_FIMD_SET_WINDOW_COLORMAP    \
        CTL_CODE(SVE_DEVICE_TYPE, SVE_FIMD_SET_WINDOW_COLORMAP, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define    IOCTL_SVE_FIMD_SET_WINDOW_ENABLE    \
        CTL_CODE(SVE_DEVICE_TYPE, SVE_FIMD_SET_WINDOW_ENABLE, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define    IOCTL_SVE_FIMD_SET_WINDOW_DISABLE    \
        CTL_CODE(SVE_DEVICE_TYPE, SVE_FIMD_SET_WINDOW_DISABLE, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define    IOCTL_SVE_FIMD_SET_WINDOW_BLEND_DISABLE    \
        CTL_CODE(SVE_DEVICE_TYPE, SVE_FIMD_SET_WINDOW_BLEND_DISABLE, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define    IOCTL_SVE_FIMD_SET_WINDOW_BLEND_COLORKEY    \
        CTL_CODE(SVE_DEVICE_TYPE, SVE_FIMD_SET_WINDOW_BLEND_COLORKEY, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define    IOCTL_SVE_FIMD_SET_WINDOW_BLEND_ALPHA        \
        CTL_CODE(SVE_DEVICE_TYPE, SVE_FIMD_SET_WINDOW_BLEND_ALPHA, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define    IOCTL_SVE_FIMD_WAIT_FRAME_INTERRUPT    \
        CTL_CODE(SVE_DEVICE_TYPE, SVE_FIMD_WAIT_FRAME_INTERRUPT, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define    IOCTL_SVE_FIMD_GET_OUTPUT_STATUS    \
        CTL_CODE(SVE_DEVICE_TYPE, SVE_FIMD_GET_OUTPUT_STATUS, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define    IOCTL_SVE_FIMD_GET_WINDOW_STATUS    \
        CTL_CODE(SVE_DEVICE_TYPE, SVE_FIMD_GET_WINDOW_STATUS, METHOD_BUFFERED, FILE_ANY_ACCESS)

// Post Processor IOCTL
#define    IOCTL_SVE_POST_SET_PROCESSING_PARAM    \
        CTL_CODE(SVE_DEVICE_TYPE, SVE_POST_SET_PROCESSING_PARAM, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define    IOCTL_SVE_POST_SET_SOURCE_BUFFER    \
        CTL_CODE(SVE_DEVICE_TYPE, SVE_POST_SET_SOURCE_BUFFER, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define    IOCTL_SVE_POST_SET_NEXT_SOURCE_BUFFER    \
        CTL_CODE(SVE_DEVICE_TYPE, SVE_POST_SET_NEXT_SOURCE_BUFFER, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define    IOCTL_SVE_POST_SET_DESTINATION_BUFFER    \
        CTL_CODE(SVE_DEVICE_TYPE, SVE_POST_SET_DESTINATION_BUFFER, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define    IOCTL_SVE_POST_SET_NEXT_DESTINATION_BUFFER    \
        CTL_CODE(SVE_DEVICE_TYPE, SVE_POST_SET_NEXT_DESTINATION_BUFFER, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define    IOCTL_SVE_POST_SET_PROCESSING_START    \
        CTL_CODE(SVE_DEVICE_TYPE, SVE_POST_SET_PROCESSING_START, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define    IOCTL_SVE_POST_SET_PROCESSING_STOP    \
        CTL_CODE(SVE_DEVICE_TYPE, SVE_POST_SET_PROCESSING_STOP, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define    IOCTL_SVE_POST_WAIT_PROCESSING_DONE    \
        CTL_CODE(SVE_DEVICE_TYPE, SVE_POST_WAIT_PROCESSING_DONE, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define    IOCTL_SVE_POST_GET_PROCESSING_STATUS    \
        CTL_CODE(SVE_DEVICE_TYPE, SVE_POST_GET_PROCESSING_STATUS, METHOD_BUFFERED, FILE_ANY_ACCESS)

// Local Path IOCTL
#define    IOCTL_SVE_LOCALPATH_SET_WIN0_START    \
        CTL_CODE(SVE_DEVICE_TYPE, SVE_LOCALPATH_SET_WIN0_START, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define    IOCTL_SVE_LOCALPATH_SET_WIN0_STOP    \
        CTL_CODE(SVE_DEVICE_TYPE, SVE_LOCALPATH_SET_WIN0_STOP, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define    IOCTL_SVE_LOCALPATH_SET_WIN1_START    \
        CTL_CODE(SVE_DEVICE_TYPE, SVE_LOCALPATH_SET_WIN1_START, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define    IOCTL_SVE_LOCALPATH_SET_WIN1_STOP    \
        CTL_CODE(SVE_DEVICE_TYPE, SVE_LOCALPATH_SET_WIN1_STOP, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define    IOCTL_SVE_LOCALPATH_SET_WIN2_START    \
        CTL_CODE(SVE_DEVICE_TYPE, SVE_LOCALPATH_SET_WIN2_START, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define    IOCTL_SVE_LOCALPATH_SET_WIN2_STOP    \
        CTL_CODE(SVE_DEVICE_TYPE, SVE_LOCALPATH_SET_WIN2_STOP, METHOD_BUFFERED, FILE_ANY_ACCESS)

// Image Rotator IOCTL
#define    IOCTL_SVE_ROTATOR_SET_OPERATION_PARAM    \
        CTL_CODE(SVE_DEVICE_TYPE, SVE_ROTATOR_SET_OPERATION_PARAM, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define    IOCTL_SVE_ROTATOR_SET_SOURCE_BUFFER    \
        CTL_CODE(SVE_DEVICE_TYPE, SVE_ROTATOR_SET_SOURCE_BUFFER, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define    IOCTL_SVE_ROTATOR_SET_DESTINATION_BUFFER    \
        CTL_CODE(SVE_DEVICE_TYPE, SVE_ROTATOR_SET_DESTINATION_BUFFER, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define    IOCTL_SVE_ROTATOR_SET_OPERATION_START    \
        CTL_CODE(SVE_DEVICE_TYPE, SVE_ROTATOR_SET_OPERATION_START, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define    IOCTL_SVE_ROTATOR_SET_OPERATION_STOP    \
        CTL_CODE(SVE_DEVICE_TYPE, SVE_ROTATOR_SET_OPERATION_STOP, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define    IOCTL_SVE_ROTATOR_WAIT_OPERATION_DONE    \
        CTL_CODE(SVE_DEVICE_TYPE, SVE_ROTATOR_WAIT_OPERATION_DONE, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define    IOCTL_SVE_ROTATOR_GET_STATUS    \
        CTL_CODE(SVE_DEVICE_TYPE, SVE_ROTATOR_GET_STATUS, METHOD_BUFFERED, FILE_ANY_ACCESS)

// TV Scaler IOCTL
#define    IOCTL_SVE_TVSC_SET_PROCESSING_PARAM    \
        CTL_CODE(SVE_DEVICE_TYPE, SVE_TVSC_SET_PROCESSING_PARAM, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define    IOCTL_SVE_TVSC_SET_SOURCE_BUFFER    \
        CTL_CODE(SVE_DEVICE_TYPE, SVE_TVSC_SET_SOURCE_BUFFER, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define    IOCTL_SVE_TVSC_SET_NEXT_SOURCE_BUFFER    \
        CTL_CODE(SVE_DEVICE_TYPE, SVE_TVSC_SET_NEXT_SOURCE_BUFFER, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define    IOCTL_SVE_TVSC_SET_DESTINATION_BUFFER    \
        CTL_CODE(SVE_DEVICE_TYPE, SVE_TVSC_SET_DESTINATION_BUFFER, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define    IOCTL_SVE_TVSC_SET_NEXT_DESTINATION_BUFFER    \
        CTL_CODE(SVE_DEVICE_TYPE, SVE_TVSC_SET_NEXT_DESTINATION_BUFFER, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define    IOCTL_SVE_TVSC_SET_PROCESSING_START    \
        CTL_CODE(SVE_DEVICE_TYPE, SVE_TVSC_SET_PROCESSING_START, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define    IOCTL_SVE_TVSC_SET_PROCESSING_STOP    \
        CTL_CODE(SVE_DEVICE_TYPE, SVE_TVSC_SET_PROCESSING_STOP, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define    IOCTL_SVE_TVSC_WAIT_PROCESSING_DONE    \
        CTL_CODE(SVE_DEVICE_TYPE, SVE_TVSC_WAIT_PROCESSING_DONE, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define    IOCTL_SVE_TVSC_GET_PROCESSING_STATUS    \
        CTL_CODE(SVE_DEVICE_TYPE, SVE_TVSC_GET_PROCESSING_STATUS, METHOD_BUFFERED, FILE_ANY_ACCESS)

// TV Encoder IOCTL
#define    IOCTL_SVE_TVENC_SET_INTERFACE_PARAM    \
        CTL_CODE(SVE_DEVICE_TYPE, SVE_TVENC_SET_INTERFACE_PARAM, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define    IOCTL_SVE_TVENC_SET_ENCODER_ON    \
        CTL_CODE(SVE_DEVICE_TYPE, SVE_TVENC_SET_ENCODER_ON, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define    IOCTL_SVE_TVENC_SET_ENCODER_OFF        \
        CTL_CODE(SVE_DEVICE_TYPE, SVE_TVENC_SET_ENCODER_OFF, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define    IOCTL_SVE_TVENC_GET_INTERFACE_STATUS    \
        CTL_CODE(SVE_DEVICE_TYPE, SVE_TVENC_GET_INTERFACE_STATUS, METHOD_BUFFERED, FILE_ANY_ACCESS)

// Power Management IOCTL
#define    IOCTL_SVE_PM_SET_POWER_ON        \
        CTL_CODE(SVE_DEVICE_TYPE, SVE_PM_SET_POWER_ON, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define    IOCTL_SVE_PM_SET_POWER_OFF        \
        CTL_CODE(SVE_DEVICE_TYPE, SVE_PM_SET_POWER_OFF, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define    IOCTL_SVE_PM_GET_POWER_STATUS    \
        CTL_CODE(SVE_DEVICE_TYPE, SVE_PM_GET_POWER_STATUS, METHOD_BUFFERED, FILE_ANY_ACCESS)

typedef struct
{
    tDevInfo tRGBDevInfo;            // RGB I/F Device Information
    DWORD dwTVOutScreenWidth;        // TV Out Device Screen Width
    DWORD dwTVOutScreenHeight;    // TV Out Device Screen Height
} SVEARG_FIMD_OUTPUT_IF;

typedef struct
{
    DWORD dwLineCnt;            // Line Counter
    DWORD dwVerticalStatus;        // Vertical Status
    DWORD dwHorizontalStatus;    // Horizontal Status
    BOOL bENVID;                // ENVID Field of VIDCON0
} SVEARG_FIMD_OUTPUT_STAT;

typedef struct
{
    DWORD dwWinMode;    // FIMD Window Mode
    DWORD dwBPP;        // BitPerPixel
    DWORD dwWidth;        // Window Horizontal Pixel
    DWORD dwHeight;    // Window Vertical Pixel
    DWORD dwOffsetX;    // Window Horizontal Offset
    DWORD dwOffsetY;    // Window Vertical Offset
} SVEARG_FIMD_WIN_MODE;

typedef struct
{
    DWORD dwWinNum;    // FIMD Window Number
    DWORD dwOffsetX;    // Window Horizontal Offset
    DWORD dwOffsetY;    // Window Vertical Offset
} SVEARG_FIMD_WIN_POS;

typedef struct
{
    DWORD dwWinNum;        // FIMD Window Number
    DWORD dwFrameBuffer;    // Frame Buffer Address
    BOOL bWaitForVSync;        // Blocked Operation
} SVEARG_FIMD_WIN_FRAMEBUFFER;

typedef struct
{
    DWORD dwWinNum;        // FIMD Window Number
    DWORD dwDirection;        // Keying Direction
    DWORD dwColorKey;        // Color Key Value
    DWORD dwCompareKey;    // Compare Key Value
    BOOL bOnOff;            // Color Key Enable/Disable
} SVEARG_FIMD_WIN_COLORKEY;

typedef struct
{
    DWORD dwWinNum;    // FIMD Window Number
    DWORD dwMethod;    // Blending Method (per Pixel or per Plane)
    DWORD dwAlpha0;    // Alpha Value 0
    DWORD dwAlpha1;    // Alpha Value 1
} SVEARG_FIMD_WIN_ALPHA;

typedef struct
{
    DWORD dwOpMode;        // Operation Mode (Frame or Free Run)
    DWORD dwScanMode;        // Scan Mode (Progressive or Interace)
    DWORD dwSrcType;        // Src Image Type
    DWORD dwSrcBaseWidth;
    DWORD dwSrcBaseHeight;
    DWORD dwSrcWidth;
    DWORD dwSrcHeight;
    DWORD dwSrcOffsetX;
    DWORD dwSrcOffsetY;
    DWORD dwDstType;        // Dst Image Type
    DWORD dwDstBaseWidth;
    DWORD dwDstBaseHeight;
    DWORD dwDstWidth;
    DWORD dwDstHeight;
    DWORD dwDstOffsetX;
    DWORD dwDstOffsetY;
} SVEARG_POST_PARAMETER;

typedef struct
{
    DWORD dwBufferRGBY;
    DWORD dwBufferCb;
    DWORD dwBufferCr;
    BOOL bWaitForVSync;        // Blocked Operation
} SVEARG_POST_BUFFER;

typedef struct
{
    DWORD dwImgFormat;    // Source Image Format
    DWORD dwOpType;        // Rotator Operation Type
    DWORD dwSrcWidth;
    DWORD dwSrcHeight;
} SVEARG_ROTATOR_PARAMETER;

typedef struct
{
    DWORD dwBufferRGBY;
    DWORD dwBufferCb;
    DWORD dwBufferCr;
} SVEARG_ROTATOR_BUFFER;

typedef struct
{
    DWORD dwOpMode;        // Operation Mode (Frame or Free Run)
    DWORD dwScanMode;        // Scan Mode (Progressive or Interace)
    DWORD dwSrcType;        // Src Image Type
    DWORD dwSrcBaseWidth;
    DWORD dwSrcBaseHeight;
    DWORD dwSrcWidth;
    DWORD dwSrcHeight;
    DWORD dwSrcOffsetX;
    DWORD dwSrcOffsetY;
    DWORD dwDstType;        // Dst Image Type
    DWORD dwDstBaseWidth;
    DWORD dwDstBaseHeight;
    DWORD dwDstWidth;
    DWORD dwDstHeight;
    DWORD dwDstOffsetX;
    DWORD dwDstOffsetY;
} SVEARG_TVSC_PARAMETER;

typedef struct
{
    DWORD dwBufferRGBY;
    DWORD dwBufferCb;
    DWORD dwBufferCr;
    BOOL bWaitForVSync;        // Blocked Operation
} SVEARG_TVSC_BUFFER;

typedef struct
{
    DWORD dwOutputType;        // Output Interface Type
    DWORD dwOutputStandard;    // Output System
    DWORD dwMVisionPattern;    // Macrovision Pattern
    DWORD dwSrcWidth;
    DWORD dwSrcHeight;
} SVEARG_TVENC_PARAMETER;

#define SVE_ERROR_BASE                (0x20000000)        // Non system error code

#endif    // _SVE_API_H_

