/*!
* Copyright (c) 2014, Freescale Semiconductor, Inc.
* Copyright 2016-2017 NXP
*
* \file
* This is a source file for the main application.
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
/* Drv */
#include "LED.h"
#include "Keyboard.h"

/* Fwk */
#include "fsl_os_abstraction.h"
#include "MemManager.h"
#include "TimersManager.h"
#include "RNG_Interface.h"
#include "Messaging.h"
#include "Flash_Adapter.h"
#include "SecLib.h"
#include "Panic.h"
#include "TMR_Adapter.h"		// added by NAR
#include "pin_mux.h"			// added by NAR
#include "fsl_tpm.h"			// added by NAR
#include "fsl_gpio.h"			// added by NAR
#include "fsl_port.h"			// added by NAR
#include "fsl_adc16.h"			// added by NAR
#include "fsl_pit.h"			// added by NAR
#include "fsl_i2c.h"			// added by NAR

#if gFsciIncluded_c    
#include "FsciInterface.h"
#include "FsciCommands.h"
#endif

/* KSDK */
#include "board.h"

/* Bluetooth Low Energy */
#include "gatt_interface.h"
#include "gatt_server_interface.h"
#include "gatt_client_interface.h"
#include "gap_interface.h"
#include "ble_init.h"
#include "ble_config.h"
#include "l2ca_cb_interface.h"

#ifdef cPWR_UsePowerDownMode
#if (cPWR_UsePowerDownMode)
#include "PWR_Interface.h"
#endif
#endif

#ifdef FSL_RTOS_FREE_RTOS
#include "FreeRTOSConfig.h"
#endif

#include "ApplMain.h"


#if gAppUseNvm_d
/* include NVM interface */
#include "NVM_Interface.h"
#endif
/************************************************************************************
*************************************************************************************
* Private macros
*************************************************************************************
************************************************************************************/

/* NVM Dataset identifiers */
#if gAppUseNvm_d
#define nvmId_BondingHeaderId_c          0x4011
#define nvmId_BondingDataDynamicId_c     0x4012
#define nvmId_BondingDataStaticId_c      0x4013
#define nvmId_BondingDataDeviceInfoId_c  0x4014
#define nvmId_BondingDataDescriptorId_c  0x4015
#endif

/* Application Events */
#define gAppEvtMsgFromHostStack_c       (1 << 0)
#define gAppEvtAppCallback_c            (1 << 1)

#ifdef FSL_RTOS_FREE_RTOS
    #if (configUSE_IDLE_HOOK)
        #define mAppIdleHook_c 1
    #endif
#endif

#ifndef mAppIdleHook_c
#define mAppIdleHook_c 0
#endif



/************************************************************************************
*************************************************************************************
* Private type definitions
*************************************************************************************
************************************************************************************/
/* Host to Application Messages Types */
typedef enum {
    gAppGapGenericMsg_c = 0,
    gAppGapConnectionMsg_c,
    gAppGapAdvertisementMsg_c,
    gAppGapScanMsg_c,
    gAppGattServerMsg_c,
    gAppGattClientProcedureMsg_c,
    gAppGattClientNotificationMsg_c,
    gAppGattClientIndicationMsg_c,
    gAppL2caLeDataMsg_c,
    gAppL2caLeControlMsg_c,
}appHostMsgType_tag;

typedef uint8_t appHostMsgType_t;

/* Host to Application Connection Message */
typedef struct connectionMsg_tag{
    deviceId_t              deviceId;
    gapConnectionEvent_t    connEvent;
}connectionMsg_t;

/* Host to Application GATT Server Message */
typedef struct gattServerMsg_tag{
    deviceId_t          deviceId;
    gattServerEvent_t   serverEvent;
}gattServerMsg_t;

/* Host to Application GATT Client Procedure Message */
typedef struct gattClientProcMsg_tag{
    deviceId_t              deviceId;
    gattProcedureType_t     procedureType;
    gattProcedureResult_t   procedureResult;
    bleResult_t             error;
}gattClientProcMsg_t;

/* Host to Application GATT Client Notification/Indication Message */
typedef struct gattClientNotifIndMsg_tag{
    uint8_t*    aValue;
    uint16_t    characteristicValueHandle;
    uint16_t    valueLength;
    deviceId_t  deviceId;
}gattClientNotifIndMsg_t;

/* L2ca to Application Data Message */
typedef struct l2caLeCbDataMsg_tag{
    deviceId_t  deviceId;
    uint16_t    lePsm;
    uint16_t    packetLength;
    uint8_t     aPacket[0];
}l2caLeCbDataMsg_t;

/* L2ca to Application Control Message */
typedef struct l2caLeCbControlMsg_tag{
    l2capControlMessageType_t   messageType;
    uint16_t                    padding;
    uint8_t                     aMessage[0];
}l2caLeCbControlMsg_t;

typedef struct appMsgFromHost_tag{
    appHostMsgType_t    msgType;
    union {
        gapGenericEvent_t       genericMsg;
        gapAdvertisingEvent_t   advMsg;
        connectionMsg_t         connMsg;
        gapScanningEvent_t      scanMsg;
        gattServerMsg_t         gattServerMsg;
        gattClientProcMsg_t     gattClientProcMsg;
        gattClientNotifIndMsg_t gattClientNotifIndMsg;
        l2caLeCbDataMsg_t       l2caLeCbDataMsg;
        l2caLeCbControlMsg_t    l2caLeCbControlMsg;
    } msgData;
}appMsgFromHost_t;

typedef struct appMsgCallback_tag{
    appCallbackHandler_t   handler;
    appCallbackParam_t     param;
}appMsgCallback_t;


// Enable BAT_MEASUREMENT and interrupt
#define 	BAT_MEASUREMENT_ENABLE	TRUE

// Power and bike Speed values in uint16_t format
typedef struct bikeValues_tag
{
	uint64_t		power;	   // in W
	uint64_t		bikeSpeed; // in 10*m/s
} bikeValues_t;


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

uint8_t 	pwmInstance;
uint8_t 	pwmChannel;
uint16_t	pwmValue;
uint8_t 	pdatedDutycycle;

tpm_config_t 				tpmInfo;
tpm_chnl_pwm_signal_param_t	tpmParam;
tpm_pwm_level_select_t 		pwmLevel;
uint8_t						counter;
bikeValues_t 				bikeValues;


#define		LED_SHOW_ACCEL		1
#if LED_SHOW_ACCEL
#define		LED_SHOW_SPEED		0
#else
#define		LED_SHOW_SPEED		1
#endif
/*! PWM */
#define		SERVO_FREQUENCY		(50)
#define		SERVO_INIT_VALUE	(3)	// not used
#define 	SERVO_TPM_BASEADDR 	(TPM0)
#define 	SERVO_TPM_CHANNEL 	(3)

// Source clock for TPM driver
#define TPM_SOURCE_CLOCK 		CLOCK_GetFreq(kCLOCK_McgFllClk)

uint8_t 	pwmInstance;
uint8_t 	pwmChannel;
uint16_t	pwmValue;
uint8_t 	updatedDutycycle;

tpm_config_t tpmInfo;
tpm_chnl_pwm_signal_param_t	tpmParam;
tpm_pwm_level_select_t pwmLevel;

/* ADC */
#if BAT_MEASUREMENT_ENABLE
#define ACCU_ADC16_BASEADDR 		ADC0
#define ACCU_ADC16_CHANNEL_GROUP 	0U
#define ACCU_ADC16_USER_CHANNEL 	0U
#define ADC16_IRQn 					ADC0_IRQn
#define ADC16_IRQ_HANDLER_FUNC 		ADC0_IRQHandler
#define	ACCU_UNDER_VOLTAGE			(36800U)
#define ACCU_CHECK_PERIOD_SEC		5 // Time the accu will be checked!

volatile bool g_Adc16ConversionDoneFlag = false;
volatile uint32_t g_Adc16ConversionValue = 0;
adc16_channel_config_t g_adc16ChannelConfigStruct;
#endif

/* PIT */
#define PIT_ADC_HANDLER 	PIT_IRQHandler
#define PIT_IRQ_ID 			PIT_IRQn
/* Get source clock for PIT driver */
#define PIT_SOURCE_CLOCK CLOCK_GetFreq(kCLOCK_BusClk)
volatile bool pitIsrFlag = false;

/* I2C */
#define BOARD_I2C_BASEADDR I2C0

#define ACCEL_I2C_CLK_SRC 		I2C0_CLK_SRC
#define ACCEL_I2C_CLK_FREQ 		CLOCK_GetFreq(I2C0_CLK_SRC)

#define I2C_RELEASE_SDA_PORT 	PORTB
#define I2C_RELEASE_SCL_PORT 	PORTB
#define I2C_RELEASE_SDA_GPIO 	GPIOB
#define I2C_RELEASE_SDA_PIN 	1U
#define I2C_RELEASE_SCL_GPIO 	GPIOB
#define I2C_RELEASE_SCL_PIN 	0U
#define I2C_RELEASE_BUS_COUNT 	100U
#define I2C_BAUDRATE 			100000U

// Device ID's on board
#define MMA8452_WHOAMI 			0x2AU
#define MPL3115_WHOAMI			0xC4U


/*! Register according to datasheet MMA8452 */
typedef enum accel_register_t
{
	accel_state 			= 0x00U,	// State register (read only)
	accel_whoami			= 0x0DU,	// Device ID register address(read only)
	accel_transient_cfg		= 0x1DU,	// transient acceleration detection (read/write)
	accel_hp_filter			= 0x0FU,	// high-pass filter register (read/write)
	accel_xyz_data_cfg 		= 0x0EU,	// Data config (read/write)
	accel_ctrl_reg1			= 0x2AU,	// Control-register 1 (read/write)
	accel_ctrl_reg2			= 0x2BU,	// Control-register 2 (read/write)
	accel_ctrl_reg3			= 0x2CU,	// Control-register 3 (read/write)
	accel_ctrl_reg4			= 0x2DU,	// Control-register 4 (read/write)
	accel_ctrl_reg5			= 0x2EU,	// Control-register 5 (read/write)
} accel_register;

/* FXOS8700 and MMA8451 have the same who_am_i register address. */
#define ACCEL_READ_TIMES 		10U

const uint8_t slave_address[] = {0x1CU, 0x60U,  0x1DU, 0x1EU, 0x1FU};
i2c_master_handle_t g_m_handle;

uint8_t slave_I2C_addr_found = 0x00;
int16_t x, y, z;
uint8_t readBuffAccel[7];
uint8_t readBuffAltitude[4];

volatile bool completionFlag = false;
volatile bool nakFlag = false;

/* Device ID from other Accel sensors */
#define FXOS8700_WHOAMI 		0xC7U
#define MMA8451_WHOAMI 			0x1AU

/*! Register according to datasheet MMA8452 */
typedef enum altitude_register_t
{
	altitude_state 			= 0x00U,	// State register (read only)
	altitude_data_msb		= 0x01U,	// Register for Data output MSB (read only)
	altitude_data_csb		= 0x02U,	// Register for Data output CSB (read only)
	altitude_data_lsb		= 0x03U,	// Register for Data output LSB (read only)
	altitude_whoami			= 0x0CU,	// Device ID register address(read only)
	altitude_FIFO_setup		= 0x0FU,	// FIFO setup register (read only)
	altitude_int_source		= 0x12U,	// interrupt source register (read only)
	altitude_sensor_data	= 0x13U,	// Sensor data register (read only)
	altitude_ctrl_reg1		= 0x26U,	// Control-register 1 (read/write)
	altitude_ctrl_reg2		= 0x27U,	// Control-register 2 (read/write)
	altitude_ctrl_reg3		= 0x28U,	// Control-register 3 (read/write)
	altitude_ctrl_reg4		= 0x29U,	// Control-register 4 (read/write)
	altitude_ctrl_reg5		= 0x2AU		// Control-register 5 (read/write)
} altitude_register;

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

altitude_value altitude;

/*
 * @brief   Application entry point.
 */
/************************************************************************************
*************************************************************************************
* Private prototypes
*************************************************************************************
************************************************************************************/
#if (cPWR_UsePowerDownMode || gAppUseNvm_d)
#if (mAppIdleHook_c)
    #define AppIdle_TaskInit()
    #define App_Idle_Task()
#else
    static osaStatus_t AppIdle_TaskInit(void);
    static void App_Idle_Task(osaTaskParam_t argument);
#endif
#endif

#if gKeyBoardSupported_d && (gKBD_KeysCount_c > 0)   
static void App_KeyboardCallBack(uint8_t events);
#endif

