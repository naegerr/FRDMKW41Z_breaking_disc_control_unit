/*
 * TEXT BELOW IS USED AS SETTING FOR TOOLS *************************************
!!GlobalInfo
product: Pins v3.0
processor: MKW31Z256xxx4
package_id: MKW31Z256VHT4
mcu_data: ksdk2_0
processor_version: 2.0.1
pin_labels:
- {pin_num: '45', pin_signal: TSI0_CH4/PTC16/LLWU_P0/SPI0_SCK/I2C0_SDA/UART0_RTS_b/TPM0_CH3, label: PWM_SERVO, identifier: PWM_SERVO}
- {pin_num: '23', pin_signal: DAC0_OUT/ADC0_SE4/CMP0_IN2/PTB18/I2C1_SCL/TPM_CLKIN0/TPM0_CH0/NMI_b, label: LED_ORANGE, identifier: LED_ORANGE}
- {pin_num: '24', pin_signal: ADC0_DP0/CMP0_IN0, label: ACCU_MEAS, identifier: ACCU_MEAS}
- {pin_num: '27', pin_signal: VREFH/VREF_OUT, label: VREF}
- {pin_num: '4', pin_signal: TSI0_CH10/PTA16/LLWU_P4/SPI1_SOUT/TPM0_CH0, label: BUTTON, identifier: BUTTON}
- {pin_num: '5', pin_signal: TSI0_CH11/PTA17/LLWU_P5/RF_RESET/SPI1_SIN/TPM_CLKIN1, label: LED_GREEN, identifier: LED_GREEN}
- {pin_num: '6', pin_signal: TSI0_CH12/PTA18/LLWU_P6/SPI1_SCK/TPM2_CH0, label: LED_RED, identifier: LED_RED}
- {pin_num: '7', pin_signal: TSI0_CH13/ADC0_SE5/PTA19/LLWU_P7/SPI1_PCS0/TPM2_CH1, label: BUTTON_2, identifier: BUTTON_2}
 * BE CAREFUL MODIFYING THIS COMMENT - IT IS YAML SETTINGS FOR TOOLS ***********
 */

#include "fsl_common.h"
#include "fsl_port.h"
#include "pin_mux.h"

/*FUNCTION**********************************************************************
 * 
 * Function Name : BOARD_InitBootPins
 * Description   : Calls initialization functions.
 * 
 *END**************************************************************************/
void BOARD_InitBootPins(void) {
    BOARD_InitPins();
}

#define PCR_SRE_SLOW                  0x01u   /*!< Slew Rate Enable: Slow slew rate is configured on the corresponding pin, if the pin is configured as a digital output. */
#define PIN16_IDX                       16u   /*!< Pin number for pin 16 in a port */
#define PIN19_IDX                       19u   /*!< Pin number for pin 19 in a port */

/*
 * TEXT BELOW IS USED AS SETTING FOR TOOLS *************************************
BOARD_InitButtons:
- options: {coreID: core0, enableClock: 'true'}
- pin_list:
  - {pin_num: '4', peripheral: GPIOA, signal: 'GPIO, 16', pin_signal: TSI0_CH10/PTA16/LLWU_P4/SPI1_SOUT/TPM0_CH0, direction: INPUT, slew_rate: slow, pull_select: no_init}
  - {pin_num: '7', peripheral: GPIOA, signal: 'GPIO, 19', pin_signal: TSI0_CH13/ADC0_SE5/PTA19/LLWU_P7/SPI1_PCS0/TPM2_CH1, direction: INPUT, slew_rate: slow, pull_select: no_init}
 * BE CAREFUL MODIFYING THIS COMMENT - IT IS YAML SETTINGS FOR TOOLS ***********
 */

/*FUNCTION**********************************************************************
 *
 * Function Name : BOARD_InitButtons
 * Description   : Configures pin routing and optionally pin electrical features.
 *
 *END**************************************************************************/
void BOARD_InitButtons(void) {
  CLOCK_EnableClock(kCLOCK_PortA);                           /* Port A Clock Gate Control: Clock enabled */

  PORT_SetPinMux(PORTA, PIN16_IDX, kPORT_MuxAsGpio);         /* PORTA16 (pin 4) is configured as PTA16 */
  PORTA->PCR[16] = ((PORTA->PCR[16] &
    (~(PORT_PCR_SRE_MASK | PORT_PCR_ISF_MASK)))              /* Mask bits to zero which are setting */
      | PORT_PCR_SRE(PCR_SRE_SLOW)                           /* Slew Rate Enable: Slow slew rate is configured on the corresponding pin, if the pin is configured as a digital output. */
    );
  PORT_SetPinMux(PORTA, PIN19_IDX, kPORT_MuxAsGpio);         /* PORTA19 (pin 7) is configured as PTA19 */
  PORTA->PCR[19] = ((PORTA->PCR[19] &
    (~(PORT_PCR_SRE_MASK | PORT_PCR_ISF_MASK)))              /* Mask bits to zero which are setting */
      | PORT_PCR_SRE(PCR_SRE_SLOW)                           /* Slew Rate Enable: Slow slew rate is configured on the corresponding pin, if the pin is configured as a digital output. */
    );
}


