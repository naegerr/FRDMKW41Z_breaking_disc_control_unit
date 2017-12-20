/*
 * Copyright (c) 2016, NXP Semiconductor, Inc.
 * All rights reserved.
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
 * o Neither the name of NXP Semiconductor, Inc. nor the names of its
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
 
/**
 * @file    MKW31Z256xxx4_Testproject.c
 * @brief   Application entry point.
 */
#include <stdio.h>
#include "board.h"

#include "pin_mux.h"
#include "clock_config.h"
#include "MKW31Z4.h"
#include "fsl_debug_console.h"
#include "fsl_gpio.h"


/* TODO: insert other include files here. */
#include "fsl_tpm.h"
#include "fsl_adc16.h"
/* TODO: insert other definitions and declarations here. */

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
#define DEMO_ADC16_BASEADDR 		ADC0
#define DEMO_ADC16_CHANNEL_GROUP 	0 // DP0
#define DEMO_ADC16_USER_CHANNEL 	0

#define DEMO_ADC16_IRQn 			ADC0_IRQn
#define DEMO_ADC16_IRQ_HANDLER_FUNC ADC0_IRQHandler
volatile bool g_Adc16ConversionDoneFlag = false;
volatile uint32_t g_Adc16ConversionValue = 0;
adc16_channel_config_t g_adc16ChannelConfigStruct;
/*
 * @brief   Application entry point.
 */


void DEMO_ADC16_IRQ_HANDLER_FUNC(void)
{
    g_Adc16ConversionDoneFlag = true;
    /* Read conversion result to clear the conversion completed flag. */
    g_Adc16ConversionValue = ADC16_GetChannelConversionValue(DEMO_ADC16_BASEADDR, DEMO_ADC16_CHANNEL_GROUP);
}
void InitGPIO(void)
{
    // added by NAR for initialize GPIO pins
    gpio_pin_config_t ledConfig;
    ledConfig.pinDirection = kGPIO_DigitalOutput;
    ledConfig.outputLogic = 0;
    GPIO_PinInit(GPIOB, 18U, &ledConfig);
    GPIO_PinInit(GPIOA, 17U, &ledConfig);
    GPIO_PinInit(GPIOA, 18U, &ledConfig);
    ledConfig.pinDirection = kGPIO_DigitalInput;
    GPIO_PinInit(GPIOA, 16U, &ledConfig);
}

void InitPWM(void)
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

void InitADC(void)
{
	/* ADC INIT */
	adc16_channel_config_t 		g_adc16ChannelConfigStruct;
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
	ADC16_Init(DEMO_ADC16_BASEADDR, &adc16ConfigStruct);
	/* Make sure the software trigger is used. */
	ADC16_EnableHardwareTrigger(DEMO_ADC16_BASEADDR, false);
#if defined(FSL_FEATURE_ADC16_HAS_CALIBRATION) && FSL_FEATURE_ADC16_HAS_CALIBRATION
	if (kStatus_Success == ADC16_DoAutoCalibration(DEMO_ADC16_BASEADDR))
	{
		GPIO_WritePinOutput(GPIOB, 18U, 1);
	}
#endif /* FSL_FEATURE_ADC16_HAS_CALIBRATION */

	/* Prepare ADC channel setting */
	g_adc16ChannelConfigStruct.channelNumber = DEMO_ADC16_USER_CHANNEL;
	g_adc16ChannelConfigStruct.enableInterruptOnConversionCompleted = true;
#if defined(FSL_FEATURE_ADC16_HAS_DIFF_MODE) && FSL_FEATURE_ADC16_HAS_DIFF_MODE
	g_adc16ChannelConfigStruct.enableDifferentialConversion = false;
#endif /* FSL_FEATURE_ADC16_HAS_DIFF_MODE */
}

int main(void) {
  	/* Init board hardware. */
    BOARD_InitBootPins();
    BOARD_InitBootClocks();
  	/* Init FSL debug console. */
	BOARD_InitDebugConsole();
    EnableIRQ(DEMO_ADC16_IRQn);

	// Initialize Buttons and LED
	InitGPIO();
	// Initialize PWM for Servo
	InitPWM();
    // Initialize ADC
	InitADC();

	/* Force the counter to be placed into memory. */
    volatile static int i = 0 ;
    updatedDutycycle = 0;
    /* Enter an infinite loop, just incrementing a counter. */
    while(1) {
    	TPM_UpdatePwmDutycycle(SERVO_TPM_BASEADDR, SERVO_TPM_CHANNEL, kTPM_EdgeAlignedPwm, updatedDutycycle);

//    	if(updatedDutycycle < 10)
//		{
//    		updatedDutycycle++;
//		}
//    	if(updatedDutycycle > 9)
//		{
//			updatedDutycycle--;
//		}
        i++ ;
        updatedDutycycle++;
        g_Adc16ConversionDoneFlag = false;
        ADC16_SetChannelConfig(DEMO_ADC16_BASEADDR, DEMO_ADC16_CHANNEL_GROUP, &g_adc16ChannelConfigStruct);
      /*  while (!g_Adc16ConversionDoneFlag)
		{
		}*/
        while(GPIO_ReadPinInput(GPIOA, 16U))
        {

        }

		GPIO_WritePinOutput(GPIOB, 18U, 1);
		GPIO_WritePinOutput(GPIOA, 17U, 1);
		//GPIO_WritePinOutput(GPIOA, 18U, 1);
		GPIO_WritePinOutput(GPIOB, 18U, 0);
		//GPIO_WritePinOutput(GPIOA, 17U, 0);
		//GPIO_WritePinOutput(GPIOA, 18U, 0);
    }
    return 0 ;
}