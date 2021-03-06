/*! *********************************************************************************
* \addtogroup Heart Rate Sensor
* @{
********************************************************************************** */
/*!
* Copyright (c) 2014, Freescale Semiconductor, Inc.
* Copyright 2016-2017 NXP
*
* \file
*
* This file is the source file for the Heart Rate Sensor application
*
* Redistribution and use in source and binary forms, with or without modification,
* are permitted provided that the following conditions are met:
*
* o Redistributions of source code must retain the above copyright notice, this list
*   of conditions and the following disclaimer.
*
* o Redistributions in binary form must reproduce the above copyright notice, this
*   list of conditions and the following disclaimer in the documentation and/or
*   other materials provided with the distribution.
*
* o Neither the name of Freescale Semiconductor, Inc. nor the names of its
*   contributors may be used to endorse or promote products derived from this
*   software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
* ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
* ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
* ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
* SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/************************************************************************************
*************************************************************************************
* Include
*************************************************************************************
************************************************************************************/
/* Framework / Drivers */
#include "RNG_Interface.h"
#include "Keyboard.h"
#include "LED.h"
#include "TimersManager.h"
#include "FunctionLib.h"
#include "MemManager.h"
#include "Panic.h"


#if (cPWR_UsePowerDownMode)
#include "PWR_Interface.h"
#include "PWR_Configuration.h"
#endif

/* BLE Host Stack */
#include "gatt_server_interface.h"
#include "gatt_client_interface.h"
#include "gap_interface.h"
#include "gatt_db_handles.h"

/* Profile / Services */
#include "battery_interface.h"
#include "device_info_interface.h"
#include "heart_rate_interface.h"
#include "bike_simulator_interface.h"

/* Connection Manager */
#include "ble_conn_manager.h"

#include "board.h"
#include "ApplMain.h"
#include "heart_rate_sensor.h"

/************************************************************************************
*************************************************************************************
* Private macros
*************************************************************************************
************************************************************************************/
#define mHeartRateLowerLimit_c          (40) /* Heart beat lower limit, 8-bit value */
#define mHeartRateUpperLimit_c          (201) /* Heart beat upper limit, 8-bit value */
#define mHeartRateRange_c               (mHeartRateUpperLimit_c - mHeartRateLowerLimit_c) /* Range = [ADC16_HB_LOWER_LIMIT .. ADC16_HB_LOWER_LIMIT + ADC16_HB_DYNAMIC_RANGE] */
#define mHeartRateReportInterval_c      (1)        /* heart rate report interval in seconds  */
#define mBatteryLevelReportInterval_c   (10)        /* battery level report interval in seconds  */
/************************************************************************************
*************************************************************************************
* Private type definitions
*************************************************************************************
************************************************************************************/
typedef enum appEvent_tag{
    mAppEvt_PeerConnected_c,
    mAppEvt_PairingComplete_c,
    mAppEvt_GattProcComplete_c,
    mAppEvt_GattProcError_c
}appEvent_t;

typedef enum appState_tag{
    mAppIdle_c,
    mAppExchangeMtu_c,
    mAppPrimaryServiceDisc_c,
    mAppCharServiceDisc_c,
    mAppDescriptorSetup_c,
    mAppRunning_c,
}appState_t;

typedef enum
{
#if gAppUseBonding_d
    fastWhiteListAdvState_c,
#endif
    fastAdvState_c,
    slowAdvState_c
}advType_t;

typedef struct advState_tag{
    bool_t      advOn;
    advType_t   advType;
}advState_t;

// Persistent information for client
typedef struct appCustomInfo_tag
{
    bscConfig_t     bsClientConfig;
    /* Add persistent information here */
}appCustomInfo_t;

typedef struct appPeerInfo_tag
{
    deviceId_t      deviceId;
    appCustomInfo_t customInfo;
    bool_t          isBonded;
    appState_t      appState;
}appPeerInfo_t;

/************************************************************************************
*************************************************************************************
* Private memory declarations
*************************************************************************************
************************************************************************************/

