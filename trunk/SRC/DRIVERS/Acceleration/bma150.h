/* $Date: 2007.12.23$
* $Revision: 1.0.1.229$
*
*/

/*
* Copyright (C) 2007 Bosch Sensortec GmbH
*
* MODULE NAME
*
* Usage: DESCRIPTION
*
*
* Disclaimer
*
* Common:
* Bosch Sensortec products are developed for the consumer goods industry. They may only be used
* within the parameters of the respective valid product data sheet. Bosch Sensortec products are
* provided with the express understanding that there is no warranty of fitness for a particular purpose.
* They are not fit for use in life-sustaining, safety or security sensitive systems or any system or device
* that may lead to bodily harm or property damage if the system or device malfunctions. In addition,
* Bosch Sensortec products are not fit for use in products which interact with motor vehicle systems.
* The resale and/or use of products are at the purchaser's own risk and his own responsibility. The
* examination of fitness for the intended use is the sole responsibility of the Purchaser.
*
* The purchaser shall indemnify Bosch Sensortec from all third party claims, including any claims for
* incidental, or consequential damages, arising from any product use not covered by the parameters of
* the respective valid product data sheet or not approved by Bosch Sensortec and reimburse Bosch
* Sensortec for all costs in connection with such claims.
*
* The purchaser must monitor the market for the purchased products, particularly with regard to
* product safety and inform Bosch Sensortec without delay of all security relevant incidents.
*
* Engineering Samples are marked with an asterisk (*) or (e). Samples may vary from the valid
* technical specifications of the product series. They are therefore not intended or fit for resale to third
* parties or for use in end products. Their sole purpose is internal client testing. The testing of an
* engineering sample may in no way replace the testing of a product series. Bosch Sensortec
* assumes no liability for the use of engineering samples. By accepting the engineering samples, the
* Purchaser agrees to indemnify Bosch Sensortec from all claims arising from the use of engineering
* samples.
*
* Special:
* This software module (hereinafter called "Software") and any information on application-sheets
* (hereinafter called "Information") is provided free of charge for the sole purpose to support your
* application work. The Software and Information is subject to the following terms and conditions:
*
* The Software is specifically designed for the exclusive use for Bosch Sensortec products by
* personnel who have special experience and training. Do not use this Software if you do not have the
* proper experience or training.
*
* This Software package is provided `` as is `` and without any expressed or implied warranties,
* including without limitation, the implied warranties of merchantability and fitness for a particular
* purpose.
*
* Bosch Sensortec and their representatives and agents deny any liability for the functional impairment
* of this Software in terms of fitness, performance and safety. Bosch Sensortec and their
* representatives and agents shall not be liable for any direct or indirect damages or injury, except as
* otherwise stipulated in mandatory applicable law.
*
* The Information provided is believed to be accurate and reliable. Bosch Sensortec assumes no
* responsibility for the consequences of use of such Information nor for any infringement of patents or
* other rights of third parties which may result from its use. No license is granted by implication or
* otherwise under any patent or patent rights of Bosch. Specifications mentioned in the Information are
* subject to change without notice.
*
* It is not allowed to deliver the source code of the Software to any third party without permission of
* Bosch Sensortec.
*/

/** \file bma150.h
    \brief Header file for all #define constants and function prototypes for native bma150 driver dll.
*/

#ifndef __BMA150_H__
#define __BMA150_H__

/* BMA150 Macro for read and write commincation */


/**
   define for used read and write macros 
*/

//init place
#define DoInitInDriverLoad
#define BMA150_I2C_ADDR		0x70


/*
	BMA150 API error codes
*/
#define E_BMA_NULL_PTR		(char)-1
#define E_COMM_RES		    (char)-2
#define E_OUT_OF_RANGE		(char)-3
#define E_INVALID_PARAMETER (char)-4
#define E_IIC_INIT_FAIL		(char)-5



/* 
 *	register definitions 	
 */
#define BMA150_EEP_OFFSET   0x20
#define BMA150_IMAGE_BASE	0x0b
#define BMA150_IMAGE_LEN	19

