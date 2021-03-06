/*
 * Accu_monitor.c
 *
 *  Created on: 30.12.2017
 *      Author: Robert Näger
 */

#include "fsl_gpio.h"
#include "fsl_adc16.h"
#include "fsl_pit.h"
#include "pin_mux.h"
#include "Accu_monitor.h"

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

/* PIT */
#define PIT_ADC_HANDLER 	PIT_IRQHandler
#define PIT_IRQ_ID 			PIT_IRQn
/* Get source clock for PIT driver */
#define PIT_SOURCE_CLOCK CLOCK_GetFreq(kCLOCK_BusClk)
volatile bool pitIsrFlag = false;

/*! *********************************************************************************
* \brief  InitADC
*
* \remarks	The Analog and Digital conversion. 16Bit and no differential conversion.
* No interrupts, because conversion is triggered in PIT interrupt, because Software trigger
* is used.
********************************************************************************** */
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
