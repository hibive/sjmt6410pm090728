
#ifndef __S3C6410_ACCELERATION_H__
#define __S3C6410_ACCELERATION_H__


#include <windows.h>


#ifndef	FILE_DEVICE_HAL
#define FILE_DEVICE_HAL		0x00000101
#endif
#ifndef	METHOD_BUFFERED
#define METHOD_BUFFERED		0
#endif
#ifndef	FILE_ANY_ACCESS
#define FILE_ANY_ACCESS		0
#endif
#ifndef	CTL_CODE
#define CTL_CODE(DeviceType, Function, Method, Access) (((DeviceType)<<16) | ((Access)<<14) | ((Function)<<2) | (Method))
#endif


#define ACC_DRIVER_NAME		_T("ACC1:")
#define	ACC_DRIVER_DEVICE	FILE_DEVICE_HAL
#define	ACC_DRIVER_FUNCTION	3000


// None
#define IOCTL_ACC_INIT\
	CTL_CODE(ACC_DRIVER_DEVICE, ACC_DRIVER_FUNCTION+0x00, METHOD_BUFFERED, FILE_ANY_ACCESS)
// None
#define IOCTL_ACC_DEINIT\
	CTL_CODE(ACC_DRIVER_DEVICE, ACC_DRIVER_FUNCTION+0x01, METHOD_BUFFERED, FILE_ANY_ACCESS)

// None
#define IOCTL_ACC_SOFT_RESET\
	CTL_CODE(ACC_DRIVER_DEVICE, ACC_DRIVER_FUNCTION+0x02, METHOD_BUFFERED, FILE_ANY_ACCESS)

// None
#define IOCTL_ACC_UPDATE_IMAGE\
	CTL_CODE(ACC_DRIVER_DEVICE, ACC_DRIVER_FUNCTION+0x03, METHOD_BUFFERED, FILE_ANY_ACCESS)

// if (pBufIn != NULL && dwLenIn == sizeof(IMAGE_T))
#define IOCTL_ACC_SET_IMAGE\
	CTL_CODE(ACC_DRIVER_DEVICE, ACC_DRIVER_FUNCTION+0x04, METHOD_BUFFERED, FILE_ANY_ACCESS)
// if (pBufOut != NULL && dwLenOut == sizeof(IMAGE_T))
#define IOCTL_ACC_GET_IMAGE\
	CTL_CODE(ACC_DRIVER_DEVICE, ACC_DRIVER_FUNCTION+0x05, METHOD_BUFFERED, FILE_ANY_ACCESS)

// if (pBufIn != NULL && dwLenIn == sizeof(OFFSET_T))
#define IOCTL_ACC_SET_OFFSET\
	CTL_CODE(ACC_DRIVER_DEVICE, ACC_DRIVER_FUNCTION+0x06, METHOD_BUFFERED, FILE_ANY_ACCESS)
// if (pBufOut != NULL && dwLenOut == sizeof(OFFSET_T))
#define IOCTL_ACC_GET_OFFSET\
	CTL_CODE(ACC_DRIVER_DEVICE, ACC_DRIVER_FUNCTION+0x07, METHOD_BUFFERED, FILE_ANY_ACCESS)

// if (pBufIn != NULL && dwLenIn == sizeof(OFFSET_T))
#define IOCTL_ACC_SET_OFFSET_EEPROM\
	CTL_CODE(ACC_DRIVER_DEVICE, ACC_DRIVER_FUNCTION+0x08, METHOD_BUFFERED, FILE_ANY_ACCESS)

// if (pBufIn != NULL && dwLenIn == sizeof(EE_W_TYPE))
#define IOCTL_ACC_SET_EE_W\
	CTL_CODE(ACC_DRIVER_DEVICE, ACC_DRIVER_FUNCTION+0x09, METHOD_BUFFERED, FILE_ANY_ACCESS)

// if (pBufIn !=NULL && dwLenIn == sizeof(EE_T))
#define IOCTL_ACC_WRITE_EE\
	CTL_CODE(ACC_DRIVER_DEVICE, ACC_DRIVER_FUNCTION+0x0A, METHOD_BUFFERED, FILE_ANY_ACCESS)