#define CHIP_ID_REG			0x00
#define VERSION_REG			0x01
#define X_AXIS_LSB_REG		0x02
#define X_AXIS_MSB_REG		0x03
#define Y_AXIS_LSB_REG		0x04
#define Y_AXIS_MSB_REG		0x05
#define Z_AXIS_LSB_REG		0x06
#define Z_AXIS_MSB_REG		0x07
#define TEMP_RD_REG			0x08
#define BMA150_STATUS_REG	0x09
#define BMA150_CTRL_REG		0x0a
#define BMA150_CONF1_REG	0x0b
#define LG_THRESHOLD_REG	0x0c
#define LG_DURATION_REG		0x0d
#define HG_THRESHOLD_REG	0x0e
#define HG_DURATION_REG		0x0f
#define MOTION_THRS_REG		0x10
#define HYSTERESIS_REG		0x11
#define CUSTOMER1_REG		0x12
#define CUSTOMER2_REG		0x13
#define RANGE_BWIDTH_REG	0x14
#define BMA150_CONF2_REG	0x15

#define OFFS_GAIN_X_REG		0x16
#define OFFS_GAIN_Y_REG		0x17
#define OFFS_GAIN_Z_REG		0x18
#define OFFS_GAIN_T_REG		0x19
#define OFFSET_X_REG		0x1a
#define OFFSET_Y_REG		0x1b
#define OFFSET_Z_REG		0x1c
#define OFFSET_T_REG		0x1d


/* register write and read delays */
#define MDELAY_DATA_TYPE	unsigned int
#define BMA150_EE_W_DELAY	28	/* delay after EEP write is 28 msec */


/** bma150 typedef structure
	\brief This structure holds all relevant information about BMA150 and links communication to the 
*/
typedef struct {	
	int (* bus_write)(unsigned char, unsigned char, unsigned char *, unsigned char);	/**< function pointer to the SPI/I2C write function */
	int (* bus_read)(unsigned char, unsigned char, unsigned char *, unsigned char);		/**< function pointer to the SPI/I2C read function */
	void (*delay_msec)(DWORD);	/**< function pointer to a pause in mili seconds function */

	unsigned char	chip_id,	/**< save BMA150's chip id which has to be 0x02 after calling bma150_init() */
					ml_version, /**< holds the BMA150 ML_version number */	
					al_version; /**< holds the BMA150 AL_version number */

	unsigned char	mode;		/**< save current BMA150 operation mode */
} bma150_t;

/* 
 *	bit slice positions in registers
 */
#define EEPROM_BEGIN	0x2b
#define EEPROM_END		0x42


/** \cond BITSLICE */

#define CHIP_ID__POS		0
#define CHIP_ID__MSK		0x07
#define CHIP_ID__LEN		3
#define CHIP_ID__REG		CHIP_ID_REG

#define ML_VERSION__POS		0
#define ML_VERSION__LEN		4
#define ML_VERSION__MSK		0x0F
#define ML_VERSION__REG		VERSION_REG

#define AL_VERSION__POS  	4
#define AL_VERSION__LEN  	4
#define AL_VERSION__MSK		0xF0
#define AL_VERSION__REG		VERSION_REG


/* DATA REGISTERS */

#define NEW_DATA_X__POS  	0
#define NEW_DATA_X__LEN  	1
#define NEW_DATA_X__MSK  	0x01
#define NEW_DATA_X__REG		X_AXIS_LSB_REG

#define ACC_X_LSB__POS   	6
#define ACC_X_LSB__LEN   	2
#define ACC_X_LSB__MSK		0xC0
#define ACC_X_LSB__REG		X_AXIS_LSB_REG

#define ACC_X_MSB__POS   	0
#define ACC_X_MSB__LEN   	8
#define ACC_X_MSB__MSK		0xFF
#define ACC_X_MSB__REG		X_AXIS_MSB_REG

#define NEW_DATA_Y__POS  	0
#define NEW_DATA_Y__LEN  	1
#define NEW_DATA_Y__MSK  	0x01
#define NEW_DATA_Y__REG		Y_AXIS_LSB_REG

#define ACC_Y_LSB__POS   	6
#define ACC_Y_LSB__LEN   	2
#define ACC_Y_LSB__MSK   	0xC0
#define ACC_Y_LSB__REG		Y_AXIS_LSB_REG

#define ACC_Y_MSB__POS   	0
#define ACC_Y_MSB__LEN   	8
#define ACC_Y_MSB__MSK   	0xFF
#define ACC_Y_MSB__REG		Y_AXIS_MSB_REG

#define NEW_DATA_Z__POS  	0
#define NEW_DATA_Z__LEN  	1
#define NEW_DATA_Z__MSK		0x01
#define NEW_DATA_Z__REG		Z_AXIS_LSB_REG