static void App_Thread (uint32_t param);
static void App_HandleHostMessageInput(appMsgFromHost_t* pMsg);
static void App_GenericCallback (gapGenericEvent_t* pGenericEvent);
static void App_ConnectionCallback (deviceId_t peerDeviceId, gapConnectionEvent_t* pConnectionEvent);
static void App_AdvertisingCallback (gapAdvertisingEvent_t* pAdvertisingEvent);
static void App_ScanningCallback (gapScanningEvent_t* pAdvertisingEvent);
static void App_GattServerCallback (deviceId_t peerDeviceId, gattServerEvent_t* pServerEvent);
static void App_GattClientProcedureCallback
(
    deviceId_t              deviceId,
    gattProcedureType_t     procedureType,
    gattProcedureResult_t   procedureResult,
    bleResult_t             error
);
static void App_GattClientNotificationCallback
(
    deviceId_t      deviceId,
    uint16_t        characteristicValueHandle,
    uint8_t*        aValue,
    uint16_t        valueLength
);
static void App_GattClientIndicationCallback
(
    deviceId_t      deviceId,
    uint16_t        characteristicValueHandle,
    uint8_t*        aValue,
    uint16_t        valueLength
);

static void App_L2caLeDataCallback
(
    deviceId_t deviceId,
    uint16_t   lePsm,
    uint8_t* pPacket,
    uint16_t packetLength
);

static void App_L2caLeControlCallback
(
    l2capControlMessageType_t  messageType,
    void* pMessage
);

#if !gUseHciTransportDownward_d
static void BLE_SignalFromISRCallback(void);
#endif


/*! *********************************************************************************
*************************************************************************************
* Public prototypes
*************************************************************************************
********************************************************************************** */
extern void BleApp_Init(void);
extern void BleApp_HandleKeys(key_event_t events);

#if gUseHciTransportUpward_d
#define BleApp_GenericCallback(param)
#else
extern void BleApp_GenericCallback (gapGenericEvent_t* pGenericEvent);
#endif

#if !gUseHciTransportDownward_d
extern void (*pfBLE_SignalFromISR)(void);
#endif /* gUseHciTransportDownward_d */

/************************************************************************************
*************************************************************************************
* Private memory declarations
*************************************************************************************
************************************************************************************/
#if (cPWR_UsePowerDownMode || gAppUseNvm_d)
#if (!mAppIdleHook_c)
OSA_TASK_DEFINE( App_Idle_Task, gAppIdleTaskPriority_c, 1, gAppIdleTaskStackSize_c, FALSE );
osaTaskId_t gAppIdleTaskId = 0;
#endif
#endif  /* cPWR_UsePowerDownMode */

#if gAppUseNvm_d
static bleBondIdentityHeaderBlob_t*  aBondingHeader[gMaxBondedDevices_c];
static bleBondDataDynamicBlob_t*     aBondingDataDynamic[gMaxBondedDevices_c];
static bleBondDataStaticBlob_t*      aBondingDataStatic[gMaxBondedDevices_c];
static bleBondDataDeviceInfoBlob_t*  aBondingDataDeviceInfo[gMaxBondedDevices_c];
static bleBondDataDescriptorBlob_t* aBondingDataDescriptor[gMaxBondedDevices_c * gcGapMaximumSavedCccds_c];

NVM_RegisterDataSet(aBondingHeader, gMaxBondedDevices_c, gBleBondIdentityHeaderSize_c, nvmId_BondingHeaderId_c, gNVM_NotMirroredInRamAutoRestore_c);
NVM_RegisterDataSet(aBondingDataDynamic, gMaxBondedDevices_c, gBleBondDataDynamicSize_c, nvmId_BondingDataDynamicId_c, gNVM_NotMirroredInRamAutoRestore_c);
NVM_RegisterDataSet(aBondingDataStatic, gMaxBondedDevices_c, gBleBondDataStaticSize_c, nvmId_BondingDataStaticId_c, gNVM_NotMirroredInRamAutoRestore_c);
NVM_RegisterDataSet(aBondingDataDeviceInfo, gMaxBondedDevices_c, gBleBondDataDeviceInfoSize_c, nvmId_BondingDataDeviceInfoId_c, gNVM_NotMirroredInRamAutoRestore_c);
NVM_RegisterDataSet(aBondingDataDescriptor, gMaxBondedDevices_c * gcGapMaximumSavedCccds_c, gBleBondDataDescriptorSize_c, nvmId_BondingDataDescriptorId_c, gNVM_NotMirroredInRamAutoRestore_c);
#else
static bleBondDataBlob_t          maBondDataBlobs[gMaxBondedDevices_c] = {{{{0}}}};
#endif

static osaEventId_t  mAppEvent;

/* Application input queues */
static anchor_t mHostAppInputQueue;
static anchor_t mAppCbInputQueue;

static uint8_t platformInitialized = 0;

static gapGenericCallback_t pfGenericCallback = NULL;
static gapAdvertisingCallback_t pfAdvCallback = NULL;
static gapScanningCallback_t pfScanCallback = NULL;
static gapConnectionCallback_t  pfConnCallback = NULL;
static gattServerCallback_t pfGattServerCallback = NULL;
static gattClientProcedureCallback_t pfGattClientProcCallback = NULL;
static gattClientNotificationCallback_t pfGattClientNotifCallback = NULL;
static gattClientNotificationCallback_t pfGattClientIndCallback = NULL;
static l2caLeCbDataCallback_t           pfL2caLeCbDataCallback = NULL;
static l2caLeCbControlCallback_t        pfL2caLeCbControlCallback = NULL;

/************************************************************************************
*************************************************************************************
* Public memory declarations
*************************************************************************************
************************************************************************************/
#if gRNG_HWSupport_d == gRNG_NoHWSupport_d
extern uint32_t mRandomNumber;
#endif
extern const uint8_t gUseRtos_c;

/************************************************************************************
*************************************************************************************
* Public functions
*************************************************************************************
************************************************************************************/
/*! *********************************************************************************
* \brief  InitGPIO
* \remarks	initalizes buttons and LED's
********************************************************************************** */
void InitGPIO(void)
{
    gpio_pin_config_t ledConfig;
    ledConfig.pinDirection = kGPIO_DigitalOutput;
    ledConfig.outputLogic = 0;
    GPIO_PinInit(BOARD_INITPINS_LED_ORANGE_GPIO, BOARD_INITPINS_LED_ORANGE_GPIO_PIN, &ledConfig);
    GPIO_PinInit(BOARD_INITPINS_LED_GREEN_GPIO, BOARD_INITPINS_LED_GREEN_GPIO_PIN, &ledConfig);
    GPIO_PinInit(BOARD_INITPINS_LED_RED_GPIO, BOARD_INITPINS_LED_RED_GPIO_PIN, &ledConfig);
    ledConfig.pinDirection = kGPIO_DigitalInput;
    GPIO_PinInit(BOARD_INITBUTTONS_BUTTON_GPIO, BOARD_INITBUTTONS_BUTTON_GPIO_PIN, &ledConfig);
}
/*! *********************************************************************************
* \brief  InitServoPWM
*
* \remarks	Init for the PWM signal for the Servo. 50Hz Timer.
********************************************************************************** */
void InitServoPWM(void)
{
	 /* PWM Init */
	// Init von PWM
	CLOCK_SetTpmClock(1U);
	pwmLevel = kTPM_LowTrue;
	updatedDutycycle = 5;

	tpmParam.chnlNumber = (tpm_chnl_t) SERVO_TPM_CHANNEL;
	tpmParam.level= pwmLevel;
	tpmParam.dutyCyclePercent = updatedDutycycle;
	TPM_GetDefaultConfig(&tpmInfo);
	tpmInfo.prescale = kTPM_Prescale_Divide_128;
	tpmInfo.enableDebugMode = true;	// has to be set to true
	// Initialize TPM module
	TPM_Init(SERVO_TPM_BASEADDR, &tpmInfo);
	TPM_SetupPwm(SERVO_TPM_BASEADDR, &tpmParam, 1U,kTPM_EdgeAlignedPwm , SERVO_FREQUENCY, TPM_SOURCE_CLOCK);
	TPM_StartTimer(SERVO_TPM_BASEADDR, kTPM_SystemClock);
	TPM_UpdateChnlEdgeLevelSelect(SERVO_TPM_BASEADDR, SERVO_TPM_CHANNEL, kTPM_HighTrue);
}

/*! *********************************************************************************
* \brief  PWM_updateServo
* \remarks	The PWM of the Servo is updated and send to peripherie according to new bikespeed.
* In future, update should be made using the power value.
********************************************************************************** */
void PWM_updateServo()
{
	updatedDutycycle = bikeValues.bikeSpeed / 5U + 3U;
	if(updatedDutycycle > 2 && updatedDutycycle < 14)
	{
		TPM_UpdatePwmDutycycle(SERVO_TPM_BASEADDR, (tpm_chnl_t)SERVO_TPM_CHANNEL, kTPM_EdgeAlignedPwm, updatedDutycycle);
	}
}
/*! *********************************************************************************
* \brief  InitADC
*
* \remarks	The Analog and Digital conversion. 16Bit and no differential conversion.
* No interrupts, because conversion is triggered in PIT interrupt, because Software trigger
* is used.
********************************************************************************** */
#if BAT_MEASUREMENT_ENABLE
void InitADC(void)
{
	/* ADC INIT */
	adc16_config_t 				adc16ConfigStruct;
	/* Configure the ADC16. */
	/*
	 * adc16ConfigStruct.referenceVoltageSource = kADC16_ReferenceVoltageSourceVref;
	 * adc16ConfigStruct.clockSource = kADC16_ClockSourceAsynchronousClock;
	 * adc16ConfigStruct.enableAsynchronousClock = true;
	 * adc16ConfigStruct.clockDivider = kADC16_ClockDivider8;
	 * adc16ConfigStruct.resolution = kADC16_ResolutionSE12Bit;
	 * adc16ConfigStruct.longSampleMode = kADC16_LongSampleDisabled;
	 * adc16ConfigStruct.enableHighSpeed = false;
	 * adc16ConfigStruct.enableLowPower = false;
	 * adc16ConfigStruct.enableContinuousConversion = false;
	 */
	ADC16_GetDefaultConfig(&adc16ConfigStruct);
	adc16ConfigStruct.resolution = kADC16_Resolution16Bit;
	ADC16_Init(ACCU_ADC16_BASEADDR, &adc16ConfigStruct);

	/* Make sure the software trigger is used. */
	ADC16_EnableHardwareTrigger(ACCU_ADC16_BASEADDR, false);
#if defined(FSL_FEATURE_ADC16_HAS_CALIBRATION) && FSL_FEATURE_ADC16_HAS_CALIBRATION
	if (kStatus_Success == ADC16_DoAutoCalibration(ACCU_ADC16_BASEADDR))
	{
		GPIO_WritePinOutput(GPIOB, 18U, 1);
	}
#endif /* FSL_FEATURE_ADC16_HAS_CALIBRATION */

	/* Prepare ADC channel setting */
	g_adc16ChannelConfigStruct.channelNumber = ACCU_ADC16_USER_CHANNEL;
	g_adc16ChannelConfigStruct.enableInterruptOnConversionCompleted = false;
#if defined(FSL_FEATURE_ADC16_HAS_DIFF_MODE) && FSL_FEATURE_ADC16_HAS_DIFF_MODE
	g_adc16ChannelConfigStruct.enableDifferentialConversion = false;
#endif /* FSL_FEATURE_ADC16_HAS_DIFF_MODE */
}

/*! *********************************************************************************
* \brief  PIT_ADC_HANDLER
*
* \remarks	The interrupt subroutine which is called every couple of seconds.
* It starts the ADC conversion and sets or resets the orange LED.
*
********************************************************************************** */
void PIT_ADC_HANDLER(void)
{
    /* Clear interrupt flag */
    PIT_ClearStatusFlags(PIT, kPIT_Chnl_0, kPIT_TimerFlag);
    ADC16_SetChannelConfig(ACCU_ADC16_BASEADDR, ACCU_ADC16_CHANNEL_GROUP, &g_adc16ChannelConfigStruct); // Triggers Conversion

    while (kADC16_ChannelConversionDoneFlag != ADC16_GetChannelStatusFlags(ACCU_ADC16_BASEADDR, ACCU_ADC16_CHANNEL_GROUP))
    {
    }
    g_Adc16ConversionValue = ADC16_GetChannelConversionValue(ACCU_ADC16_BASEADDR, ACCU_ADC16_CHANNEL_GROUP);
    GPIO_WritePinOutput(BOARD_INITPINS_LED_ORANGE_GPIO, BOARD_INITPINS_LED_ORANGE_GPIO_PIN, 1);
    if(g_Adc16ConversionValue < (ACCU_UNDER_VOLTAGE))
	{
		GPIO_WritePinOutput(BOARD_INITPINS_LED_ORANGE_GPIO, BOARD_INITPINS_LED_ORANGE_GPIO_PIN, 0);
	}
}

