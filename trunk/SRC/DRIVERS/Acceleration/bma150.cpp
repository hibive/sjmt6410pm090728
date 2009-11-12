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


#include <windows.h>
#include "s3c6410_acceleration.h"
#include "iic.h"
#include "bma150.h"


#define MYMSG(x)	RETAILMSG(0, x)
#define MYERR(x)	RETAILMSG(1, x)


static BOOL iic_init(void);
static void iic_deinit(void);
static BOOL iic_write(UCHAR dev_addr, UCHAR reg_addr, UCHAR *data, UCHAR datasize);
static BOOL iic_read(UCHAR dev_addr, UCHAR reg_addr, UCHAR *data, UCHAR datasize);


static bma150_t *p_bma150 = NULL;	/**< pointer to BMA150 device structure  */
HANDLE g_hIIC = INVALID_HANDLE_VALUE;


int bma150_init(bma150_t *pbma150)
{
	unsigned char data;
	if (NULL == pbma150)
		return E_BMA_NULL_PTR;
	if (FALSE == iic_init())
		return E_IIC_INIT_FAIL;

	p_bma150 = pbma150;
	p_bma150->bus_write  = iic_write;
	p_bma150->bus_read   = iic_read;
	p_bma150->delay_msec = Sleep;
	
	data = 0;
	p_bma150->bus_read(BMA150_I2C_ADDR, CHIP_ID__REG, &data, 1);	/* read Chip Id */
	p_bma150->chip_id = BMA150_GET_BITSLICE(data, CHIP_ID);			/* get bitslice */
	data = 0;
	p_bma150->bus_read(BMA150_I2C_ADDR, ML_VERSION__REG, &data, 1); /* read Version reg */
	p_bma150->ml_version = BMA150_GET_BITSLICE(data, ML_VERSION);	/* get ML Version */
	p_bma150->al_version = BMA150_GET_BITSLICE(data, AL_VERSION);	/* get AL Version */
	MYERR((_T("[BMA150] chip_id(%d), ml_version(%d), al_version(%d)\r\n"),
		  p_bma150->chip_id, p_bma150->ml_version, p_bma150->al_version));

	return 0;
}

int bma150_deinit(bma150_t *pbma150)
{
	if (NULL == p_bma150)
		return E_BMA_NULL_PTR;

	return 0;
}


/** Perform soft reset of BMA150 via bus command
*/
int bma150_soft_reset(void)
{
	unsigned char data;
	if (NULL == p_bma150)
		return E_BMA_NULL_PTR;

	data = BMA150_SET_BITSLICE(data, SOFT_RESET, 1);
	p_bma150->bus_write(BMA150_I2C_ADDR, SOFT_RESET__REG, &data, 1);

	return 0;
}

/** call BMA150s update image function
*/
int bma150_update_image(void)
{
	unsigned char data;
	if (NULL == p_bma150)
		return E_BMA_NULL_PTR;

	data = BMA150_SET_BITSLICE(0, UPDATE_IMAGE, 1);
	p_bma150->bus_write(BMA150_I2C_ADDR, UPDATE_IMAGE__REG, &data, 1);

	return 0;
}

/** copy image from image structure to BMA150 image memory
   \param bma150Image Pointer to IMAGE_T
*/
int bma150_set_image(IMAGE_T *bma150Image)
{
	unsigned char data;
	int i=0;
	if (NULL == p_bma150 || NULL == bma150Image)
		return E_BMA_NULL_PTR;

    p_bma150->bus_read(BMA150_I2C_ADDR, EE_W__REG, &data, 1);
	data = BMA150_SET_BITSLICE(data, EE_W, EE_W_UNLOCK);
	p_bma150->bus_write(BMA150_I2C_ADDR, EE_W__REG, &data, 1);

	for (i=0; i<BMA150_IMAGE_LEN; i++)
		p_bma150->bus_write(BMA150_I2C_ADDR, BMA150_IMAGE_BASE+i, (unsigned char *)(bma150Image)+i, 1);

	p_bma150->bus_read(BMA150_I2C_ADDR, EE_W__REG, &data, 1);
	data = BMA150_SET_BITSLICE(data, EE_W, EE_W_LOCK);
	p_bma150->bus_write(BMA150_I2C_ADDR, EE_W__REG, &data, 1);

	return 0;
}

/** read out image from BMA150 and store it to IMAGE_T structure
   \param bma150Image pointer to IMAGE_T 
*/
int bma150_get_image(IMAGE_T *bma150Image)
{
	unsigned char data;
	if (NULL == p_bma150 || NULL == bma150Image)
		return E_BMA_NULL_PTR;

    p_bma150->bus_read(BMA150_I2C_ADDR, EE_W__REG,&data, 1);
	data = BMA150_SET_BITSLICE(data, EE_W, EE_W_UNLOCK);
	p_bma150->bus_write(BMA150_I2C_ADDR, EE_W__REG, &data, 1);
	p_bma150->bus_read(BMA150_I2C_ADDR, BMA150_IMAGE_BASE, (unsigned char *)bma150Image, BMA150_IMAGE_LEN);
	data = BMA150_SET_BITSLICE(data, EE_W, EE_W_LOCK);
	p_bma150->bus_write(BMA150_I2C_ADDR, EE_W__REG, &data, 1);

	return 0;
}

/** write offset data to BMA150 image
   \param xyzt select axis x=0, y=1, z=2, t =3
   \param offset value to write (offset is in offset binary representation
   \note use bma150_set_ee_w() function to enable access to offset registers 
*/
int bma150_set_offset(unsigned char xyzt, unsigned short offset)
{
	unsigned char data;
	if (NULL == p_bma150)
		return E_BMA_NULL_PTR;

	bma150_set_ee_w(EE_W_UNLOCK);
	p_bma150->bus_read(BMA150_I2C_ADDR, (OFFSET_X_LSB__REG+xyzt), &data, 1);
	data = BMA150_SET_BITSLICE(data, OFFSET_X_LSB, offset);
	p_bma150->bus_write(BMA150_I2C_ADDR, (OFFSET_X_LSB__REG+xyzt), &data, 1);
	p_bma150->bus_read(BMA150_I2C_ADDR, (OFFSET_X_MSB__REG+xyzt), &data, 1);
	data = (offset&0x3ff)>>2;
	p_bma150->bus_write(BMA150_I2C_ADDR, (OFFSET_X_MSB__REG+xyzt), &data, 1);
	bma150_set_ee_w(EE_W_LOCK);

	return 0;
}

