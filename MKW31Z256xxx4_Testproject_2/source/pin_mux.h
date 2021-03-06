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

/* PORTB18 (number 23), LED */
#define BOARD_INITPINS_LED_GPIO                                            GPIOB   /*!< GPIO device name: GPIOB */
#define BOARD_INITPINS_LED_PORT                                            PORTB   /*!< PORT device name: PORTB */
#define BOARD_INITPINS_LED_GPIO_PIN                                          18U   /*!< PORTB pin index: 18 */
#define BOARD_INITPINS_LED_PIN_NAME                                        PTB18   /*!< Pin name */
#define BOARD_INITPINS_LED_LABEL                                           "LED"   /*!< Label */
#define BOARD_INITPINS_LED_NAME                                            "LED"   /*!< Identifier name */
#define BOARD_INITPINS_LED_DIRECTION                    kPIN_MUX_DirectionOutput   /*!< Direction */

/* PORTA17 (number 5), LED_GREEN */
#define BOARD_INITPINS_LED_GREEN_GPIO                                      GPIOA   /*!< GPIO device name: GPIOA */
#define BOARD_INITPINS_LED_GREEN_PORT                                      PORTA   /*!< PORT device name: PORTA */
#define BOARD_INITPINS_LED_GREEN_GPIO_PIN                                    17U   /*!< PORTA pin index: 17 */
#define BOARD_INITPINS_LED_GREEN_PIN_NAME                                  PTA17   /*!< Pin name */
#define BOARD_INITPINS_LED_GREEN_LABEL                               "LED_GREEN"   /*!< Label */
#define BOARD_INITPINS_LED_GREEN_NAME                                "LED_GREEN"   /*!< Identifier name */
#define BOARD_INITPINS_LED_GREEN_DIRECTION              kPIN_MUX_DirectionOutput   /*!< Direction */

/* PORTA18 (number 6), LED_RED */
#define BOARD_INITPINS_LED_RED_GPIO                                        GPIOA   /*!< GPIO device name: GPIOA */
#define BOARD_INITPINS_LED_RED_PORT                                        PORTA   /*!< PORT device name: PORTA */
#define BOARD_INITPINS_LED_RED_GPIO_PIN                                      18U   /*!< PORTA pin index: 18 */
#define BOARD_INITPINS_LED_RED_PIN_NAME                                    PTA18   /*!< Pin name */
#define BOARD_INITPINS_LED_RED_LABEL                                   "LED_RED"   /*!< Label */
#define BOARD_INITPINS_LED_RED_NAME                                    "LED_RED"   /*!< Identifier name */
#define BOARD_INITPINS_LED_RED_DIRECTION                kPIN_MUX_DirectionOutput   /*!< Direction */

/* PORTA16 (number 4), BUTTON */
#define BOARD_INITPINS_BUTTON_GPIO                                         GPIOA   /*!< GPIO device name: GPIOA */
#define BOARD_INITPINS_BUTTON_PORT                                         PORTA   /*!< PORT device name: PORTA */
#define BOARD_INITPINS_BUTTON_GPIO_PIN                                       16U   /*!< PORTA pin index: 16 */
#define BOARD_INITPINS_BUTTON_PIN_NAME                                     PTA16   /*!< Pin name */
#define BOARD_INITPINS_BUTTON_LABEL                                     "BUTTON"   /*!< Label */
#define BOARD_INITPINS_BUTTON_NAME                                      "BUTTON"   /*!< Identifier name */
#define BOARD_INITPINS_BUTTON_DIRECTION                  kPIN_MUX_DirectionInput   /*!< Direction */

/* ADC0_DP0 (number 24), ACCU_MEAS */
#define BOARD_INITPINS_ACCU_MEAS_PERIPHERAL                                 ADC0   /*!< Device name: ADC0 */
#define BOARD_INITPINS_ACCU_MEAS_SIGNAL                                       DP   /*!< ADC0 signal: DP */
#define BOARD_INITPINS_ACCU_MEAS_CHANNEL                                       0   /*!< ADC0 DP channel: 0 */
#define BOARD_INITPINS_ACCU_MEAS_PIN_NAME                               ADC0_DP0   /*!< Pin name */
#define BOARD_INITPINS_ACCU_MEAS_LABEL                               "ACCU_MEAS"   /*!< Label */
#define BOARD_INITPINS_ACCU_MEAS_NAME                                "ACCU_MEAS"   /*!< Identifier name */


/*!
 * @brief Configures pin routing and optionally pin electrical features.
 *
 */
void BOARD_InitPins(void);

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