// if (pBufIn != NULL && dwLenIn == sizeof(SELF_TEST_TYPE))
#define IOCTL_ACC_SELFTEST\
	CTL_CODE(ACC_DRIVER_DEVICE, ACC_DRIVER_FUNCTION+0x0B, METHOD_BUFFERED, FILE_ANY_ACCESS)

// if (pBufIn != NULL && dwLenIn == sizeof(RANGE_TYPE))
#define IOCTL_ACC_SET_RANGE\
	CTL_CODE(ACC_DRIVER_DEVICE, ACC_DRIVER_FUNCTION+0x0C, METHOD_BUFFERED, FILE_ANY_ACCESS)
// if (pBufOut != NULL && dwLenOut == sizeof(RANGE_TYPE))
#define IOCTL_ACC_GET_RANGE\
	CTL_CODE(ACC_DRIVER_DEVICE, ACC_DRIVER_FUNCTION+0x0D, METHOD_BUFFERED, FILE_ANY_ACCESS)

// if (pBufIn != NULL && dwLenIn == sizeof(MODE_TYPE))
#define IOCTL_ACC_SET_MODE\
	CTL_CODE(ACC_DRIVER_DEVICE, ACC_DRIVER_FUNCTION+0x0E, METHOD_BUFFERED, FILE_ANY_ACCESS)
// if (pBufOut != NULL && dwLenOut == sizeof(MODE_TYPE))
#define IOCTL_ACC_GET_MODE\
	CTL_CODE(ACC_DRIVER_DEVICE, ACC_DRIVER_FUNCTION+0x0F, METHOD_BUFFERED, FILE_ANY_ACCESS)

// if (pBufIn != NULL && dwLenIn == sizeof(BANDWIDTH_TYPE))
#define IOCTL_ACC_SET_BANDWIDTH\
	CTL_CODE(ACC_DRIVER_DEVICE, ACC_DRIVER_FUNCTION+0x10, METHOD_BUFFERED, FILE_ANY_ACCESS)
// if (pBufOut != NULL && dwLenOut == sizeof(BANDWIDTH_TYPE))
#define IOCTL_ACC_GET_BANDWIDTH\
	CTL_CODE(ACC_DRIVER_DEVICE, ACC_DRIVER_FUNCTION+0x11, METHOD_BUFFERED, FILE_ANY_ACCESS)

// if (pBufIn != NULL && dwLenIn == sizeof(WAKE_UP_PAUSE_TYPE))
#define IOCTL_ACC_SET_WAKE_UP_PAUSE\
	CTL_CODE(ACC_DRIVER_DEVICE, ACC_DRIVER_FUNCTION+0x12, METHOD_BUFFERED, FILE_ANY_ACCESS)
// if (pBufOut != NULL && dwLenOut == sizeof(WAKE_UP_PAUSE_TYPE))
#define IOCTL_ACC_GET_WAKE_UP_PAUSE\
	CTL_CODE(ACC_DRIVER_DEVICE, ACC_DRIVER_FUNCTION+0x13, METHOD_BUFFERED, FILE_ANY_ACCESS)

// if (pBufIn != NULL && dwLenIn == sizeof(BYTE))
#define IOCTL_ACC_SET_LOW_G_THRESHOLD\
	CTL_CODE(ACC_DRIVER_DEVICE, ACC_DRIVER_FUNCTION+0x14, METHOD_BUFFERED, FILE_ANY_ACCESS)
// if (pBufOut != NULL && dwLenOut == sizeof(BYTE))
#define IOCTL_ACC_GET_LOW_G_THRESHOLD\
	CTL_CODE(ACC_DRIVER_DEVICE, ACC_DRIVER_FUNCTION+0x15, METHOD_BUFFERED, FILE_ANY_ACCESS)

// if (pBufIn != NULL && dwLenIn == sizeof(COUNTER_TYPE))
#define IOCTL_ACC_SET_LOW_G_COUNTDOWN\
	CTL_CODE(ACC_DRIVER_DEVICE, ACC_DRIVER_FUNCTION+0x16, METHOD_BUFFERED, FILE_ANY_ACCESS)
// if (pBufOut != NULL && dwLenOut == sizeof(COUNTER_TYPE))
#define IOCTL_ACC_GET_LOW_G_COUNTDOWN\
	CTL_CODE(ACC_DRIVER_DEVICE, ACC_DRIVER_FUNCTION+0x17, METHOD_BUFFERED, FILE_ANY_ACCESS)