/* Adv State */
static advState_t  mAdvState;
static bool_t      mRestartAdv;
static uint32_t    mAdvTimeout;
static deviceId_t  mPeerDeviceId = gInvalidDeviceId_c;

/* Service Data*/
static basConfig_t      basServiceConfig = {service_battery, 0};
static hrsUserData_t    hrsUserData;
static hrsConfig_t 		hrsServiceConfig = {service_heart_rate, TRUE, TRUE, TRUE, gHrs_BodySensorLocChest_c, &hrsUserData};
static uint16_t 		cpHandles[1] = { value_hr_ctrl_point };
static bssConfig_t		bssServiceConfig = {service_bike_simulator, 0, 0};
static uint16_t			cbHandles[2] = { value_bike_notify, value_bike_write};

/* Application specific data*/
static bool_t mToggle16BitHeartRate = FALSE;
static bool_t mContactStatus = TRUE;
static tmrTimerID_t mAdvTimerId;
static tmrTimerID_t mMeasurementTimerId;
static tmrTimerID_t mBatteryMeasurementTimerId;


/************************************************************************************
*************************************************************************************
* Private functions prototypes
*************************************************************************************
************************************************************************************/
/* Gatt and Att callbacks */
static void BleApp_AdvertisingCallback (gapAdvertisingEvent_t* pAdvertisingEvent);
static void BleApp_ConnectionCallback (deviceId_t peerDeviceId, gapConnectionEvent_t* pConnectionEvent);
static void BleApp_GattServerCallback (deviceId_t deviceId, gattServerEvent_t* pServerEvent);
static void BleApp_Config();

/* Timer Callbacks */
static void AdvertisingTimerCallback(void *);
static void TimerMeasurementCallback(void *);
static void BatteryMeasurementTimerCallback(void *);

static void BleApp_Advertise(void);

/************************************************************************************
*************************************************************************************
* Public functions
*************************************************************************************
************************************************************************************/

/*! *********************************************************************************
* \brief    Stores handles for the specified service and characteristic
*
********************************************************************************** */

static void BleApp_StoreServiceHandles(gattService_t *pService)
{
    uint8_t i;
    // Checks if it is bike simulator service
    if ((pService->uuidType == gBleUuidType128_c) && FLib_MemCmp(pService->uuid.uuid128, uuid_service_bike_simulator, 16))
    {
        /* Store bike simulator service handle */
        //mPeerInformation.customInfo.humClientConfig.hService = pService->startHandle;

        // Check all characteristics
        for (i = 0; i < pService->cNumCharacteristics; i++)
        {
            if ((pService->aCharacteristics[i].value.uuidType == gBleUuidType128_c) &&
                (pService->aCharacteristics[i].value.uuid.uuid128 == uuid_characteristic_bike_notify))
            {
            	/* Store Bike notify Characteristic */
                //mPeerInformation.customInfo.humClientConfig.hHumidity = pService->aCharacteristics[i].value.handle;
            }
            if ((pService->aCharacteristics[i].value.uuidType == gBleUuidType128_c) &&
            	(pService->aCharacteristics[i].value.uuid.uuid128 == uuid_characteristic_bike_write))
            {
                /* Store Bike write Characteristic */
                //mPeerInformation.customInfo.humClientConfig.hHumidity = pService->aCharacteristics[i].value.handle;
            }
        }
    }
}

