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

// Checks if this device is supported.
static bool_t CheckScanEvent(gapScannedDevice_t* pData)
{
 uint8_t index = 0;
 uint8_t name[10];
 uint8_t nameLength;
 bool_t foundMatch = FALSE;

 while (index < pData->dataLength)
 {
        gapAdStructure_t adElement;

        adElement.length = pData->data[index];
        adElement.adType = (gapAdType_t)pData->data[index + 1];
        adElement.aData = &pData->data[index + 2];

         /* Search for Bike simulator Custom Service */
        if ((adElement.adType == gAdIncomplete128bitServiceList_c) ||
          (adElement.adType == gAdComplete128bitServiceList_c))
        {
            foundMatch = MatchDataInAdvElementList(&adElement, &uuid_service_bike_simulator, 16);
        }

        if ((adElement.adType == gAdShortenedLocalName_c) ||
          (adElement.adType == gAdCompleteLocalName_c))
        {
            nameLength = MIN(adElement.length, 10);
            FLib_MemCpy(name, adElement.aData, nameLength);
        }

        /* Move on to the next AD elemnt type */
        index += adElement.length + sizeof(uint8_t);
 }

 if (foundMatch)
 {
        /* UI */
        //shell_write("\r\nFound device: \r\n");
       // shell_writeN((char*)name, nameLength-1);
        //SHELL_NEWLINE();
        //shell_writeHex(pData->aAddress, 6);
 }
 return foundMatch;
}

/************************************************************************************
*************************************************************************************
* SERVER
*************************************************************************************
************************************************************************************/
bleResult_t Bss_Start(bssConfig_t *pServiceConfig)
{
	mBs_SubscribedClientId = gInvalidDeviceId_c;

	return Bss_ReceiveData(pServiceConfig->serviceHandle);
}



// Receive braking power value -> maybe adapt parameter
bleResult_t Bss_ReceiveData(uint16_t serviceHandle)
{
    uint16_t  bsValue;
    bleResult_t result = gBleSuccess_c;
    //UUID von braking
    bleUuid_t uuid; //Service UUID

    /* Get handle of bike_simulator_notify characteristic */
    //result = GattDb_FindCharValueHandleInService(serviceHandle, gBleUuidType128_c, &uuid_characteristic_bike_notify, &bsValue);

    if (result != gBleSuccess_c)
        return result;


    return gBleSuccess_c;
}

bleResult_t Bsc_Subscribe(deviceId_t deviceId)
{
	mBs_SubscribedClientId = deviceId;
    return gBleSuccess_c;
}

bleResult_t Bsc_Unsubscribe(void)
{
    mBs_SubscribedClientId = gInvalidDeviceId_c;
    return gBleSuccess_c;
}




