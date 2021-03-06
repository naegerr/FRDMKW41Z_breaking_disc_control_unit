/*
 * Accel_Altitude_sensor.h
 *
 *  Created on: 30.12.2017
 *      Author: Robert Näger
 */

#ifndef COMMON_ACCEL_ALTITUDE_SENSOR_H_
#define COMMON_ACCEL_ALTITUDE_SENSOR_H_

#include "fsl_common.h"
#include "Accel_Altitude_sensor.h"

/************************************************************************************
*************************************************************************************
* Public Declarations
*************************************************************************************
************************************************************************************/
// Altitude value
typedef struct altitude_value_t
{
	int16_t		altitudeInteger;		// Altitude in [m] / MSBit gives sign: 1 -> negative
	uint8_t		altitudeFraction;		// Altitude fractions in [m] only Bit 4-7 used!
										/* from datasheet MPL3115 attachement
										 * 0b1000 = 0.5 m
										 * 0b0100 = 0.25 m
										 * 0b0010 = 0.125 m
										 * 0b0001 = 0.0625 m
										 */
} altitude_value;

// 3D acceleration values
typedef struct accel_values_t
{
	int16_t	x;
	int16_t	y;
	int16_t	z;
}accel_values;

/*! *********************************************************************************
* \brief  BOARD_I2C_ReleaseBus
*
* \remarks	Sends start- and stop signals to I2C bus
********************************************************************************** */
 void BOARD_I2C_ReleaseBus(void);
 /*! *********************************************************************************
 * \brief  I2C_getAccel
 * \remarks		Gets the accelsensor values x,y,z from MMA8452 according to settings of config
 * 				uses polling and waites until data is ready.
 * 				Further information in datasheet MMA8452
 ********************************************************************************** */
 accel_values I2C_getAccel();
 /*! *********************************************************************************
 * \brief  I2C_getAltitude
 * \remarks		Gets the altitude from MPL3115 according to settings of config
 * 				uses polling and waites until data is ready.
 * 				Further information in datashset MPL3115
 ********************************************************************************** */
 altitude_value I2C_getAltitude();
 /*! *********************************************************************************
 * \brief  I2C_Accel_Config
 * \remarks		Writes settings to I2C Slave
 * 				Further informatin in datashset MMA8452
 ********************************************************************************** */
 void I2C_Accel_Config();
 /*! *********************************************************************************
 * \brief  I2C_Altitude_Config
 * \remarks		Writes settings to I2C Slave
 * 				Further information in datashset MPL3115
 ********************************************************************************** */
 void I2C_Altitude_Config(void);

#endif /* COMMON_ACCEL_ALTITUDE_SENSOR_H_ */