/** read out offset data from 
   \param xyzt select axis x=0, y=1, z=2, t = 3
   \param *offset pointer to offset value (offset is in offset binary representation
   \note use bma150_set_ee_w() function to enable access to offset registers 
*/
int bma150_get_offset(unsigned char xyzt, unsigned short *offset)
{
	unsigned char data;
	if (NULL == p_bma150 || NULL == offset)
		return E_BMA_NULL_PTR;

	bma150_set_ee_w(EE_W_UNLOCK);
	p_bma150->bus_read(BMA150_I2C_ADDR, (OFFSET_X_LSB__REG+xyzt), &data, 1);
	data = BMA150_GET_BITSLICE(data, OFFSET_X_LSB);
	*offset = data;
	p_bma150->bus_read(BMA150_I2C_ADDR, (OFFSET_X_MSB__REG+xyzt), &data, 1);
	*offset |= (data<<2);
	bma150_set_ee_w(EE_W_LOCK);

	return 0;
}

/** write offset data to BMA150 image
   \param xyzt select axis x=0, y=1, z=2, t = 3
   \param offset value to write to eeprom(offset is in offset binary representation
   \note use bma150_set_ee_w() function to enable access to offset registers in EEPROM space
*/
int bma150_set_offset_eeprom(unsigned char xyzt, unsigned short offset)
{
	unsigned char data;
	if (NULL == p_bma150)
		return E_BMA_NULL_PTR;   

	bma150_set_ee_w(EE_W_UNLOCK);
	p_bma150->bus_read(BMA150_I2C_ADDR, (OFFSET_X_LSB__REG+xyzt), &data, 1);
	data = BMA150_SET_BITSLICE(data, OFFSET_X_LSB, offset);
	p_bma150->bus_write(BMA150_I2C_ADDR, (BMA150_EEP_OFFSET+OFFSET_X_LSB__REG + xyzt), &data, 1);
	p_bma150->delay_msec(34);
	data = (offset&0x3ff)>>2;
	p_bma150->bus_write(BMA150_I2C_ADDR, (BMA150_EEP_OFFSET+ OFFSET_X_MSB__REG+xyzt), &data, 1);
	p_bma150->delay_msec(34);
	bma150_set_ee_w(EE_W_LOCK);

	return 0;
}

/** write offset data to BMA150 image
   \param eew 0 = lock EEPROM 1 = unlock EEPROM 
*/
int bma150_set_ee_w(unsigned char eew)
{
	unsigned char data;
	if (NULL == p_bma150)
		return E_BMA_NULL_PTR;

	p_bma150->bus_read(BMA150_I2C_ADDR, EE_W__REG, &data, 1);
	data = BMA150_SET_BITSLICE(data, EE_W, eew);
	p_bma150->bus_write(BMA150_I2C_ADDR, EE_W__REG, &data, 1);

	return 0;
}

/** write byte to BMA150 EEPROM
   \param addr address to write to
   \param data byte content to write 
*/
int bma150_write_ee(unsigned char addr, unsigned char data)
{
	if (NULL == p_bma150 || 0 == p_bma150->delay_msec)
	    return E_BMA_NULL_PTR;
	if (EEPROM_BEGIN > addr || EEPROM_END < addr)
		return E_INVALID_PARAMETER;

    bma150_set_ee_w(EE_W_UNLOCK);
	bma150_write_reg(addr, &data, 1);
	p_bma150->delay_msec(BMA150_EE_W_DELAY);
	bma150_set_ee_w(EE_W_LOCK);

	return 0;
}

/**	start BMA150s integrated selftest function
   \param st 1 = selftest0, 3 = selftest1 (see also)
     \see BMA150_SELF_TEST0_ON
     \see BMA150_SELF_TEST1_ON
 */
int bma150_selftest(unsigned char st)
{
	unsigned char data;
	if (NULL == p_bma150)
		 return E_BMA_NULL_PTR;

	if (1 == st || 3 == st)
	{
		p_bma150->bus_read(BMA150_I2C_ADDR, SELF_TEST__REG, &data, 1);
		data = BMA150_SET_BITSLICE(data, SELF_TEST, st);
		p_bma150->bus_write(BMA150_I2C_ADDR, SELF_TEST__REG, &data, 1);  
	}
	else
   		return E_INVALID_PARAMETER;
	return 0;
}

/**	set bma150s range 
   \param range 
     \see BMA150_RANGE_2G		
     \see BMA150_RANGE_4G			
     \see BMA150_RANGE_8G			
*/
int bma150_set_range(unsigned char range)
{			
	unsigned char data;
	if (NULL == p_bma150)
		return E_BMA_NULL_PTR;

	if (3 > range && 0 <= range)
	{
		p_bma150->bus_read(BMA150_I2C_ADDR, RANGE__REG, &data, 1);
		data = BMA150_SET_BITSLICE(data, RANGE, range);
		p_bma150->bus_write(BMA150_I2C_ADDR, RANGE__REG, &data, 1);
	}
	else
		return E_INVALID_PARAMETER;
	return 0;
}

/* readout select range from BMA150 
   \param *range pointer to range setting
     \see BMA150_RANGE_2G, BMA150_RANGE_4G, BMA150_RANGE_8G		
     \see bma150_set_range()
*/
int bma150_get_range(unsigned char *range)
{
	if (NULL == p_bma150 || NULL == range)
		return E_BMA_NULL_PTR;

	p_bma150->bus_read(BMA150_I2C_ADDR, RANGE__REG, range, 1);
	*range = BMA150_GET_BITSLICE(*range, RANGE);

	return 0;
}

