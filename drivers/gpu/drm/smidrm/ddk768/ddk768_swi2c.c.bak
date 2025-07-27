#include <linux/delay.h>

#include "ddk768_reg.h"
#include "ddk768_chip.h"
#include "ddk768_power.h"
#include "ddk768_timer.h"
#include "ddk768_swi2c.h"
#include "ddk768_help.h"


/*******************************************************************
 * I2C Software Master Driver:
 * ===========================
 * Each i2c cycle is split into 4 sections. Each of these section marks
 * a point in time where the SCL or SDA may be changed.
 *
 * 1 Cycle == |  Section I. |  Section 2. |  Section 3. |  Section 4. |
 *            +-------------+-------------+-------------+-------------+
 *            | SCL set LOW |SCL no change| SCL set HIGH|SCL no change|
 *
 *                                          ____________ _____________
 * SCL == XXXX _____________ ____________ /
 *
 * I.e. the SCL may only be changed in section 1. and section 3. while
 * the SDA may only be changed in section 2. and section 4. The table
 * below gives the changes for these 2 lines in the varios sections.
 *
 * Section changes Table:
 * ======================
 * blank = no change, L = set bit LOW, H = set bit HIGH
 *
 *                                | 1.| 2.| 3.| 4.|
 *                 ---------------+---+---+---+---+
 *                 Tx Start   SDA |   | H |   | L |
 *                            SCL | L |   | H |   |
 *                 ---------------+---+---+---+---+
 *                 Tx Stop    SDA |   | L |   | H |
 *                            SCL | L |   | H |   |
 *                 ---------------+---+---+---+---+
 *                 Tx bit H   SDA |   | H |   |   |
 *                            SCL | L |   | H |   |
 *                 ---------------+---+---+---+---+
 *                 Tx bit L   SDA |   | L |   |   |
 *                            SCL | L |   | H |   |
 *                 ---------------+---+---+---+---+
 *
 ******************************************************************/

/* GPIO pins used for this I2C. It ranges from 0 to 31. */
unsigned char g_i2cClockGPIO = DEFAULT_I2C0_SCL;
unsigned char g_i2cDataGPIO = DEFAULT_I2C0_SDA;

/*
 *  Below is the variable declaration for the GPIO pin register usage
 *  for the i2c Clock and i2c Data.
 *
 *  Note:
 *      Notice that the GPIO usage for the i2c clock and i2c Data are
 *      separated. This is to make this code flexible enough when 
 *      two separate GPIO pins for the clock and data are located
 *      in two different GPIO register set (worst case).
 */

/* i2c Clock GPIO Register usage */
static unsigned long g_i2cClkGPIOMuxReg = GPIO_MUX;
static unsigned long g_i2cClkGPIODataReg = GPIO_DATA;
static unsigned long g_i2cClkGPIODataDirReg = GPIO_DATA_DIRECTION;

/* i2c Data GPIO Register usage */
static unsigned long g_i2cDataGPIOMuxReg = GPIO_MUX;
static unsigned long g_i2cDataGPIODataReg = GPIO_DATA;
static unsigned long g_i2cDataGPIODataDirReg = GPIO_DATA_DIRECTION;

/*
 *  This function puts a delay between command
 */        
static void swI2CWait(void)
{
    //SM768 has build-in timer. Use it instead of SW loop.
    timerWaitTicks(3, 0x3ff);
}

static void swI2CSCL(unsigned char value, unsigned char i2cClockGPIO)
{
    unsigned long ulGPIOData;
    unsigned long ulGPIODirection;

    ulGPIODirection = peekRegisterDWord(g_i2cClkGPIODataDirReg);
    if (value) /* High */
    {
        /* Set direction as input. This will automatically pull the signal up. */
        ulGPIODirection &= ~(1 << i2cClockGPIO);
        pokeRegisterDWord(g_i2cClkGPIODataDirReg, ulGPIODirection);
    }
    else /* Low */
    {
        /* Set the signal down */
        ulGPIOData = peekRegisterDWord(g_i2cClkGPIODataReg);
        ulGPIOData &= ~(1 << i2cClockGPIO);
        pokeRegisterDWord(g_i2cClkGPIODataReg, ulGPIOData);

        /* Set direction as output */
        ulGPIODirection |= (1 << i2cClockGPIO);
        pokeRegisterDWord(g_i2cClkGPIODataDirReg, ulGPIODirection);
    }
}

/*
 *  This function read the data from the SDA GPIO pin
 *
 *  Return Value:
 *      The SDA data bit sent by the Slave
 */