// if (pBufIn != NULL && dwLenIn == sizeof(BYTE))
#define IOCTL_ACC_SET_LOW_G_DURATION\
	CTL_CODE(ACC_DRIVER_DEVICE, ACC_DRIVER_FUNCTION+0x18, METHOD_BUFFERED, FILE_ANY_ACCESS)
// if (pBufOut != NULL && dwLenOut == sizeof(BYTE))
#define IOCTL_ACC_GET_LOW_G_DURATION\
	CTL_CODE(ACC_DRIVER_DEVICE, ACC_DRIVER_FUNCTION+0x19, METHOD_BUFFERED, FILE_ANY_ACCESS)

// if (pBufIn != NULL && dwLenIn == sizeof(BYTE))
#define IOCTL_ACC_SET_LOW_G_HYST\
	CTL_CODE(ACC_DRIVER_DEVICE, ACC_DRIVER_FUNCTION+0x1A, METHOD_BUFFERED, FILE_ANY_ACCESS)
// if (pBufOut != NULL && dwLenOut == sizeof(BYTE))
#define IOCTL_ACC_GET_LOW_G_HYST\
	CTL_CODE(ACC_DRIVER_DEVICE, ACC_DRIVER_FUNCTION+0x1B, METHOD_BUFFERED, FILE_ANY_ACCESS)

// if (pBufIn != NULL && dwLenIn == sizeof(BYTE))
#define IOCTL_ACC_SET_HIGH_G_THRESHOLD\
	CTL_CODE(ACC_DRIVER_DEVICE, ACC_DRIVER_FUNCTION+0x1C, METHOD_BUFFERED, FILE_ANY_ACCESS)
// if (pBufOut != NULL && dwLenOut == sizeof(BYTE))
#define IOCTL_ACC_GET_HIGH_G_THRESHOLD\
	CTL_CODE(ACC_DRIVER_DEVICE, ACC_DRIVER_FUNCTION+0x1D, METHOD_BUFFERED, FILE_ANY_ACCESS)

// if (pBufIn != NULL && dwLenIn == sizeof(COUNTER_TYPE))
#define IOCTL_ACC_SET_HIGH_G_COUNTDOWN\
	CTL_CODE(ACC_DRIVER_DEVICE, ACC_DRIVER_FUNCTION+0x1E, METHOD_BUFFERED, FILE_ANY_ACCESS)
// if (pBufOut != NULL && dwLenOut == sizeof(COUNTER_TYPE))
#define IOCTL_ACC_GET_HIGH_G_COUNTDOWN\
	CTL_CODE(ACC_DRIVER_DEVICE, ACC_DRIVER_FUNCTION+0x1F, METHOD_BUFFERED, FILE_ANY_ACCESS)

// if (pBufIn != NULL && dwLenIn == sizeof(BYTE))
#define IOCTL_ACC_SET_HIGH_G_DURATION\
	CTL_CODE(ACC_DRIVER_DEVICE, ACC_DRIVER_FUNCTION+0x20, METHOD_BUFFERED, FILE_ANY_ACCESS)
// if (pBufOut != NULL && dwLenOut == sizeof(BYTE))
#define IOCTL_ACC_GET_HIGH_G_DURATION\
	CTL_CODE(ACC_DRIVER_DEVICE, ACC_DRIVER_FUNCTION+0x21, METHOD_BUFFERED, FILE_ANY_ACCESS)

// if (pBufIn != NULL && dwLenIn == sizeof(BYTE))
#define IOCTL_ACC_SET_HIGH_G_HYST\
	CTL_CODE(ACC_DRIVER_DEVICE, ACC_DRIVER_FUNCTION+0x22, METHOD_BUFFERED, FILE_ANY_ACCESS)
// if (pBufOut != NULL && dwLenOut == sizeof(BYTE))
#define IOCTL_ACC_GET_HIGH_G_HYST\
	CTL_CODE(ACC_DRIVER_DEVICE, ACC_DRIVER_FUNCTION+0x23, METHOD_BUFFERED, FILE_ANY_ACCESS)