/** set BMA150s operation mode
   \param mode 0 = normal, 2 = sleep, 3 = auto wake up
   \note Available constants see below
     \see BMA150_MODE_NORMAL, BMA150_MODE_SLEEP, BMA150_MODE_WAKE_UP     
	 \see bma150_get_mode()
*/
int bma150_set_mode(unsigned char mode)
{
	unsigned char data1=0, data2=0;
	if (NULL == p_bma150)
		return E_BMA_NULL_PTR;

	if (4 > mode && 1 != mode && 0 <= mode)
	{
		p_bma150->bus_read(BMA150_I2C_ADDR, WAKE_UP__REG, &data1, 1);
		data1  = BMA150_SET_BITSLICE(data1, WAKE_UP, mode);
        p_bma150->bus_read(BMA150_I2C_ADDR, SLEEP__REG, &data2, 1);
		data2  = BMA150_SET_BITSLICE(data2, SLEEP, (mode>>1));
        p_bma150->bus_write(BMA150_I2C_ADDR, WAKE_UP__REG, &data1, 1);
	 	p_bma150->bus_write(BMA150_I2C_ADDR, SLEEP__REG, &data2, 1);
	 	p_bma150->mode = mode;
	}
	else
		return E_INVALID_PARAMETER;
	return 0;
}

/** get selected mode
   \return used mode
   \note this function returns the mode stored in \ref bma150_t structure
     \see BMA150_MODE_NORMAL, BMA150_MODE_SLEEP, BMA150_MODE_WAKE_UP
     \see bma150_set_mode()
*/
int bma150_get_mode(unsigned char *mode) 
{
	if (NULL == p_bma150 || NULL == mode)
		return E_BMA_NULL_PTR;	

	*mode = p_bma150->mode;

	return 0;	
}

/** set BMA150 internal filter bandwidth
   \param bw bandwidth (see bandwidth constants)
     \see #define BMA150_BW_25HZ, BMA150_BW_50HZ, BMA150_BW_100HZ, BMA150_BW_190HZ, BMA150_BW_375HZ, BMA150_BW_750HZ, BMA150_BW_1500HZ
     \see bma150_get_bandwidth()
*/
int bma150_set_bandwidth(unsigned char bw)
{
	unsigned char data;
	if (NULL == p_bma150)
		return E_BMA_NULL_PTR;

	if (6 >= bw && 0 <= bw)
	{
		p_bma150->bus_read(BMA150_I2C_ADDR, RANGE__REG, &data, 1);
		data = BMA150_SET_BITSLICE(data, BANDWIDTH, bw);
		p_bma150->bus_write(BMA150_I2C_ADDR, RANGE__REG, &data, 1);
	}
	else
		return E_INVALID_PARAMETER;
    return 0;
}

/** read selected bandwidth from BMA150 
   \param *bw pointer to bandwidth return value
     \see #define BMA150_BW_25HZ, BMA150_BW_50HZ, BMA150_BW_100HZ, BMA150_BW_190HZ, BMA150_BW_375HZ, BMA150_BW_750HZ, BMA150_BW_1500HZ
     \see bma150_set_bandwidth()
*/
int bma150_get_bandwidth(unsigned char *bw)
{
	if (NULL == p_bma150 || NULL == bw)
		return E_BMA_NULL_PTR;

	p_bma150->bus_read(BMA150_I2C_ADDR, BANDWIDTH__REG, bw, 1);
	*bw = BMA150_GET_BITSLICE(*bw, BANDWIDTH);

	return 0;
}

/** set BMA150 auto wake up pause
   \param wup wake_up_pause parameters
     \see BMA150_WAKE_UP_PAUSE_20MS, BMA150_WAKE_UP_PAUSE_80MS, BMA150_WAKE_UP_PAUSE_320MS, BMA150_WAKE_UP_PAUSE_2560MS
     \see bma150_get_wake_up_pause()
*/
int bma150_set_wake_up_pause(unsigned char wup)
{
	unsigned char data;
	if (NULL == p_bma150)
		return E_BMA_NULL_PTR;

	if (0 <= wup && 3 >= wup)
	{
		p_bma150->bus_read(BMA150_I2C_ADDR, WAKE_UP_PAUSE__REG, &data, 1);
		data = BMA150_SET_BITSLICE(data, WAKE_UP_PAUSE, wup);
		p_bma150->bus_write(BMA150_I2C_ADDR, WAKE_UP_PAUSE__REG, &data, 1);
	}
	else
		return E_INVALID_PARAMETER;
	return 0;
}

/** read BMA150 auto wake up pause from image
   \param *wup wake up pause read back pointer
     \see BMA150_WAKE_UP_PAUSE_20MS, BMA150_WAKE_UP_PAUSE_80MS, BMA150_WAKE_UP_PAUSE_320MS, BMA150_WAKE_UP_PAUSE_2560MS
     \see bma150_set_wake_up_pause()
*/
int bma150_get_wake_up_pause(unsigned char *wup)
{
	unsigned char data;
	if (NULL == p_bma150 || NULL == wup)
		return E_BMA_NULL_PTR;

	p_bma150->bus_read(BMA150_I2C_ADDR, WAKE_UP_PAUSE__REG, &data, 1);
	*wup = BMA150_GET_BITSLICE(data, WAKE_UP_PAUSE);

	return 0;
}


/* Thresholds and Interrupt Configuration */

/** set low-g interrupt threshold
   \param th set the threshold
   \note the threshold depends on configured range. A macro \ref BMA150_LG_THRES_IN_G() for range to register value conversion is available.
     \see BMA150_LG_THRES_IN_G()   
     \see bma150_get_low_g_threshold()
*/
int bma150_set_low_g_threshold(unsigned char th)
{
	if (NULL == p_bma150)
		return E_BMA_NULL_PTR;

	p_bma150->bus_write(BMA150_I2C_ADDR, LG_THRES__REG, &th, 1);

	return 0;
}

/** get low-g interrupt threshold
   \param *th get the threshold  value from sensor image
     \see bma150_set_low_g_threshold()
*/
int bma150_get_low_g_threshold(unsigned char *th)
{
	if (NULL == p_bma150 || NULL == th)
		return E_BMA_NULL_PTR;

	p_bma150->bus_read(BMA150_I2C_ADDR, LG_THRES__REG, th, 1);

	return 0;
}

