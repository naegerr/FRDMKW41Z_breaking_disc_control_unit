/*!
 * This file is the interface file for the Bike simulator service.
 *
 * Author:		Robert Näger
 * Date:		15. Nov. 2017
 */

#ifndef _BIKE_SIMULATOR_INTERFACE_H_
#define _BIKE_SIMULATOR_INTERFACE_H_

/************************************************************************************
*************************************************************************************
* Include
*************************************************************************************
************************************************************************************/

/************************************************************************************
*************************************************************************************
* Public constants & macros
*************************************************************************************
************************************************************************************/
#define			MAX_MESSAGE_SIZE	45

typedef double	double_t;

/************************************************************************************
*************************************************************************************
* Public type definitions
*************************************************************************************
************************************************************************************/

/*! Bike simulator Client - Configuration */
typedef struct bscConfig_tag
{
    uint16_t    bsService;           /*!<Service Handle */
    double_t    bsBrakingPower;			 /*! Braking power */
    double_t	bsSpeed;				 /*! Bicycle speed */
    uint16_t	bsCharacteristic;	/* Characteristic of Service */
} bscConfig_t;

/*! message types according to Mobile App */
typedef enum bssMessageType_tag
{
	connectionTest = 0x00,
	connectionTestResponse,
	start,
	stop,
	pause,
	brake,				// integer in [W]
	acceleration,
	bicycleSpeed,		// [m/s]
	windSpeed,			// [m/s]
	windDirection,		// 0 front, 1/2PI right, PI back, 3/2PI left
	inclination,		// [grade]
	airDensity,			// [kg/m^3]
	temperature,		// [°C]
	gpsLatitude,		// DecDeg Format
	gpsLongitude,		// DecDeg Format
	gpsAltitude,		// [m] above Referenzellipsoid WGS 84
} bssMessageType;

/*! Data structure of bike_simulator according to BDA mobile app
 * either in application level*/
typedef struct bssData_tag
{
	uint16_t			startSign;		// [
	bssMessageType		messageType;
	uint16_t			delimeter;		// ,
	double_t			messageData;
	uint16_t			endSign;		// ]
}bssData;

/************************************************************************************
*************************************************************************************
* SERVER
*************************************************************************************
************************************************************************************/
/*! Bike simulator Service - Configuration -> Server */
typedef struct bssConfig_tag
{
    uint16_t    serviceHandle;   /*!<Service handle */
    uint8_t*	valueBikeNotify;  /*!<Input report field */
    uint8_t*	valueBikeWrite;	/*!<Output report field */
} bssConfig_t;

/*! Start and stop service functions */
bleResult_t Bss_Start(bssConfig_t *bServiceConfig);
bleResult_t Bss_Stop(bssConfig_t *bServiceConfig);

/*! Subscribe and unsubscribe functions */
bleResult_t Bss_Subscribe(deviceId_t clientDeviceId);
bleResult_t Bss_Unsubscribe();

/*! Functions for read, write, update characteristics */
bleResult_t Bss_WriteData(uint16_t serviceHandle, double_t newBrakingValue, double_t newSpeedValue);
bleResult_t Bss_ReadData(uint16_t serviceHandle, uint8_t* messageNotify, uint8_t* messageWrite);

/*! Notification function */
bleResult_t Bss_SendNotificiation(uint16_t handle);
#endif /* _BIKE_SIMULATOR_INTERFACE_H_ */