#define ACC_Z_LSB__POS   	6
#define ACC_Z_LSB__LEN   	2
#define ACC_Z_LSB__MSK		0xC0
#define ACC_Z_LSB__REG		Z_AXIS_LSB_REG

#define ACC_Z_MSB__POS   	0
#define ACC_Z_MSB__LEN   	8
#define ACC_Z_MSB__MSK		0xFF
#define ACC_Z_MSB__REG		Z_AXIS_MSB_REG

#define TEMPERATURE__POS 	0
#define TEMPERATURE__LEN 	8
#define TEMPERATURE__MSK 	0xFF
#define TEMPERATURE__REG	TEMP_RD_REG


/* STATUS BITS */

#define STATUS_HG__POS		0
#define STATUS_HG__LEN		1
#define STATUS_HG__MSK		0x01
#define STATUS_HG__REG		BMA150_STATUS_REG

#define STATUS_LG__POS		1
#define STATUS_LG__LEN		1
#define STATUS_LG__MSK		0x02
#define STATUS_LG__REG		BMA150_STATUS_REG

#define HG_LATCHED__POS  	2
#define HG_LATCHED__LEN  	1
#define HG_LATCHED__MSK		0x04
#define HG_LATCHED__REG		BMA150_STATUS_REG

#define LG_LATCHED__POS		3
#define LG_LATCHED__LEN		1
#define LG_LATCHED__MSK		8
#define LG_LATCHED__REG		BMA150_STATUS_REG

#define ALERT_PHASE__POS	4
#define ALERT_PHASE__LEN	1
#define ALERT_PHASE__MSK	0x10
#define ALERT_PHASE__REG	BMA150_STATUS_REG

#define ST_RESULT__POS		7
#define ST_RESULT__LEN		1
#define ST_RESULT__MSK		0x80
#define ST_RESULT__REG		BMA150_STATUS_REG


/* CONTROL BITS */

#define SLEEP__POS			0
#define SLEEP__LEN			1
#define SLEEP__MSK			0x01
#define SLEEP__REG			BMA150_CTRL_REG

#define SOFT_RESET__POS		1
#define SOFT_RESET__LEN		1
#define SOFT_RESET__MSK		0x02
#define SOFT_RESET__REG		BMA150_CTRL_REG

#define SELF_TEST__POS		2
#define SELF_TEST__LEN		2
#define SELF_TEST__MSK		0x0C
#define SELF_TEST__REG		BMA150_CTRL_REG

#define SELF_TEST0__POS		2
#define SELF_TEST0__LEN		1
#define SELF_TEST0__MSK		0x04
#define SELF_TEST0__REG		BMA150_CTRL_REG

#define SELF_TEST1__POS		3
#define SELF_TEST1__LEN		1
#define SELF_TEST1__MSK		0x08
#define SELF_TEST1__REG		BMA150_CTRL_REG

#define EE_W__POS			4
#define EE_W__LEN			1
#define EE_W__MSK			0x10
#define EE_W__REG			BMA150_CTRL_REG

#define UPDATE_IMAGE__POS	5
#define UPDATE_IMAGE__LEN	1
#define UPDATE_IMAGE__MSK	0x20
#define UPDATE_IMAGE__REG	BMA150_CTRL_REG

#define RESET_INT__POS		6
#define RESET_INT__LEN		1
#define RESET_INT__MSK		0x40
#define RESET_INT__REG		BMA150_CTRL_REG


/* LOW-G, HIGH-G settings */

#define ENABLE_LG__POS		0
#define ENABLE_LG__LEN		1
#define ENABLE_LG__MSK		0x01
#define ENABLE_LG__REG		BMA150_CONF1_REG

#define ENABLE_HG__POS		1
#define ENABLE_HG__LEN		1
#define ENABLE_HG__MSK		0x02
#define ENABLE_HG__REG		BMA150_CONF1_REG


/* LG/HG counter */

#define COUNTER_LG__POS		2
#define COUNTER_LG__LEN		2
#define COUNTER_LG__MSK		0x0C
#define COUNTER_LG__REG		BMA150_CONF1_REG
	
#define COUNTER_HG__POS		4
#define COUNTER_HG__LEN		2
#define COUNTER_HG__MSK		0x30
#define COUNTER_HG__REG		BMA150_CONF1_REG


/* LG/HG duration is in ms */