/** set low-g interrupt countdown
   \param cnt get the countdown value from sensor image
     \see bma150_get_low_g_countdown()
*/
int bma150_set_low_g_countdown(unsigned char cnt)
{
	unsigned char data;
	if (NULL == p_bma150)
		return E_BMA_NULL_PTR;

	if (0 <= cnt && 3 >= cnt)
	{
		p_bma150->bus_read(BMA150_I2C_ADDR, COUNTER_LG__REG, &data, 1);
		data = BMA150_SET_BITSLICE(data, COUNTER_LG, cnt);
		p_bma150->bus_write(BMA150_I2C_ADDR, COUNTER_LG__REG, &data, 1);
	}
	else
		return E_INVALID_PARAMETER;
	return 0;
}

/** get low-g interrupt countdown
   \param cnt get the countdown  value from sensor image
     \see bma150_set_low_g_countdown()
*/
int bma150_get_low_g_countdown(unsigned char *cnt)
{
	unsigned char data;
	if (NULL == p_bma150 || NULL == cnt)
		return E_BMA_NULL_PTR;

	p_bma150->bus_read(BMA150_I2C_ADDR, COUNTER_LG__REG, &data, 1);
	*cnt = BMA150_GET_BITSLICE(data, COUNTER_LG);

	return 0;
}

/** configure low-g duration value
   \param dur low-g duration in miliseconds
     \see bma150_get_low_g_duration(), bma150_get_high_g_duration(), bma150_set_high_g_duration()
*/
int bma150_set_low_g_duration(unsigned char dur)
{
	if (NULL == p_bma150)
		return E_BMA_NULL_PTR;

	p_bma150->bus_write(BMA150_I2C_ADDR, LG_DUR__REG, &dur, 1);

	return 0;
}

/** read out low-g duration value from sensor image
   \param dur low-g duration in miliseconds
     \see bma150_set_low_g_duration(), bma150_get_high_g_duration(), bma150_set_high_g_duration()
*/
int bma150_get_low_g_duration(unsigned char *dur)
{
	if (NULL == p_bma150 || NULL == dur)
		return E_BMA_NULL_PTR;

	p_bma150->bus_read(BMA150_I2C_ADDR, LG_DUR__REG, dur, 1);

	return 0;
}

/** set low-g hystersis
   \param hysc set the hystersis value to sensor image
     \see bma150_get_low_g_hysteresis()
*/
int bma150_set_low_g_hysteresis(unsigned char hysc)
{
	if (NULL == p_bma150)
		return E_BMA_NULL_PTR;

	p_bma150->bus_write(BMA150_I2C_ADDR, LG_HYST__REG, &hysc, 1);

	return 0;
}

/** get low-g hystersis
   \param *hysc get the hystersis value from sensor image
   \see bma150_set_low_g_hysteresis()
*/
int bma150_get_low_g_hysteresis(unsigned char *hysc)
{
	if (NULL == p_bma150 || NULL == hysc)
		return E_BMA_NULL_PTR;

	p_bma150->bus_read(BMA150_I2C_ADDR, LG_HYST__REG, hysc, 1);

	return 0;
}

/** set low-g interrupt threshold
   \param th set the threshold
   \note the threshold depends on configured range. A macro \ref BMA150_HG_THRES_IN_G() for range to register value conversion is available.
     \see BMA150_HG_THRES_IN_G()   
     \see bma150_get_high_g_threshold()
*/
int bma150_set_high_g_threshold(unsigned char th)
{
	if (NULL == p_bma150)
		return E_BMA_NULL_PTR;

	p_bma150->bus_write(BMA150_I2C_ADDR, HG_THRES__REG, &th, 1);

	return 0;
}

/** get high-g interrupt threshold
   \param *th get the threshold  value from sensor image
     \see bma150_set_high_g_threshold()
*/
int bma150_get_high_g_threshold(unsigned char *th)
{
	if (NULL == p_bma150 || NULL == th)
		return E_BMA_NULL_PTR;

	p_bma150->bus_read(BMA150_I2C_ADDR, HG_THRES__REG, th, 1);

	return 0;
}

/** set high-g interrupt countdown
   \param cnt get the countdown value from sensor image
     \see bma150_get_high_g_countdown()
*/
int bma150_set_high_g_countdown(unsigned char cnt)
{
	unsigned char data;
	if (NULL == p_bma150)
		return E_BMA_NULL_PTR;

	if (0 <= cnt && 3 >= cnt)
	{
		p_bma150->bus_read(BMA150_I2C_ADDR, COUNTER_HG__REG, &data, 1);
		data = BMA150_SET_BITSLICE(data, COUNTER_HG, cnt);
		p_bma150->bus_write(BMA150_I2C_ADDR, COUNTER_HG__REG, &data, 1);
	}
	else
		return E_INVALID_PARAMETER;
	return 0;
}

/** get high-g interrupt countdown
   \param cnt get the countdown  value from sensor image
     \see bma150_set_high_g_countdown()
*/
int bma150_get_high_g_countdown(unsigned char *cnt)
{
	unsigned char data;
	if (NULL == p_bma150 || NULL == cnt)
		return E_BMA_NULL_PTR;

	p_bma150->bus_read(BMA150_I2C_ADDR, COUNTER_HG__REG, &data, 1);
	*cnt = BMA150_GET_BITSLICE(data, COUNTER_HG);

	return 0;
}

/** configure high-g duration value
   \param dur high-g duration in miliseconds
     \see  bma150_get_high_g_duration(), bma150_set_low_g_duration(), bma150_get_low_g_duration()
*/
int bma150_set_high_g_duration(unsigned char dur)
{
	if (NULL == p_bma150)
		return E_BMA_NULL_PTR;

	p_bma150->bus_write(BMA150_I2C_ADDR, HG_DUR__REG, &dur, 1);

	return 0;
}

/** read out high-g duration value from sensor image
   \param dur high-g duration in miliseconds
     \see  bma150_set_high_g_duration(), bma150_get_low_g_duration(), bma150_set_low_g_duration(),
*/
int bma150_get_high_g_duration(unsigned char *dur)
{
	if (NULL == p_bma150 || NULL == dur)
		return E_BMA_NULL_PTR;

	p_bma150->bus_read(BMA150_I2C_ADDR, HG_DUR__REG, dur, 1);

	return 0;
}

