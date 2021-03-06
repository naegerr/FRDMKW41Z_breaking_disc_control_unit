/*
 * Accel_Altitude_sensor.c
 *
 *  Created on: 30.12.2017
 *      Author: rober_000
 */

#include "fsl_gpio.h"
#include "fsl_i2c.h"
#include "fsl_port.h"
#include "pin_mux.h"
#include "Accel_Altitude_sensor.h"

/************************************************************************************
*************************************************************************************
* DECLARATIONS
*************************************************************************************
************************************************************************************/
#define BOARD_I2C_BASEADDR 		I2C0

#define ACCEL_I2C_CLK_SRC 		I2C0_CLK_SRC
#define ACCEL_I2C_CLK_FREQ 		CLOCK_GetFreq(I2C0_CLK_SRC)

#define I2C_RELEASE_SDA_PORT 	PORTB
#define I2C_RELEASE_SCL_PORT 	PORTB
#define I2C_RELEASE_SDA_GPIO 	GPIOB
#define I2C_RELEASE_SDA_PIN 	1U
#define I2C_RELEASE_SCL_GPIO 	GPIOB
#define I2C_RELEASE_SCL_PIN 	0U
#define I2C_RELEASE_BUS_COUNT 	100U
#define I2C_BAUDRATE 			100000U

// Device ID's on board
#define MMA8452_WHOAMI 			0x2AU
#define MPL3115_WHOAMI			0xC4U

/* FXOS8700 and MMA8451 have the same who_am_i register address. */
#define ACCEL_READ_TIMES 		10U

i2c_master_handle_t g_m_handle;
uint8_t slave_I2C_addr_found = 0x00;

volatile bool completionFlag = false;
volatile bool nakFlag = false;

/* Device ID from other Accel sensors */
#define FXOS8700_WHOAMI 		0xC7U
#define MMA8451_WHOAMI 			0x1AU

/*! Register according to datasheet MMA8452 */
typedef enum accel_register_t
{
	accel_state 			= 0x00U,	// State register (read only)
	accel_whoami			= 0x0DU,	// Device ID register address(read only)
	accel_transient_cfg		= 0x1DU,	// transient acceleration detection (read/write)
	accel_hp_filter			= 0x0FU,	// high-pass filter register (read/write)
	accel_xyz_data_cfg 		= 0x0EU,	// Data config (read/write)
	accel_ctrl_reg1			= 0x2AU,	// Control-register 1 (read/write)
	accel_ctrl_reg2			= 0x2BU,	// Control-register 2 (read/write)
	accel_ctrl_reg3			= 0x2CU,	// Control-register 3 (read/write)
	accel_ctrl_reg4			= 0x2DU,	// Control-register 4 (read/write)
	accel_ctrl_reg5			= 0x2EU,	// Control-register 5 (read/write)
} accel_register;

/*! Register according to datasheet MPL3115 */
typedef enum altitude_register_t
{
	altitude_state 			= 0x00U,	// State register (read only)
	altitude_data_msb		= 0x01U,	// Register for Data output MSB (read only)
	altitude_data_csb		= 0x02U,	// Register for Data output CSB (read only)
	altitude_data_lsb		= 0x03U,	// Register for Data output LSB (read only)
	altitude_whoami			= 0x0CU,	// Device ID register address(read only)
	altitude_FIFO_setup		= 0x0FU,	// FIFO setup register (read only)
	altitude_int_source		= 0x12U,	// interrupt source register (read only)
	altitude_sensor_data	= 0x13U,	// Sensor data register (read only)
	altitude_ctrl_reg1		= 0x26U,	// Control-register 1 (read/write)
	altitude_ctrl_reg2		= 0x27U,	// Control-register 2 (read/write)
	altitude_ctrl_reg3		= 0x28U,	// Control-register 3 (read/write)
	altitude_ctrl_reg4		= 0x29U,	// Control-register 4 (read/write)
	altitude_ctrl_reg5		= 0x2AU		// Control-register 5 (read/write)
} altitude_register;

uint8_t readBuffAccel[7];		// Buffer Acceleration sensor
uint8_t readBuffAltitude[4];	// Buffer Altitude sensor