/*! *********************************************************************************
* \brief    Stores handles for the descriptors
*
********************************************************************************** */
#if DESCRIPTORS_ENABLE
static void BleApp_StoreCharHandles(gattCharacteristic_t *pChar)
{
    uint8_t i;

    // No desriptors anyway
    if ((pChar->value.uuidType == gBleUuidType128_c) &&
        (pChar->value.uuid.uuid128 == gBleSig_Humidity_d))
    {
        for (i = 0; i < pChar->cNumDescriptors; i++)
        {
            if (pChar->aDescriptors[i].uuidType == gBleUuidType16_c)
            {
                switch (pChar->aDescriptors[i].uuid.uuid16)
                {
                    case gBleSig_CharPresFormatDescriptor_d:
                    {
                        //mPeerInformation.customInfo.humClientConfig.hHumDesc = pChar->aDescriptors[i].handle;
                        break;
                    }
                    case gBleSig_CCCD_d:
                    {
                        //mPeerInformation.customInfo.humClientConfig.hHumCccd = pChar->aDescriptors[i].handle;
                        break;
                    }
                    default:
                        break;
                }
            }
        }
    }
}
#endif
/*! *********************************************************************************
* \brief    Stores the values for the descriptors
*
********************************************************************************** */
#if DESCRIPTORS_ENABLE
static void BleApp_StoreDescValues(gattAttribute_t *pDesc)
{

    if (pDesc->handle == mPeerInformation.customInfo.humClientConfig.hHumDesc)
    {
        /* Store Humidity format*/
        FLib_MemCpy(&mPeerInformation.customInfo.humClientConfig.humFormat,
                    pDesc->paValue,
                    pDesc->valueLength);
    }
}
#endif

/*! *********************************************************************************
* \brief    Initializes application specific functionality before the BLE stack init.
*
********************************************************************************** */
void BleApp_Init(void)
{
    /* Initialize application support for drivers */
    BOARD_InitAdc();
}

/*! *********************************************************************************
* \brief    Starts the BLE application.
*
********************************************************************************** */
void BleApp_Start(void)
{
    /* Device is not connected and not advertising*/
    if (!mAdvState.advOn)
    {
#if gAppUseBonding_d
        if (gcBondedDevices > 0)
        {
            mAdvState.advType = fastWhiteListAdvState_c;
        }
        else
        {
#endif
        mAdvState.advType = fastAdvState_c;
#if gAppUseBonding_d
        }
#endif
        BleApp_Advertise();
    }
    
#if (cPWR_UsePowerDownMode)    
    PWR_ChangeDeepSleepMode(1); /* MCU=LLS3, LL=DSM, wakeup on GPIO/LL */
    PWR_AllowDeviceToSleep();
#endif       
}

/*! *********************************************************************************
* \brief        Handles keyboard events.
*
* \param[in]    events    Key event structure.
********************************************************************************** */
void BleApp_HandleKeys(key_event_t events)
{
    switch (events)
    {
        case gKBD_EventPressPB1_c:
        {
            if (mPeerDeviceId == gInvalidDeviceId_c)
            {
            	/* start advertising */
                BleApp_Start();
            }
            break;
        }
        case gKBD_EventPressPB2_c:
        {
            mToggle16BitHeartRate = (mToggle16BitHeartRate)?FALSE:TRUE;
        }
        break;
        case gKBD_EventLongPB1_c:
        {
            if (mPeerDeviceId != gInvalidDeviceId_c)
            {
                Gap_Disconnect(mPeerDeviceId);
            }
            break;
        }
        case gKBD_EventLongPB2_c:
        {
            mContactStatus = mContactStatus?FALSE:TRUE;
            Hrs_SetContactStatus(service_heart_rate, mContactStatus);
            break;
        }
        default:
            break;
    }
}

/*! *********************************************************************************
* \brief        Handles BLE generic callback.
*
* \param[in]    pGenericEvent    Pointer to gapGenericEvent_t.
********************************************************************************** */
void BleApp_GenericCallback (gapGenericEvent_t* pGenericEvent)
{
    /* Call BLE Conn Manager */
    BleConnManager_GenericEvent(pGenericEvent);
    
    switch (pGenericEvent->eventType)
    {
        case gInitializationComplete_c:    
        {
            BleApp_Config();
        }
        break;    
        
        case gAdvertisingParametersSetupComplete_c:
        {
            App_StartAdvertising(BleApp_AdvertisingCallback, BleApp_ConnectionCallback);
        }
        break;         

        default: 
        break;
    }
}