static unsigned char swI2CReadSCL(unsigned char i2cClockGPIO)
{
    unsigned long ulGPIODirection;
    unsigned long ulGPIOData;

    /* Make sure that the direction is input (High) */
    ulGPIODirection = peekRegisterDWord(g_i2cClkGPIODataDirReg);
    if ((ulGPIODirection & (1 << i2cClockGPIO)) != (~(1 << i2cClockGPIO)))
    {
        ulGPIODirection &= ~(1 << i2cClockGPIO);
        pokeRegisterDWord(g_i2cClkGPIODataDirReg, ulGPIODirection);
    }

    /* Now read the SCL line */
    ulGPIOData = peekRegisterDWord(g_i2cClkGPIODataReg);
    if (ulGPIOData & (1 << i2cClockGPIO))
        return 1;
    else
        return 0;
}

static void swI2CSDA(unsigned char value, unsigned char i2cDataGPIO)
{
    unsigned long ulGPIOData;
    unsigned long ulGPIODirection;

    ulGPIODirection = peekRegisterDWord(g_i2cDataGPIODataDirReg);
    if (value) /* High */
    {
        /* Set direction as input. This will automatically pull the signal up. */
        ulGPIODirection &= ~(1 << i2cDataGPIO);
        pokeRegisterDWord(g_i2cDataGPIODataDirReg, ulGPIODirection);
    }
    else /* Low */
    {
        /* Set the signal down */
        ulGPIOData = peekRegisterDWord(g_i2cDataGPIODataReg);
        ulGPIOData &= ~(1 << i2cDataGPIO);
        pokeRegisterDWord(g_i2cDataGPIODataReg, ulGPIOData);

        /* Set direction as output */
        ulGPIODirection |= (1 << i2cDataGPIO);
        pokeRegisterDWord(g_i2cDataGPIODataDirReg, ulGPIODirection);
    }
}

/*
 *  This function read the data from the SDA GPIO pin
 *
 *  Return Value:
 *      The SDA data bit sent by the Slave
 */
static unsigned char swI2CReadSDA(unsigned char i2cDataGPIO)
{
    unsigned long ulGPIODirection;
    unsigned long ulGPIOData;

    /* Make sure that the direction is input (High) */
    ulGPIODirection = peekRegisterDWord(g_i2cDataGPIODataDirReg);
    if ((ulGPIODirection & (1 << i2cDataGPIO)) != (~(1 << i2cDataGPIO)))
    {
        ulGPIODirection &= ~(1 << i2cDataGPIO);
        pokeRegisterDWord(g_i2cDataGPIODataDirReg, ulGPIODirection);
    }

    /* Now read the SDA line */
    ulGPIOData = peekRegisterDWord(g_i2cDataGPIODataReg);
    if (ulGPIOData & (1 << i2cDataGPIO))
        return 1;
    else
        return 0;
}

/*
 *  This function set/reset the SDA GPIO pin
 *
 *  Parameters:
 *      value    - Bit value to set to the SCL or SDA (0 = low, 1 = high)
 *
 *  Notes:
 *      When setting SCL to high, just set the GPIO as input where the pull up
 *      resistor will pull the signal up. Do not use software to pull up the
 *      signal because the i2c will fail when other device try to drive the
 *      signal due to SM50x will drive the signal to always high.
 */
void ddk768_swI2CSDA(unsigned char value)
{
    unsigned long ulGPIOData;
    unsigned long ulGPIODirection;

    ulGPIODirection = peekRegisterDWord(g_i2cDataGPIODataDirReg);
    if (value)      /* High */
    {
        /* Set direction as input. This will automatically pull the signal up. */
        ulGPIODirection &= ~(1 << g_i2cDataGPIO);
        pokeRegisterDWord(g_i2cDataGPIODataDirReg, ulGPIODirection);
    }
    else            /* Low */
    {
        /* Set the signal down */
        ulGPIOData = peekRegisterDWord(g_i2cDataGPIODataReg);
        ulGPIOData &= ~(1 << g_i2cDataGPIO);
        pokeRegisterDWord(g_i2cDataGPIODataReg, ulGPIOData);

        /* Set direction as output */
        ulGPIODirection |= (1 << g_i2cDataGPIO);
        pokeRegisterDWord(g_i2cDataGPIODataDirReg, ulGPIODirection);
    }
}


/*
 *  This function set/reset the SCL GPIO pin
 *
 *  Parameters:
 *      value    - Bit value to set to the SCL or SDA (0 = low, 1 = high)
 *
 *  Notes:
 *      When setting SCL to high, just set the GPIO as input where the pull up
 *      resistor will pull the signal up. Do not use software to pull up the
 *      signal because the i2c will fail when other device try to drive the
 *      signal due to SM50x will drive the signal to always high.
 */