/** set high-g hystersis
   \param hysc set the hystersis value to sensor image
     \see bma150_get_high_g_hysteresis()
*/
int bma150_set_high_g_hysteresis(unsigned char hysc)
{
	if (NULL == p_bma150)
		return E_BMA_NULL_PTR;

	p_bma150->bus_write(BMA150_I2C_ADDR, HG_HYST__REG, &hysc, 1);

	return 0;
}

/** get high-g hystersis
   \param *hysc get the hystersis value from sensor image
     \see bma150_set_high_g_hysteresis()
*/
int bma150_get_high_g_hysteresis(unsigned char *hysc)
{
	if (NULL == p_bma150 || NULL == hysc)
		return E_BMA_NULL_PTR;

	p_bma150->bus_read(BMA150_I2C_ADDR, HG_HYST__REG, hysc, 1);

	return 0;
}

/**  set threshold value for any_motion feature
   \param th set the threshold a macro \ref BMA150_ANY_MOTION_THRES_IN_G()  is available for that
     \see BMA150_ANY_MOTION_THRES_IN_G()
*/
int bma150_set_any_motion_threshold(unsigned char th)
{
	if (NULL == p_bma150)
		return E_BMA_NULL_PTR;

	p_bma150->bus_write(BMA150_I2C_ADDR, ANY_MOTION_THRES__REG, &th, 1);

	return 0;
}

/**  get threshold value for any_motion feature
   \param *th read back any_motion threshold from image register
     \see BMA150_ANY_MOTION_THRES_IN_G()
*/
int bma150_get_any_motion_threshold(unsigned char *th)
{
	if (NULL == p_bma150 || NULL == th)
		return E_BMA_NULL_PTR;

	p_bma150->bus_read(BMA150_I2C_ADDR, ANY_MOTION_THRES__REG, th, 1);

	return 0;
}

/**  set counter value for any_motion feature
   \param amc set the counter value, constants are available for that
     \see BMA150_ANY_MOTION_DUR_1, BMA150_ANY_MOTION_DUR_3, BMA150_ANY_MOTION_DUR_5, BMA150_ANY_MOTION_DUR_7
*/
int bma150_set_any_motion_count(unsigned char amc)
{
	unsigned char data;
	if (NULL == p_bma150)
		return E_BMA_NULL_PTR;

	if (0 <= amc && 3 >= amc)
	{
	 	p_bma150->bus_read(BMA150_I2C_ADDR, ANY_MOTION_DUR__REG, &data, 1);
		data = BMA150_SET_BITSLICE(data, ANY_MOTION_DUR, amc);
		p_bma150->bus_write(BMA150_I2C_ADDR, ANY_MOTION_DUR__REG, &data, 1);
	}
	else
		return E_INVALID_PARAMETER;
	return 0;
}

/**  get counter value for any_motion feature from image register
   \param *amc readback pointer for counter value
     \see BMA150_ANY_MOTION_DUR_1, BMA150_ANY_MOTION_DUR_3, BMA150_ANY_MOTION_DUR_5, BMA150_ANY_MOTION_DUR_7
*/
int bma150_get_any_motion_count(unsigned char *amc)
{
	unsigned char data;
	if (NULL == p_bma150 || NULL == amc)
		return E_BMA_NULL_PTR;

	p_bma150->bus_read(BMA150_I2C_ADDR, ANY_MOTION_DUR__REG, &data, 1);
	*amc = BMA150_GET_BITSLICE(data, ANY_MOTION_DUR);

	return 0;
}

/** set the interrupt mask for BMA150's interrupt features in one mask
   \param mask input for interrupt mask
     \see BMA150_INT_ALERT, BMA150_INT_ANY_MOTION, BMA150_INT_EN_ADV_INT, BMA150_INT_NEW_DATA, BMA150_INT_LATCH, BMA150_INT_HG, BMA150_INT_LG
*/
int bma150_set_interrupt_mask(unsigned char mask)
{
	unsigned char data[4]={0,};
	if (NULL == p_bma150)
		return E_BMA_NULL_PTR;

	data[0] = mask & BMA150_CONF1_INT_MSK;
	data[2] = ((mask<<1) & BMA150_CONF2_INT_MSK);
	p_bma150->bus_read(BMA150_I2C_ADDR, BMA150_CONF1_REG, &data[1], 1);
	p_bma150->bus_read(BMA150_I2C_ADDR, BMA150_CONF2_REG, &data[3], 1);
	data[1] &= (~BMA150_CONF1_INT_MSK);
	data[1] |= data[0];
	data[3] &=(~(BMA150_CONF2_INT_MSK));
	data[3] |= data[2];
	p_bma150->bus_write(BMA150_I2C_ADDR, BMA150_CONF1_REG, &data[1], 1);
	p_bma150->bus_write(BMA150_I2C_ADDR, BMA150_CONF2_REG, &data[3], 1);

	return 0;
}

/** get the current interrupt mask settings from BMA150 image registers
   \param *mask return variable pointer for interrupt mask
     \see BMA150_INT_ALERT, BMA150_INT_ANY_MOTION, BMA150_INT_EN_ADV_INT, BMA150_INT_NEW_DATA, BMA150_INT_LATCH, BMA150_INT_HG, BMA150_INT_LG
*/
int bma150_get_interrupt_mask(unsigned char *mask)
{
	unsigned char data;
	if (NULL == p_bma150 || NULL == mask)
		return E_BMA_NULL_PTR;

	p_bma150->bus_read(BMA150_I2C_ADDR, BMA150_CONF1_REG, &data, 1);
	*mask = data & BMA150_CONF1_INT_MSK;
	data = 0;
	p_bma150->bus_read(BMA150_I2C_ADDR, BMA150_CONF2_REG, &data, 1);
	*mask = *mask | ((data & BMA150_CONF2_INT_MSK)>>1);

	return 0;
}

/** resets the BMA150 interrupt status 
   \note this feature can be used to reset a latched interrupt
*/
int bma150_reset_interrupt(void)
{
	unsigned char data=(1<<RESET_INT__POS);
	if (NULL == p_bma150)
		return E_BMA_NULL_PTR;

	p_bma150->bus_write(BMA150_I2C_ADDR, RESET_INT__REG, &data, 1);

	return 0;
}