/************************************************************************************
*************************************************************************************
* Private functions
*************************************************************************************
************************************************************************************/

/*! *********************************************************************************
* \brief        Configures BLE Stack after initialization. Usually used for
*               configuring advertising, scanning, white list, services, et al.
*
********************************************************************************** */
static void BleApp_Config()
{
	/* Read public address from controller */
	Gap_ReadPublicDeviceAddress();

    /* Configure as GAP peripheral */
    BleConnManager_GapPeripheralConfig();

    /* Register for callbacks */
    GattServer_RegisterHandlesForWriteNotifications(NumberOfElements(cpHandles), cpHandles);
    GattServer_RegisterHandlesForWriteNotifications(NumberOfElements(cbHandles), cbHandles);

    /* Callback, when client makes a request */
    App_RegisterGattServerCallback(BleApp_GattServerCallback);

    mAdvState.advOn = FALSE;

    /* Start services */
    hrsServiceConfig.sensorContactDetected = mContactStatus;
#if gHrs_EnableRRIntervalMeasurements_d    
    hrsServiceConfig.pUserData->pStoredRrIntervals = MEM_BufferAlloc(sizeof(uint16_t) * gHrs_NumOfRRIntervalsRecorded_c);
#endif    
    Hrs_Start(&hrsServiceConfig);

    basServiceConfig.batteryLevel = BOARD_GetBatteryLevel();
    Bas_Start(&basServiceConfig);

    /* Start bike simulator service */
    Bss_Start(&bssServiceConfig);

    /* Allocate application timers */
    mAdvTimerId = TMR_AllocateTimer();
    mMeasurementTimerId = TMR_AllocateTimer();
    mBatteryMeasurementTimerId = TMR_AllocateTimer();

#if (cPWR_UsePowerDownMode)    
    PWR_ChangeDeepSleepMode(3); /* MCU=LLS3, LL=IDLE, wakeup on GPIO/LL */
    PWR_AllowDeviceToSleep();    
#endif    
}

/*! *********************************************************************************
* \brief        Configures GAP Advertise parameters. Advertise will start after
*               the parameters are set.
*
********************************************************************************** */
static void BleApp_Advertise(void)
{
    switch (mAdvState.advType)
    {
#if gAppUseBonding_d
        case fastWhiteListAdvState_c:
        {
            gAdvParams.minInterval = gFastConnMinAdvInterval_c;
            gAdvParams.maxInterval = gFastConnMaxAdvInterval_c;
            gAdvParams.filterPolicy = gProcessWhiteListOnly_c;
            mAdvTimeout = gFastConnWhiteListAdvTime_c;
        }
        break;
#endif
        case fastAdvState_c:
        {
            gAdvParams.minInterval = gFastConnMinAdvInterval_c;
            gAdvParams.maxInterval = gFastConnMaxAdvInterval_c;
            gAdvParams.filterPolicy = gProcessAll_c;
            mAdvTimeout = gFastConnAdvTime_c - gFastConnWhiteListAdvTime_c;
        }
        break;

        case slowAdvState_c:
        {
            gAdvParams.minInterval = gReducedPowerMinAdvInterval_c;
            gAdvParams.maxInterval = gReducedPowerMinAdvInterval_c;
            gAdvParams.filterPolicy = gProcessAll_c;
            mAdvTimeout = gReducedPowerAdvTime_c;
        }
        break;
    }

    /* Set advertising parameters and start advertising*/
    Gap_SetAdvertisingParameters(&gAdvParams);
}