void ddk768_swI2CSCL(unsigned char value)
{
    unsigned long ulGPIOData;
    unsigned long ulGPIODirection;

    ulGPIODirection = peekRegisterDWord(g_i2cClkGPIODataDirReg);
    if (value)      /* High */
    {
        /* Set direction as input. This will automatically pull the signal up. */
        ulGPIODirection &= ~(1 << g_i2cClockGPIO);
        pokeRegisterDWord(g_i2cClkGPIODataDirReg, ulGPIODirection);
    }
    else            /* Low */
    {
        /* Set the signal down */
        ulGPIOData = peekRegisterDWord(g_i2cClkGPIODataReg);
        ulGPIOData &= ~(1 << g_i2cClockGPIO);
        pokeRegisterDWord(g_i2cClkGPIODataReg, ulGPIOData);

        /* Set direction as output */
        ulGPIODirection |= (1 << g_i2cClockGPIO);
        pokeRegisterDWord(g_i2cClkGPIODataDirReg, ulGPIODirection);
    }
}
#pragma GCC push_options
#pragma GCC optimize("O0")
/*
 *  This function sends ACK signal
 */
static void swI2CAck(unsigned char ack)
{
        if(ack)
    {
        ddk768_swI2CSCL(0);
        ddk768_swI2CSDA(0);
        swI2CWait();
        ddk768_swI2CSCL(1);
        swI2CWait();
        ddk768_swI2CSCL(0);
        ddk768_swI2CSDA(0);
        swI2CWait();
    }
    else
    {
        ddk768_swI2CSCL(0);
        ddk768_swI2CSDA(1);
        swI2CWait();
        ddk768_swI2CSCL(1);
        swI2CWait();
        ddk768_swI2CSCL(0);
        ddk768_swI2CSDA(0);
        swI2CWait();
    }

    return;  /* Single byte read is ok without it. */
}


/*
 *  This function sends the start command to the slave device
 */
void ddk768_swI2CStart(void)
{
    /* Start I2C */
    swI2CSDA(1, g_i2cDataGPIO);
    swI2CSCL(1, g_i2cClockGPIO);
    swI2CSDA(0, g_i2cDataGPIO);
}

/*
 *  This function sends the stop command to the slave device
 */
void ddk768_swI2CStop(void)
{
    /* Stop the I2C */
    swI2CSCL(1, g_i2cClockGPIO);
    swI2CSDA(0, g_i2cDataGPIO);
    swI2CSDA(1, g_i2cDataGPIO);
}

void ddk768_swI2CClean(void)
{
    swI2CSCL(0, g_i2cClockGPIO);
    swI2CWait();
    swI2CSCL(1, g_i2cClockGPIO);
    swI2CWait();
}

/*
 *  This function writes one byte to the slave device
 *
 *  Parameters:
 *      data    - Data to be write to the slave device
 *
 *  Return Value:
 *       0   - Success
 *      -1   - Fail to write byte
 */
long ddk768_swI2CWriteByte(unsigned char data) 
{
    unsigned char value = data;
    int i;

    /* Sending the data bit by bit */
    for (i=0; i<8; i++)
    {
        /* Set SCL to low */
        swI2CSCL(0, g_i2cClockGPIO);

        /* Send data bit */
        if ((value & 0x80) != 0)
            swI2CSDA(1, g_i2cDataGPIO);
        else
            swI2CSDA(0, g_i2cDataGPIO);

        swI2CWait();

        /* Toggle clk line to one */
        swI2CSCL(1, g_i2cClockGPIO);
      

        /* Shift byte to be sent */
        value = value << 1;
    }

    /* Set the SCL Low and SDA High (prepare to get input) */
    swI2CSCL(0, g_i2cClockGPIO);
    swI2CSDA(1, g_i2cDataGPIO);

    /* Set the SCL High for ack */
    swI2CWait();
    swI2CSCL(1, g_i2cClockGPIO);


    /* Read SDA, until SDA==0 */
    for(i=0; i<0xf; i++) 
    {
        if (!swI2CReadSDA(g_i2cDataGPIO))
            break;


        swI2CWait();

        swI2CWait();
    }

    /* Set the SCL Low and SDA High */
    swI2CSCL(0, g_i2cClockGPIO);
    swI2CSDA(1, g_i2cDataGPIO);

    if (i<0xf)
        return 0;
    else
        return (-1);
}

