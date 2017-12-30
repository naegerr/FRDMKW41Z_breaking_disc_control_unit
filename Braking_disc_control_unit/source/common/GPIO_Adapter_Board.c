/*
 * GPIO_Adapter.c
 *
 *  Created on: 30.12.2017
 *      Author: Robert Näger
 */

#include <GPIO_Adapter_Board.h>
#include "pin_mux.h"
#include "fsl_gpio.h"

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