// if (pBufIn != NULL && dwLenIn == sizeof(BYTE))
#define IOCTL_ACC_SET_ANY_MOTION_THRESHOLD\
	CTL_CODE(ACC_DRIVER_DEVICE, ACC_DRIVER_FUNCTION+0x24, METHOD_BUFFERED, FILE_ANY_ACCESS)
// if (pBufOut != NULL && dwLenOut == sizeof(BYTE))
#define IOCTL_ACC_GET_ANY_MOTION_THRESHOLD\
	CTL_CODE(ACC_DRIVER_DEVICE, ACC_DRIVER_FUNCTION+0x25, METHOD_BUFFERED, FILE_ANY_ACCESS)

// if (pBufIn != NULL && dwLenIn == sizeof(ANY_MOTION_DUR_TYPE))
#define IOCTL_ACC_SET_ANY_MOTION_COUNT\
	CTL_CODE(ACC_DRIVER_DEVICE, ACC_DRIVER_FUNCTION+0x26, METHOD_BUFFERED, FILE_ANY_ACCESS)
// if (pBufOut != NULL && dwLenOut == sizeof(ANY_MOTION_DUR_TYPE))
#define IOCTL_ACC_GET_ANY_MOTION_COUNT\
	CTL_CODE(ACC_DRIVER_DEVICE, ACC_DRIVER_FUNCTION+0x27, METHOD_BUFFERED, FILE_ANY_ACCESS)

// if (pBufIn != NULL && dwLenIn == sizeof(INT_MASK_TYPE))
#define IOCTL_ACC_SET_INTERRUPT_MASK\
	CTL_CODE(ACC_DRIVER_DEVICE, ACC_DRIVER_FUNCTION+0x28, METHOD_BUFFERED, FILE_ANY_ACCESS)
// if (pBufOut != NULL && dwLenOut == sizeof(INT_MASK_TYPE))
#define IOCTL_ACC_GET_INTERRUPT_MASK\
	CTL_CODE(ACC_DRIVER_DEVICE, ACC_DRIVER_FUNCTION+0x29, METHOD_BUFFERED, FILE_ANY_ACCESS)

// None
#define IOCTL_ACC_RESET_INTERRUPT\
	CTL_CODE(ACC_DRIVER_DEVICE, ACC_DRIVER_FUNCTION+0x2A, METHOD_BUFFERED, FILE_ANY_ACCESS)

// if (pBufOut != NULL && dwLenOut == sizeof(short))
#define IOCTL_ACC_READ_ACCEL_X\
	CTL_CODE(ACC_DRIVER_DEVICE, ACC_DRIVER_FUNCTION+0x2B, METHOD_BUFFERED, FILE_ANY_ACCESS)
// if (pBufOut != NULL && dwLenOut == sizeof(short))
#define IOCTL_ACC_READ_ACCEL_Y\
	CTL_CODE(ACC_DRIVER_DEVICE, ACC_DRIVER_FUNCTION+0x2C, METHOD_BUFFERED, FILE_ANY_ACCESS)
// if (pBufOut != NULL && dwLenOut == sizeof(short))
#define IOCTL_ACC_READ_ACCEL_Z\
	CTL_CODE(ACC_DRIVER_DEVICE, ACC_DRIVER_FUNCTION+0x2D, METHOD_BUFFERED, FILE_ANY_ACCESS)
// if (pBufOut != NULL && dwLenOut == sizeof(BYTE))
#define IOCTL_ACC_READ_TEMPERATURE\
	CTL_CODE(ACC_DRIVER_DEVICE, ACC_DRIVER_FUNCTION+0x2E, METHOD_BUFFERED, FILE_ANY_ACCESS)
// if (pBufOut != NULL && dwLenOut == sizeof(ACC_T))
#define IOCTL_ACC_READ_ACCEL_XYZT\
	CTL_CODE(ACC_DRIVER_DEVICE, ACC_DRIVER_FUNCTION+0x2F, METHOD_BUFFERED, FILE_ANY_ACCESS)

// if (pBufOut != NULL && dwLenOut == sizeof(INT_STATUS_TYPE))
#define IOCTL_ACC_GET_INTERRUPT_STATUS\
	CTL_CODE(ACC_DRIVER_DEVICE, ACC_DRIVER_FUNCTION+0x30, METHOD_BUFFERED, FILE_ANY_ACCESS)