// Some slave addresses from the sensors
const uint8_t slave_address[] = {0x1CU, 0x60U,  0x1DU, 0x1EU, 0x1FU};

/************************************************************************************
 *************************************************************************************
 * Private functions prototype
 *************************************************************************************
 ************************************************************************************/
static bool I2C_ReadAccelWhoAmI(void);
static bool I2C_ReadAltitudeWhoAmI(void);
static bool I2C_WriteRegister(I2C_Type *base, uint8_t device_addr, uint8_t reg_addr, uint8_t value);
static bool I2C_ReadRegister(I2C_Type *base, uint8_t device_addr, uint8_t reg_addr, uint8_t *rxBuff, uint32_t rxSize);
static void i2c_release_bus_delay(void);
static void i2c_master_callback(I2C_Type *base, i2c_master_handle_t *handle, status_t status, void *userData);
/************************************************************************************
 *************************************************************************************
 * Public functions
 *************************************************************************************
 ************************************************************************************/

 /*! *********************************************************************************
 * \brief  BOARD_I2C_ReleaseBus
 *
 * \remarks	Sends start- and stop signals to I2C bus
 ********************************************************************************** */
 void BOARD_I2C_ReleaseBus(void)
 {
     uint8_t i = 0;
     gpio_pin_config_t pin_config;
     port_pin_config_t i2c_pin_config = {0};

     /* Config pin mux as gpio */
     i2c_pin_config.pullSelect = kPORT_PullUp; // onboard pullups
     i2c_pin_config.mux = kPORT_MuxAsGpio;

     pin_config.pinDirection = kGPIO_DigitalOutput;
     pin_config.outputLogic = 1U;
     CLOCK_EnableClock(kCLOCK_PortB);
     PORT_SetPinConfig(I2C_RELEASE_SCL_PORT, I2C_RELEASE_SCL_PIN, &i2c_pin_config);
     PORT_SetPinConfig(I2C_RELEASE_SCL_PORT, I2C_RELEASE_SDA_PIN, &i2c_pin_config);

     GPIO_PinInit(I2C_RELEASE_SCL_GPIO, I2C_RELEASE_SCL_PIN, &pin_config);
     GPIO_PinInit(I2C_RELEASE_SDA_GPIO, I2C_RELEASE_SDA_PIN, &pin_config);

     /* Drive SDA low first to simulate a start */
     GPIO_WritePinOutput(I2C_RELEASE_SDA_GPIO, I2C_RELEASE_SDA_PIN, 0U);
     i2c_release_bus_delay();

     /* Send 9 pulses on SCL and keep SDA low */
     for (i = 0; i < 9; i++)
     {
         GPIO_WritePinOutput(I2C_RELEASE_SCL_GPIO, I2C_RELEASE_SCL_PIN, 0U);
         i2c_release_bus_delay();

         GPIO_WritePinOutput(I2C_RELEASE_SDA_GPIO, I2C_RELEASE_SDA_PIN, 1U);
         i2c_release_bus_delay();

         GPIO_WritePinOutput(I2C_RELEASE_SCL_GPIO, I2C_RELEASE_SCL_PIN, 1U);
         i2c_release_bus_delay();
         i2c_release_bus_delay();
     }

     /* Send stop */
     GPIO_WritePinOutput(I2C_RELEASE_SCL_GPIO, I2C_RELEASE_SCL_PIN, 0U);
     i2c_release_bus_delay();

     GPIO_WritePinOutput(I2C_RELEASE_SDA_GPIO, I2C_RELEASE_SDA_PIN, 0U);
     i2c_release_bus_delay();

     GPIO_WritePinOutput(I2C_RELEASE_SCL_GPIO, I2C_RELEASE_SCL_PIN, 1U);
     i2c_release_bus_delay();

     GPIO_WritePinOutput(I2C_RELEASE_SDA_GPIO, I2C_RELEASE_SDA_PIN, 1U);
     i2c_release_bus_delay();
 }

 /*! *********************************************************************************
 * \brief  I2C_Accel_Config
 * \remarks		Writes settings to I2C Slave
 * 				Further informatin in datashset MMA8452
 ********************************************************************************** */
 void I2C_Accel_Config(void)
 {
 	bool isThereAccel = false;
 	I2C_MasterTransferCreateHandle(BOARD_I2C_BASEADDR, &g_m_handle, i2c_master_callback, NULL);
 	isThereAccel = I2C_ReadAccelWhoAmI();
     /*  read the accel xyz value if there is accel device on board */
     if (isThereAccel)
     {
         uint8_t databyte = 0;
         uint8_t write_reg = 0;

         /*  SET 0 TO SET TO STANDBY MODE */
         /*  write 0000 0000 = 0x00 to accelerometer control register 1 */
         /*  standby */
         /*  [7-1] = 0000 000 */
         /*  [0]: active=0 */
         write_reg = accel_ctrl_reg1;
         databyte = 0;
         I2C_WriteRegister(BOARD_I2C_BASEADDR, slave_I2C_addr_found, write_reg, databyte);

         /*  write 0000 0000 = 0x00 to XYZ_DATA_CFG register */
         /*  [7]: reserved */
         /*  [6]: reserved */
         /*  [5]: reserved */
         /*  [4]: hpf_out=0 */
         /*  [3]: reserved */
         /*  [2]: reserved */
         /*  [1-0]: fs=00 for accelerometer range of +/-2g range with 0.488mg/LSB */
         /*  databyte = 0x00; */
         write_reg = accel_xyz_data_cfg;
         databyte = 0x00;
         I2C_WriteRegister(BOARD_I2C_BASEADDR, slave_I2C_addr_found, write_reg, databyte);

         /*  write 0000 0000 = 0x00 to accelerometer control register 2 */
 	    /*  [7]: ST=0 for Self-test disabled */
 	    /*  [6]: RST=0 for Softwarereset disabled */
 	    /*  [5]: Reserved */
 	    /*  [4-3]: SMODS=00 for sleep pwer mode disabled */
         /*  [2]: SLPE=0 for Auto-sleep mode disabled */
 	    /*  [1-0]: MODS=00 for Normal power mode */
 	    /*  databyte = 0xFD; */
 	    write_reg = accel_ctrl_reg2;
 	    databyte = 0x00;
 	    I2C_WriteRegister(BOARD_I2C_BASEADDR, slave_I2C_addr_found, write_reg, databyte);

         /*  write 0000 0000 = 0x00 to accelerometer control register 4 */
 	    /*  [7]: INT_EN_ASLP=0 for Auto-sleep interrupt disabled */
 	    /*  [6]: Reserved */
 	    /*  [5]: INT_EN_TRANS=0 for Transient interrupt disabled */
 	    /*  [4]: INT_EN_LNDPRT=0 for orientation interrupt disabled */
 	    /*  [3]: INT_EN_PULSE=0 for pulse detection interrupt disabled */
         /*  [2]: INT_CFG_FF_MT=0 for Freefall/Motion interrupt disabled */
 	    /*  [1]: Reserved */
 	    /*  [0]: INT_EN_DRDY=0 for data-ready interrupt disabled */
 	    /*  databyte = 0x00; */
 	    write_reg = accel_ctrl_reg4;
 	    databyte = 0x00;
 	    I2C_WriteRegister(BOARD_I2C_BASEADDR, slave_I2C_addr_found, write_reg, databyte);

 	    /*  write 0000 0000 = 0x00 to accelerometer control register 5 */
 		/*  [7]: INT_CFG_ASLP=0 for Auto-sleep interrupt to INT2 */
 		/*  [6]: Reserved */
 		/*  [5]: INT_EN_TRANS=0 for Transient interrupt to INT2 */
 		/*  [4]: INT_EN_LNDPRT=0 for orientation interrupt to INT2 */
 		/*  [3]: INT_EN_PULSE=0 for pulse detection interrupt to INT2 */
 		/*  [2]: INT_CFG_FF_MT=0 for Freefall/Motion interrupt to INT2 */
 		/*  [1]: Reserved */
 		/*  [0]: INT_EN_DRDY=0 for data-ready interrupt to INT2 */
 		/*  databyte = 0x00; */
 		write_reg = accel_ctrl_reg5;
 		databyte = 0x00;
 		I2C_WriteRegister(BOARD_I2C_BASEADDR, slave_I2C_addr_found, write_reg, databyte);

         /*  write 1111 1101 = 0xFD to accelerometer control register 1 */
         /*  [7-6]: aslp_rate=11 for 1.56 Hz ODR when Sleep mode */
         /*  [5-3]: dr=111 for 1.56Hz data rate (when in hybrid mode) */
         /*  [2]: lnoise=1 for low noise mode */
         /*  [1]: f_read=0 for normal 16 bit reads */
         /*  [0]: active=1 active and enable sampling */
         /*  databyte = 0xFD; */
         write_reg = accel_ctrl_reg1;
         databyte = 0xFD;
         I2C_WriteRegister(BOARD_I2C_BASEADDR, slave_I2C_addr_found, write_reg, databyte);

     }
 }
 /*! *********************************************************************************
 * \brief  I2C_Altitude_Config
 * \remarks		Writes settings to I2C Slave
 * 				Further information in datashset MPL3115
 ********************************************************************************** */
 void I2C_Altitude_Config(void)
 {
 	bool isThereAltitude = false;
 	I2C_MasterTransferCreateHandle(BOARD_I2C_BASEADDR, &g_m_handle, i2c_master_callback, NULL);
 	isThereAltitude = I2C_ReadAltitudeWhoAmI();
     /*  read the accel xyz value if there is accel device on board */
     if (isThereAltitude)
     {
         uint8_t databyte = 0;
         uint8_t write_reg = 0;

         /*  SET 0 TO SET TO STANDBY MODE */
         /*  write 0000 0000 = 0x00 to altimeter control register 1 */
         /*  standby */
         /*  [7-1] = 0000 000 */
         /*  [0]: active=0 */
         write_reg = altitude_ctrl_reg1;
         databyte = 0;
         I2C_WriteRegister(BOARD_I2C_BASEADDR, slave_I2C_addr_found, write_reg, databyte);

         /*  write 0010 0000 = 0x20 to altimeter sensor data register */
 		/*  [7-3] = 0000 000 */
 		/*  [2]: DREM=0 for data ready event flag disable */
         /*  [1]: PDEFE=1 for data ready event flag enable */
         /*  [0]: TDEFE=0 for data ready event flag disable */
         /*  databyte = 0x02; */
 		write_reg = altitude_sensor_data;
 		databyte = 0x02;
 		I2C_WriteRegister(BOARD_I2C_BASEADDR, slave_I2C_addr_found, write_reg, databyte);

         /*  write 0010 0000 = 0x20 to altimeter control register 2 */
 	    /*  [7-6]: Reserved */
 	    /*  [5]: LOAD_OUTPUT=1 for loading OUT as target values */
 	    /*  [4]: Reserved */
 	    /*  [3-0]: ST=00 for autoacquisition time step = 1s */
 	    /*  databyte = 0x20; */
 	    write_reg = altitude_ctrl_reg2;
 	    databyte = 0x20;
 	    I2C_WriteRegister(BOARD_I2C_BASEADDR, slave_I2C_addr_found, write_reg, databyte);

 	    /*  write 0010 0010 = 0x22 to altimeter control register 3 */
 		/*  [7-6]: Reserved */
 		/*  [5]: IPOL1=1 for interrupt polarity = Active high */
 		/*  [4]: PP_OD1=0 for internal pullup enabled */
 		/*  [3-2]: Reserved */
 	    /*  [1]: IPOL2=1 for interrupt polarity = Active high */
 	    /*  [0]: PP_OD2=0 for internal pullup enabled */
 		/*  databyte = 0x22; */
 		write_reg = altitude_ctrl_reg3;
 		databyte = 0x22;
 		I2C_WriteRegister(BOARD_I2C_BASEADDR, slave_I2C_addr_found, write_reg, databyte);

 		/*  write 0000 0000 = 0x00 to altimeter control register 4 */
 		/*  [7]: INT_EN_DRDY=0 for data ready interrupt disabled */
 		/*  [6]: INT_EN_FIFO=0 for FIFO interrupt disabled */
 		/*  [5]: INT_EN_PW=0 for pressure window interrupt disabled */
 		/*  [4]: INT_EN_TW=0 for temp window interrupt disabled */
 		/*  [3]: INT_EN_PTH=0 for pressure thresh interrupt disabled */
 		/*  [2]: INT_EN_TTH=0 for temp thresh interrupt disabled */
 		/*  [1]: INT_EN_PCHG=0 for pressure change interrupt disabled */
 		/*  [0]: INT_EN_TCHG=0 for temp change interrupt disabled */
 		/*  databyte = 0x00; */
 		write_reg = altitude_ctrl_reg4;
 		databyte = 0x00;
 		I2C_WriteRegister(BOARD_I2C_BASEADDR, slave_I2C_addr_found, write_reg, databyte);

 		/*  write 0000 0000 = 0x00 to altimeter control register 5 */
 		/*  [7]: INT_CFG_DRDY=0 for data-ready interrupt to INT2 */
 		/*  [6]: INT_CFG_FIFO=0 for FIFO interrupt to INT2 */
 		/*  [5]: INT_CFG_PW=0 for pressure window interrupt to INT2 */
 		/*  [4]: INT_CFG_TW=0 for temp window interrupt to INT2 */
 		/*  [3]: INT_CFG_PTH=0 for pressure thresh interrupt to INT2 */
 		/*  [2]: INT_CFG_TTH=0 for temp thresh interrupt to INT2 */
 		/*  [1]: INT_CFG_PCHG=0 for pressure change interrupt to INT2 */
 		/*  [0]: INT_CFG_TCHG=0 for temp change interrupt to INT2 */
 		/*  databyte = 0x00; */
 		write_reg = altitude_ctrl_reg5;
 		databyte = 0x00;
 		I2C_WriteRegister(BOARD_I2C_BASEADDR, slave_I2C_addr_found, write_reg, databyte);

 	    /*  write 1000 0011 = 0x00 to altimeter control register 1 */
 		/*  [7]: ALT=1 for for altimeter mode */
 		/*  [6]: RAW=0 for NOT RAW mode */
 		/*  [5-3]: OS=000 for Oversampling = 1 */
 		/*  [2]: RST=0 for device reset disabled */
 		/*  [1]: OST=1 for initating a measurement */
 		/*  [0]: SBYB=1 for active mode */
 		/*  databyte = 0x83; */
 		write_reg = altitude_ctrl_reg1;
 		databyte = 0x83;
 		I2C_WriteRegister(BOARD_I2C_BASEADDR, slave_I2C_addr_found, write_reg, databyte);

     }
 }
 /*! *********************************************************************************
 * \brief  I2C_getAccel
 * \remarks		Gets the accelsensor values x,y,z from MMA8452 according to settings of config
 * 				uses polling and waites until data is ready.
 * 				Further information in datasheet MMA8452
 ********************************************************************************** */
 accel_values I2C_getAccel()
 {
	accel_values acceleration;
 	/*  Multiple-byte Read from STATUS (0x00) register */
 	I2C_ReadRegister(BOARD_I2C_BASEADDR, slave_address[0], accel_state, readBuffAccel, 7);
 	//status0_value = readBuff[0];
 	// Divide by 4 because of further calculation to not dedrease the amount of bytes
 	acceleration.x = ((int16_t)(((readBuffAccel[1] * 256U) | readBuffAccel[2]))) / 4U;
 	acceleration.y = ((int16_t)(((readBuffAccel[3] * 256U) | readBuffAccel[4]))) / 4U;
 	acceleration.z = ((int16_t)(((readBuffAccel[5] * 256U) | readBuffAccel[6]))) / 4U;
 	return acceleration;
 }

 /*! *********************************************************************************
 * \brief  I2C_getAltitude
 * \remarks		Gets the altitude from MPL3115 according to settings of config
 * 				uses polling and waites until data is ready.
 * 				Further information in datashset MPL3115
 ********************************************************************************** */
 altitude_value I2C_getAltitude()
 {
	altitude_value altitude;
 	uint8_t status0_value = 0;
 	// Request altitude values
 	I2C_WriteRegister(BOARD_I2C_BASEADDR, slave_address[1], altitude_ctrl_reg1, 0x83U);
 	/*  wait for new data are ready. */
 	while ((status0_value & (1<<2)) != 0x04)
 	{
 		I2C_ReadRegister(BOARD_I2C_BASEADDR, slave_address[1], altitude_state, &status0_value, 1);
 	}

 	/*  Multiple-byte Read from STATUS (0x00) register */
 	I2C_ReadRegister(BOARD_I2C_BASEADDR, slave_address[1], altitude_state, readBuffAltitude, 4);
 	altitude.altitudeInteger = ((int16_t)(((readBuffAltitude[1] * 256U) | readBuffAltitude[2])));
 	altitude.altitudeFraction = readBuffAltitude[3] & 0xF0U;

 	/* Clear OST-Bit again, because auto-clear is not available */
 	/* To initiate another measurement -> set OST Bit = 1 */
 	I2C_WriteRegister(BOARD_I2C_BASEADDR, slave_address[1], altitude_ctrl_reg1, 0x81U);
 	return altitude;
 }


