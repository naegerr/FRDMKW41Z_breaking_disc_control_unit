#ifndef _PIN_MUX_H_
#define _PIN_MUX_H_


/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*! @brief Direction type  */
typedef enum _pin_mux_direction
{
  kPIN_MUX_DirectionInput = 0U,         /* Input direction */
  kPIN_MUX_DirectionOutput = 1U,        /* Output direction */
  kPIN_MUX_DirectionInputOrOutput = 2U  /* Input or output direction */
} pin_mux_direction_t;

/*!
 * @addtogroup pin_mux
 * @{
 */

/*******************************************************************************
 * API
 ******************************************************************************/

#if defined(__cplusplus)
extern "C" {
#endif

/*!
 * @brief Calls initialization functions.
 *
 */
void BOARD_InitBootPins(void);

/* PORTA16 (number 4), J2[4]/D11 */
#define BOARD_INITPINS_TPM0_CH0_PERIPHERAL                                  TPM0   /*!< Device name: TPM0 */
#define BOARD_INITPINS_TPM0_CH0_SIGNAL                                        CH   /*!< TPM0 signal: CH */
#define BOARD_INITPINS_TPM0_CH0_CHANNEL                                        0   /*!< TPM0 channel: 0 */
#define BOARD_INITPINS_TPM0_CH0_PIN_NAME                                TPM0_CH0   /*!< Pin name */
#define BOARD_INITPINS_TPM0_CH0_LABEL                                "J2[4]/D11"   /*!< Label */
#define BOARD_INITPINS_TPM0_CH0_NAME                                  "TPM0_CH0"   /*!< Identifier name */

/* ADC0_DP0 (number 24), J4[6]/J35[2]/V_BATT/THER_A/THERMISTOR */
#define BOARD_INITPINS_ADC0_BATT_MON_PERIPHERAL                             ADC0   /*!< Device name: ADC0 */
#define BOARD_INITPINS_ADC0_BATT_MON_SIGNAL                                   DP   /*!< ADC0 signal: DP */
#define BOARD_INITPINS_ADC0_BATT_MON_CHANNEL                                   0   /*!< ADC0 DP channel: 0 */
#define BOARD_INITPINS_ADC0_BATT_MON_PIN_NAME                           ADC0_DP0   /*!< Pin name */
#define BOARD_INITPINS_ADC0_BATT_MON_LABEL "J4[6]/J35[2]/V_BATT/THER_A/THERMISTOR" /*!< Label */
#define BOARD_INITPINS_ADC0_BATT_MON_NAME                        "ADC0_BATT_MON"   /*!< Identifier name */


/*!
 * @brief Configures pin routing and optionally pin electrical features.
 *
 */
void BOARD_InitPins(void);

/* PORTC4 (number 40), J1[5]/D4/SW3 */
#define BOARD_INITBUTTONS_SW3_GPIO                                         GPIOC   /*!< GPIO device name: GPIOC */
#define BOARD_INITBUTTONS_SW3_PORT                                         PORTC   /*!< PORT device name: PORTC */
#define BOARD_INITBUTTONS_SW3_GPIO_PIN                                        4U   /*!< PORTC pin index: 4 */
#define BOARD_INITBUTTONS_SW3_PIN_NAME                                      PTC4   /*!< Pin name */
#define BOARD_INITBUTTONS_SW3_LABEL                               "J1[5]/D4/SW3"   /*!< Label */
#define BOARD_INITBUTTONS_SW3_NAME                                         "SW3"   /*!< Identifier name */

/* PORTC5 (number 41), J3[1]/SW4 */
#define BOARD_INITBUTTONS_SW4_GPIO                                         GPIOC   /*!< GPIO device name: GPIOC */
#define BOARD_INITBUTTONS_SW4_PORT                                         PORTC   /*!< PORT device name: PORTC */
#define BOARD_INITBUTTONS_SW4_GPIO_PIN                                        5U   /*!< PORTC pin index: 5 */
#define BOARD_INITBUTTONS_SW4_PIN_NAME                                      PTC5   /*!< Pin name */
#define BOARD_INITBUTTONS_SW4_LABEL                                  "J3[1]/SW4"   /*!< Label */
#define BOARD_INITBUTTONS_SW4_NAME                                         "SW4"   /*!< Identifier name */


/*!
 * @brief Configures pin routing and optionally pin electrical features.
 *
 */
void BOARD_InitButtons(void);

/* PORTA19 (number 7), J2[3]/D10/RGB_GREEN */
#define BOARD_INITLEDS_LED_GREEN_GPIO                                      GPIOA   /*!< GPIO device name: GPIOA */
#define BOARD_INITLEDS_LED_GREEN_PORT                                      PORTA   /*!< PORT device name: PORTA */
#define BOARD_INITLEDS_LED_GREEN_GPIO_PIN                                    19U   /*!< PORTA pin index: 19 */
#define BOARD_INITLEDS_LED_GREEN_PIN_NAME                                  PTA19   /*!< Pin name */
#define BOARD_INITLEDS_LED_GREEN_LABEL                     "J2[3]/D10/RGB_GREEN"   /*!< Label */
#define BOARD_INITLEDS_LED_GREEN_NAME                                "LED_GREEN"   /*!< Identifier name */
#define BOARD_INITLEDS_LED_GREEN_DIRECTION              kPIN_MUX_DirectionOutput   /*!< Direction */

/* PORTA18 (number 6), J2[6]/D13/RGB_BLUE */
#define BOARD_INITLEDS_LED_BLUE_GPIO                                       GPIOA   /*!< GPIO device name: GPIOA */
#define BOARD_INITLEDS_LED_BLUE_PORT                                       PORTA   /*!< PORT device name: PORTA */
#define BOARD_INITLEDS_LED_BLUE_GPIO_PIN                                     18U   /*!< PORTA pin index: 18 */
#define BOARD_INITLEDS_LED_BLUE_PIN_NAME                                   PTA18   /*!< Pin name */
#define BOARD_INITLEDS_LED_BLUE_LABEL                       "J2[6]/D13/RGB_BLUE"   /*!< Label */
#define BOARD_INITLEDS_LED_BLUE_NAME                                  "LED_BLUE"   /*!< Identifier name */
#define BOARD_INITLEDS_LED_BLUE_DIRECTION               kPIN_MUX_DirectionOutput   /*!< Direction */

/* PORTC18 (number 47), J1[7]/U4[8]/D6 */
#define BOARD_INITLEDS_FLASH_SO_GPIO                                       GPIOC   /*!< GPIO device name: GPIOC */
#define BOARD_INITLEDS_FLASH_SO_PORT                                       PORTC   /*!< PORT device name: PORTC */
#define BOARD_INITLEDS_FLASH_SO_GPIO_PIN                                     18U   /*!< PORTC pin index: 18 */
#define BOARD_INITLEDS_FLASH_SO_PIN_NAME                                   PTC18   /*!< Pin name */
#define BOARD_INITLEDS_FLASH_SO_LABEL                           "J1[7]/U4[8]/D6"   /*!< Label */
#define BOARD_INITLEDS_FLASH_SO_NAME                                  "FLASH_SO"   /*!< Identifier name */


/*!
 * @brief Configures pin routing and optionally pin electrical features.
 *
 */
void BOARD_InitLEDs(void);

/* PORTA19 (number 7), J2[3]/D10/RGB_GREEN */
#define BOARD_INITRGB_LED_GREEN_PERIPHERAL                                  TPM2   /*!< Device name: TPM2 */
#define BOARD_INITRGB_LED_GREEN_SIGNAL                                        CH   /*!< TPM2 signal: CH */
#define BOARD_INITRGB_LED_GREEN_CHANNEL                                        1   /*!< TPM2 channel: 1 */
#define BOARD_INITRGB_LED_GREEN_PIN_NAME                                TPM2_CH1   /*!< Pin name */
#define BOARD_INITRGB_LED_GREEN_LABEL                      "J2[3]/D10/RGB_GREEN"   /*!< Label */
#define BOARD_INITRGB_LED_GREEN_NAME                                 "LED_GREEN"   /*!< Identifier name */
#define BOARD_INITRGB_LED_GREEN_DIRECTION               kPIN_MUX_DirectionOutput   /*!< Direction */

/* PORTA18 (number 6), J2[6]/D13/RGB_BLUE */
#define BOARD_INITRGB_LED_BLUE_PERIPHERAL                                   TPM2   /*!< Device name: TPM2 */
#define BOARD_INITRGB_LED_BLUE_SIGNAL                                         CH   /*!< TPM2 signal: CH */
#define BOARD_INITRGB_LED_BLUE_CHANNEL                                         0   /*!< TPM2 channel: 0 */
#define BOARD_INITRGB_LED_BLUE_PIN_NAME                                 TPM2_CH0   /*!< Pin name */
#define BOARD_INITRGB_LED_BLUE_LABEL                        "J2[6]/D13/RGB_BLUE"   /*!< Label */
#define BOARD_INITRGB_LED_BLUE_NAME                                   "LED_BLUE"   /*!< Identifier name */
#define BOARD_INITRGB_LED_BLUE_DIRECTION                kPIN_MUX_DirectionOutput   /*!< Direction */


/*!
 * @brief Configures pin routing and optionally pin electrical features.
 *
 */
void BOARD_InitRGB(void);

/* PORTC18 (number 47), J1[7]/U4[8]/D6 */
#define BOARD_INITSPI_FLASH_SO_PERIPHERAL                                   SPI0   /*!< Device name: SPI0 */
#define BOARD_INITSPI_FLASH_SO_SIGNAL                                        SIN   /*!< SPI0 signal: SIN */
#define BOARD_INITSPI_FLASH_SO_PIN_NAME                                 SPI0_SIN   /*!< Pin name */
#define BOARD_INITSPI_FLASH_SO_LABEL                            "J1[7]/U4[8]/D6"   /*!< Label */
#define BOARD_INITSPI_FLASH_SO_NAME                                   "FLASH_SO"   /*!< Identifier name */

/* PORTC17 (number 46), J1[6]/U4[1]/D5 */
#define BOARD_INITSPI_FLASH_SI_PERIPHERAL                                   SPI0   /*!< Device name: SPI0 */
#define BOARD_INITSPI_FLASH_SI_SIGNAL                                       SOUT   /*!< SPI0 signal: SOUT */
#define BOARD_INITSPI_FLASH_SI_PIN_NAME                                SPI0_SOUT   /*!< Pin name */
#define BOARD_INITSPI_FLASH_SI_LABEL                            "J1[6]/U4[1]/D5"   /*!< Label */
#define BOARD_INITSPI_FLASH_SI_NAME                                   "FLASH_SI"   /*!< Identifier name */

/* PORTA16 (number 4), J2[4]/D11 */
#define BOARD_INITSPI_TPM0_CH0_PERIPHERAL                                   SPI1   /*!< Device name: SPI1 */
#define BOARD_INITSPI_TPM0_CH0_SIGNAL                                       SOUT   /*!< SPI1 signal: SOUT */
#define BOARD_INITSPI_TPM0_CH0_PIN_NAME                                SPI1_SOUT   /*!< Pin name */
#define BOARD_INITSPI_TPM0_CH0_LABEL                                 "J2[4]/D11"   /*!< Label */
#define BOARD_INITSPI_TPM0_CH0_NAME                                   "TPM0_CH0"   /*!< Identifier name */

/* PORTA18 (number 6), J2[6]/D13/RGB_BLUE */
#define BOARD_INITSPI_LED_BLUE_PERIPHERAL                                   SPI1   /*!< Device name: SPI1 */
#define BOARD_INITSPI_LED_BLUE_SIGNAL                                        SCK   /*!< SPI1 signal: SCK */
#define BOARD_INITSPI_LED_BLUE_PIN_NAME                                 SPI1_SCK   /*!< Pin name */
#define BOARD_INITSPI_LED_BLUE_LABEL                        "J2[6]/D13/RGB_BLUE"   /*!< Label */
#define BOARD_INITSPI_LED_BLUE_NAME                                   "LED_BLUE"   /*!< Identifier name */

/* PORTA19 (number 7), J2[3]/D10/RGB_GREEN */
#define BOARD_INITSPI_LED_GREEN_PERIPHERAL                                  SPI1   /*!< Device name: SPI1 */
#define BOARD_INITSPI_LED_GREEN_SIGNAL                                   PCS0_SS   /*!< SPI1 signal: PCS0_SS */
#define BOARD_INITSPI_LED_GREEN_PIN_NAME                               SPI1_PCS0   /*!< Pin name */
#define BOARD_INITSPI_LED_GREEN_LABEL                      "J2[3]/D10/RGB_GREEN"   /*!< Label */
#define BOARD_INITSPI_LED_GREEN_NAME                                 "LED_GREEN"   /*!< Identifier name */


/*!
 * @brief Configures pin routing and optionally pin electrical features.
 *
 */
void BOARD_InitSPI(void);

/* PORTC6 (number 42), J1[1]/D0/UART0_RX_TGTMCU */
#define BOARD_INITLPUART_DEBUG_UART_RX_PERIPHERAL                        LPUART0   /*!< Device name: LPUART0 */
#define BOARD_INITLPUART_DEBUG_UART_RX_SIGNAL                                 RX   /*!< LPUART0 signal: RX */
#define BOARD_INITLPUART_DEBUG_UART_RX_PIN_NAME                         UART0_RX   /*!< Pin name */
#define BOARD_INITLPUART_DEBUG_UART_RX_LABEL          "J1[1]/D0/UART0_RX_TGTMCU"   /*!< Label */
#define BOARD_INITLPUART_DEBUG_UART_RX_NAME                      "DEBUG_UART_RX"   /*!< Identifier name */

/* PORTC7 (number 43), J1[2]/D1/UART0_TX_TGTMCU */
#define BOARD_INITLPUART_DEBUG_UART_TX_PERIPHERAL                        LPUART0   /*!< Device name: LPUART0 */
#define BOARD_INITLPUART_DEBUG_UART_TX_SIGNAL                                 TX   /*!< LPUART0 signal: TX */
#define BOARD_INITLPUART_DEBUG_UART_TX_PIN_NAME                         UART0_TX   /*!< Pin name */
#define BOARD_INITLPUART_DEBUG_UART_TX_LABEL          "J1[2]/D1/UART0_TX_TGTMCU"   /*!< Label */
#define BOARD_INITLPUART_DEBUG_UART_TX_NAME                      "DEBUG_UART_TX"   /*!< Identifier name */


/*!
 * @brief Configures pin routing and optionally pin electrical features.
 *
 */
void BOARD_InitLPUART(void);

/* PORTB1 (number 17), J4[5]/IR_SIG */
#define BOARD_INITI2C_IR_TX_PERIPHERAL                                      I2C0   /*!< Device name: I2C0 */
#define BOARD_INITI2C_IR_TX_SIGNAL                                           SDA   /*!< I2C0 signal: SDA */
#define BOARD_INITI2C_IR_TX_PIN_NAME                                    I2C0_SDA   /*!< Pin name */
#define BOARD_INITI2C_IR_TX_LABEL                                 "J4[5]/IR_SIG"   /*!< Label */
#define BOARD_INITI2C_IR_TX_NAME                                         "IR_TX"   /*!< Identifier name */

/* PORTC2 (number 38), J2[10]/U9[4]/D15/I2C1_SCL */
#define BOARD_INITI2C_ACCEL_SCL_PERIPHERAL                                  I2C1   /*!< Device name: I2C1 */
#define BOARD_INITI2C_ACCEL_SCL_SIGNAL                                       CLK   /*!< I2C1 signal: CLK */
#define BOARD_INITI2C_ACCEL_SCL_PIN_NAME                                I2C1_SCL   /*!< Pin name */
#define BOARD_INITI2C_ACCEL_SCL_LABEL                "J2[10]/U9[4]/D15/I2C1_SCL"   /*!< Label */
#define BOARD_INITI2C_ACCEL_SCL_NAME                                 "ACCEL_SCL"   /*!< Identifier name */

/* PORTC3 (number 39), J2[9]/U9[6]/D14/I2C1_SDA */
#define BOARD_INITI2C_ACCEL_SDA_PERIPHERAL                                  I2C1   /*!< Device name: I2C1 */
#define BOARD_INITI2C_ACCEL_SDA_SIGNAL                                       SDA   /*!< I2C1 signal: SDA */
#define BOARD_INITI2C_ACCEL_SDA_PIN_NAME                                I2C1_SDA   /*!< Pin name */
#define BOARD_INITI2C_ACCEL_SDA_LABEL                 "J2[9]/U9[6]/D14/I2C1_SDA"   /*!< Label */
#define BOARD_INITI2C_ACCEL_SDA_NAME                                 "ACCEL_SDA"   /*!< Identifier name */


/*!
 * @brief Configures pin routing and optionally pin electrical features.
 *
 */
void BOARD_InitI2C(void);

#if defined(__cplusplus)
}
#endif

/*!
 * @}
 */
#endif /* _PIN_MUX_H_ */

/*******************************************************************************
 * EOF
 ******************************************************************************/
