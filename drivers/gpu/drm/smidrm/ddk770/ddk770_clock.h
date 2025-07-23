/*******************************************************************
* 
*         Copyright (c) 2007 by Silicon Motion, Inc. (SMI)
* 
*  All rights are reserved. Reproduction or in part is prohibited
*  without the written consent of the copyright owner.
* 
*  CLOCK.H --- Voyager GX DDK 
* 
*******************************************************************/
#ifndef _DDK770_CLOCK_H_
#define _DDK770_CLOCK_H_

#define MHz(x) (x*1000000) /* Don't use this macro if x is fraction number */

#define CPU_CLOCK  MHz(400)


#define DEFAULT_INPUT_CLOCK MHz(25) /* Default reference clock */

typedef enum {
    DDR4_3200 = 0x0,
    DDR4_1600 = 0x1,
} ddr_fre_type_t;

typedef enum {
    LPDDR4_3P2K_FD,
    LPDDR4_3P2K_HD,
    DDR4_3P2K_FD,
    DDR4_3P2K_HD,
    DDR4_2P4K_FD,
    DDR4_2P4K_HD,
    LPDDR4_2P4K_FD,
    LPDDR4_2P4K_HD,
    
} ddr_type_t;

typedef struct pll_value_t
{
    unsigned long inputFreq; /* Input clock frequency to the PLL */
    unsigned long DIV;
    unsigned long VCO;
}
pll_value_t;

/*
 * Given a requested clock frequency, this function calculates the 
 * best INT, FRAC, VCO and BS values for the PLL.
 * 
 * Input: Requested pixel clock in Hz unit.
 *        The followiing fields of PLL has to be set up properly:
 *        pPLL->inputFreq.
 *
 * Output: Update the PLL structure with the proper values
 * Return: The actual clock in Hz that the PLL is able to set up.
 *
 */
unsigned long ddk770_calcPllValue(
unsigned long ulRequestClk, /* Required pixel clock in Hz unit */
pll_value_t *pPLL           /* Structure to hold the value to be set in PLL */
);

/*
 * Set up the corresponding bit field of the programmable PLL register.
 *
 * Input: Pointer to PLL structure with all values set up properly.
 *
 */
unsigned long ddk770_formatPllReg(pll_value_t *pPLL,unsigned long oriPllValue);


unsigned long SetPllReg(unsigned long pllReg, pll_value_t *pPLL);


long ddk770_setVclock(unsigned dispCtrl, unsigned long pixel_clock);

unsigned int Check_DDR_Rate(void);

void Sys_Init_PLL(ddr_type_t type);

void Set_JPUVPU_PLL2_600(void);


#endif /*_CLOCK_H_*/