/*! *********************************************************************************
* \brief  InitPIT
*
* \remarks	The Periodic interrupt timer is intialized: Ch0,every couple of second
*
********************************************************************************** */
void InitPIT(void)
{
	pit_config_t pitConfig;
	pitConfig.enableRunInDebug = true;
	/* Init pit module */
	PIT_Init(PIT, &pitConfig);
	/* Set timer period for channel 0 */
	PIT_SetTimerPeriod(PIT, kPIT_Chnl_0, USEC_TO_COUNT(ACCU_CHECK_PERIOD_SEC*1000000U, PIT_SOURCE_CLOCK));
	/* Enable timer interrupts for channel 0 */
	PIT_EnableInterrupts(PIT, kPIT_Chnl_0, kPIT_TimerInterruptEnable);
	/* Enable at the NVIC */
	EnableIRQ(PIT_IRQ_ID);
	PIT_StartTimer(PIT, kPIT_Chnl_0);
}
#endif
/*! *********************************************************************************
* \brief  i2c_release_bus_delay
*
* \remarks	Delay for I2C bus
********************************************************************************** */
static void i2c_release_bus_delay(void)
{
    uint32_t i = 0;
    for (i = 0; i < I2C_RELEASE_BUS_COUNT; i++)
    {
        __NOP();
    }
}
/*! *********************************************************************************
* \brief  BOARD_I2C_ReleaseBus
*
* \remarks	Sends start- and stop signals to I2C bus
********************************************************************************** */
void BOARD_I2C_ReleaseBus(void)
{
    uint8_t i = 0;
    gpio_pin_config_t pin_config;
    port_pin_config_t i2c_pin_config = {0};

    /* Config pin mux as gpio */
    i2c_pin_config.pullSelect = kPORT_PullUp; // onboard pullups
    i2c_pin_config.mux = kPORT_MuxAsGpio;

    pin_config.pinDirection = kGPIO_DigitalOutput;
    pin_config.outputLogic = 1U;
    CLOCK_EnableClock(kCLOCK_PortB);
    PORT_SetPinConfig(I2C_RELEASE_SCL_PORT, I2C_RELEASE_SCL_PIN, &i2c_pin_config);
    PORT_SetPinConfig(I2C_RELEASE_SCL_PORT, I2C_RELEASE_SDA_PIN, &i2c_pin_config);

    GPIO_PinInit(I2C_RELEASE_SCL_GPIO, I2C_RELEASE_SCL_PIN, &pin_config);
    GPIO_PinInit(I2C_RELEASE_SDA_GPIO, I2C_RELEASE_SDA_PIN, &pin_config);

    /* Drive SDA low first to simulate a start */
    GPIO_WritePinOutput(I2C_RELEASE_SDA_GPIO, I2C_RELEASE_SDA_PIN, 0U);
    i2c_release_bus_delay();

    /* Send 9 pulses on SCL and keep SDA low */
    for (i = 0; i < 9; i++)
    {
        GPIO_WritePinOutput(I2C_RELEASE_SCL_GPIO, I2C_RELEASE_SCL_PIN, 0U);
        i2c_release_bus_delay();

        GPIO_WritePinOutput(I2C_RELEASE_SDA_GPIO, I2C_RELEASE_SDA_PIN, 1U);
        i2c_release_bus_delay();

        GPIO_WritePinOutput(I2C_RELEASE_SCL_GPIO, I2C_RELEASE_SCL_PIN, 1U);
        i2c_release_bus_delay();
        i2c_release_bus_delay();
    }

    /* Send stop */
    GPIO_WritePinOutput(I2C_RELEASE_SCL_GPIO, I2C_RELEASE_SCL_PIN, 0U);
    i2c_release_bus_delay();

    GPIO_WritePinOutput(I2C_RELEASE_SDA_GPIO, I2C_RELEASE_SDA_PIN, 0U);
    i2c_release_bus_delay();

    GPIO_WritePinOutput(I2C_RELEASE_SCL_GPIO, I2C_RELEASE_SCL_PIN, 1U);
    i2c_release_bus_delay();

    GPIO_WritePinOutput(I2C_RELEASE_SDA_GPIO, I2C_RELEASE_SDA_PIN, 1U);
    i2c_release_bus_delay();
}
/*! *********************************************************************************
* \brief  I2C_WriteRegister
* \param[in]	base			I2C bus
* \param[in]	device_addr		Device addr of Slave
* \param[in]	reg_addr		Register to address
* \param[in]	value			Data value which is sent to Register
* \return		True: Successfully write to device. Otherwise false.
* \remarks	Writes a databyte the a register
********************************************************************************** */
static bool I2C_WriteRegister(I2C_Type *base, uint8_t device_addr, uint8_t reg_addr, uint8_t value)
{
    i2c_master_transfer_t masterXfer;
    memset(&masterXfer, 0, sizeof(masterXfer));

    masterXfer.slaveAddress = device_addr;
    masterXfer.direction = kI2C_Write;
    masterXfer.subaddress = reg_addr;
    masterXfer.subaddressSize = 1;
    masterXfer.data = &value;
    masterXfer.dataSize = 1;
    masterXfer.flags = kI2C_TransferDefaultFlag;

    /*  direction=write : start+device_write;cmdbuff;xBuff; */
    /*  direction=receive : start+device_write;cmdbuff;repeatStart+device_read;xBuff; */
    I2C_MasterTransferNonBlocking(BOARD_I2C_BASEADDR, &g_m_handle, &masterXfer);

    /*  wait for transfer completed. */
    while ((!nakFlag) && (!completionFlag))
    {
    }

    nakFlag = false;
    if (completionFlag == true)
    {
        completionFlag = false;
        return true;
    }
    else
    {
        return false;
    }
}
/*! *********************************************************************************
* \brief  I2C_ReadRegister
* \param[in]	base			I2C bus
* \param[in]	device_addr		Device addr of Slave
* \param[in]	reg_addr		Register to address
* \param[out]	rxBuff			Buffer for received data
* \param[in]	rxSize			Size of the buffer
* \return		True: Successfully Read from device. Otherwise false.
* \remarks		Receives one or several data bytes from Slave
********************************************************************************** */
static bool I2C_ReadRegister(I2C_Type *base, uint8_t device_addr, uint8_t reg_addr, uint8_t *rxBuff, uint32_t rxSize)
{
    i2c_master_transfer_t masterXfer;
    memset(&masterXfer, 0, sizeof(masterXfer));
    masterXfer.slaveAddress = device_addr;
    masterXfer.direction = kI2C_Read;
    masterXfer.subaddress = reg_addr;
    masterXfer.subaddressSize = 1;
    masterXfer.data = rxBuff;
    masterXfer.dataSize = rxSize;
    masterXfer.flags = kI2C_TransferDefaultFlag;

    /*  direction=write : start+device_write;cmdbuff;xBuff; */
    /*  direction=recive : start+device_write;cmdbuff;repeatStart+device_read;xBuff; */

    I2C_MasterTransferNonBlocking(BOARD_I2C_BASEADDR, &g_m_handle, &masterXfer);
    /*  wait for transfer completed. */
    while ((!nakFlag) && (!completionFlag))
    {
    }

    nakFlag = false;
    if (completionFlag == true)
    {
        completionFlag = false;
        return true;
    }
    else
    {
        return false;
    }
}
/*! *********************************************************************************
* \brief  i2c_master_callback
* \param[in]	base	I2C bus
* \param[in]	handle	handle of I2C
* \param[in]	status	states of the signal
* \param[in]	data 	sent/received. May be NULL
* \remarks	Sends start- and stop signals to I2C bus
********************************************************************************** */
static void i2c_master_callback(I2C_Type *base, i2c_master_handle_t *handle, status_t status, void *userData)
{
    /* Signal transfer success when received success status. */
    if (status == kStatus_Success)
    {
        completionFlag = true;
    }
    /* Signal transfer success when received success status. */
    if ((status == kStatus_I2C_Nak) || (status == kStatus_I2C_Addr_Nak))
    {
        nakFlag = true;
    }
}

/*! *********************************************************************************
* \brief  I2C_ReadAccelWhoAmI
* \return		True: Successfully read a device. Otherwise false.
* \remarks	Checks if accelsensor available with set Device ID.
********************************************************************************** */
static bool I2C_ReadAccelWhoAmI(void)
{
    /*
	How to read the device who_am_I value ?
	Start + Device_address_Write , who_am_I_register;
	Repeart_Start + Device_address_Read , who_am_I_value.
	*/
	uint8_t who_am_i_reg = accel_whoami;
	uint8_t who_am_i_value = 0x00;
	uint8_t accel_addr_array_size = 0x00;
	bool find_device = false;
	uint8_t i = 0;
	uint32_t sourceClock = 0;

	i2c_master_config_t masterConfig;

	/*
	 * masterConfig.baudRate_Bps = 100000U;
	 * masterConfig.enableStopHold = false;
	 * masterConfig.glitchFilterWidth = 0U;
	 * masterConfig.enableMaster = true;
	 */
	/* Init only once per bus */
	I2C_MasterGetDefaultConfig(&masterConfig);
	masterConfig.baudRate_Bps = I2C_BAUDRATE;
	sourceClock = ACCEL_I2C_CLK_FREQ;
	I2C_MasterInit(BOARD_I2C_BASEADDR, &masterConfig, sourceClock);

	i2c_master_transfer_t masterXfer;
	memset(&masterXfer, 0, sizeof(masterXfer));


	masterXfer.slaveAddress = slave_address[0];
	masterXfer.direction = kI2C_Write;
	masterXfer.subaddress = 0;
	masterXfer.subaddressSize = 0;
	masterXfer.data = &who_am_i_reg;
	masterXfer.dataSize = 1;
	masterXfer.flags = kI2C_TransferNoStopFlag;


	accel_addr_array_size = sizeof(slave_address) / sizeof(slave_address[0]);

	for (i = 0; i < accel_addr_array_size; i++)
	{
		masterXfer.slaveAddress = slave_address[i];
		I2C_MasterTransferNonBlocking(BOARD_I2C_BASEADDR, &g_m_handle, &masterXfer);
		/*  wait for transfer completed. */
		while ((!nakFlag) && (!completionFlag))
		{
		}

		nakFlag = false;

		if (completionFlag == true)
		{
			completionFlag = false;
			find_device = true;
			slave_I2C_addr_found = masterXfer.slaveAddress;
			break;
		}
	}

	if (find_device == true)
	{
		masterXfer.direction = kI2C_Read;
		masterXfer.subaddress = 0;
		masterXfer.subaddressSize = 0;
		masterXfer.data = &who_am_i_value;
		masterXfer.dataSize = 1;
		masterXfer.flags = kI2C_TransferRepeatedStartFlag;

		I2C_MasterTransferNonBlocking(BOARD_I2C_BASEADDR, &g_m_handle, &masterXfer);

		/*  wait for transfer completed. */
		while ((!nakFlag) && (!completionFlag))
		{
		}
		nakFlag = false;
		if (completionFlag == true)
		{
			completionFlag = false;
			if (who_am_i_value == MMA8452_WHOAMI)
			{
				GPIO_WritePinOutput(BOARD_INITPINS_LED_GREEN_GPIO, BOARD_INITPINS_LED_GREEN_GPIO_PIN, 1);
				return true;
			}
			if (who_am_i_value == MPL3115_WHOAMI)
			{
				GPIO_WritePinOutput(BOARD_INITPINS_LED_RED_GPIO, BOARD_INITPINS_LED_RED_GPIO_PIN, 1);
				return true;
			}
		}
	}
}