/*! *********************************************************************************
* \brief        Handles BLE Advertising callback from host stack.
*
* \param[in]    pAdvertisingEvent    Pointer to gapAdvertisingEvent_t.
********************************************************************************** */
static void BleApp_AdvertisingCallback (gapAdvertisingEvent_t* pAdvertisingEvent)
{
    switch (pAdvertisingEvent->eventType)
    {
        case gAdvertisingStateChanged_c:
        {
            mAdvState.advOn = !mAdvState.advOn;
            
            if (!mAdvState.advOn && mRestartAdv)
            {
                BleApp_Advertise();
                break;
            }                

#if (cPWR_UsePowerDownMode)
            if(!mAdvState.advOn)
            {
                Led1Off();
                PWR_ChangeDeepSleepMode(3);
                PWR_SetDeepSleepTimeInMs(cPWR_DeepSleepDurationMs);
                PWR_AllowDeviceToSleep();    
            }  
            else
            {
                PWR_ChangeDeepSleepMode(1);
                /* Start advertising timer */
                TMR_StartLowPowerTimer(mAdvTimerId,gTmrLowPowerSecondTimer_c,
                         TmrSeconds(mAdvTimeout), AdvertisingTimerCallback, NULL);             
                Led1On();
            }
#else
            LED_StopFlashingAllLeds();
            Led1Flashing();

            if(!mAdvState.advOn)
            {
                Led2Flashing();
                Led3Flashing();
                Led4Flashing();
            }
            else
            {
                TMR_StartLowPowerTimer(mAdvTimerId,gTmrLowPowerSecondTimer_c,
                        TmrSeconds(mAdvTimeout), AdvertisingTimerCallback, NULL);  
            }
#endif 
        }
        break;
        case gAdvertisingCommandFailed_c:
        {
            panic(0,0,0,0);
        }
        break;

        default:
            break;
    }
}

