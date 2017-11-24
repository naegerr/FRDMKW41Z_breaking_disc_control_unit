/*!
 * This file is the c file for the Bike simulator service.
 *
 * Author:		Robert Näger
 * Date:		15. Nov. 2017
 */

/************************************************************************************
*************************************************************************************
* Include
*************************************************************************************
************************************************************************************/
#include "FunctionLib.h"

#include "ble_general.h"
#include "gatt_db_app_interface.h"
//#include "gatt_db.h"				// added by NAR

#include "gatt_db_handles.h"
#include "gatt_server_interface.h"
#include "gap_interface.h"
#include "bike_simulator_interface.h"


/************************************************************************************
*************************************************************************************
* Private constants & macros
*************************************************************************************
************************************************************************************/
/*! Bike simulator Service - Subscribed Client */
static deviceId_t mBs_SubscribedClientId;
static uint16_t	counter = 0;

/************************************************************************************
*************************************************************************************
* Private type definitions
*************************************************************************************
************************************************************************************/

/************************************************************************************
*************************************************************************************
* Public functions
*************************************************************************************
************************************************************************************/
bleResult_t Bss_Start(bssConfig_t *pServiceConfig)
{
	mBs_SubscribedClientId = gInvalidDeviceId_c;

	return Bss_ReadData(pServiceConfig->serviceHandle,
			pServiceConfig->valueBikeNotify, pServiceConfig->valueBikeWrite);
}



/*! Read data packet from client and store into data base */
bleResult_t Bss_ReadData(uint16_t serviceHandle,
		uint8_t* dataNotify, uint8_t* dataWrite)
{
    bleResult_t result = gBleSuccess_c;
    bleUuid_t 	uuidBikeCharacteristicNotify;
    bleUuid_t 	uuidBikeCharacteristicWrite;
    uint16_t	characteristicNotifyhandle;
    uint16_t	characteristicWritehandle;
    FLib_MemCpy(uuidBikeCharacteristicNotify.uuid128, uuid_characteristic_bike_notify, 16);
    FLib_MemCpy(uuidBikeCharacteristicWrite.uuid128, uuid_characteristic_bike_write, 16);

    counter++;
    /* Get handle of bike_simulator_Notify characteristic */
    result 	= GattDb_FindCharValueHandleInService(serviceHandle, gBleUuidType128_c,
    			&uuidBikeCharacteristicNotify, &characteristicNotifyhandle);
    /* Get handle of bike_simulator_Write characteristic */
    result |= GattDb_FindCharValueHandleInService(serviceHandle, gBleUuidType128_c,
    			&uuidBikeCharacteristicWrite, &characteristicWritehandle);

    if (result != gBleSuccess_c)
        return result;

    /* Update characteristics value */
    result 	= GattDb_WriteAttribute(characteristicNotifyhandle,
    		sizeof(dataNotify), dataNotify);
    result |= GattDb_WriteAttribute(characteristicWritehandle,
    		sizeof(dataWrite), dataWrite);

    if (result != gBleSuccess_c)
	    return result;

    Bss_SendNotificiation(characteristicNotifyhandle);
    return gBleSuccess_c;
}
/*! Write Data to client */
bleResult_t Bss_WriteData(uint16_t serviceHandle, double_t newBrakingValue, double_t newSpeedValue)
{
	// send data back
	return gBleSuccess_c;
}


bleResult_t Bss_Subscribe(deviceId_t deviceId)
{
	mBs_SubscribedClientId = deviceId;
    return gBleSuccess_c;
}

bleResult_t Bss_Unsubscribe(void)
{
    mBs_SubscribedClientId = gInvalidDeviceId_c;
    return gBleSuccess_c;
}

bleResult_t Bss_SendNotificiation(uint16_t handleChar)
{
	uint16_t handleCccd;
	bool_t isNotifciationActive;
	bleResult_t result;

	/*! Get handle of Cccd */
	result = GattDb_FindCccdHandleForCharValueHandle(handleChar, &handleCccd);

	if (result != gBleSuccess_c)
		return result;

	if(gBleSuccess_c == Gap_CheckNotificationStatus(mBs_SubscribedClientId,
			handleCccd, &isNotifciationActive) && isNotifciationActive)
	{
		/*! Send notification to characteristic of the GattServer */
		GattServer_SendNotification(mBs_SubscribedClientId, handleChar);
	}
	return gBleSuccess_c;
}