// if (pBufIn != NULL && dwLenIn == sizeof(BOOL))
#define IOCTL_ACC_SET_LOW_G_INT\
	CTL_CODE(ACC_DRIVER_DEVICE, ACC_DRIVER_FUNCTION+0x31, METHOD_BUFFERED, FILE_ANY_ACCESS)
// if (pBufIn != NULL && dwLenIn == sizeof(BOOL))
#define IOCTL_ACC_SET_HIGH_G_INT\
	CTL_CODE(ACC_DRIVER_DEVICE, ACC_DRIVER_FUNCTION+0x32, METHOD_BUFFERED, FILE_ANY_ACCESS)
// if (pBufIn != NULL && dwLenIn == sizeof(BOOL))
#define IOCTL_ACC_SET_ANY_MOTION_INT\
	CTL_CODE(ACC_DRIVER_DEVICE, ACC_DRIVER_FUNCTION+0x33, METHOD_BUFFERED, FILE_ANY_ACCESS)
// if (pBufIn != NULL && dwLenIn == sizeof(BOOL))
#define IOCTL_ACC_SET_ALERT_INT\
	CTL_CODE(ACC_DRIVER_DEVICE, ACC_DRIVER_FUNCTION+0x34, METHOD_BUFFERED, FILE_ANY_ACCESS)
// if (pBufIn != NULL && dwLenIn == sizeof(BOOL))
#define IOCTL_ACC_SET_ADVANCED_INT\
	CTL_CODE(ACC_DRIVER_DEVICE, ACC_DRIVER_FUNCTION+0x35, METHOD_BUFFERED, FILE_ANY_ACCESS)
// if (pBufIn != NULL && dwLenIn == sizeof(BOOL))
#define IOCTL_ACC_LATCH_INT\
	CTL_CODE(ACC_DRIVER_DEVICE, ACC_DRIVER_FUNCTION+0x36, METHOD_BUFFERED, FILE_ANY_ACCESS)
// if (pBufIn != NULL && dwLenIn == sizeof(BOOL))
#define IOCTL_ACC_SET_NEW_DATA_INT\
	CTL_CODE(ACC_DRIVER_DEVICE, ACC_DRIVER_FUNCTION+0x37, METHOD_BUFFERED, FILE_ANY_ACCESS)

// if (pBufIn != NULL && dwLenIn == sizeof(int))
#define IOCTL_ACC_PAUSE\
	CTL_CODE(ACC_DRIVER_DEVICE, ACC_DRIVER_FUNCTION+0x38, METHOD_BUFFERED, FILE_ANY_ACCESS)

// if (pBufIn != NULL && dwLenIn == sizeof(REG_T))
#define IOCTL_ACC_WRITE_REG\
	CTL_CODE(ACC_DRIVER_DEVICE, ACC_DRIVER_FUNCTION+0x39, METHOD_BUFFERED, FILE_ANY_ACCESS)
// if (pBufOut != NULL && dwLenOut == sizeof(REG_T))
#define IOCTL_ACC_READ_REG\
	CTL_CODE(ACC_DRIVER_DEVICE, ACC_DRIVER_FUNCTION+0x3A, METHOD_BUFFERED, FILE_ANY_ACCESS)

// if (pBufOut != NULL && dwLenOut == sizeof(INT_MASK_TYPE))
#define IOCTL_ACC_WAIT_INTERRUPT\
	CTL_CODE(ACC_DRIVER_DEVICE, ACC_DRIVER_FUNCTION+0x3B, METHOD_BUFFERED, FILE_ANY_ACCESS)



typedef enum {
	AXIS_X=0,
	AXIS_Y,
	AXIS_Z,
	AXIS_T,
} AXIS_TYPE;

typedef enum {
	EE_W_LOCK=0,
	EE_W_UNLOCK
} EE_W_TYPE;

typedef enum {
	SELF_TEST0_ON=1,
	SELF_TEST1_ON=3,
} SELF_TEST_TYPE;

typedef enum {
	RANGE_2G=0,
	RANGE_4G,
	RANGE_8G,
} RANGE_TYPE;

typedef enum {
	MODE_NORMAL=0,
	MODE_SLEEP=2,
	MODE_WAKE_UP=3,
} MODE_TYPE;

