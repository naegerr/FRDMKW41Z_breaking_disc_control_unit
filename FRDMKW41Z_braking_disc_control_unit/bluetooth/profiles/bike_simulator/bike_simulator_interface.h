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

/*! Data structure of bike_simulator according to BDA mobile app */
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
    uint16_t    serviceHandle;                 /*!<Service handle */
    double_t	initialPower;            /*!<Input report field */
    double_t	initialSpeed;		            /*!<Input report field */
} bssConfig_t;

/*! Start and stop service functions */
bleResult_t Bss_Start(bssConfig_t *bServiceConfig);
bleResult_t Bss_Stop(bssConfig_t *bServiceConfig);

/*! Subscribe and unsubscribe functions */
bleResult_t Bss_Subscribe(deviceId_t clientDeviceId);
bleResult_t Bss_Unsubscribe();

/*! Functions for read, write, update characteristics */
bleResult_t Bss_SendData(uint16_t serviceHandle, uint16_t newBrakingValue);
bleResult_t Bss_ReceiveData(uint16_t serviceHandle); // TODO: change Return data type

#endif /* _BIKE_SIMULATOR_INTERFACE_H_ */