#define LG_DUR__POS			0
#define LG_DUR__LEN			8
#define LG_DUR__MSK			0xFF
#define LG_DUR__REG			LG_DURATION_REG

#define HG_DUR__POS			0
#define HG_DUR__LEN			8
#define HG_DUR__MSK			0xFF
#define HG_DUR__REG			HG_DURATION_REG

#define LG_THRES__POS		0
#define LG_THRES__LEN		8
#define LG_THRES__MSK		0xFF
#define LG_THRES__REG		LG_THRESHOLD_REG

#define HG_THRES__POS		0
#define HG_THRES__LEN		8
#define HG_THRES__MSK		0xFF
#define HG_THRES__REG		HG_THRESHOLD_REG

#define LG_HYST__POS		0
#define LG_HYST__LEN		3
#define LG_HYST__MSK		0x07
#define LG_HYST__REG		HYSTERESIS_REG

#define HG_HYST__POS		3
#define HG_HYST__LEN		3
#define HG_HYST__MSK		0x38
#define HG_HYST__REG		HYSTERESIS_REG


/* ANY MOTION and ALERT settings */

#define EN_ANY_MOTION__POS	6
#define EN_ANY_MOTION__LEN	1
#define EN_ANY_MOTION__MSK	0x40
#define EN_ANY_MOTION__REG	BMA150_CONF1_REG


/* ALERT settings */

#define ALERT__POS			7
#define ALERT__LEN			1
#define ALERT__MSK			0x80
#define ALERT__REG			BMA150_CONF1_REG


/* ANY MOTION Duration */

#define ANY_MOTION_THRES__POS	0
#define ANY_MOTION_THRES__LEN	8
#define ANY_MOTION_THRES__MSK	0xFF
#define ANY_MOTION_THRES__REG	MOTION_THRS_REG

#define ANY_MOTION_DUR__POS		6
#define ANY_MOTION_DUR__LEN		2
#define ANY_MOTION_DUR__MSK		0xC0	
#define ANY_MOTION_DUR__REG		HYSTERESIS_REG

#define CUSTOMER_RESERVED1__POS	0
#define CUSTOMER_RESERVED1__LEN	8
#define CUSTOMER_RESERVED1__MSK	0xFF
#define CUSTOMER_RESERVED1__REG	CUSTOMER1_REG

#define CUSTOMER_RESERVED2__POS	0
#define CUSTOMER_RESERVED2__LEN	8
#define CUSTOMER_RESERVED2__MSK	0xFF
#define CUSTOMER_RESERVED2__REG	CUSTOMER2_REG


/* BANDWIDTH dependend definitions */

#define BANDWIDTH__POS			0
#define BANDWIDTH__LEN		 	3
#define BANDWIDTH__MSK		 	0x07
#define BANDWIDTH__REG			RANGE_BWIDTH_REG


/* RANGE */

#define RANGE__POS				3
#define RANGE__LEN				2
#define RANGE__MSK				0x18	
#define RANGE__REG				RANGE_BWIDTH_REG


/* WAKE UP */

#define WAKE_UP__POS			0
#define WAKE_UP__LEN			1
#define WAKE_UP__MSK			0x01
#define WAKE_UP__REG			BMA150_CONF2_REG

#define WAKE_UP_PAUSE__POS		1
#define WAKE_UP_PAUSE__LEN		2
#define WAKE_UP_PAUSE__MSK		0x06
#define WAKE_UP_PAUSE__REG		BMA150_CONF2_REG


/* ACCELERATION DATA SHADOW */

#define SHADOW_DIS__POS			3
#define SHADOW_DIS__LEN			1
#define SHADOW_DIS__MSK			0x08
#define SHADOW_DIS__REG			BMA150_CONF2_REG


/* LATCH Interrupt */

#define LATCH_INT__POS			4
#define LATCH_INT__LEN			1
#define LATCH_INT__MSK			0x10
#define LATCH_INT__REG			BMA150_CONF2_REG


/* new data interrupt */

#define NEW_DATA_INT__POS		5
#define NEW_DATA_INT__LEN		1
#define NEW_DATA_INT__MSK		0x20
#define NEW_DATA_INT__REG		BMA150_CONF2_REG

#define ENABLE_ADV_INT__POS		6
#define ENABLE_ADV_INT__LEN		1
#define ENABLE_ADV_INT__MSK		0x40
#define ENABLE_ADV_INT__REG		BMA150_CONF2_REG

