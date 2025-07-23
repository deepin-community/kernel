#ifndef _DDK770_IIS_H_
#define _DDK770_IIS_H_

#define MHz(x) (x*1000000) /* Don't use this macro if x is fraction number */

#define IIS_REF_CLOCK MHz(40)


void ddk770_iis_Init(void);

/*
 * Set up I2S and GPIO registers to transmit/receive data.
 */
void ddk770_iisOpen(
   unsigned long wordLength, //Number of bits in IIS data: 16 bit, 24 bit, 32 bit
   unsigned long sampleRate  //Sampling rate.
);

/*
 *    Turn off I2S and close GPIO 
 */
void ddk770_iisClose(void);

/*
 *  This function set up I2S to DMA data from SRAM.
 *
 *  SRAM area has max size of 2048 bytes (or 512 DWords).
 *  Max size of each I2S DMA session is 256 DWords.
 *
 *  Inputs: 
 *        offset address in SRAM to start DMA (DWord aligned)
 *        Number of bytes to DMA (DWord aligned)
 */
void ddk770_iisTxDmaSetup(
    unsigned long offset, /* Offset from start of SRAM area */
    unsigned long len     /* Number of bytes to DMA */
    );

/*
 * Return current IIS DMA position.
 */
unsigned long ddk770_iisDmaPointer(void);

/*
 * This function start IIS without enabling Tx line.
 * It can be used to flush left over SRAM data without
 * sending them to Codec.
 */
void ddk770_iisStartNoTx(void);

/*
 * This function is needed only when I2S is intended to operate in master mode.
 *
 * For slave mode, just use iisOpen() is enough, because I2S will start
 * functioning as soon as an external clock is detected after iisOpen().
 *
 */
void ddk770_iisStart(void);

/*
 * This function is useful only when I2S is operating in master mode.
 *
 * For slave mode, clock is external and cannot be stopped by IIS
 * control register.
 *
 */
void ddk770_iisStop(void);

/*
 * Set values for left Tx and right Tx register.
 */
void ddk770_iisSetTx(
    unsigned long left, //Data for left channel Tx
    unsigned long right //Data for right channel Tx
    );

/*
 * This function clears the RAW interrupt status of I2S.
 * 
 * When I2S completes sending data, the raw interrupt bit will be set.
 * It has to be cleared, in order to distinguish between different sessions of countdown.
 * 
 */
void ddk770_iisClearRawInt(void);

/* 
 * This function returns the INT mask for IIS.
 *
 */
unsigned long ddk770_iisIntMask(void);

/*
 * This is a reference sample showing how to implement ISR for I2S.
 * It works wiht libsrc\intr.c together.
 * 
 * Refer to Apps\iis\tstiis.c on how to hook up this function with system
 * interrupt under WATCOM DOS extender.
 * 
 */


#endif /* _IIS_H_ */