/* Data Readout */

/** X-axis acceleration data readout
   \param *a_x pointer for 16 bit 2's complement data output (LSB aligned)
*/
int bma150_read_accel_x(short *a_x)
{
	unsigned char data[2]={0,};
	if (NULL == p_bma150 || NULL == a_x)
		return E_BMA_NULL_PTR;

	p_bma150->bus_read(BMA150_I2C_ADDR, ACC_X_LSB__REG, data, 2);
	*a_x = BMA150_GET_BITSLICE(data[0],ACC_X_LSB) | BMA150_GET_BITSLICE(data[1],ACC_X_MSB)<<ACC_X_LSB__LEN;
	*a_x = *a_x << (sizeof(short)*8-(ACC_X_LSB__LEN+ACC_X_MSB__LEN));
	*a_x = *a_x >> (sizeof(short)*8-(ACC_X_LSB__LEN+ACC_X_MSB__LEN));

	return 0;
}

/** Y-axis acceleration data readout
   \param *a_y pointer for 16 bit 2's complement data output (LSB aligned)
*/
int bma150_read_accel_y(short *a_y)
{
	unsigned char data[2]={0,};
	if (NULL == p_bma150 || NULL == a_y)
		return E_BMA_NULL_PTR;

	p_bma150->bus_read(BMA150_I2C_ADDR, ACC_Y_LSB__REG, data, 2);
	*a_y = BMA150_GET_BITSLICE(data[0],ACC_Y_LSB) | BMA150_GET_BITSLICE(data[1],ACC_Y_MSB)<<ACC_Y_LSB__LEN;
	*a_y = *a_y << (sizeof(short)*8-(ACC_Y_LSB__LEN+ACC_Y_MSB__LEN));
	*a_y = *a_y >> (sizeof(short)*8-(ACC_Y_LSB__LEN+ACC_Y_MSB__LEN));

	return 0;
}

/** Z-axis acceleration data readout
	\param *a_z pointer for 16 bit 2's complement data output (LSB aligned)
*/
int bma150_read_accel_z(short *a_z)
{
	unsigned char data[2]={0,};
	if (NULL == p_bma150 || NULL == a_z)
		return E_BMA_NULL_PTR;

	p_bma150->bus_read(BMA150_I2C_ADDR, ACC_Z_LSB__REG, data, 2);
	*a_z = BMA150_GET_BITSLICE(data[0],ACC_Z_LSB) | BMA150_GET_BITSLICE(data[1],ACC_Z_MSB)<<ACC_Z_LSB__LEN;
	*a_z = *a_z << (sizeof(short)*8-(ACC_Z_LSB__LEN+ACC_Z_MSB__LEN));
	*a_z = *a_z >> (sizeof(short)*8-(ACC_Z_LSB__LEN+ACC_Z_MSB__LEN));

	return 0;
}

/** 8 bit temperature data readout
   \param *temp pointer for 8 bit temperature output (offset binary)
   \note: an output of 0 equals -30°C, 1 LSB equals 0.5°C
*/
int bma150_read_temperature(unsigned char *temp)
{
	if (NULL == p_bma150 || NULL == temp)
		return E_BMA_NULL_PTR;

	p_bma150->bus_read(BMA150_I2C_ADDR, TEMPERATURE__REG, temp, 1);

	return 0;
}

/** X,Y and Z-axis acceleration data readout
   \param *acc pointer to \ref ACC_T structure for x,y,z,t data readout
   \note data will be read by multi-byte protocol into a 7 byte structure
*/
int bma150_read_accel_xyzt(ACC_T * acc)
{
	unsigned char data[7]={0,};
	if (NULL == p_bma150 || NULL == acc)
		return E_BMA_NULL_PTR;

	p_bma150->bus_read(BMA150_I2C_ADDR, ACC_X_LSB__REG, data, 7);
	acc->x = BMA150_GET_BITSLICE(data[0],ACC_X_LSB) | BMA150_GET_BITSLICE(data[1],ACC_X_MSB)<<ACC_X_LSB__LEN;
	acc->x = acc->x << (sizeof(short)*8-(ACC_X_LSB__LEN+ACC_X_MSB__LEN));
	acc->x = acc->x >> (sizeof(short)*8-(ACC_X_LSB__LEN+ACC_X_MSB__LEN));
	acc->y = BMA150_GET_BITSLICE(data[2],ACC_Y_LSB) | BMA150_GET_BITSLICE(data[3],ACC_Y_MSB)<<ACC_Y_LSB__LEN;
	acc->y = acc->y << (sizeof(short)*8-(ACC_Y_LSB__LEN + ACC_Y_MSB__LEN));
	acc->y = acc->y >> (sizeof(short)*8-(ACC_Y_LSB__LEN + ACC_Y_MSB__LEN));
	acc->z = BMA150_GET_BITSLICE(data[4],ACC_Z_LSB) | BMA150_GET_BITSLICE(data[5],ACC_Z_MSB)<<ACC_Z_LSB__LEN;
	acc->z = acc->z << (sizeof(short)*8-(ACC_Z_LSB__LEN+ACC_Z_MSB__LEN));
	acc->z = acc->z >> (sizeof(short)*8-(ACC_Z_LSB__LEN+ACC_Z_MSB__LEN));
	acc->t = data[6];

	return 0;
}

/** check current interrupt status from interrupt status register in BMA150 image register
   \param *ist pointer to interrupt status byte
     \see BMA150_INT_STATUS_HG, BMA150_INT_STATUS_LG, BMA150_INT_STATUS_HG_LATCHED, BMA150_INT_STATUS_LG_LATCHED, BMA150_INT_STATUS_ALERT, BMA150_INT_STATUS_ST_RESULT
*/
int bma150_get_interrupt_status(unsigned char *ist)
{
	if (NULL == p_bma150 || NULL == ist)
		return E_BMA_NULL_PTR;

	p_bma150->bus_read(BMA150_I2C_ADDR, BMA150_STATUS_REG, ist, 1);

	return 0;
}