/*! *********************************************************************************
* \brief  I2C_ReadAltitudeWhoAmI
* \return		True: Successfully read a device. Otherwise false.
* \remarks	Checks if Altitude sensor available with set Device ID.
********************************************************************************** */
static bool I2C_ReadAltitudeWhoAmI(void)
{
    /*
	How to read the device who_am_I value ?
	Start + Device_address_Write , who_am_I_register;
	Repeart_Start + Device_address_Read , who_am_I_value.
	*/
	uint8_t who_am_i_reg = altitude_whoami;
	uint8_t who_am_i_value = 0x00;
	uint8_t accel_addr_array_size = 0x00;
	bool find_device = false;
	uint8_t i = 0;


	/*
	 * masterConfig.baudRate_Bps = 100000U;
	 * masterConfig.enableStopHold = false;
	 * masterConfig.glitchFilterWidth = 0U;
	 * masterConfig.enableMaster = true;
	 */
	/* Init only once per bus */

	i2c_master_transfer_t masterXfer;
	memset(&masterXfer, 0, sizeof(masterXfer));

	masterXfer.slaveAddress = slave_address[1];
	masterXfer.direction = kI2C_Write;
	masterXfer.subaddress = 0;
	masterXfer.subaddressSize = 0;
	masterXfer.data = &who_am_i_reg;
	masterXfer.dataSize = 1;
	masterXfer.flags = kI2C_TransferNoStopFlag;

	accel_addr_array_size = sizeof(slave_address) / sizeof(slave_address[1]);

	for (i = 1; i < accel_addr_array_size; i++)
	{
		masterXfer.slaveAddress = slave_address[i];
		I2C_MasterTransferNonBlocking(BOARD_I2C_BASEADDR, &g_m_handle, &masterXfer);
		/*  wait for transfer completed. */
		while ((!nakFlag) && (!completionFlag))
		{
		}
		nakFlag = false;
		if (completionFlag == true)
		{
			completionFlag = false;
			find_device = true;
			slave_I2C_addr_found = masterXfer.slaveAddress;
			break;
		}
	}

	if (find_device == true)
	{
		masterXfer.direction = kI2C_Read;
		masterXfer.subaddress = 0;
		masterXfer.subaddressSize = 0;
		masterXfer.data = &who_am_i_value;
		masterXfer.dataSize = 1;
		masterXfer.flags = kI2C_TransferRepeatedStartFlag;

		I2C_MasterTransferNonBlocking(BOARD_I2C_BASEADDR, &g_m_handle, &masterXfer);

		/*  wait for transfer completed. */
		while ((!nakFlag) && (!completionFlag))
		{
		}
		nakFlag = false;
		if (completionFlag == true)
		{
			completionFlag = false;
			if (who_am_i_value == MMA8452_WHOAMI)
			{
				GPIO_WritePinOutput(BOARD_INITPINS_LED_GREEN_GPIO, BOARD_INITPINS_LED_GREEN_GPIO_PIN, 1);
				return true;
			}
			if (who_am_i_value == MPL3115_WHOAMI)
			{
				GPIO_WritePinOutput(BOARD_INITPINS_LED_RED_GPIO, BOARD_INITPINS_LED_RED_GPIO_PIN, 1);
				return true;
			}
		}
	}
}

/*! *********************************************************************************
* \brief  I2C_Accel_Config
* \remarks		Writes settings to I2C Slave
* 				Further informatin in datashset MMA8452
********************************************************************************** */
void I2C_Accel_Config(void)
{
	bool isThereAccel = false;
	I2C_MasterTransferCreateHandle(BOARD_I2C_BASEADDR, &g_m_handle, i2c_master_callback, NULL);
	isThereAccel = I2C_ReadAccelWhoAmI();
    /*  read the accel xyz value if there is accel device on board */
    if (isThereAccel)
    {
        uint8_t databyte = 0;
        uint8_t write_reg = 0;
        uint8_t status0_value = 0;
        uint32_t i = 0U;

        /*  SET 0 TO SET TO STANDBY MODE */
        /*  write 0000 0000 = 0x00 to accelerometer control register 1 */
        /*  standby */
        /*  [7-1] = 0000 000 */
        /*  [0]: active=0 */
        write_reg = accel_ctrl_reg1;
        databyte = 0;
        I2C_WriteRegister(BOARD_I2C_BASEADDR, slave_I2C_addr_found, write_reg, databyte);

        /*  write 0000 0000 = 0x00 to XYZ_DATA_CFG register */
        /*  [7]: reserved */
        /*  [6]: reserved */
        /*  [5]: reserved */
        /*  [4]: hpf_out=0 */
        /*  [3]: reserved */
        /*  [2]: reserved */
        /*  [1-0]: fs=00 for accelerometer range of +/-2g range with 0.488mg/LSB */
        /*  databyte = 0x00; */
        write_reg = accel_xyz_data_cfg;
        databyte = 0x00;
        I2C_WriteRegister(BOARD_I2C_BASEADDR, slave_I2C_addr_found, write_reg, databyte);

        /*  write 0000 0000 = 0x00 to accelerometer control register 2 */
	    /*  [7]: ST=0 for Self-test disabled */
	    /*  [6]: RST=0 for Softwarereset disabled */
	    /*  [5]: Reserved */
	    /*  [4-3]: SMODS=00 for sleep pwer mode disabled */
        /*  [2]: SLPE=0 for Auto-sleep mode disabled */
	    /*  [1-0]: MODS=00 for Normal power mode */
	    /*  databyte = 0xFD; */
	    write_reg = accel_ctrl_reg2;
	    databyte = 0x00;
	    I2C_WriteRegister(BOARD_I2C_BASEADDR, slave_I2C_addr_found, write_reg, databyte);

        /*  write 0000 0000 = 0x00 to accelerometer control register 4 */
	    /*  [7]: INT_EN_ASLP=0 for Auto-sleep interrupt disabled */
	    /*  [6]: Reserved */
	    /*  [5]: INT_EN_TRANS=0 for Transient interrupt disabled */
	    /*  [4]: INT_EN_LNDPRT=0 for orientation interrupt disabled */
	    /*  [3]: INT_EN_PULSE=0 for pulse detection interrupt disabled */
        /*  [2]: INT_CFG_FF_MT=0 for Freefall/Motion interrupt disabled */
	    /*  [1]: Reserved */
	    /*  [0]: INT_EN_DRDY=0 for data-ready interrupt disabled */
	    /*  databyte = 0x00; */
	    write_reg = accel_ctrl_reg4;
	    databyte = 0x00;
	    I2C_WriteRegister(BOARD_I2C_BASEADDR, slave_I2C_addr_found, write_reg, databyte);

	    /*  write 0000 0000 = 0x00 to accelerometer control register 5 */
		/*  [7]: INT_CFG_ASLP=0 for Auto-sleep interrupt to INT2 */
		/*  [6]: Reserved */
		/*  [5]: INT_EN_TRANS=0 for Transient interrupt to INT2 */
		/*  [4]: INT_EN_LNDPRT=0 for orientation interrupt to INT2 */
		/*  [3]: INT_EN_PULSE=0 for pulse detection interrupt to INT2 */
		/*  [2]: INT_CFG_FF_MT=0 for Freefall/Motion interrupt to INT2 */
		/*  [1]: Reserved */
		/*  [0]: INT_EN_DRDY=0 for data-ready interrupt to INT2 */
		/*  databyte = 0x00; */
		write_reg = accel_ctrl_reg5;
		databyte = 0x00;
		I2C_WriteRegister(BOARD_I2C_BASEADDR, slave_I2C_addr_found, write_reg, databyte);

        /*  write 1111 1101 = 0xFD to accelerometer control register 1 */
        /*  [7-6]: aslp_rate=11 for 1.56 Hz ODR when Sleep mode */
        /*  [5-3]: dr=111 for 1.56Hz data rate (when in hybrid mode) */
        /*  [2]: lnoise=1 for low noise mode */
        /*  [1]: f_read=0 for normal 16 bit reads */
        /*  [0]: active=1 active and enable sampling */
        /*  databyte = 0xFD; */
        write_reg = accel_ctrl_reg1;
        databyte = 0xFD;
        I2C_WriteRegister(BOARD_I2C_BASEADDR, slave_I2C_addr_found, write_reg, databyte);

    }
}
/*! *********************************************************************************
* \brief  I2C_Altitude_Config
* \remarks		Writes settings to I2C Slave
* 				Further information in datashset MPL3115
********************************************************************************** */
void I2C_Altitude_Config(void)
{
	bool isThereAltitude = false;
	I2C_MasterTransferCreateHandle(BOARD_I2C_BASEADDR, &g_m_handle, i2c_master_callback, NULL);
	isThereAltitude = I2C_ReadAltitudeWhoAmI();
    /*  read the accel xyz value if there is accel device on board */
    if (isThereAltitude)
    {
        uint8_t databyte = 0;
        uint8_t write_reg = 0;
        uint8_t status0_value = 0;
        uint32_t i = 0U;

        /*  SET 0 TO SET TO STANDBY MODE */
        /*  write 0000 0000 = 0x00 to altimeter control register 1 */
        /*  standby */
        /*  [7-1] = 0000 000 */
        /*  [0]: active=0 */
        write_reg = altitude_ctrl_reg1;
        databyte = 0;
        I2C_WriteRegister(BOARD_I2C_BASEADDR, slave_I2C_addr_found, write_reg, databyte);

        /*  write 0010 0000 = 0x20 to altimeter sensor data register */
		/*  [7-3] = 0000 000 */
		/*  [2]: DREM=0 for data ready event flag disable */
        /*  [1]: PDEFE=1 for data ready event flag enable */
        /*  [0]: TDEFE=0 for data ready event flag disable */
        /*  databyte = 0x02; */
		write_reg = altitude_sensor_data;
		databyte = 0x02;
		I2C_WriteRegister(BOARD_I2C_BASEADDR, slave_I2C_addr_found, write_reg, databyte);

        /*  write 0010 0000 = 0x20 to altimeter control register 2 */
	    /*  [7-6]: Reserved */
	    /*  [5]: LOAD_OUTPUT=1 for loading OUT as target values */
	    /*  [4]: Reserved */
	    /*  [3-0]: ST=00 for autoacquisition time step = 1s */
	    /*  databyte = 0x20; */
	    write_reg = altitude_ctrl_reg2;
	    databyte = 0x20;
	    I2C_WriteRegister(BOARD_I2C_BASEADDR, slave_I2C_addr_found, write_reg, databyte);

	    /*  write 0010 0010 = 0x22 to altimeter control register 3 */
		/*  [7-6]: Reserved */
		/*  [5]: IPOL1=1 for interrupt polarity = Active high */
		/*  [4]: PP_OD1=0 for internal pullup enabled */
		/*  [3-2]: Reserved */
	    /*  [1]: IPOL2=1 for interrupt polarity = Active high */
	    /*  [0]: PP_OD2=0 for internal pullup enabled */
		/*  databyte = 0x22; */
		write_reg = altitude_ctrl_reg3;
		databyte = 0x22;
		I2C_WriteRegister(BOARD_I2C_BASEADDR, slave_I2C_addr_found, write_reg, databyte);

		/*  write 0000 0000 = 0x00 to altimeter control register 4 */
		/*  [7]: INT_EN_DRDY=0 for data ready interrupt disabled */
		/*  [6]: INT_EN_FIFO=0 for FIFO interrupt disabled */
		/*  [5]: INT_EN_PW=0 for pressure window interrupt disabled */
		/*  [4]: INT_EN_TW=0 for temp window interrupt disabled */
		/*  [3]: INT_EN_PTH=0 for pressure thresh interrupt disabled */
		/*  [2]: INT_EN_TTH=0 for temp thresh interrupt disabled */
		/*  [1]: INT_EN_PCHG=0 for pressure change interrupt disabled */
		/*  [0]: INT_EN_TCHG=0 for temp change interrupt disabled */
		/*  databyte = 0x00; */
		write_reg = altitude_ctrl_reg4;
		databyte = 0x00;
		I2C_WriteRegister(BOARD_I2C_BASEADDR, slave_I2C_addr_found, write_reg, databyte);

		/*  write 0000 0000 = 0x00 to altimeter control register 5 */
		/*  [7]: INT_CFG_DRDY=0 for data-ready interrupt to INT2 */
		/*  [6]: INT_CFG_FIFO=0 for FIFO interrupt to INT2 */
		/*  [5]: INT_CFG_PW=0 for pressure window interrupt to INT2 */
		/*  [4]: INT_CFG_TW=0 for temp window interrupt to INT2 */
		/*  [3]: INT_CFG_PTH=0 for pressure thresh interrupt to INT2 */
		/*  [2]: INT_CFG_TTH=0 for temp thresh interrupt to INT2 */
		/*  [1]: INT_CFG_PCHG=0 for pressure change interrupt to INT2 */
		/*  [0]: INT_CFG_TCHG=0 for temp change interrupt to INT2 */
		/*  databyte = 0x00; */
		write_reg = altitude_ctrl_reg5;
		databyte = 0x00;
		I2C_WriteRegister(BOARD_I2C_BASEADDR, slave_I2C_addr_found, write_reg, databyte);

	    /*  write 1000 0011 = 0x00 to altimeter control register 1 */
		/*  [7]: ALT=1 for for altimeter mode */
		/*  [6]: RAW=0 for NOT RAW mode */
		/*  [5-3]: OS=000 for Oversampling = 1 */
		/*  [2]: RST=0 for device reset disabled */
		/*  [1]: OST=1 for initating a measurement */
		/*  [0]: SBYB=1 for active mode */
		/*  databyte = 0x83; */
		write_reg = altitude_ctrl_reg1;
		databyte = 0x83;
		I2C_WriteRegister(BOARD_I2C_BASEADDR, slave_I2C_addr_found, write_reg, databyte);

	    for (i = 0; i < ACCEL_READ_TIMES; i++)
		{
			status0_value = 0;
			/*  wait for new data are ready. */
			while ((status0_value & (1<<2)) != 0x04)
			{
				I2C_ReadRegister(BOARD_I2C_BASEADDR, slave_I2C_addr_found, altitude_state, &status0_value, 1);
			}

			/*  Multiple-byte Read from STATUS (0x00) register */
			I2C_ReadRegister(BOARD_I2C_BASEADDR, slave_I2C_addr_found, altitude_state, readBuffAltitude, 4);

			status0_value = readBuffAltitude[0];
			altitude.altitudeInteger = ((int16_t)(((readBuffAltitude[1] * 256U) | readBuffAltitude[2])));
			altitude.altitudeFraction = readBuffAltitude[3] & 0xF0U;
			altitude.altitudeInteger = ((int16_t)(((readBuffAltitude[1] * 256U) | readBuffAltitude[2])));

            /* Clear OST-Bit again, because auto-clear is not avaible */
            /* To initiate another measurement -> set OST Bit = 1 */
			write_reg = altitude_ctrl_reg1;
			databyte = 0x81;
			I2C_WriteRegister(BOARD_I2C_BASEADDR, slave_I2C_addr_found, write_reg, databyte);

		}

    }
}
/*! *********************************************************************************
* \brief  I2C_getAccel
* \remarks		Gets the accelsensor values x,y,z from MMA8452 according to settings of config
* 				uses polling and waites until data is ready.
* 				Further information in datasheet MMA8452
********************************************************************************** */
void I2C_updateAccel()
{
	/*  Multiple-byte Read from STATUS (0x00) register */
	I2C_ReadRegister(BOARD_I2C_BASEADDR, slave_address[0], accel_state, readBuffAccel, 7);
	//status0_value = readBuff[0];
	x = ((int16_t)(((readBuffAccel[1] * 256U) | readBuffAccel[2]))) / 4U;
	y = ((int16_t)(((readBuffAccel[3] * 256U) | readBuffAccel[4]))) / 4U;
	z = ((int16_t)(((readBuffAccel[5] * 256U) | readBuffAccel[6]))) / 4U;
}

