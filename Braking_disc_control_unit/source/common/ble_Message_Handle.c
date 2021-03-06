/*
 * Parse_Message.c
 *
 *  Created on: 30.12.2017
 *      Author: Robert Näger
 */
#include "fsl_common.h"
#include "gatt_server_interface.h"
#include "ble_Message_Handle.h"

// Save some Unicode signs
typedef enum controlChars_tag
{
	angledBrace_open	= 0x5B, /* [ */
	angledBrace_off 	= 0x5D, /* ] */
	curlyBrace_open		= 0x7B, /* { */
	curlyBrace_off		= 0x7D, /* } */
	comma				= 0x2C, /* , */
	point				= 0x2E	/* . */
} controlChars_t;


/*! message types according to Mobile App */
typedef enum bssMessage_tag
{
	connectionTest = 48,
	connectionTestResponse,
	start,
	stop,
	pause,
	brake,				// integer in [W]
	acceleration,
	bikeSpeed,			// [m/s]
	windSpeed,			// [m/s]
	windDirection,		// 0 front, 1/2PI right, PI back, 3/2PI left
	inclination,		// [grade]
	airDensity,			// [kg/m^3]
	temperature,		// [°C]
	gpsLatitude,		// DecDeg Format
	gpsLongitude,		// DecDeg Format
	gpsAltitude,		// [m] above Referenzellipsoid WGS 84
	testMessage			// For developing purposes
} bssMessage_t;

/*! *********************************************************************************
* \brief  isUnicodeNumber
*
* \param[in] 	character	character which has to be checked
*
* \return  		TRUE -> character value is '0' until '9'
* 				otherwise FALSE
*
* \remarks		Checks if character is UNICODE number
*
********************************************************************************** */
bool_t isUnicodeNumber(char character)
{
	return (character < 48 || character > 57)?FALSE:TRUE;
}

/*! *********************************************************************************
* \brief  n_pow
*
* \param[in] x			Base of the operation exponation
* \param[in] expo		exponent of the operation exponation
*
* \return  result
*
* \remarks Exponation function
*
********************************************************************************** */
uint16_t n_pow(uint16_t x, uint16_t expo)
{
	uint16_t i = 1;
	if(expo == 0)
	{
		return 1;
	}
	if(expo == 1)
	{
		return x;
	}
	while(i<=(expo-1))
	{
		x*=x;
		i++;
	}
	return x;
}
/*! *********************************************************************************
* \brief  parseBikeMsg
*
* \param[in] writtenMsg		WrittenEvent from Gattserver
* \return  bikeValues (Power and bikespeed)
*
* \remarks Parses the char* message into uint16_t values for power and speed
*  This function gets called to get integer values from the message received
*
********************************************************************************** */
bikeValues_t parseBikeMsg(gattServerAttributeWrittenEvent_t writtenMsg)
{
	bikeValues_t bikeValues;
	uint8_t counter = 0;
	char* strPtr1;
	char* strPtr2;
	bikeValues.bikeSpeed = 0;
	bikeValues.power = 0;

	// Check if still connected
	if(writtenMsg.aValue[0] == 89)
	{
		bikeValues.power = 0;
		bikeValues.bikeSpeed = 0;
		return bikeValues;
	}
	// Check if Services discovered
	if(writtenMsg.aValue[0] == 191)
	{
		bikeValues.power = 0;
		bikeValues.bikeSpeed = 0;
		return bikeValues;
	}
	// char compare
	if(writtenMsg.aValue[0] == curlyBrace_open && writtenMsg.aValue[1] == angledBrace_open)
	{
		if(writtenMsg.aValue[2] == start)
		{
			bikeValues.power = 0;
			bikeValues.bikeSpeed = 0;
			return bikeValues;
		}
		if(writtenMsg.aValue[2] == pause)
		{
			bikeValues.power = 0;
			bikeValues.bikeSpeed = 0;
			return bikeValues;
		}
		if(writtenMsg.aValue[2] == stop)
		{
			bikeValues.power = 0;
			bikeValues.bikeSpeed = 0;
			return bikeValues;
		}
		// Is brake value?
		if(writtenMsg.aValue[2] == brake)
		{
			// Assumption, no floating values added yet!
			strPtr1 = strchr((char*)writtenMsg.aValue, angledBrace_off);
			strPtr2 = strPtr1; // Save value for next message
			strPtr2 += 2;
			strPtr1--;
			// Iterate from LS character and sum up
			do{
				if(isUnicodeNumber(*strPtr1))
				{
					bikeValues.power += ((*strPtr1)-48)*n_pow(10, counter);
					counter++;
				}
				strPtr1--;
			}while(*strPtr1 != comma);
		}

		counter = 0;
		// Is speed value?
		if(*strPtr2 == bikeSpeed)
		{
			strPtr1 = strchr((char*)strPtr2, angledBrace_off);
			strPtr1--;
			// Iterate from LS character and sum up characters
			do{
				if(isUnicodeNumber(*strPtr1))
				{
					bikeValues.bikeSpeed += ((*strPtr1)-48)*n_pow(10, counter);
					counter++;
				}

				strPtr1--;
			}while(*strPtr1 != comma);
		}
	}
	return bikeValues;
}
