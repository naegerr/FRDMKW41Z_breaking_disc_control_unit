/*
 * Parse_Message.h
 *
 *  Created on: 30.12.2017
 *      Author: rober_000
 */

#ifndef COMMON_BLE_MESSAGE_HANDLE_H_
#define COMMON_BLE_MESSAGE_HANDLE_H_

#include "fsl_common.h"
#include "gatt_server_interface.h"

// Power and bike Speed values in uint16_t format
typedef struct bikeValues_tag
{
	uint64_t		power;	   // in W
	uint64_t		bikeSpeed; // in 10*m/s
} bikeValues_t;

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
bikeValues_t parseBikeMsg(gattServerAttributeWrittenEvent_t writtenMsg);

#endif /* COMMON_BLE_MESSAGE_HANDLE_H_ */