/*! *********************************************************************************
* \brief  I2C_updateAltitude
* \remarks		Gets the altitude from MPL3115 according to settings of config
* 				uses polling and waites until data is ready.
* 				Further information in datashset MPL3115
********************************************************************************** */
void I2C_updateAltitude()
{
	uint8_t status0_value = 0;
	// Request altitude values
	I2C_WriteRegister(BOARD_I2C_BASEADDR, slave_address[1], altitude_ctrl_reg1, 0x83U);
	/*  wait for new data are ready. */
	while ((status0_value & (1<<2)) != 0x04)
	{
		I2C_ReadRegister(BOARD_I2C_BASEADDR, slave_address[1], altitude_state, &status0_value, 1);
	}

	/*  Multiple-byte Read from STATUS (0x00) register */
	I2C_ReadRegister(BOARD_I2C_BASEADDR, slave_address[1], altitude_state, readBuffAltitude, 4);
	altitude.altitudeInteger = ((int16_t)(((readBuffAltitude[1] * 256U) | readBuffAltitude[2])));
	altitude.altitudeFraction = readBuffAltitude[3] & 0xF0U;
	altitude.altitudeInteger = ((int16_t)(((readBuffAltitude[1] * 256U) | readBuffAltitude[2])));

	/* Clear OST-Bit again, because auto-clear is not avaible */
	/* To initiate another measurement -> set OST Bit = 1 */
	I2C_WriteRegister(BOARD_I2C_BASEADDR, slave_address[1], altitude_ctrl_reg1, 0x81U);
}

/*! *********************************************************************************
* \brief  This is the first task created by the OS. This task will initialize 
*         the system
*
* \param[in]  param
*
********************************************************************************** */
void main_task(uint32_t param)
{  
    if (!platformInitialized)
    {
        uint8_t pseudoRNGSeed[20] = {0};
        platformInitialized = 1;
        hardware_init();
        /* Framework init */
        MEM_Init();
        TMR_Init();       
        LED_Init();
        SecLib_Init();
        BOARD_InitPins();
        //BOARD_InitRGB();
        BOARD_BootClockRUN();

        // Init GPIO pins
        InitGPIO();

        // I2C init and configuration
        BOARD_I2C_ReleaseBus();
        BOARD_I2C_InitPins();
        I2C_Accel_Config();
        I2C_Altitude_Config();

        CLOCK_SetTpmClock(1U);
        /* Init board hardware. */
		BOARD_InitBootPins();
		BOARD_InitButtons();
		BOARD_InitBootClocks();

        // Initialization for peripherals
        InitServoPWM();

#if BAT_MEASUREMENT_ENABLE
        InitADC();
        InitPIT();
#endif

        /* Testing purposes */

        RNG_Init();   
        RNG_GetRandomNo((uint32_t*)(&(pseudoRNGSeed[0])));
        RNG_GetRandomNo((uint32_t*)(&(pseudoRNGSeed[4])));
        RNG_GetRandomNo((uint32_t*)(&(pseudoRNGSeed[8])));
        RNG_GetRandomNo((uint32_t*)(&(pseudoRNGSeed[12])));
        RNG_GetRandomNo((uint32_t*)(&(pseudoRNGSeed[16])));
        RNG_SetPseudoRandomNoSeed(pseudoRNGSeed);
        
#if gKeyBoardSupported_d && (gKBD_KeysCount_c > 0)        
        KBD_Init(App_KeyboardCallBack);
#endif

#if gAppUseNvm_d        
        /* Initialize NV module */
        NvModuleInit();
#endif
        
#if !gUseHciTransportDownward_d
        pfBLE_SignalFromISR = BLE_SignalFromISRCallback;        
#endif /* !gUseHciTransportDownward_d */
        
//#if (cPWR_UsePowerDownMode || gAppUseNvm_d)
//#if (!mAppIdleHook_c)
//        AppIdle_TaskInit();
//#endif
//#endif
//#if (cPWR_UsePowerDownMode)
//        PWR_Init();
//        PWR_DisallowDeviceToSleep();
//#else
//        Led1Flashing();
//        Led2Flashing();
//        Led3Flashing();
//        Led4Flashing();
//#endif
       
        /* Initialize peripheral drivers specific to the application */
        //BleApp_Init();		// Second Init from ADC
            
        /* Create application event */
        mAppEvent = OSA_EventCreate(TRUE);
        if( NULL == mAppEvent )
        {
            panic(0,0,0,0);
            return;
        }

        /* Prepare application input queue.*/
        MSG_InitQueue(&mHostAppInputQueue);
        
        /* Prepare callback input queue.*/
        MSG_InitQueue(&mAppCbInputQueue);
        
        /* BLE Host Stack Init */
        if (Ble_Initialize(App_GenericCallback) != gBleSuccess_c)
        {
            panic(0,0,0,0);
            return;
        }
    }

    /* Call application task */
    App_Thread( param );
}

/*! *********************************************************************************
* \brief  This function represents the Application task. 
*         This task reuses the stack alocated for the MainThread.
*         This function is called to process all events for the task. Events 
*         include timers, messages and any other user defined events.
* \param[in]  argument
*
********************************************************************************** */
void App_Thread (uint32_t param)
{
    osaEventFlags_t event;

    while(1)
    { 
        OSA_EventWait(mAppEvent, osaEventFlagsAll_c, FALSE, osaWaitForever_c , &event);
        
        /* Dequeue the host to app message */
        if (event & gAppEvtMsgFromHostStack_c)
        {
            /* Pointer for storing the messages from host. */
            appMsgFromHost_t *pMsgIn = NULL;  
            
            /* Check for existing messages in queue */
            while(MSG_Pending(&mHostAppInputQueue))
            {
                pMsgIn = MSG_DeQueue(&mHostAppInputQueue);
            
                if (pMsgIn)
                {
                    /* Process it */
                    App_HandleHostMessageInput(pMsgIn);
                    
                    /* Messages must always be freed. */
                    MSG_Free(pMsgIn);
                    pMsgIn = NULL;
                }
            }
        }
        
        /* Dequeue the callback message */
        if (event & gAppEvtAppCallback_c)
        {
            /* Pointer for storing the callback messages. */
            appMsgCallback_t *pMsgIn = NULL;  
            
            /* Check for existing messages in queue */
            while(MSG_Pending(&mAppCbInputQueue))
            {
                pMsgIn = MSG_DeQueue(&mAppCbInputQueue);
            
                if (pMsgIn)
                {
                    /* Execute callback handler */
                    if (pMsgIn->handler)
                    {
                        pMsgIn->handler (pMsgIn->param);
                    }
                    
                    /* Messages must always be freed. */
                    MSG_Free(pMsgIn);
                    pMsgIn = NULL;
                }
            }
        }

        /* For BareMetal break the while(1) after 1 run */
        if( gUseRtos_c == 0 )
        {
            break;
        }
    }
}

#if (cPWR_UsePowerDownMode)

static void App_Idle(void)
{
    PWRLib_WakeupReason_t wakeupReason;
    
    if( PWR_CheckIfDeviceCanGoToSleep() )
    {
        /* Enter Low Power */
        wakeupReason = PWR_EnterLowPower();

#if gFSCI_IncludeLpmCommands_c
        /* Send Wake Up indication to FSCI */
        FSCI_SendWakeUpIndication();
#endif

#if gKBD_KeysCount_c > 0      
        /* Woke up on Keyboard Press */
        if(wakeupReason.Bits.FromKeyBoard)
        {
            KBD_SwitchPressedOnWakeUp();
            PWR_DisallowDeviceToSleep();
        }
#endif   
    }
    else
    {
        /* Enter MCU Sleep */
        PWR_EnterSleep(); 
    }
}

#endif /* cPWR_UsePowerDownMode */

#if (mAppIdleHook_c)

void vApplicationIdleHook(void)
{
#if (gAppUseNvm_d)
    NvIdle();
#endif
#if (cPWR_UsePowerDownMode)
    App_Idle();  
#endif  
}

#else /* mAppIdleHook_c */

#if (cPWR_UsePowerDownMode || gAppUseNvm_d)
static void App_Idle_Task(osaTaskParam_t argument)
{
    while(1)
    {   
#if gAppUseNvm_d
        NvIdle();
#endif
        
#if (cPWR_UsePowerDownMode)
        App_Idle();
#endif
 
        /* For BareMetal break the while(1) after 1 run */
        if (gUseRtos_c == 0)
        {
            break;
        }
    }
}

static osaStatus_t AppIdle_TaskInit(void)
{	     
    if(gAppIdleTaskId)
    {      
        return osaStatus_Error;
    }
   
    /* Task creation */
    gAppIdleTaskId = OSA_TaskCreate(OSA_TASK(App_Idle_Task), NULL);
    
    if( NULL == gAppIdleTaskId )
    {
        panic(0,0,0,0);
        return osaStatus_Error;
    }

    return osaStatus_Success;
}
#endif /* cPWR_UsePowerDownMode */

#endif /* mAppIdleHook_c */

bleResult_t App_Connect(
    gapConnectionRequestParameters_t*   pParameters,
    gapConnectionCallback_t             connCallback
)
{
    pfConnCallback = connCallback;
    
    return Gap_Connect(pParameters, App_ConnectionCallback);
}

bleResult_t App_StartAdvertising(
    gapAdvertisingCallback_t    advertisingCallback,
    gapConnectionCallback_t     connectionCallback
)
{
    pfAdvCallback = advertisingCallback;
    pfConnCallback = connectionCallback;
    
    return Gap_StartAdvertising(App_AdvertisingCallback, App_ConnectionCallback);
}

