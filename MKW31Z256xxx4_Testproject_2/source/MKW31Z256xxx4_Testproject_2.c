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
 * @file    MKW31Z256xxx4_Testproject_2.c
 * @brief   Application entry point.
 */
#include <stdio.h>
#include "board.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "MKW31Z4.h"
#include "fsl_debug_console.h"

/* TODO: insert other include files here. */
#include "fsl_gpio.h"
#include "fsl_adc16.h"

/* TODO: insert other definitions and declarations here. */
/*******************************************************************************
 * Variables
 ******************************************************************************/
#define DEMO_ADC16_BASEADDR 		ADC0
#define DEMO_ADC16_CHANNEL_GROUP 	0U
#define DEMO_ADC16_USER_CHANNEL 	0U
#define DEMO_ADC16_IRQn 			ADC0_IRQn
#define DEMO_ADC16_IRQ_HANDLER_FUNC ADC0_IRQHandler

volatile bool g_Adc16ConversionDoneFlag = false;
volatile uint32_t g_Adc16ConversionValue = 0;
adc16_channel_config_t g_adc16ChannelConfigStruct;
/*
 * @brief   Application entry point.
 */

void ADC_Init(void)
{
    adc16_config_t adc16ConfigStruct;
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
  #if defined(BOARD_ADC_USE_ALT_VREF)
      adc16ConfigStruct.referenceVoltageSource = kADC16_ReferenceVoltageSourceValt;
  #endif
      ADC16_Init(DEMO_ADC16_BASEADDR, &adc16ConfigStruct);

      /* Make sure the software trigger is used. */
      ADC16_EnableHardwareTrigger(DEMO_ADC16_BASEADDR, false);
  #if defined(FSL_FEATURE_ADC16_HAS_CALIBRATION) && FSL_FEATURE_ADC16_HAS_CALIBRATION
      if (kStatus_Success == ADC16_DoAutoCalibration(DEMO_ADC16_BASEADDR))
      {
          ;
      }

  #endif /* FSL_FEATURE_ADC16_HAS_CALIBRATION */

      /* Prepare ADC channel setting */
      g_adc16ChannelConfigStruct.channelNumber = DEMO_ADC16_USER_CHANNEL;
      g_adc16ChannelConfigStruct.enableInterruptOnConversionCompleted = true;

  #if defined(FSL_FEATURE_ADC16_HAS_DIFF_MODE) && FSL_FEATURE_ADC16_HAS_DIFF_MODE
      g_adc16ChannelConfigStruct.enableDifferentialConversion = false;
  #endif /* FSL_FEATURE_ADC16_HAS_DIFF_MODE */
  }

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
      GPIO_WritePinOutput(GPIOA, 17U, 1);
  }
int main(void) {
  	/* Init board hardware. */
    BOARD_InitBootPins();
    BOARD_InitBootClocks();
  	/* Init FSL debug console. */
	BOARD_InitDebugConsole();

    EnableIRQ(DEMO_ADC16_IRQn);
	InitGPIO();
	ADC_Init();


    /* Force the counter to be placed into memory. */
    volatile static int i = 0 ;
    /* Enter an infinite loop, just incrementing a counter. */
    while(1) {
    	g_Adc16ConversionDoneFlag = false;
		ADC16_SetChannelConfig(DEMO_ADC16_BASEADDR, DEMO_ADC16_CHANNEL_GROUP, &g_adc16ChannelConfigStruct);

		while (!g_Adc16ConversionDoneFlag)
		{
			GPIO_WritePinOutput(GPIOA, 17U, 0);
		}

		GPIO_WritePinOutput(GPIOA, 17U, 1);
    }
    return 0 ;
}