#define PCR_PFE_DISABLED              0x00u   /*!< Passive Filter Enable: Passive input filter is disabled on the corresponding pin. */

#define PCR_SRE_SLOW                  0x01u   /*!< Slew Rate Enable: Slow slew rate is configured on the corresponding pin, if the pin is configured as a digital output. */

#define PIN16_IDX                       16u   /*!< Pin number for pin 16 in a port */
#define PIN17_IDX                       17u   /*!< Pin number for pin 17 in a port */
#define PIN18_IDX                       18u   /*!< Pin number for pin 18 in a port */

/*
 * TEXT BELOW IS USED AS SETTING FOR TOOLS *************************************
BOARD_InitPins:
- options: {callFromInitBoot: 'true', coreID: core0, enableClock: 'true'}
- pin_list:
  - {pin_num: '23', peripheral: GPIOB, signal: 'GPIO, 18', pin_signal: DAC0_OUT/ADC0_SE4/CMP0_IN2/PTB18/I2C1_SCL/TPM_CLKIN0/TPM0_CH0/NMI_b, direction: OUTPUT, slew_rate: slow,
    passive_filter: disable}
  - {pin_num: '45', peripheral: TPM0, signal: 'CH, 3', pin_signal: TSI0_CH4/PTC16/LLWU_P0/SPI0_SCK/I2C0_SDA/UART0_RTS_b/TPM0_CH3, direction: OUTPUT}
  - {pin_num: '24', peripheral: ADC0, signal: 'DP, 0', pin_signal: ADC0_DP0/CMP0_IN0}
  - {pin_num: '27', peripheral: ADC0, signal: VREFH, pin_signal: VREFH/VREF_OUT}
  - {pin_num: '5', peripheral: GPIOA, signal: 'GPIO, 17', pin_signal: TSI0_CH11/PTA17/LLWU_P5/RF_RESET/SPI1_SIN/TPM_CLKIN1, direction: OUTPUT}
  - {pin_num: '6', peripheral: GPIOA, signal: 'GPIO, 18', pin_signal: TSI0_CH12/PTA18/LLWU_P6/SPI1_SCK/TPM2_CH0, direction: OUTPUT}
 * BE CAREFUL MODIFYING THIS COMMENT - IT IS YAML SETTINGS FOR TOOLS ***********
 */

/*FUNCTION**********************************************************************
 *
 * Function Name : BOARD_InitPins
 * Description   : Configures pin routing and optionally pin electrical features.
 *
 *END**************************************************************************/
void BOARD_InitPins(void) {
  CLOCK_EnableClock(kCLOCK_PortA);                           /* Port A Clock Gate Control: Clock enabled */
  CLOCK_EnableClock(kCLOCK_PortB);                           /* Port B Clock Gate Control: Clock enabled */
  CLOCK_EnableClock(kCLOCK_PortC);                           /* Port C Clock Gate Control: Clock enabled */

  PORT_SetPinMux(PORTA, PIN17_IDX, kPORT_MuxAsGpio);         /* PORTA17 (pin 5) is configured as PTA17 */
  PORT_SetPinMux(PORTA, PIN18_IDX, kPORT_MuxAsGpio);         /* PORTA18 (pin 6) is configured as PTA18 */
  PORT_SetPinMux(PORTB, PIN18_IDX, kPORT_MuxAsGpio);         /* PORTB18 (pin 23) is configured as PTB18 */
  PORTB->PCR[18] = ((PORTB->PCR[18] &
    (~(PORT_PCR_SRE_MASK | PORT_PCR_PFE_MASK | PORT_PCR_ISF_MASK))) /* Mask bits to zero which are setting */
      | PORT_PCR_SRE(PCR_SRE_SLOW)                           /* Slew Rate Enable: Slow slew rate is configured on the corresponding pin, if the pin is configured as a digital output. */
      | PORT_PCR_PFE(PCR_PFE_DISABLED)                       /* Passive Filter Enable: Passive input filter is disabled on the corresponding pin. */
    );
  PORT_SetPinMux(PORTC, PIN16_IDX, kPORT_MuxAlt5);           /* PORTC16 (pin 45) is configured as TPM0_CH3 */
}

/*******************************************************************************
 * EOF
 ******************************************************************************/