/** enable/ disable low-g interrupt feature
   \param onoff enable=1, disable=0
*/
int bma150_set_low_g_int(unsigned char onoff)
{
	unsigned char data;
	if (NULL == p_bma150)
		return E_BMA_NULL_PTR;

	if (0 <= onoff && 1 >= onoff)
	{
		p_bma150->bus_read(BMA150_I2C_ADDR, ENABLE_LG__REG, &data, 1);
		data = BMA150_SET_BITSLICE(data, ENABLE_LG, onoff);
		p_bma150->bus_write(BMA150_I2C_ADDR, ENABLE_LG__REG, &data, 1);
	}
	else
		return E_INVALID_PARAMETER;
	return 0;
}

/** enable/ disable high-g interrupt feature
   \param onoff enable=1, disable=0
*/
int bma150_set_high_g_int(unsigned char onoff)
{
	unsigned char data;
	if (NULL == p_bma150)
		return E_BMA_NULL_PTR;

	if (0 <= onoff && 1 >= onoff)
	{
		p_bma150->bus_read(BMA150_I2C_ADDR, ENABLE_HG__REG, &data, 1);
		data = BMA150_SET_BITSLICE(data, ENABLE_HG, onoff);
		p_bma150->bus_write(BMA150_I2C_ADDR, ENABLE_HG__REG, &data, 1);
	}
	else
		return E_INVALID_PARAMETER;
	return 0;
}

/** enable/ disable any_motion interrupt feature
   \param onoff enable=1, disable=0
   \note for any_motion interrupt feature usage see also \ref bma150_set_advanced_int()
*/
int bma150_set_any_motion_int(unsigned char onoff)
{
	unsigned char data;
	if (NULL == p_bma150)
		return E_BMA_NULL_PTR;

	if (0 <= onoff && 1 >= onoff)
	{
		p_bma150->bus_read(BMA150_I2C_ADDR, EN_ANY_MOTION__REG, &data, 1);
		data = BMA150_SET_BITSLICE(data, EN_ANY_MOTION, onoff);
		p_bma150->bus_write(BMA150_I2C_ADDR, EN_ANY_MOTION__REG, &data, 1);
	}
	else
		return E_INVALID_PARAMETER;
	return 0;
}

/** enable/ disable alert-int interrupt feature
   \param onoff enable=1, disable=0
   \note for any_motion interrupt feature usage see also \ref bma150_set_advanced_int()
*/
int bma150_set_alert_int(unsigned char onoff)
{
	unsigned char data;
	if (NULL == p_bma150) 
		return E_BMA_NULL_PTR;

	if (0 <= onoff && 1 >= onoff)
	{
		p_bma150->bus_read(BMA150_I2C_ADDR, ALERT__REG, &data, 1);
		data = BMA150_SET_BITSLICE(data, ALERT, onoff);
		p_bma150->bus_write(BMA150_I2C_ADDR, ALERT__REG, &data, 1);
	}
	else
		return E_INVALID_PARAMETER;
	return 0;
}

/** enable/ disable advanced interrupt feature
   \param onoff enable=1, disable=0
     \see bma150_set_any_motion_int()
     \see bma150_set_alert_int()
*/
int bma150_set_advanced_int(unsigned char onoff)
{
	unsigned char data;
	if (NULL == p_bma150)
		return E_BMA_NULL_PTR;

	if (0 <= onoff && 1 >= onoff)
	{
		p_bma150->bus_read(BMA150_I2C_ADDR, ENABLE_ADV_INT__REG, &data, 1);
		data = BMA150_SET_BITSLICE(data, EN_ANY_MOTION, onoff);
		p_bma150->bus_write(BMA150_I2C_ADDR, ENABLE_ADV_INT__REG, &data, 1);
	}
	else
		return E_INVALID_PARAMETER;
	return 0;
}

/** enable/disable latched interrupt for all interrupt feature (global option)
   \param latched (=1 for latched interrupts), (=0 for unlatched interrupts)
*/
int bma150_latch_int(unsigned char latched)
{
	unsigned char data;
	if (NULL == p_bma150)
		return E_BMA_NULL_PTR;

	if (0 <= latched && 1 >= latched)
	{
		p_bma150->bus_read(BMA150_I2C_ADDR, LATCH_INT__REG, &data, 1);
		data = BMA150_SET_BITSLICE(data, LATCH_INT, latched);
		p_bma150->bus_write(BMA150_I2C_ADDR, LATCH_INT__REG, &data, 1);
	}
	else
		return E_INVALID_PARAMETER;
	return 0;
}

/** enable/ disable new data interrupt feature
   \param onoff enable=1, disable=0
*/
int bma150_set_new_data_int(unsigned char onoff)
{
	unsigned char data;
	if (NULL == p_bma150) 
		return E_BMA_NULL_PTR;

	if (0 <= onoff && 1 >= onoff)
	{
		p_bma150->bus_read(BMA150_I2C_ADDR, NEW_DATA_INT__REG, &data, 1);
		data = BMA150_SET_BITSLICE(data, NEW_DATA_INT, onoff);
		p_bma150->bus_write(BMA150_I2C_ADDR, NEW_DATA_INT__REG, &data, 1);
	}
	else
		return E_INVALID_PARAMETER;
	return 0;
}


/* MISC functions */

/** calls the linked wait function
   \param msec amount of mili seconds to pause
*/
int bma150_pause(int msec)
{
	if (p_bma150==NULL || p_bma150->delay_msec == NULL)
		return E_BMA_NULL_PTR;
	else if (msec <= 0)
		return E_INVALID_PARAMETER;
	else
	  	p_bma150->delay_msec(msec);

	return 0;
}

/** write function for raw register access
   \param addr register address
   \param *data pointer to data array for register write
   \param len number of bytes to be written starting from addr	
*/
int bma150_write_reg(unsigned char addr, unsigned char *data, unsigned char len)
{
	if (NULL == p_bma150)
		return E_BMA_NULL_PTR;
	if (0 >= len || 0 > addr)
		return E_INVALID_PARAMETER;

	p_bma150->bus_write(BMA150_I2C_ADDR, addr, data, len);

	return 0;
}