#pragma GCC pop_options

long ddk768_swI2CSetGPIO(
    unsigned char i2cClkGPIO,
    unsigned char i2cDataGPIO)
{
    /* Return 0 if the GPIO pins to be used is out of range. The range is only from [0..63] */
    if ((i2cClkGPIO > 31) || (i2cDataGPIO > 31))
        return (-1);

    /* Initialize the GPIO pin for the i2c Clock Register */
    g_i2cClkGPIOMuxReg = GPIO_MUX;
    g_i2cClkGPIODataReg = GPIO_DATA;
    g_i2cClkGPIODataDirReg = GPIO_DATA_DIRECTION;



    /* Initialize the GPIO pin for the i2c Data Register */
    g_i2cDataGPIOMuxReg = GPIO_MUX;
    g_i2cDataGPIODataReg = GPIO_DATA;
    g_i2cDataGPIODataDirReg = GPIO_DATA_DIRECTION;



    /* Enable the GPIO pins for the i2c Clock and Data (GPIO MUX) */
    pokeRegisterDWord(g_i2cClkGPIOMuxReg,
                      peekRegisterDWord(g_i2cClkGPIOMuxReg) & ~(1 << i2cClkGPIO));
    pokeRegisterDWord(g_i2cDataGPIOMuxReg,
                      peekRegisterDWord(g_i2cDataGPIOMuxReg) & ~(1 << i2cDataGPIO));

    /* Enable GPIO power */
    //enableGPIO(1);
    
    return 0;
}

/*
 * This function initializes the i2c attributes and bus
 *
 * Parameters:
 *      i2cClkGPIO      - The GPIO pin to be used as i2c SCL
 *      i2cDataGPIO     - The GPIO pin to be used as i2c SDA
 *
 * Return Value:
 *      -1   - Fail to initialize the i2c
 *       0   - Success
 */
long ddk768_swI2CInit(unsigned char i2cClkGPIO, unsigned char i2cDataGPIO)
{

    /* GPIO pins used for this I2C. It ranges from 0 to 31. */
    g_i2cClockGPIO = i2cClkGPIO;
    g_i2cDataGPIO = i2cDataGPIO;

    if (ddk768_swI2CSetGPIO(i2cClkGPIO, i2cDataGPIO))
        return (-1);

#if 0
    /* Clear the i2c lines. */
    {
        int i = 0;
        for (i = 0; i < 9; i++)
            ddk768_swI2CStop();
    }
#endif
    return 0;
}

static void smi_ddc_setsda(void *data, int state)
{
    struct smi_connector *connector = data;
    swI2CSDA(state, connector->i2c_sda);
    /* smi_set_i2c_signal(data, I2C_SDA_MASK, state); */
}

static void smi_ddc_setscl(void *data, int state)
{
    struct smi_connector *connector = data;
    swI2CSCL(state, connector->i2c_scl);
    /* smi_set_i2c_signal(data, I2C_SCL_MASK, state); */
}

static int smi_ddc_getsda(void *data)
{

    struct smi_connector *connector = data;
    return (int)swI2CReadSDA(connector->i2c_sda);
    /* return smi_get_i2c_signal(data, I2C_SDA_MASK); */
}

static int smi_ddc_getscl(void *data)
{
    struct smi_connector *connector = data;
    return (int)swI2CReadSCL(connector->i2c_scl);
    /* return smi_get_i2c_signal(data, I2C_SCL_MASK); */
}

static int smi_ddc_create(struct smi_connector *connector)
{
    connector->adapter.owner = THIS_MODULE;
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 8, 0)
    connector->adapter.class = I2C_CLASS_DDC;
#endif
    snprintf(connector->adapter.name, I2C_NAME_SIZE, "SMI SW I2C Bit Bus");
    connector->adapter.dev.parent = connector->base.dev->dev;
    i2c_set_adapdata(&connector->adapter, connector);
    connector->adapter.algo_data = &connector->bit_data;

    connector->bit_data.udelay = 2; /* 0x3ff ticks, 168MHZ */
    connector->bit_data.timeout = usecs_to_jiffies(2200);
    connector->bit_data.data = connector;
    connector->bit_data.setsda = smi_ddc_setsda;
    connector->bit_data.setscl = smi_ddc_setscl;
    connector->bit_data.getsda = smi_ddc_getsda;
    connector->bit_data.getscl = smi_ddc_getscl;

    if (i2c_bit_add_bus(&connector->adapter))
    {
        return -1;
    }

    return 0;
}


