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


/*!
 * @brief Configures pin routing and optionally pin electrical features.
 *
 */
void BOARD_InitPins(void);


/*!
 * @brief Configures pin routing and optionally pin electrical features.
 *
 */
void BOARD_InitButtons(void);


/*!
 * @brief Configures pin routing and optionally pin electrical features.
 *
 */
void BOARD_InitLEDs(void);


/*!
 * @brief Configures pin routing and optionally pin electrical features.
 *
 */
void BOARD_InitRGB(void);

/* PORTA16 (number 4), J2[4]/D11 */
#define BOARD_INITSPI_TPM0_CH0_PERIPHERAL                                   SPI1   /*!< Device name: SPI1 */
#define BOARD_INITSPI_TPM0_CH0_SIGNAL                                       SOUT   /*!< SPI1 signal: SOUT */
#define BOARD_INITSPI_TPM0_CH0_PIN_NAME                                SPI1_SOUT   /*!< Pin name */
#define BOARD_INITSPI_TPM0_CH0_LABEL                                 "J2[4]/D11"   /*!< Label */
#define BOARD_INITSPI_TPM0_CH0_NAME                                   "TPM0_CH0"   /*!< Identifier name */


/*!
 * @brief Configures pin routing and optionally pin electrical features.
 *
 */
void BOARD_InitSPI(void);


/*!
 * @brief Configures pin routing and optionally pin electrical features.
 *
 */
void BOARD_InitLPUART(void);


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