/*! *********************************************************************************
* \brief        Handles BLE Connection callback from host stack. Only when connection
* 				established during advertising.
*
* \param[in]    peerDeviceId        Peer device ID.
* \param[in]    pConnectionEvent    Pointer to gapConnectionEvent_t.
********************************************************************************** */
static void BleApp_ConnectionCallback (deviceId_t peerDeviceId, gapConnectionEvent_t* pConnectionEvent)
{
	/* Connection Manager to handle Host Stack interactions */
	BleConnManager_GapPeripheralEvent(peerDeviceId, pConnectionEvent);

    switch (pConnectionEvent->eventType)
    {
        case gConnEvtConnected_c:
        {
            mPeerDeviceId = peerDeviceId;

            /* Advertising stops when connected */
            mAdvState.advOn = FALSE;            
        
            /* Subscribe client */
            Bss_Subscribe(peerDeviceId);
            Bas_Subscribe(peerDeviceId);        
            Hrs_Subscribe(peerDeviceId);
                                    
#if (!cPWR_UsePowerDownMode)  
            /* UI */            
            LED_StopFlashingAllLeds();
            Led1On();            
#endif            
            
            /* Stop Advertising Timer*/
            mAdvState.advOn = FALSE;
            TMR_StopTimer(mAdvTimerId);
            
            /* Start measurements */
            TMR_StartLowPowerTimer(mMeasurementTimerId, gTmrLowPowerIntervalMillisTimer_c,
                       TmrSeconds(mHeartRateReportInterval_c), TimerMeasurementCallback, NULL);

            /* Start battery measurements */
            TMR_StartLowPowerTimer(mBatteryMeasurementTimerId, gTmrLowPowerIntervalMillisTimer_c,
                       TmrSeconds(mBatteryLevelReportInterval_c), BatteryMeasurementTimerCallback, NULL);            

#if (cPWR_UsePowerDownMode)
    PWR_SetDeepSleepTimeInMs(900);
    PWR_ChangeDeepSleepMode(1);
    PWR_AllowDeviceToSleep();    
#endif
        }
        break;
        
        case gConnEvtDisconnected_c:
        {
            /* Unsubscribe client */
        	Bss_Unsubscribe();
            Bas_Unsubscribe();
            Hrs_Unsubscribe();

            mPeerDeviceId = gInvalidDeviceId_c;
            
            TMR_StopTimer(mMeasurementTimerId);
            TMR_StopTimer(mBatteryMeasurementTimerId);            

#if (cPWR_UsePowerDownMode)            
            /* UI */
            Led1Off();
            
            /* Go to sleep */
            PWR_ChangeDeepSleepMode(3); /* MCU=LLS3, LL=IDLE, wakeup on swithes/LL */
            PWR_SetDeepSleepTimeInMs(cPWR_DeepSleepDurationMs);
            PWR_AllowDeviceToSleep();
#else
            if (pConnectionEvent->eventData.disconnectedEvent.reason == gHciConnectionTimeout_c)
            {
                /* Link loss detected*/
                BleApp_Start();
            }
            else
            {
              /* Connection was terminated by peer or application */
                BleApp_Start();
            }
#endif			
        }
        break;
        case gConnEvtLongTermKeyRequest_c:
        {
        	// When already connected once.
        	// Not implemented on mobile app

        }
        break;
        case gConnEvtPairingRequest_c:
        {
        	// Accept Pairing with own parameters, SMP not implemented
        	// Gap_AcceptPairingRequest(peerDeviceId, )
        }
        break;
        case gConnEvtLeDataLengthChanged_c:
        {
        	// Data length has changed
        }
    default:
        break;
    }
}
/*! *********************************************************************************
* \brief        Send response by ATT protocol
*
* \param[in]    pDeviceId    		Device ID to send response
* \param[in]	pGattServerEvent    Event, responsing to
* \param[in]	pResult				Result of Receive attribute
********************************************************************************** */
static void BleApp_SendAttWriteResponse(deviceId_t* pDeviceId, gattServerEvent_t* pGattServerEvent, bleResult_t* pResult)
{
	attErrorCode_t attErrorCode;

	// Determine response to send (OK or Error)
	if(*pResult == gBleSuccess_c)
	{
		attErrorCode = gAttErrCodeNoError_c;
	}
	else
	{
		attErrorCode = (attErrorCode_t)(*pResult & 0x00FF);
	}
	// Send response to client
	GattServer_SendAttributeWrittenStatus(*pDeviceId, pGattServerEvent->eventData.attributeWrittenEvent.handle, attErrorCode);
}
/*! *********************************************************************************
* \brief        Handles GATT server callback from host stack.
*
* \param[in]    deviceId        Peer device ID.
* \param[in]    pServerEvent    Pointer to gattServerEvent_t.
********************************************************************************** */
static void BleApp_GattServerCallback (deviceId_t deviceId, gattServerEvent_t* pServerEvent)
{
    switch (pServerEvent->eventType)
    {
    	/* Write request from client */
        case gEvtAttributeWritten_c:
        {
        	/*
        		Attribute write handler: Create a case for your registered attribute and
        	    execute callback action accordingly
        	*/
        	/* Store value in GATT data base */
    		switch(pServerEvent->eventData.attributeWrittenEvent.handle)
            {
            	case value_bike_notify:
            	{
            		GattDb_WriteAttribute(pServerEvent->eventData.attributeWrittenEvent.handle,
            		            		  pServerEvent->eventData.attributeWrittenEvent.cValueLength,
            							  pServerEvent->eventData.attributeWrittenEvent.aValue);

            		GattServer_SendAttributeWrittenStatus(deviceId, value_bike_notify, gAttErrCodeNoError_c);
            	}
            	break;
            	case value_bike_write:
            	{
            		GattDb_WriteAttribute(pServerEvent->eventData.attributeWrittenEvent.handle,
            		    				  pServerEvent->eventData.attributeWrittenEvent.cValueLength,
            							  pServerEvent->eventData.attributeWrittenEvent.aValue);

            		GattServer_SendAttributeWrittenStatus(deviceId, value_bike_write, gAttErrCodeNoError_c);
            	}
            	break;
            	default:
            	break;
            }

            // For heart measurement!
            if (pServerEvent->eventData.attributeWrittenEvent.handle == value_hr_ctrl_point)
            {
                Hrs_ControlPointHandler(&hrsUserData, pServerEvent->eventData.attributeWrittenEvent.aValue[0]);
            }
            GattServer_SendAttributeWrittenStatus(deviceId, pServerEvent->eventData.attributeWrittenEvent.handle, gAttErrCodeNoError_c);
        }
        break;

        /* Read request from client */
        case gEvtAttributeRead_c:
        {
        	/*
				Attribute read handler: Create a case for your registered attribute and
				execute callback action accordingly
			*/
        	switch(pServerEvent->eventData.attributeReadEvent.handle)
        	{
				case value_bike_notify:
				{
					GattServer_SendAttributeReadStatus(deviceId, value_bike_notify, gAttErrCodeNoError_c);
				}
				break;
				case value_bike_write:
				{
					GattServer_SendAttributeReadStatus(deviceId, value_bike_write, gAttErrCodeNoError_c);
				}
				break;
				default:
				break;

			}
        }
        /* Any CCCD is changed */
        case gEvtCharacteristicCccdWritten_c:
        {
        	/* Which CCCD was changed? */
        	switch(pServerEvent->eventData.charCccdWrittenEvent.handle)
        	{
        		case cccd_bike_notify:
        		{
        			/* Is notification enabled? */
        			if (pServerEvent->eventData.charCccdWrittenEvent.newCccd)
        			{
        				/* Timer starts */
        				//TMR_StartTimer(tsiTimerId, timerType, timeInMilliseconds, callback, param)
        			}
        			else
        			{
        				/* Timer stops */
        			}
        		}
        		break;
        		default:
        		break;
           	}

        }
        break;

    default:
    break;
    }
}