bleResult_t App_StartScanning(
    gapScanningParameters_t*    pScanningParameters,
    gapScanningCallback_t       scanningCallback
)
{
    pfScanCallback = scanningCallback;
    
    return Gap_StartScanning(pScanningParameters, App_ScanningCallback);
}

bleResult_t App_RegisterGattServerCallback(gattServerCallback_t  serverCallback)
{
    pfGattServerCallback = serverCallback;
    
    return GattServer_RegisterCallback(App_GattServerCallback);
}

bleResult_t App_RegisterGattClientProcedureCallback(gattClientProcedureCallback_t  callback)
{
    pfGattClientProcCallback = callback;

    return GattClient_RegisterProcedureCallback(App_GattClientProcedureCallback);
}

bleResult_t App_RegisterGattClientNotificationCallback(gattClientNotificationCallback_t  callback)
{
    pfGattClientNotifCallback = callback;

    return GattClient_RegisterNotificationCallback(App_GattClientNotificationCallback);
}

bleResult_t App_RegisterGattClientIndicationCallback(gattClientIndicationCallback_t  callback)
{
    pfGattClientIndCallback = callback;

    return GattClient_RegisterIndicationCallback(App_GattClientIndicationCallback);
}

bleResult_t App_RegisterLeCbCallbacks
(
    l2caLeCbDataCallback_t      pCallback,
    l2caLeCbControlCallback_t   pCtrlCallback
)
{
    pfL2caLeCbDataCallback = pCallback;
    pfL2caLeCbControlCallback = pCtrlCallback;

    return L2ca_RegisterLeCbCallbacks(App_L2caLeDataCallback, App_L2caLeControlCallback);
}

bleResult_t App_PostCallbackMessage
(
    appCallbackHandler_t   handler,
    appCallbackParam_t     param
)
{
    appMsgCallback_t *pMsgIn = NULL;

    /* Allocate a buffer with enough space to store the packet */
    pMsgIn = MSG_Alloc(sizeof (appMsgCallback_t));

    if (!pMsgIn)
    {
        return gBleOutOfMemory_c;
    }

    pMsgIn->handler = handler;
    pMsgIn->param = param;

    /* Put message in the Cb App queue */
    MSG_Queue(&mAppCbInputQueue, pMsgIn);

    /* Signal application */
    OSA_EventSet(mAppEvent, gAppEvtAppCallback_c);
    
    return gBleSuccess_c;
}

void App_NvmErase(uint8_t mEntryIdx)
{
    if(mEntryIdx >= gMaxBondedDevices_c)
    {
        return;
    }
#if gAppUseNvm_d  
    NvErase((void**)&aBondingHeader[mEntryIdx]);
    NvErase((void**)&aBondingDataDynamic[mEntryIdx]);
    NvErase((void**)&aBondingDataStatic[mEntryIdx]);
    NvErase((void**)&aBondingDataDeviceInfo[mEntryIdx]);
    
    uint8_t mDescIdx;
    
    for(mDescIdx = (mEntryIdx * gcGapMaximumSavedCccds_c);
        mDescIdx < (mEntryIdx + 1) * gcGapMaximumSavedCccds_c; mDescIdx++)
    {
        NvErase((void**)&aBondingDataDescriptor[mDescIdx]);
    }
#else
    FLib_MemSet(&maBondDataBlobs[mEntryIdx], 0, gBleBondDataSize_c);
#endif
}

void App_NvmWrite
(
    uint8_t  mEntryIdx,
    void*    pBondHeader,
    void*    pBondDataDynamic,
    void*    pBondDataStatic,
    void*    pBondDataDeviceInfo,
    void*    pBondDataDescriptor,
    uint8_t  mDescriptorIndex
)
{
    if(mEntryIdx >= gMaxBondedDevices_c)
    {
        return;
    }
#if gAppUseNvm_d  
    uint8_t  idx   = 0;
    uint32_t mSize = 0;
    void**   ppNvmData = NULL;;
    void*    pRamData = NULL;
    

    for(idx = 0; idx < 5; idx++)
    {
        ppNvmData = NULL;
        switch(idx)
        {
        case 0:
            if(pBondHeader != NULL)
            {
                ppNvmData = (void**)&aBondingHeader[mEntryIdx];
                pRamData  = pBondHeader;
                mSize     = gBleBondIdentityHeaderSize_c;
            }
            break;
        case 1:
            if(pBondDataDynamic != NULL)
            {
                ppNvmData = (void**)&aBondingDataDynamic[mEntryIdx];
                pRamData  = pBondDataDynamic;;
                mSize     = gBleBondDataDynamicSize_c;
            }
            break;
        case 2:
            if(pBondDataStatic != NULL)
            {
                ppNvmData = (void**)&aBondingDataStatic[mEntryIdx];
                pRamData  = pBondDataStatic;
                mSize     = gBleBondDataStaticSize_c;
            }
            break;
        case 3:
            if(pBondDataDeviceInfo != NULL)
            {
                ppNvmData = (void**)&aBondingDataDeviceInfo[mEntryIdx];
                pRamData  = pBondDataDeviceInfo;
                mSize     = gBleBondDataDeviceInfoSize_c;
            }
            break;
        case 4:
            if(pBondDataDescriptor != NULL)
            {
                if(mDescriptorIndex < gcGapMaximumSavedCccds_c)
                {
                    ppNvmData = (void**)&aBondingDataDescriptor[mEntryIdx * gcGapMaximumSavedCccds_c + mDescriptorIndex];
                    pRamData  = pBondDataDescriptor;
                    mSize     = gBleBondDataDescriptorSize_c;
                }
            }
            break;
        default:
            break;
        }
        if(ppNvmData != NULL)
        {
            if(gNVM_OK_c == NvMoveToRam(ppNvmData))
            {
                FLib_MemCpy(*ppNvmData, pRamData, mSize);
                NvSaveOnIdle(ppNvmData, FALSE);
            }
        }
    }
#else
    
    if(pBondHeader != NULL)
    {
        FLib_MemCpy(&maBondDataBlobs[mEntryIdx].bondHeader, pBondHeader, gBleBondIdentityHeaderSize_c);
    }
    
    if(pBondDataDynamic != NULL)
    {
        FLib_MemCpy((uint8_t*)&maBondDataBlobs[mEntryIdx].bondDataBlobDynamic,
                    pBondDataDynamic,
                    gBleBondDataDynamicSize_c
                        );
    }
    
    if(pBondDataStatic != NULL)
    {
        FLib_MemCpy((uint8_t*)&maBondDataBlobs[mEntryIdx].bondDataBlobStatic,
                    pBondDataStatic,
                    gBleBondDataStaticSize_c
                        );
    }
    
    if(pBondDataDeviceInfo != NULL)
    {
        FLib_MemCpy((uint8_t*)&maBondDataBlobs[mEntryIdx].bondDataBlobDeviceInfo,
                    pBondDataDeviceInfo,
                    gBleBondDataDeviceInfoSize_c
                        );
    }
    
    if(pBondDataDescriptor != NULL && mDescriptorIndex != gcGapMaximumSavedCccds_c)
    {
        FLib_MemCpy((uint8_t*)&(maBondDataBlobs[mEntryIdx].bondDataDescriptors[mDescriptorIndex]),
                    pBondDataDescriptor,
                    gBleBondDataDescriptorSize_c
                        );
    }
    
#endif
}

void App_NvmRead
(
    uint8_t  mEntryIdx,
    void*    pBondHeader,
    void*    pBondDataDynamic,
    void*    pBondDataStatic,
    void*    pBondDataDeviceInfo,
    void*    pBondDataDescriptor,
    uint8_t  mDescriptorIndex
)
{
    if(mEntryIdx >= gMaxBondedDevices_c)
    {
        return;
    }
#if gAppUseNvm_d  
    uint8_t  idx = 0;
    uint32_t mSize = 0;
    void**   ppNvmData = NULL;;
    void*    pRamData = NULL;
    
    for(idx = 0; idx < 5; idx++)
    {
        ppNvmData = NULL;
        switch(idx)
        {
        case 0:
            if(pBondHeader != NULL)
            {
                ppNvmData = (void**)&aBondingHeader[mEntryIdx];
                pRamData  = pBondHeader;
                mSize     = gBleBondIdentityHeaderSize_c;
            }
            break;
        case 1:
            if(pBondDataDynamic != NULL)
            {
                ppNvmData = (void**)&aBondingDataDynamic[mEntryIdx];
                pRamData  = pBondDataDynamic;;
                mSize     = gBleBondDataDynamicSize_c;
            }
            break;
        case 2:
            if(pBondDataStatic != NULL)
            {
                ppNvmData = (void**)&aBondingDataStatic[mEntryIdx];
                pRamData  = pBondDataStatic;
                mSize     = gBleBondDataStaticSize_c;
            }
            break;
        case 3:
            if(pBondDataDeviceInfo != NULL)
            {
                ppNvmData = (void**)&aBondingDataDeviceInfo[mEntryIdx];
                pRamData  = pBondDataDeviceInfo;
                mSize     = gBleBondDataDeviceInfoSize_c;
            }
            break;
        case 4:
            if(pBondDataDescriptor != NULL)
            {
                if(mDescriptorIndex < gcGapMaximumSavedCccds_c)
                {
                    ppNvmData = (void**)&aBondingDataDescriptor[mEntryIdx * gcGapMaximumSavedCccds_c + mDescriptorIndex];
                    pRamData  = pBondDataDescriptor;
                    mSize     = gBleBondDataDescriptorSize_c;
                }
            }
            break;
        default:
            break;
        }
        if(ppNvmData != NULL)
        {
            if(*ppNvmData != NULL && pRamData != NULL)
            {
                FLib_MemCpy(pRamData, *ppNvmData, mSize);
            }
        }
    }
#else
    
    if(pBondHeader != NULL)
    {
        FLib_MemCpy(pBondHeader, &maBondDataBlobs[mEntryIdx].bondHeader, gBleBondIdentityHeaderSize_c);
    }
    
    if(pBondDataDynamic != NULL)
    {
        FLib_MemCpy(pBondDataDynamic,
                    (uint8_t*)&maBondDataBlobs[mEntryIdx].bondDataBlobDynamic,
                    gBleBondDataDynamicSize_c
                        );
    }
   
    if(pBondDataStatic != NULL)
    {
        FLib_MemCpy(pBondDataStatic,
                    (uint8_t*)&maBondDataBlobs[mEntryIdx].bondDataBlobStatic,
                    gBleBondDataStaticSize_c
                        );
    }
    
    if(pBondDataDeviceInfo != NULL)
    {
        FLib_MemCpy(pBondDataDeviceInfo,
                    (uint8_t*)&maBondDataBlobs[mEntryIdx].bondDataBlobDeviceInfo,
                    gBleBondDataDeviceInfoSize_c
                        );
    }
    
    if(pBondDataDescriptor != NULL && mDescriptorIndex < gcGapMaximumSavedCccds_c)
    {
        FLib_MemCpy(pBondDataDescriptor,
                    (uint8_t*)&(maBondDataBlobs[mEntryIdx].bondDataDescriptors[mDescriptorIndex]),
                    gBleBondDataDescriptorSize_c
                        );
    }
        
#endif
}

/************************************************************************************
*************************************************************************************
* Private functions
*************************************************************************************
************************************************************************************/

/*****************************************************************************
* Handles all key events for this device.
* Interface assumptions: None
* Return value: None
*****************************************************************************/
#if gKeyBoardSupported_d && (gKBD_KeysCount_c > 0)   
static void App_KeyboardCallBack
  (
  uint8_t events  /*IN: Events from keyboard module  */
  )
{
    BleApp_HandleKeys(events);
}
#endif



/*! *********************************************************************************
* \brief  Checks character if it is a character '0' until '9'.
*
* \param[in] 	character	character which has to be checked
*
* \return  		TRUE -> character value is '0' until '9'
* 				otherwise FALSE
*
* \remarks
*
********************************************************************************** */
bool_t isUnicodeNumber(char character)
{
	return (character < 48 || character > 57)?FALSE:TRUE;
}