/************************************************************************************
*************************************************************************************
* Private functions
*************************************************************************************
************************************************************************************/

/*! *********************************************************************************
 * \brief  i2c_release_bus_delay
 *
 * \remarks	Delay for I2C bus
 ********************************************************************************** */
 static void i2c_release_bus_delay(void)
 {
     uint32_t i = 0;
     for (i = 0; i < I2C_RELEASE_BUS_COUNT; i++)
     {
         __NOP();
     }
 }
 /*! *********************************************************************************
  * \brief  I2C_WriteRegister
  * \param[in]	base			I2C bus
  * \param[in]	device_addr		Device addr of Slave
  * \param[in]	reg_addr		Register to address
  * \param[in]	value			Data value which is sent to Register
  * \return		True: Successfully write to device. Otherwise false.
  * \remarks	Writes a databyte the a register
  ********************************************************************************** */
 static bool I2C_WriteRegister(I2C_Type *base, uint8_t device_addr, uint8_t reg_addr, uint8_t value)
  {
      i2c_master_transfer_t masterXfer;
      memset(&masterXfer, 0, sizeof(masterXfer));

      masterXfer.slaveAddress = device_addr;
      masterXfer.direction = kI2C_Write;
      masterXfer.subaddress = reg_addr;
      masterXfer.subaddressSize = 1;
      masterXfer.data = &value;
      masterXfer.dataSize = 1;
      masterXfer.flags = kI2C_TransferDefaultFlag;

      /*  direction=write : start+device_write;cmdbuff;xBuff; */
      /*  direction=receive : start+device_write;cmdbuff;repeatStart+device_read;xBuff; */
      I2C_MasterTransferNonBlocking(BOARD_I2C_BASEADDR, &g_m_handle, &masterXfer);

      /*  wait for transfer completed. */
      while ((!nakFlag) && (!completionFlag))
      {
      }

      nakFlag = false;
      if (completionFlag == true)
      {
          completionFlag = false;
          return true;
      }
      else
      {
          return false;
      }
  }
  /*! *********************************************************************************
  * \brief  I2C_ReadRegister
  * \param[in]	base			I2C bus
  * \param[in]	device_addr		Device addr of Slave
  * \param[in]	reg_addr		Register to address
  * \param[out]	rxBuff			Buffer for received data
  * \param[in]	rxSize			Size of the buffer
  * \return		True: Successfully Read from device. Otherwise false.
  * \remarks		Receives one or several data bytes from Slave
  ********************************************************************************** */
  static bool I2C_ReadRegister(I2C_Type *base, uint8_t device_addr, uint8_t reg_addr, uint8_t *rxBuff, uint32_t rxSize)
  {
      i2c_master_transfer_t masterXfer;
      memset(&masterXfer, 0, sizeof(masterXfer));
      masterXfer.slaveAddress = device_addr;
      masterXfer.direction = kI2C_Read;
      masterXfer.subaddress = reg_addr;
      masterXfer.subaddressSize = 1;
      masterXfer.data = rxBuff;
      masterXfer.dataSize = rxSize;
      masterXfer.flags = kI2C_TransferDefaultFlag;

      /*  direction=write : start+device_write;cmdbuff;xBuff; */
      /*  direction=recive : start+device_write;cmdbuff;repeatStart+device_read;xBuff; */

      I2C_MasterTransferNonBlocking(BOARD_I2C_BASEADDR, &g_m_handle, &masterXfer);
      /*  wait for transfer completed. */
      while ((!nakFlag) && (!completionFlag))
      {
      }

      nakFlag = false;
      if (completionFlag == true)
      {
          completionFlag = false;
          return true;
      }
      else
      {
          return false;
      }
  }
  /*! *********************************************************************************
  * \brief  i2c_master_callback
  * \param[in]	base	I2C bus
  * \param[in]	handle	handle of I2C
  * \param[in]	status	states of the signal
  * \param[in]	data 	sent/received. May be NULL
  * \remarks	Sends start- and stop signals to I2C bus
  ********************************************************************************** */
  static void i2c_master_callback(I2C_Type *base, i2c_master_handle_t *handle, status_t status, void *userData)
  {
      /* Signal transfer success when received success status. */
      if (status == kStatus_Success)
      {
          completionFlag = true;
      }
      /* Signal transfer success when received success status. */
      if ((status == kStatus_I2C_Nak) || (status == kStatus_I2C_Addr_Nak))
      {
          nakFlag = true;
      }
  }

  /*! *********************************************************************************
  * \brief  I2C_ReadAccelWhoAmI
  * \return		True: Successfully read a device. Otherwise false.
  * \remarks	Checks if accelsensor available with set Device ID.
  ********************************************************************************** */
  static bool I2C_ReadAccelWhoAmI(void)
  {
      /*
  	How to read the device who_am_I value ?
  	Start + Device_address_Write , who_am_I_register;
  	Repeart_Start + Device_address_Read , who_am_I_value.
  	*/
  	uint8_t who_am_i_reg = accel_whoami;
  	uint8_t who_am_i_value = 0x00;
  	uint8_t accel_addr_array_size = 0x00;
  	bool find_device = false;
  	uint8_t i = 0;
  	uint32_t sourceClock = 0;
  	i2c_master_config_t masterConfig;

  	/*
  	 * masterConfig.baudRate_Bps = 100000U;
  	 * masterConfig.enableStopHold = false;
  	 * masterConfig.glitchFilterWidth = 0U;
  	 * masterConfig.enableMaster = true;
  	 */
  	/* Init only once per bus */
  	I2C_MasterGetDefaultConfig(&masterConfig);
  	masterConfig.baudRate_Bps = I2C_BAUDRATE;
  	sourceClock = ACCEL_I2C_CLK_FREQ;
  	I2C_MasterInit(BOARD_I2C_BASEADDR, &masterConfig, sourceClock);

  	i2c_master_transfer_t masterXfer;
  	memset(&masterXfer, 0, sizeof(masterXfer));

  	masterXfer.slaveAddress = slave_address[0];
  	masterXfer.direction = kI2C_Write;
  	masterXfer.subaddress = 0;
  	masterXfer.subaddressSize = 0;
  	masterXfer.data = &who_am_i_reg;
  	masterXfer.dataSize = 1;
  	masterXfer.flags = kI2C_TransferNoStopFlag;

  	accel_addr_array_size = sizeof(slave_address) / sizeof(slave_address[0]);

  	for (i = 0; i < accel_addr_array_size; i++)
  	{
  		masterXfer.slaveAddress = slave_address[i];
  		I2C_MasterTransferNonBlocking(BOARD_I2C_BASEADDR, &g_m_handle, &masterXfer);
  		/*  wait for transfer completed. */
  		while ((!nakFlag) && (!completionFlag))
  		{
  		}

  		nakFlag = false;

  		if (completionFlag == true)
  		{
  			completionFlag = false;
  			find_device = true;
  			slave_I2C_addr_found = masterXfer.slaveAddress;
  			break;
  		}
  	}
  	if (find_device == true)
  	{
  		masterXfer.direction = kI2C_Read;
  		masterXfer.subaddress = 0;
  		masterXfer.subaddressSize = 0;
  		masterXfer.data = &who_am_i_value;
  		masterXfer.dataSize = 1;
  		masterXfer.flags = kI2C_TransferRepeatedStartFlag;

  		I2C_MasterTransferNonBlocking(BOARD_I2C_BASEADDR, &g_m_handle, &masterXfer);

  		/*  wait for transfer completed. */
  		while ((!nakFlag) && (!completionFlag))
  		{
  		}
  		nakFlag = false;
  		if (completionFlag == true)
  		{
  			completionFlag = false;
  			if (who_am_i_value == MMA8452_WHOAMI)
  			{
  				GPIO_WritePinOutput(BOARD_INITPINS_LED_GREEN_GPIO, BOARD_INITPINS_LED_GREEN_GPIO_PIN, 1);
  				return true;
  			}
  			if (who_am_i_value == MPL3115_WHOAMI)
  			{
  				GPIO_WritePinOutput(BOARD_INITPINS_LED_RED_GPIO, BOARD_INITPINS_LED_RED_GPIO_PIN, 1);
  				return true;
  			}
  		}
  	}
  }

  /*! *********************************************************************************
  * \brief  I2C_ReadAltitudeWhoAmI
  * \return		True: Successfully read a device. Otherwise false.
  * \remarks	Checks if Altitude sensor available with set Device ID.
  ********************************************************************************** */
  static bool I2C_ReadAltitudeWhoAmI(void)
  {
      /*
  	How to read the device who_am_I value ?
  	Start + Device_address_Write , who_am_I_register;
  	Repeart_Start + Device_address_Read , who_am_I_value.
  	*/
  	uint8_t who_am_i_reg = altitude_whoami;
  	uint8_t who_am_i_value = 0x00;
  	uint8_t accel_addr_array_size = 0x00;
  	bool find_device = false;
  	uint8_t i = 0;

  	/*
  	 * masterConfig.baudRate_Bps = 100000U;
  	 * masterConfig.enableStopHold = false;
  	 * masterConfig.glitchFilterWidth = 0U;
  	 * masterConfig.enableMaster = true;
  	 */
  	/* Init only once per bus */

  	i2c_master_transfer_t masterXfer;
  	memset(&masterXfer, 0, sizeof(masterXfer));

  	masterXfer.slaveAddress = slave_address[1];
  	masterXfer.direction = kI2C_Write;
  	masterXfer.subaddress = 0;
  	masterXfer.subaddressSize = 0;
  	masterXfer.data = &who_am_i_reg;
  	masterXfer.dataSize = 1;
  	masterXfer.flags = kI2C_TransferNoStopFlag;

  	accel_addr_array_size = sizeof(slave_address) / sizeof(slave_address[1]);

  	for (i = 1; i < accel_addr_array_size; i++)
  	{
  		masterXfer.slaveAddress = slave_address[i];
  		I2C_MasterTransferNonBlocking(BOARD_I2C_BASEADDR, &g_m_handle, &masterXfer);
  		/*  wait for transfer completed. */
  		while ((!nakFlag) && (!completionFlag))
  		{
  		}
  		nakFlag = false;
  		if (completionFlag == true)
  		{
  			completionFlag = false;
  			find_device = true;
  			slave_I2C_addr_found = masterXfer.slaveAddress;
  			break;
  		}
  	}

  	if (find_device == true)
  	{
  		masterXfer.direction = kI2C_Read;
  		masterXfer.subaddress = 0;
  		masterXfer.subaddressSize = 0;
  		masterXfer.data = &who_am_i_value;
  		masterXfer.dataSize = 1;
  		masterXfer.flags = kI2C_TransferRepeatedStartFlag;

  		I2C_MasterTransferNonBlocking(BOARD_I2C_BASEADDR, &g_m_handle, &masterXfer);

  		/*  wait for transfer completed. */
  		while ((!nakFlag) && (!completionFlag))
  		{
  		}
  		nakFlag = false;
  		if (completionFlag == true)
  		{
  			completionFlag = false;
  			if (who_am_i_value == MMA8452_WHOAMI)
  			{
  				GPIO_WritePinOutput(BOARD_INITPINS_LED_GREEN_GPIO, BOARD_INITPINS_LED_GREEN_GPIO_PIN, 1);
  				return true;
  			}
  			if (who_am_i_value == MPL3115_WHOAMI)
  			{
  				GPIO_WritePinOutput(BOARD_INITPINS_LED_RED_GPIO, BOARD_INITPINS_LED_RED_GPIO_PIN, 1);
  				return true;
  			}
  		}
  	}
  }