long ddk768_AdaptSWI2CInit(struct smi_connector *smi_connector)
{
    struct drm_connector *connector = &smi_connector->base;
    unsigned char i2cClkGPIO;
    unsigned char i2cDataGPIO;

    if (connector->connector_type == DRM_MODE_CONNECTOR_DVII)
    {
        i2cClkGPIO = 30;
        i2cDataGPIO = 31;
    }
    else if (connector->connector_type == DRM_MODE_CONNECTOR_VGA)
    {
        i2cClkGPIO = 6;
        i2cDataGPIO = 7;
    }
    else if (connector->connector_type == DRM_MODE_CONNECTOR_HDMIA)
    {
        i2cClkGPIO = 8;
        i2cDataGPIO = 9;
    }
    else
    {
        return -1;
    }

    smi_connector->i2c_scl = i2cClkGPIO;
    smi_connector->i2c_sda = i2cDataGPIO;

    if (ddk768_swI2CSetGPIO(i2cClkGPIO, i2cDataGPIO))
    {
        return (-1);
    }

#if 0
    /* Clear the i2c lines. */
    {
        int i = 0;
        for (i = 0; i < 9; i++)
            ddk768_swI2CStop();
    }
#endif

    udelay(20);

    if (smi_ddc_create(smi_connector))
    {
        return -1;
    }

    return 0;
}


/* After closing HW I2C, give a SCL clk by SW to avoid the deadlock */
long ddk768_AdaptSWI2CCleanBus(
    struct smi_connector *connector)
{
    unsigned char i2cClkGPIO = connector->i2c_scl;
    unsigned char i2cDataGPIO = connector->i2c_sda;

    if (ddk768_swI2CSetGPIO(i2cClkGPIO, i2cDataGPIO))
        return (-1);

    udelay(20);
    swI2CSCL(0, i2cClkGPIO);
    udelay(20);
    swI2CSCL(1, i2cClkGPIO);
    udelay(20);
//    ddk768_swI2CClean();

    return 0;
}


/*
 *  This function writes a value to the slave device's register
 *  (here the g_i2cClockGPIO and g_i2cDataGPIO must have been initilized) 
 *
 *  Parameters:
 *      deviceAddress   - i2c Slave device address which register
 *                        to be written
 *      registerIndex   - Slave device's register to be written
 *      data            - Data to be written to the register
 *
 *  Result:
 *          0   - Success
 *         -1   - Fail
 */
long ddk768_swI2CWriteReg(
    unsigned char deviceAddress,
    unsigned char registerIndex,
    unsigned char data)
{
    long returnValue = 0;
    
    /* Send the Start signal */
    ddk768_swI2CStart();

    /* Send the device address and read the data. All should return success
       in order for the writing processed to be successful
     */
    if ((ddk768_swI2CWriteByte(deviceAddress) != 0) ||
        (ddk768_swI2CWriteByte(registerIndex) != 0) ||
        (ddk768_swI2CWriteByte(data) != 0))
    {
        returnValue = -1;
    }

    /* Stop i2c and release the bus */
    ddk768_swI2CStop();

    return returnValue;
}


unsigned char ddk768_swI2CReadByte(unsigned char ack)
{
    int i;
    unsigned char data = 0;

    for(i=7; i>=0; i--)
    {
        /* Set the SCL to Low and SDA to High (Input) */
        ddk768_swI2CSCL(0);
        ddk768_swI2CSDA(1);
        swI2CWait();

        /* Set the SCL High */
        ddk768_swI2CSCL(1);
        swI2CWait();

        /* Read data bits from SDA */
        data |= (swI2CReadSDA(g_i2cDataGPIO) << i);
    }

       swI2CAck(ack);


    return data;
}

/*
 *  This function reads the slave device's register
 *
 *  Parameters:
 *      deviceAddress   - i2c Slave device address which register
 *                        to be read from
 *      registerIndex   - Slave device's register to be read
 *
 *  Return Value:
 *      Register value
 */


unsigned char ddk768_swI2CReadReg(
    unsigned char deviceAddress, 
    unsigned char registerIndex
)
{
    unsigned char data;

    /* Send the Start signal */
    ddk768_swI2CStart();

    /* Send the device address */
    ddk768_swI2CWriteByte(deviceAddress);                                                  

    /* Send the register index */
    ddk768_swI2CWriteByte(registerIndex);               

    /* Get the bus again and get the data from the device read address */
    ddk768_swI2CStart();
    ddk768_swI2CWriteByte(deviceAddress + 1);

       data = ddk768_swI2CReadByte(0);

    /* Stop swI2C and release the bus */
    ddk768_swI2CStop();

    return data;
}