/*! *********************************************************************************
* \brief  Exponation function
*
* \param[in] x			Base of the operation exponation
* \param[in] expo		exponent of the operation exponation
*
* \return  result
*
* \remarks
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
* \brief  Parses the char* message into uint16_t values for power and speed
*
* \param[in] writtenMsg		WrittenEvent from Gattserver
* \return  bikeValues (Power and bikespeed)
*
* \remarks This function gets called to get integer values from the message received
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


/*****************************************************************************
* Handles all messages received from the host task.
* Interface assumptions: None
* Return value: None
*****************************************************************************/
static void App_HandleHostMessageInput(appMsgFromHost_t* pMsg)
{
    switch ( pMsg->msgType ) 
    { 
        case gAppGapGenericMsg_c:
        {
            if (pfGenericCallback)
                pfGenericCallback(&pMsg->msgData.genericMsg);
            else
                BleApp_GenericCallback(&pMsg->msgData.genericMsg);
            break;
        }
        case gAppGapAdvertisementMsg_c:
        {
            if (pfAdvCallback)
                pfAdvCallback(&pMsg->msgData.advMsg);
            break;
        }
        case gAppGapScanMsg_c:
        {
            if (pfScanCallback)
                pfScanCallback(&pMsg->msgData.scanMsg);
            break;
        }
        case gAppGapConnectionMsg_c:
        {
            if (pfConnCallback)
                pfConnCallback(pMsg->msgData.connMsg.deviceId, &pMsg->msgData.connMsg.connEvent);
            break;
        }
        /* RECEIVED MESSAGE FROM CLIENT! */
        case gAppGattServerMsg_c:
        {
        	if (pfGattServerCallback)
			{
				pfGattServerCallback(pMsg->msgData.gattServerMsg.deviceId, &pMsg->msgData.gattServerMsg.serverEvent);
			}
        	// Power and Speed aus Message herauslesen
        	bikeValues = parseBikeMsg(pMsg->msgData.gattServerMsg.serverEvent.eventData.attributeWrittenEvent);
        	// Get altitude and acceleration values and update the PWMvalue
        	I2C_updateAccel();
			I2C_updateAltitude();
        	PWM_updateServo();

#if LED_SHOW_ACCEL
			if(x > 3000)
			{
				GPIO_WritePinOutput(BOARD_INITPINS_LED_RED_GPIO, BOARD_INITPINS_LED_RED_GPIO_PIN, 1);
				GPIO_WritePinOutput(BOARD_INITPINS_LED_GREEN_GPIO, BOARD_INITPINS_LED_GREEN_GPIO_PIN, 1);
			}
			else if(y > 3000)
			{
				GPIO_WritePinOutput(BOARD_INITPINS_LED_RED_GPIO, BOARD_INITPINS_LED_RED_GPIO_PIN, 0);
				GPIO_WritePinOutput(BOARD_INITPINS_LED_GREEN_GPIO, BOARD_INITPINS_LED_GREEN_GPIO_PIN, 1);
			}
			else if(z > 3000)
			{
				GPIO_WritePinOutput(BOARD_INITPINS_LED_RED_GPIO, BOARD_INITPINS_LED_RED_GPIO_PIN, 1);
				GPIO_WritePinOutput(BOARD_INITPINS_LED_GREEN_GPIO, BOARD_INITPINS_LED_GREEN_GPIO_PIN, 0);
			}
			else
			{
				GPIO_WritePinOutput(BOARD_INITPINS_LED_RED_GPIO, BOARD_INITPINS_LED_RED_GPIO_PIN, 0);
				GPIO_WritePinOutput(BOARD_INITPINS_LED_GREEN_GPIO, BOARD_INITPINS_LED_GREEN_GPIO_PIN, 0);
			}
#endif
#if LED_SHOW_SPEED
        	if(bikeValues.bikeSpeed > 60)
			{
				GPIO_WritePinOutput(BOARD_INITPINS_LED_RED_GPIO, BOARD_INITPINS_LED_RED_GPIO_PIN, 1);
				GPIO_WritePinOutput(BOARD_INITPINS_LED_GREEN_GPIO, BOARD_INITPINS_LED_GREEN_GPIO_PIN, 1);
			}
			else if(bikeValues.bikeSpeed > 40)
			{
				//rot leuchten lassen
				GPIO_WritePinOutput(BOARD_INITPINS_LED_RED_GPIO, BOARD_INITPINS_LED_RED_GPIO_PIN, 0);
				GPIO_WritePinOutput(BOARD_INITPINS_LED_GREEN_GPIO, BOARD_INITPINS_LED_GREEN_GPIO_PIN, 1);
			}
			else if(bikeValues.bikeSpeed > 20)
			{
				// grün leuchten lassen
				GPIO_WritePinOutput(BOARD_INITPINS_LED_RED_GPIO, BOARD_INITPINS_LED_RED_GPIO_PIN, 1);
				GPIO_WritePinOutput(BOARD_INITPINS_LED_GREEN_GPIO, BOARD_INITPINS_LED_GREEN_GPIO_PIN, 0);
			}
			else if(bikeValues.bikeSpeed > 0)
			{
				GPIO_WritePinOutput(BOARD_INITPINS_LED_RED_GPIO, BOARD_INITPINS_LED_RED_GPIO_PIN, 0);
				GPIO_WritePinOutput(BOARD_INITPINS_LED_GREEN_GPIO, BOARD_INITPINS_LED_GREEN_GPIO_PIN, 0);
			}
#endif
            // PRESENTATION FINISHED
            break;
        }
        case gAppGattClientProcedureMsg_c:
        {
            if (pfGattClientProcCallback)
                pfGattClientProcCallback(
                    pMsg->msgData.gattClientProcMsg.deviceId,
                    pMsg->msgData.gattClientProcMsg.procedureType,
                    pMsg->msgData.gattClientProcMsg.procedureResult,
                    pMsg->msgData.gattClientProcMsg.error);
            break;
        }
        case gAppGattClientNotificationMsg_c:
        {
            if (pfGattClientNotifCallback)
                pfGattClientNotifCallback(
                    pMsg->msgData.gattClientNotifIndMsg.deviceId,
                    pMsg->msgData.gattClientNotifIndMsg.characteristicValueHandle,
                    pMsg->msgData.gattClientNotifIndMsg.aValue,
                    pMsg->msgData.gattClientNotifIndMsg.valueLength);
            break;
        }
        case gAppGattClientIndicationMsg_c:
        {
            if (pfGattClientIndCallback)
                pfGattClientIndCallback(
                    pMsg->msgData.gattClientNotifIndMsg.deviceId,
                    pMsg->msgData.gattClientNotifIndMsg.characteristicValueHandle,
                    pMsg->msgData.gattClientNotifIndMsg.aValue,
                    pMsg->msgData.gattClientNotifIndMsg.valueLength);
            break;
        }
        case gAppL2caLeDataMsg_c:
        {
            if (pfL2caLeCbDataCallback)
                pfL2caLeCbDataCallback(
                    pMsg->msgData.l2caLeCbDataMsg.deviceId,
                    pMsg->msgData.l2caLeCbDataMsg.lePsm,                    
                    pMsg->msgData.l2caLeCbDataMsg.aPacket,
                    pMsg->msgData.l2caLeCbDataMsg.packetLength);
            break;
        }
        case gAppL2caLeControlMsg_c:
        {
            if (pfL2caLeCbControlCallback)
                pfL2caLeCbControlCallback(
                    pMsg->msgData.l2caLeCbControlMsg.messageType,
                    pMsg->msgData.l2caLeCbControlMsg.aMessage);
            break;
        }
        default:
        {
            break;
        }
    }   
}



static void App_GenericCallback (gapGenericEvent_t* pGenericEvent)
{
    appMsgFromHost_t *pMsgIn = NULL;   
    
    pMsgIn = MSG_Alloc(GetRelAddr(appMsgFromHost_t, msgData) + sizeof(gapGenericEvent_t));
          
    if (!pMsgIn)
      return;
    
    pMsgIn->msgType = gAppGapGenericMsg_c;
    FLib_MemCpy(&pMsgIn->msgData.genericMsg, pGenericEvent, sizeof(gapGenericEvent_t));

    /* Put message in the Host Stack to App queue */
    MSG_Queue(&mHostAppInputQueue, pMsgIn);

    /* Signal application */
    OSA_EventSet(mAppEvent, gAppEvtMsgFromHostStack_c);  
}

static void App_ConnectionCallback (deviceId_t peerDeviceId, gapConnectionEvent_t* pConnectionEvent)
{
    appMsgFromHost_t *pMsgIn = NULL;   
    uint8_t msgLen = GetRelAddr(appMsgFromHost_t, msgData) + sizeof(gapConnectionEvent_t);
    
    if(pConnectionEvent->eventType == gConnEvtKeysReceived_c)
    {
        gapSmpKeys_t    *pKeys = pConnectionEvent->eventData.keysReceivedEvent.pKeys;

        /* Take into account alignment */
        msgLen = GetRelAddr(appMsgFromHost_t, msgData) + GetRelAddr(connectionMsg_t, connEvent) + 
                 GetRelAddr(gapConnectionEvent_t, eventData) + sizeof(gapKeysReceivedEvent_t) + sizeof(gapSmpKeys_t);
                 
        if (pKeys->aLtk != NULL)
        {
            msgLen += 2 * sizeof(uint8_t) + pKeys->cLtkSize + pKeys->cRandSize;
        }

        msgLen += (pKeys->aIrk != NULL) ? (gcSmpIrkSize_c + gcBleDeviceAddressSize_c) : 0;
        msgLen += (pKeys->aCsrk != NULL) ? gcSmpCsrkSize_c : 0;
    }

    pMsgIn = MSG_Alloc(msgLen);
          
    if (!pMsgIn)
      return;
    
    pMsgIn->msgType = gAppGapConnectionMsg_c;
    pMsgIn->msgData.connMsg.deviceId = peerDeviceId;

    if(pConnectionEvent->eventType == gConnEvtKeysReceived_c)
    {
        gapSmpKeys_t    *pKeys = pConnectionEvent->eventData.keysReceivedEvent.pKeys;
        uint8_t         *pCursor = (uint8_t*)&pMsgIn->msgData.connMsg.connEvent.eventData.keysReceivedEvent.pKeys;

        pMsgIn->msgData.connMsg.connEvent.eventType = gConnEvtKeysReceived_c;
        pCursor += sizeof(void*);
        pMsgIn->msgData.connMsg.connEvent.eventData.keysReceivedEvent.pKeys = (gapSmpKeys_t* )pCursor;
        
        /* Copy SMP Keys structure */
        FLib_MemCpy(pCursor, pConnectionEvent->eventData.keysReceivedEvent.pKeys, sizeof(gapSmpKeys_t));
        pCursor += sizeof(gapSmpKeys_t);
        
        if (pKeys->aLtk != NULL)
        {
            /* Copy LTK */
            pMsgIn->msgData.connMsg.connEvent.eventData.keysReceivedEvent.pKeys->cLtkSize = pKeys->cLtkSize;
            pMsgIn->msgData.connMsg.connEvent.eventData.keysReceivedEvent.pKeys->aLtk = pCursor;
            FLib_MemCpy(pCursor, pKeys->aLtk, pKeys->cLtkSize);
            pCursor += pKeys->cLtkSize;

            /* Copy RAND */
            pMsgIn->msgData.connMsg.connEvent.eventData.keysReceivedEvent.pKeys->cRandSize = pKeys->cRandSize;
            pMsgIn->msgData.connMsg.connEvent.eventData.keysReceivedEvent.pKeys->aRand = pCursor;
            FLib_MemCpy(pCursor, pKeys->aRand, pKeys->cRandSize);
            pCursor += pKeys->cRandSize;
        }
        else
          

        if (pKeys->aIrk != NULL)
        {
            /* Copy IRK */
            pMsgIn->msgData.connMsg.connEvent.eventData.keysReceivedEvent.pKeys->aIrk = pCursor;
            FLib_MemCpy(pCursor, pKeys->aIrk, gcSmpIrkSize_c);
            pCursor += gcSmpIrkSize_c;

            /* Copy Address*/
            pMsgIn->msgData.connMsg.connEvent.eventData.keysReceivedEvent.pKeys->addressType = pKeys->addressType;
            pMsgIn->msgData.connMsg.connEvent.eventData.keysReceivedEvent.pKeys->aAddress = pCursor;
            FLib_MemCpy(pCursor, pKeys->aAddress, gcBleDeviceAddressSize_c);
            pCursor += gcBleDeviceAddressSize_c;
        }

        if (pKeys->aCsrk != NULL)
        {
            /* Copy CSRK */
            pMsgIn->msgData.connMsg.connEvent.eventData.keysReceivedEvent.pKeys->aCsrk = pCursor;
            FLib_MemCpy(pCursor, pKeys->aCsrk, gcSmpCsrkSize_c);
        }
    }
    else
    {
        FLib_MemCpy(&pMsgIn->msgData.connMsg.connEvent, pConnectionEvent, sizeof(gapConnectionEvent_t));
    }

    /* Put message in the Host Stack to App queue */
    MSG_Queue(&mHostAppInputQueue, pMsgIn);

    /* Signal application */
    OSA_EventSet(mAppEvent, gAppEvtMsgFromHostStack_c);  
}