typedef enum {
	BANDWIDTH_25HZ=0,
	BANDWIDTH_50HZ,
	BANDWIDTH_100HZ,
	BANDWIDTH_190HZ,
	BANDWIDTH_375HZ,
	BANDWIDTH_750HZ,
	BANDWIDTH_1500HZ,
} BANDWIDTH_TYPE;

typedef enum {
	WAKE_UP_PAUSE_20MS=0,
	WAKE_UP_PAUSE_80MS,
	WAKE_UP_PAUSE_320MS,
	WAKE_UP_PAUSE_2560MS,
} WAKE_UP_PAUSE_TYPE;

typedef enum {
	COUNTER_RST=0,
	COUNTER_1LSB,
	COUNTER_2LSB,
	COUNTER_3LSB,
} COUNTER_TYPE;

typedef enum {
	ANY_MOTION_DUR_1=0,
	ANY_MOTION_DUR_3,
	ANY_MOTION_DUR_5,
	ANY_MOTION_DUR_7,
} ANY_MOTION_DUR_TYPE;

typedef enum {
	INT_MASK_ALERT=(1<<7),
	INT_MASK_ANY_MOTION=(1<<6),
	INT_MASK_EN_ADV_INT=(1<<5),
	INT_MASK_NEW_DATA=(1<<4),
	INT_MASK_LATCH=(1<<3),
	INT_MASK_HG=(1<<1),
	INT_MASK_LG=(1<<0),
} INT_MASK_TYPE;

typedef enum {
	INT_STATUS_HG=(1<<0),
	INT_STATUS_LG=(1<<1),
	INT_STATUS_HG_LATCHED=(1<<2),
	INT_STATUS_LG_LATCHED=(1<<3),
	INT_STATUS_ALERT=(1<<4),
	INT_STATUS_ST_RESULT=(1<<7),
} INT_STATUS_TYPE;


typedef struct {
	BYTE	conf1,			/**<  image address 0x0b: interrupt enable bits, low-g settings */
			lg_threshold,	/**<  image address 0x0c: low-g threshold, depends on selected g-range */
			lg_duration,	/**<  image address 0x0d: low-g duration in ms */
			hg_threshold,	/**<  image address 0x0e: high-g threshold, depends on selected g-range */
			hg_duration,	/**<  image address 0x0f: high-g duration in ms */
			motion_thrs,	/**<  image address 0x10: any motion threshold */
			hysteresis,		/**<  image address 0x11: low-g and high-g hysteresis register */
			customer1,		/**<  image address 0x12: customer reserved register 1 */
			customer2,		/**<  image address 0x13: customer reserved register 2  */
			range_bwidth,	/**<  image address 0x14: range and bandwidth selection register */
			conf2,			/**<  image address 0x15: spi4, latched interrupt, auto-wake-up configuration */
			offs_gain_x,	/**<  image address 0x16: offset_x LSB and x-axis gain settings */
			offs_gain_y,	/**<  image address 0x17: offset_y LSB and y-axis gain settings */
			offs_gain_z,	/**<  image address 0x18: offset_z LSB and z-axis gain settings */
			offs_gain_t,	/**<  image address 0x19: offset_t LSB and temperature gain settings */
			offset_x,		/**<  image address 0x1a: offset_x calibration MSB register */
			offset_y,		/**<  image address 0x1b: offset_y calibration MSB register */ 
			offset_z,		/**<  image address 0x1c: offset_z calibration MSB register */ 
			offset_t;		/**<  image address 0x1d: temperature calibration MSB register */ 
} IMAGE_T;

typedef struct {
	AXIS_TYPE	axis;
	WORD 		offset;
} OFFSET_T;

typedef struct {
	BYTE	addr;
	BYTE	data;
} EE_T;

typedef struct	{
	short	x, /**< holds x-axis acceleration data sign extended. Range -512 to 511. */
			y, /**< holds y-axis acceleration data sign extended. Range -512 to 511. */
			z; /**< holds z-axis acceleration data sign extended. Range -512 to 511. */
	BYTE	t; /** an output of 0 equals -30°C, 1 LSB equals 0.5°C */
} ACC_T;

typedef struct {
	BYTE	addr;
	BYTE	len;
	BYTE	*data;
} REG_T;


#endif //__S3C6410_ACCELERATION_H__