#define BMA150_SPI4_OFF			0
#define BMA150_SPI4_ON			1

#define SPI4__POS				7
#define SPI4__LEN				1
#define SPI4__MSK				0x80
#define SPI4__REG				BMA150_CONF2_REG

#define OFFSET_X_LSB__POS		6
#define OFFSET_X_LSB__LEN		2
#define OFFSET_X_LSB__MSK		0xC0
#define OFFSET_X_LSB__REG		OFFS_GAIN_X_REG

#define GAIN_X__POS				0
#define GAIN_X__LEN				6
#define GAIN_X__MSK				0x3f
#define GAIN_X__REG				OFFS_GAIN_X_REG

#define OFFSET_Y_LSB__POS		6
#define OFFSET_Y_LSB__LEN		2
#define OFFSET_Y_LSB__MSK		0xC0
#define OFFSET_Y_LSB__REG		OFFS_GAIN_Y_REG

#define GAIN_Y__POS				0
#define GAIN_Y__LEN				6
#define GAIN_Y__MSK				0x3f
#define GAIN_Y__REG				OFFS_GAIN_Y_REG

#define OFFSET_Z_LSB__POS		6
#define OFFSET_Z_LSB__LEN		2
#define OFFSET_Z_LSB__MSK		0xC0
#define OFFSET_Z_LSB__REG		OFFS_GAIN_Z_REG

#define GAIN_Z__POS				0
#define GAIN_Z__LEN				6
#define GAIN_Z__MSK				0x3f
#define GAIN_Z__REG				OFFS_GAIN_Z_REG

#define OFFSET_T_LSB__POS		6
#define OFFSET_T_LSB__LEN		2
#define OFFSET_T_LSB__MSK		0xC0
#define OFFSET_T_LSB__REG		OFFS_GAIN_T_REG

#define GAIN_T__POS				0
#define GAIN_T__LEN				6
#define GAIN_T__MSK				0x3f
#define GAIN_T__REG				OFFS_GAIN_T_REG

#define OFFSET_X_MSB__POS		0
#define OFFSET_X_MSB__LEN		8
#define OFFSET_X_MSB__MSK		0xFF
#define OFFSET_X_MSB__REG		OFFSET_X_REG

#define OFFSET_Y_MSB__POS		0
#define OFFSET_Y_MSB__LEN		8
#define OFFSET_Y_MSB__MSK		0xFF
#define OFFSET_Y_MSB__REG		OFFSET_Y_REG

#define OFFSET_Z_MSB__POS		0
#define OFFSET_Z_MSB__LEN		8
#define OFFSET_Z_MSB__MSK		0xFF
#define OFFSET_Z_MSB__REG		OFFSET_Z_REG

#define OFFSET_T_MSB__POS		0
#define OFFSET_T_MSB__LEN		8
#define OFFSET_T_MSB__MSK		0xFF
#define OFFSET_T_MSB__REG		OFFSET_T_REG