static void App_AdvertisingCallback (gapAdvertisingEvent_t* pAdvertisingEvent)
{
    appMsgFromHost_t *pMsgIn = NULL;   
    
    pMsgIn = MSG_Alloc(GetRelAddr(appMsgFromHost_t, msgData) + sizeof(gapAdvertisingEvent_t));
          
    if (!pMsgIn)
      return;
    
    pMsgIn->msgType = gAppGapAdvertisementMsg_c;
    pMsgIn->msgData.advMsg.eventType = pAdvertisingEvent->eventType;
    pMsgIn->msgData.advMsg.eventData = pAdvertisingEvent->eventData;

    /* Put message in the Host Stack to App queue */
    MSG_Queue(&mHostAppInputQueue, pMsgIn);

    /* Signal application */
    OSA_EventSet(mAppEvent, gAppEvtMsgFromHostStack_c);  
}

static void App_ScanningCallback (gapScanningEvent_t* pScanningEvent)
{
    appMsgFromHost_t *pMsgIn = NULL;  
    
    uint8_t msgLen = GetRelAddr(appMsgFromHost_t, msgData) + sizeof(gapScanningEvent_t);
    
    if (pScanningEvent->eventType == gDeviceScanned_c)
    {
        msgLen += pScanningEvent->eventData.scannedDevice.dataLength;
    }
    
    pMsgIn = MSG_Alloc(msgLen);
          
    if (!pMsgIn)
      return;
    
    pMsgIn->msgType = gAppGapScanMsg_c;
    pMsgIn->msgData.scanMsg.eventType = pScanningEvent->eventType;
    
    if (pScanningEvent->eventType == gScanCommandFailed_c)
    {
        pMsgIn->msgData.scanMsg.eventData.failReason = pScanningEvent->eventData.failReason;
    }
    
    if (pScanningEvent->eventType == gDeviceScanned_c)
    {
        FLib_MemCpy(&pMsgIn->msgData.scanMsg.eventData.scannedDevice, 
                    &pScanningEvent->eventData.scannedDevice, 
                    sizeof(gapScanningEvent_t));
        
        /* Copy data after the gapScanningEvent_t structure and update the data pointer*/
        pMsgIn->msgData.scanMsg.eventData.scannedDevice.data = (uint8_t*)&pMsgIn->msgData + sizeof(gapScanningEvent_t);
        FLib_MemCpy(pMsgIn->msgData.scanMsg.eventData.scannedDevice.data, 
                    pScanningEvent->eventData.scannedDevice.data, 
                    pScanningEvent->eventData.scannedDevice.dataLength);
    }

    /* Put message in the Host Stack to App queue */
    MSG_Queue(&mHostAppInputQueue, pMsgIn);

    /* Signal application */
    OSA_EventSet(mAppEvent, gAppEvtMsgFromHostStack_c);  
}

static void App_GattServerCallback
(
    deviceId_t          deviceId,
    gattServerEvent_t*  pServerEvent)
{
    appMsgFromHost_t *pMsgIn = NULL;   
    uint16_t msgLen = GetRelAddr(appMsgFromHost_t, msgData) + sizeof(gattServerMsg_t);
    
    if (pServerEvent->eventType == gEvtAttributeWritten_c ||
        pServerEvent->eventType == gEvtAttributeWrittenWithoutResponse_c)
    {
        msgLen += pServerEvent->eventData.attributeWrittenEvent.cValueLength;
    }

    pMsgIn = MSG_Alloc(msgLen);
          
    if (!pMsgIn)
      return;
    
    pMsgIn->msgType = gAppGattServerMsg_c;
    pMsgIn->msgData.gattServerMsg.deviceId = deviceId;
    FLib_MemCpy(&pMsgIn->msgData.gattServerMsg.serverEvent, pServerEvent, sizeof(gattServerEvent_t));

    if ((pMsgIn->msgData.gattServerMsg.serverEvent.eventType == gEvtAttributeWritten_c) ||
        (pMsgIn->msgData.gattServerMsg.serverEvent.eventType == gEvtAttributeWrittenWithoutResponse_c))
    {
        /* Copy value after the gattServerEvent_t structure and update the aValue pointer*/
        pMsgIn->msgData.gattServerMsg.serverEvent.eventData.attributeWrittenEvent.aValue = 
          (uint8_t *)&pMsgIn->msgData.gattServerMsg.serverEvent.eventData.attributeWrittenEvent.aValue + sizeof(uint8_t*);
        FLib_MemCpy(pMsgIn->msgData.gattServerMsg.serverEvent.eventData.attributeWrittenEvent.aValue,
                    pServerEvent->eventData.attributeWrittenEvent.aValue,
                    pServerEvent->eventData.attributeWrittenEvent.cValueLength);

    }
    
    /* Put message in the Host Stack to App queue */
    MSG_Queue(&mHostAppInputQueue, pMsgIn);

    /* Signal application */
    OSA_EventSet(mAppEvent, gAppEvtMsgFromHostStack_c);  
}

static void App_GattClientProcedureCallback
(
    deviceId_t              deviceId,
    gattProcedureType_t     procedureType,
    gattProcedureResult_t   procedureResult,
    bleResult_t             error)
{
    appMsgFromHost_t *pMsgIn = NULL;

    pMsgIn = MSG_Alloc(GetRelAddr(appMsgFromHost_t, msgData) + sizeof(gattClientProcMsg_t));

    if (!pMsgIn)
      return;

    pMsgIn->msgType = gAppGattClientProcedureMsg_c;
    pMsgIn->msgData.gattClientProcMsg.deviceId = deviceId;
    pMsgIn->msgData.gattClientProcMsg.procedureType = procedureType;
    pMsgIn->msgData.gattClientProcMsg.error = error;
    pMsgIn->msgData.gattClientProcMsg.procedureResult = procedureResult;

    /* Put message in the Host Stack to App queue */
    MSG_Queue(&mHostAppInputQueue, pMsgIn);

    /* Signal application */
    OSA_EventSet(mAppEvent, gAppEvtMsgFromHostStack_c);
}

static void App_GattClientNotificationCallback
(
    deviceId_t      deviceId,
    uint16_t        characteristicValueHandle,
    uint8_t*        aValue,
    uint16_t        valueLength
)
{
    appMsgFromHost_t *pMsgIn = NULL;

    /* Allocate a buffer with enough space to store also the notified value */
    pMsgIn = MSG_Alloc(GetRelAddr(appMsgFromHost_t, msgData) + sizeof(gattClientNotifIndMsg_t)
                        + valueLength);

    if (!pMsgIn)
      return;

    pMsgIn->msgType = gAppGattClientNotificationMsg_c;
    pMsgIn->msgData.gattClientNotifIndMsg.deviceId = deviceId;
    pMsgIn->msgData.gattClientNotifIndMsg.characteristicValueHandle = characteristicValueHandle;
    pMsgIn->msgData.gattClientNotifIndMsg.valueLength = valueLength;

    /* Copy value after the gattClientNotifIndMsg_t structure and update the aValue pointer*/
    pMsgIn->msgData.gattClientNotifIndMsg.aValue = (uint8_t*)&pMsgIn->msgData + sizeof(gattClientNotifIndMsg_t);
    FLib_MemCpy(pMsgIn->msgData.gattClientNotifIndMsg.aValue, aValue, valueLength);

    /* Put message in the Host Stack to App queue */
    MSG_Queue(&mHostAppInputQueue, pMsgIn);

    /* Signal application */
    OSA_EventSet(mAppEvent, gAppEvtMsgFromHostStack_c);
}

static void App_GattClientIndicationCallback
(
    deviceId_t      deviceId,
    uint16_t        characteristicValueHandle,
    uint8_t*        aValue,
    uint16_t        valueLength
)
{
    appMsgFromHost_t *pMsgIn = NULL;

    /* Allocate a buffer with enough space to store also the notified value*/
    pMsgIn = MSG_Alloc(GetRelAddr(appMsgFromHost_t, msgData) + sizeof(gattClientNotifIndMsg_t)
                        + valueLength);

    if (!pMsgIn)
      return;

    pMsgIn->msgType = gAppGattClientIndicationMsg_c;
    pMsgIn->msgData.gattClientNotifIndMsg.deviceId = deviceId;
    pMsgIn->msgData.gattClientNotifIndMsg.characteristicValueHandle = characteristicValueHandle;
    pMsgIn->msgData.gattClientNotifIndMsg.valueLength = valueLength;

    /* Copy value after the gattClientIndIndMsg_t structure and update the aValue pointer*/
    pMsgIn->msgData.gattClientNotifIndMsg.aValue = (uint8_t*)&pMsgIn->msgData + sizeof(gattClientNotifIndMsg_t);
    FLib_MemCpy(pMsgIn->msgData.gattClientNotifIndMsg.aValue, aValue, valueLength);

    /* Put message in the Host Stack to App queue */
    MSG_Queue(&mHostAppInputQueue, pMsgIn);

    /* Signal application */
    OSA_EventSet(mAppEvent, gAppEvtMsgFromHostStack_c);
}

static void App_L2caLeDataCallback
(
    deviceId_t deviceId,
    uint16_t   lePsm,
    uint8_t* pPacket,
    uint16_t packetLength
)
{
    appMsgFromHost_t *pMsgIn = NULL;

    /* Allocate a buffer with enough space to store the packet */
    pMsgIn = MSG_Alloc(GetRelAddr(appMsgFromHost_t, msgData) + sizeof(l2caLeCbDataMsg_t) 
                        + packetLength);

    if (!pMsgIn)
    {
        return;
    }

    pMsgIn->msgType = gAppL2caLeDataMsg_c;
    pMsgIn->msgData.l2caLeCbDataMsg.deviceId = deviceId;
    pMsgIn->msgData.l2caLeCbDataMsg.lePsm = lePsm;
    pMsgIn->msgData.l2caLeCbDataMsg.packetLength = packetLength;

    FLib_MemCpy(pMsgIn->msgData.l2caLeCbDataMsg.aPacket, pPacket, packetLength);

    /* Put message in the Host Stack to App queue */
    MSG_Queue(&mHostAppInputQueue, pMsgIn);

    /* Signal application */
    OSA_EventSet(mAppEvent, gAppEvtMsgFromHostStack_c);
}

static void App_L2caLeControlCallback
(
    l2capControlMessageType_t  messageType,
    void* pMessage
)
{
    appMsgFromHost_t *pMsgIn = NULL;
    uint8_t messageLength = 0;

    switch (messageType) {
        case gL2ca_LePsmConnectRequest_c:
        {
            messageLength = sizeof(l2caLeCbConnectionRequest_t);
            break;
        }
        case gL2ca_LePsmConnectionComplete_c:
        {
            messageLength = sizeof(l2caLeCbConnectionComplete_t);
            break;
        }
        case gL2ca_LePsmDisconnectNotification_c:
        {
            messageLength = sizeof(l2caLeCbDisconnection_t);
            break;
        }
        case gL2ca_NoPeerCredits_c:
        {
            messageLength = sizeof(l2caLeCbNoPeerCredits_t);
            break;
        }
        case gL2ca_LocalCreditsNotification_c:
        {
            messageLength = sizeof(l2caLeCbLocalCreditsNotification_t);
            break;
        }
        default:
            return;
    }


    /* Allocate a buffer with enough space to store the biggest packet */
    pMsgIn = MSG_Alloc(GetRelAddr(appMsgFromHost_t, msgData) + sizeof(l2caLeCbConnectionComplete_t));

    if (!pMsgIn)
    {
        return;
    }

    pMsgIn->msgType = gAppL2caLeControlMsg_c;
    pMsgIn->msgData.l2caLeCbControlMsg.messageType = messageType;
    
    FLib_MemCpy(pMsgIn->msgData.l2caLeCbControlMsg.aMessage, pMessage, messageLength);

    /* Put message in the Host Stack to App queue */
    MSG_Queue(&mHostAppInputQueue, pMsgIn);

    /* Signal application */
    OSA_EventSet(mAppEvent, gAppEvtMsgFromHostStack_c);
}

#if !gUseHciTransportDownward_d
/* Called by BLE when a connect is received */
static void BLE_SignalFromISRCallback(void)
{
#if (cPWR_UsePowerDownMode)  
    PWR_DisallowDeviceToSleep();  
#endif    
}
#endif