/*! *********************************************************************************
* \brief        Handles advertising timer callback.
*
* \param[in]    pParam        Calback parameters.
********************************************************************************** */
static void AdvertisingTimerCallback(void * pParam)
{
    /* Stop and restart advertising with new parameters */
    Gap_StopAdvertising();

    switch (mAdvState.advType)
    {
#if gAppUseBonding_d
        case fastWhiteListAdvState_c:
        {
            mAdvState.advType = fastAdvState_c;
            mRestartAdv = TRUE;
        }
        break;
#endif
        case fastAdvState_c:
        {
            mAdvState.advType = slowAdvState_c;
            mRestartAdv = TRUE;
        }
        break;

        default:
        {
            mRestartAdv = FALSE;
        }
        break;
    }
}

/*! *********************************************************************************
* \brief        Handles measurement timer callback.
*
* \param[in]    pParam        Callback parameters.
********************************************************************************** */
static void TimerMeasurementCallback(void * pParam)
{

    uint16_t hr = BOARD_GetPotentiometerLevel();
    hr = (hr * mHeartRateRange_c) >> 12;

#if gHrs_EnableRRIntervalMeasurements_d    
    Hrs_RecordRRInterval(&hrsUserData, (hr & 0x0F));
    Hrs_RecordRRInterval(&hrsUserData, (hr & 0xF0));
#endif

    // NOT NECES
    // Store received data into GATT database
    //Bss_StoreData(bssServiceConfig.serviceHandle, bssServiceConfig.valueBikeWrite, bssServiceConfig.valueBikeNotify);

    if (mToggle16BitHeartRate)
    {
        Hrs_RecordHeartRateMeasurement(service_heart_rate, 0x0100 + (hr & 0xFF), &hrsUserData);
    }
    else
    {
        Hrs_RecordHeartRateMeasurement(service_heart_rate, mHeartRateLowerLimit_c + hr, &hrsUserData); 
    }
        
    Hrs_AddExpendedEnergy(&hrsUserData, 100);
}

/*! *********************************************************************************
* \brief        Handles battery measurement timer callback.
*
* \param[in]    pParam        Calback parameters.
********************************************************************************** */
static void BatteryMeasurementTimerCallback(void * pParam)
{
    basServiceConfig.batteryLevel = BOARD_GetBatteryLevel();
    Bas_RecordBatteryMeasurement(basServiceConfig.serviceHandle, basServiceConfig.batteryLevel);
}

/*! *********************************************************************************
* @}
********************************************************************************** */