#define BMA150_GET_BITSLICE(regvar, bitname)\
			(regvar & bitname##__MSK) >> bitname##__POS

#define BMA150_SET_BITSLICE(regvar, bitname, val)\
		  (regvar & ~bitname##__MSK) | ((val<<bitname##__POS)&bitname##__MSK)  

/** \endcond */


/** Macro to convert floating point low-g-thresholds in G to 8-bit register values.<br>
  * Example: BMA150_LG_THRES_IN_G(0.3, 2.0) generates the register value for 0.3G threshold in 2G mode.
  * \brief convert g-values to 8-bit value
 */
#define BMA150_LG_THRES_IN_G(gthres, range)			((256 * gthres ) / range)

/** Macro to convert floating point high-g-thresholds in G to 8-bit register values.<br>
  * Example: BMA150_HG_THRES_IN_G(1.4, 2.0) generates the register value for 1.4G threshold in 2G mode.
  * \brief convert g-values to 8-bit value
 */
#define BMA150_HG_THRES_IN_G(gthres, range)			((256 * gthres ) / range)

/** Macro to convert floating point low-g-hysteresis in G to 8-bit register values.<br>
  * Example: BMA150_LG_HYST_THRES_IN_G(0.2, 2.0) generates the register value for 0.2G threshold in 2G mode.
  * \brief convert g-values to 8-bit value
 */
#define BMA150_LG_HYST_IN_G(ghyst, range)			((32 * ghyst) / range)

/** Macro to convert floating point high-g-hysteresis in G to 8-bit register values.<br>
  * Example: BMA150_HG_HYST_THRES_IN_G(0.2, 2.0) generates the register value for 0.2G threshold in 2G mode.
  * \brief convert g-values to 8-bit value
 */
#define BMA150_HG_HYST_IN_G(ghyst, range)			((32 * ghyst) / range)

/** Macro to convert floating point G-thresholds to 8-bit register values<br>
  * Example: BMA150_ANY_MOTION_THRES_IN_G(1.2, 2.0) generates the register value for 1.2G threshold in 2G mode.
  * \brief convert g-values to 8-bit value
 */
#define BMA150_ANY_MOTION_THRES_IN_G(gthres, range)	((128 * gthres ) / range)


#define BMA150_CONF1_INT_MSK	((1<<ALERT__POS) | (1<<EN_ANY_MOTION__POS) | (1<<ENABLE_HG__POS) | (1<<ENABLE_LG__POS))
#define BMA150_CONF2_INT_MSK	((1<<ENABLE_ADV_INT__POS) | (1<<NEW_DATA_INT__POS) | (1<<LATCH_INT__POS))



/* Function prototypes */

int bma150_init(bma150_t *);

int bma150_deinit(bma150_t *);

int bma150_soft_reset(void);

int bma150_update_image(void);

int bma150_set_image(IMAGE_T *);

int bma150_get_image(IMAGE_T *);

int bma150_set_offset(unsigned char, unsigned short);

int bma150_get_offset(unsigned char, unsigned short *);

int bma150_set_offset_eeprom(unsigned char, unsigned short);

int bma150_set_ee_w(unsigned char);

int bma150_write_ee(unsigned char, unsigned char);

int bma150_selftest(unsigned char);

int bma150_set_range(unsigned char);

int bma150_get_range(unsigned char *);

int bma150_set_mode(unsigned char);

int bma150_get_mode(unsigned char *);

int bma150_set_bandwidth(unsigned char);

int bma150_get_bandwidth(unsigned char *);

int bma150_set_wake_up_pause(unsigned char);

int bma150_get_wake_up_pause(unsigned char *);

int bma150_set_low_g_threshold(unsigned char);

int bma150_get_low_g_threshold(unsigned char *);

int bma150_set_low_g_countdown(unsigned char);

int bma150_get_low_g_countdown(unsigned char *);

int bma150_set_low_g_duration(unsigned char);

int bma150_get_low_g_duration(unsigned char *);

int bma150_set_low_g_hysteresis(unsigned char);

int bma150_get_low_g_hysteresis(unsigned char *);

int bma150_set_high_g_threshold(unsigned char);

int bma150_get_high_g_threshold(unsigned char *);

int bma150_set_high_g_countdown(unsigned char);

int bma150_get_high_g_countdown(unsigned char *);

int bma150_set_high_g_duration(unsigned char);

int bma150_get_high_g_duration(unsigned char *);

int bma150_set_high_g_hysteresis(unsigned char);

int bma150_get_high_g_hysteresis(unsigned char *);

int bma150_set_any_motion_threshold(unsigned char);

int bma150_get_any_motion_threshold(unsigned char *);

int bma150_set_any_motion_count(unsigned char);

int bma150_get_any_motion_count(unsigned char *);

int bma150_set_interrupt_mask(unsigned char);

int bma150_get_interrupt_mask(unsigned char *);

int bma150_reset_interrupt(void);

int bma150_read_accel_x(short *);

int bma150_read_accel_y(short *);

int bma150_read_accel_z(short *);

int bma150_read_temperature(unsigned char *);

int bma150_read_accel_xyzt(ACC_T *);

int bma150_get_interrupt_status(unsigned char *);

int bma150_set_low_g_int(unsigned char);

int bma150_set_high_g_int(unsigned char);

int bma150_set_any_motion_int(unsigned char);

int bma150_set_alert_int(unsigned char);

int bma150_set_advanced_int(unsigned char);

int bma150_latch_int(unsigned char);

int bma150_set_new_data_int(unsigned char onoff);

int bma150_pause(int);

int bma150_write_reg(unsigned char, unsigned char *, unsigned char);

int bma150_read_reg(unsigned char, unsigned char *, unsigned char);

int bma150_wait_interrupt(unsigned char *mask);


#endif   // __BMA150_H__