/** read function for raw register access
   \param addr register address
   \param *data pointer to data array for register read back
   \param len number of bytes to be read starting from addr
*/
int bma150_read_reg(unsigned char addr, unsigned char *data, unsigned char len)
{
	if (NULL == p_bma150 || NULL == data)
		return E_BMA_NULL_PTR;
	if (0 >= len || 0 > addr)
		return E_INVALID_PARAMETER;

	p_bma150->bus_read(BMA150_I2C_ADDR, addr, data, len);

	return 0;
}

/** wait for interrupt from BMA150 chip
   \param mask input for interrupt mask
     \see BMA150_INT_ALERT, BMA150_INT_ANY_MOTION, BMA150_INT_EN_ADV_INT, BMA150_INT_NEW_DATA, BMA150_INT_LATCH, BMA150_INT_HG, BMA150_INT_LG
*/
int bma150_wait_interrupt(unsigned char *mask) 
{
#if	0
	DWORD waitreturn=0;
	if (NULL == mask)
		return E_INVALID_PARAMETER;

    waitreturn = WaitForSingleObject(p_bma150->SMBWaitInterruptEvent, 500);
	if (waitreturn == WAIT_OBJECT_0)
	{
#ifdef DEBUG
		bma150_get_interrupt_status(mask);
#endif
		*mask |= 0x40;
#ifdef DEBUG
		printf("get one interrupt in bma150 driver, pending status is %x\r\n", *mask);
		unsigned char intmask = 0;
		bma150_get_interrupt_mask(&intmask);
		printf("get one interrupt in bma150 driver, mask is %x\r\n", intmask);
#endif
		ResetEvent(p_bma150->SMBWaitInterruptEvent);
	}
	else if (waitreturn == WAIT_TIMEOUT)
	{
		*mask = 0;
#ifdef DEBUG
		printf("wait interrupt timeout in bma150 driver\r\n");
#endif
	}
	else
	{
		printf("wait for interrupt unexpected error!\r\n");
		*mask = 0;
	}

	//add identify interrupt source here
	//end
#endif
	return 0;
}



static BOOL iic_init(void)
{
	UINT32 IICClock, uiIICDelay, bytes;
	BOOL bRet;

	g_hIIC = CreateFile(_T("IIC0:"),
						GENERIC_READ|GENERIC_WRITE,
						FILE_SHARE_READ|FILE_SHARE_WRITE,
						NULL, OPEN_EXISTING, 0, 0);
	if (INVALID_HANDLE_VALUE == g_hIIC)
	{
		MYERR((_T("[BMA150] ERROR - CreateFile(%d)\r\n"), GetLastError()));
		goto iic_CleanUp;
	}

	IICClock = 2000000;	// Max 3.4 MHz
	bRet = DeviceIoControl(g_hIIC, IOCTL_IIC_SET_CLOCK,
		&IICClock, sizeof(UINT32), NULL, 0, (LPDWORD)&bytes, NULL);
	if (FALSE == bRet)
	{
		MYERR((_T("[BMA150] ERROR - DeviceIoControl(IOCTL_IIC_SET_CLOCK)\r\n")));
		goto iic_CleanUp;
	}

	uiIICDelay = Clk_0;
	bRet = DeviceIoControl(g_hIIC, IOCTL_IIC_SET_DELAY,
		&uiIICDelay, sizeof(UINT32), NULL, 0, (LPDWORD)&bytes, NULL);
	if (FALSE == bRet)
	{
		MYERR((_T("[BMA150] ERROR - DeviceIoControl(IOCTL_IIC_SET_DELAY)\r\n")));
		goto iic_CleanUp;
	}

	return TRUE;

iic_CleanUp:
	iic_deinit();

	return FALSE;
}
static void iic_deinit(void)
{
	if (INVALID_HANDLE_VALUE != g_hIIC)
	{
		CloseHandle(g_hIIC);
		g_hIIC = INVALID_HANDLE_VALUE;
	}
}
static BOOL iic_write(UCHAR dev_addr, UCHAR reg_addr, UCHAR *data, UCHAR datasize)
{
	static UCHAR Buf[256];
	IIC_IO_DESC IIC_Data;
	BOOL bRet;

	if (INVALID_HANDLE_VALUE == g_hIIC || NULL == data || 0 == datasize)
	{
		MYERR((_T("[BMA150] ERROR - iic_write()\r\n")));
		return FALSE;
	}

	Buf[0] = reg_addr;
	memcpy(&Buf[1], data, datasize);

	IIC_Data.SlaveAddress = dev_addr;
	IIC_Data.Data = Buf;
	IIC_Data.Count = 1 + datasize;

	bRet = DeviceIoControl(g_hIIC, IOCTL_IIC_WRITE,
			&IIC_Data, sizeof(IIC_IO_DESC), NULL, 0, NULL, NULL);
	if (FALSE == bRet)
		MYERR((_T("[BMA150] ERROR - IOCTL_IIC_WRITE(%d, %d)\r\n"), reg_addr, data[0]));

	return bRet;
}
static BOOL iic_read(UCHAR dev_addr, UCHAR reg_addr, UCHAR *data, UCHAR datasize)
{
	IIC_IO_DESC IIC_AddressData, IIC_Data;
	DWORD bytes;
	BOOL bRet;

	if (INVALID_HANDLE_VALUE == g_hIIC || NULL == data || 0 == datasize)
	{
		MYERR((_T("[BMA150] ERROR - iic_read()\r\n")));
		return 0;
	}

	IIC_AddressData.SlaveAddress = dev_addr;
	IIC_AddressData.Data = &reg_addr;
	IIC_AddressData.Count = 1;

	IIC_Data.SlaveAddress = dev_addr;
	IIC_Data.Data = data;
	IIC_Data.Count = datasize;

	bRet = DeviceIoControl(g_hIIC,  IOCTL_IIC_READ,
			&IIC_AddressData, sizeof(IIC_IO_DESC),
			&IIC_Data, sizeof(IIC_IO_DESC),
			&bytes, NULL);
	if (FALSE == bRet)
		MYERR((_T("[BMA150] ERROR - IOCTL_IIC_READ(%d, %d)\r\n"), reg_addr, data[0]));

	return bRet;
}

