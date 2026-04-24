/*******************************************************************
* 
*         Copyright (c) 2024 by Silicon Motion, Inc. (SMI)
* 
*  All rights are reserved. Reproduction or in part is prohibited
*  without the written consent of the copyright owner.
* 
*  CLOCK.C --- SM770 DDK 
*  This file contains source code for the SM770 PLL's
* 
*******************************************************************/
#include <linux/kernel.h>
#include <linux/delay.h>
#include "ddk770_reg.h"
#include "ddk770_hardware.h"
#include "ddk770_chip.h"
#include "ddk770_helper.h"
#include "ddk770_clock.h"
#include "ddk770_help.h"
#include "ddk770_mode.h"
#include "ddk770_ddkdebug.h"
/*
 * A local function to calculate the output frequency of a given PLL structure.
 *
 */
static unsigned long ddk770_calcPLL(pll_value_t *pPLL)
{
    unsigned long pllClk, vcoPower;

    /* Convert everything in Khz range in order to avoid calculation overflow */
    pPLL->inputFreq /= 1000;
    
    /* Work out 2 to the power of (VCO + 1) */
    vcoPower = ddk770_twoToPowerOfx(pPLL->VCO + 1);
    
    pllClk = (pPLL->inputFreq * pPLL->DIV) / vcoPower;
    
    /* Restore input frequency from Khz to hz unit */
    pPLL->inputFreq *= 1000;
    pllClk *= 1000;

	// printk("Acture pixel clock = %d\n",pllClk);

    return pllClk;
}

/*
 * Given a requested clock frequency, this function calculates the 
 * best INT, VCO  values for the PLL.
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
)
{
    unsigned long DIVIDER, VCO, diff, pllClk, vcoPower, tempRequestClk;
    unsigned long bestDiff = 0xffffffff; /* biggest 32 bit unsigned number */
	
	unsigned long DivMin,DivMax;

	if(pPLL->inputFreq ==12500000){
		DivMin = 64;
		DivMax = 128;
	}else{
		DivMin = 32;
		DivMax = 64;
	}
	// printk("divmin=%d,divmax=%d\n",DivMin,DivMax);

    /* Init PLL structure to known states */
    pPLL->DIV = 0;
    pPLL->VCO = 0;


    /* Convert everything in Khz range in order to avoid calculation overflow */
    pPLL->inputFreq /= 1000;
    tempRequestClk = ulRequestClk / 1000;

    /* If the requested clock is higher than 1 GHz, then set it to the maximum, which is
       1 GHz. */
    if (tempRequestClk > MHz(1))
        tempRequestClk = MHz(1);

    /* The maximum of VCO is 5. */
    for (VCO=0; VCO<=7; VCO++)
    {
        /* Work out 2 to the power of (VCO + 1) */
        vcoPower = ddk770_twoToPowerOfx(VCO + 1);

        DIVIDER = ddk770_roundedDiv(tempRequestClk * vcoPower , pPLL->inputFreq);

        /* 32 <= INT <= 64 */
        if ((DIVIDER >= DivMin) && (DIVIDER <= DivMax))
        {
         		/* Calculate the actual clock for a given INT & FRAC */
				pllClk = (pPLL->inputFreq * DIVIDER) / vcoPower;
				
                /* How much are we different from the requirement */
                diff = ddk770_absDiff(pllClk, tempRequestClk);

                if (diff < bestDiff)
                {
                    bestDiff = diff;
                    /* Store DIV and VCO values */
                    pPLL->DIV  = DIVIDER;
                    pPLL->VCO = VCO;
                }
         
        }
    }
    
    /* Restore input frequency from Khz to hz unit */
    pPLL->inputFreq *= 1000;

    /* Output debug information */
    // printk("ddk770_calcPllValue: Requested Frequency = %d\n", ulRequestClk);
    // printk("ddk770_calcPllValue: Input CLK = %dHz, DIV=%d, VCO=%d,\n", 
    //                     pPLL->inputFreq, pPLL->DIV,  pPLL->VCO);

    /* Return actual frequency that the PLL can set */
    return ddk770_calcPLL(pPLL);
}

/*
 * Set up the corresponding bit field of the programmable PLL register.
 *
 * Input: Pointer to PLL structure with all values set up properly.
 *
 */
unsigned long ddk770_formatPllReg(pll_value_t *pPLL, unsigned long origPllValue )
{
    unsigned long ulPllReg = 0;

    /* Note that all PLL's have the same format. Here, we just use Panel PLL parameter
       to work out the bit fields in the register.
       On returning a 32 bit number, the value can be applied to any PLL in the calling function.
    */
  	if(pPLL->inputFreq == 12500000){
	  printk("Use 12.5MHz input clock\n");
      ulPllReg = FIELD_SET(origPllValue,VCLK_PLL, PRESEL,125);
      ulPllReg = FIELD_VALUE(ulPllReg, VCLK_PLL, VCO, pPLL->VCO);
      ulPllReg = FIELD_VALUE(ulPllReg, VCLK_PLL, DIVIDER, pPLL->DIV);
  	}else{
	  ulPllReg = FIELD_SET(origPllValue,VCLK_PLL, PRESEL,250);
      ulPllReg = FIELD_VALUE(ulPllReg, VCLK_PLL, VCO, pPLL->VCO);
      ulPllReg = FIELD_VALUE(ulPllReg, VCLK_PLL, DIVIDER, pPLL->DIV); 
  	}

    printk("formatPllReg: PLL register value = 0x%08lx\n", ulPllReg);

    return(ulPllReg);
}


unsigned long SetPllReg(unsigned long pllReg, pll_value_t *pPLL)
{
	unsigned long ulTmpValue;
    unsigned int timeout = 10000;
    unsigned int pllFlag;

	switch(pllReg){
		case VCLK_PLL:
			pokeRegisterDWord(PLL_SET, FIELD_SET(peekRegisterDWord(PLL_SET),PLL_SET,VCLK0PD,ON));
			break;
		case VCLK1_PLL:
			pokeRegisterDWord(PLL_SET, FIELD_SET(peekRegisterDWord(PLL_SET),PLL_SET,VCLK1PD,ON));
			break;
		case VCLK2_PLL:
			pokeRegisterDWord(PLL_SET1, FIELD_SET(peekRegisterDWord(PLL_SET1),PLL_SET1,VCLK2PD,ON));
			break;
	}
	
	ulTmpValue = peekRegisterDWord(pllReg);
	pokeRegisterDWord(pllReg,  ddk770_formatPllReg(pPLL,ulTmpValue));


	//Set VCLK CP
	switch(pllReg){
		case VCLK_PLL:
			pokeRegisterDWord(PLL_SET1, FIELD_VALUE(peekRegisterDWord(PLL_SET1),PLL_SET1,VCLK0CP,0x2));
			break;
		case VCLK1_PLL:
			pokeRegisterDWord(VCLK1_PLL, FIELD_VALUE(peekRegisterDWord(VCLK1_PLL),VCLK1_PLL,VCLK1CP,0x2));
			break;
		case VCLK2_PLL:
			pokeRegisterDWord(VCLK2_PLL, FIELD_VALUE(peekRegisterDWord(VCLK2_PLL),VCLK2_PLL,VCLK2CP,0x2));
			break;
	}

	switch(pllReg){
		case VCLK_PLL:
			pokeRegisterDWord(PLL_SET, FIELD_SET(peekRegisterDWord(PLL_SET),PLL_SET,VCLK0PD,OFF));
			break;
		case VCLK1_PLL:
			pokeRegisterDWord(PLL_SET, FIELD_SET(peekRegisterDWord(PLL_SET),PLL_SET,VCLK1PD,OFF));
			break;
		case VCLK2_PLL:
			pokeRegisterDWord(PLL_SET1, FIELD_SET(peekRegisterDWord(PLL_SET1),PLL_SET1,VCLK2PD,OFF));
			break;
	}
		usleep_range(100, 200);

    do{
        switch(pllReg){
            case VCLK_PLL:
                pllFlag = FIELD_VAL_GET(peekRegisterDWord(PLL_SET),PLL_SET,VCLK0LOCK);
                break;
            case VCLK1_PLL:
                pllFlag = FIELD_VAL_GET(peekRegisterDWord(PLL_SET),PLL_SET,VCLK1LOCK);
                break;
            case VCLK2_PLL:
                pllFlag = FIELD_VAL_GET(peekRegisterDWord(PLL_SET),PLL_SET,VCLK2LOCK);
                break;
        }

    }while(!pllFlag && (timeout--));
		

    return 0; 
}

__attribute__((unused)) static void SetPllSSCG(unsigned long pllReg)
{
		unsigned int ulTmpValue;
		ulTmpValue = peekRegisterDWord(SSCG_CTL);
		
		ulTmpValue = FIELD_SET(ulTmpValue,SSCG_CTL, FN, ENABLE);
      	ulTmpValue = FIELD_SET(ulTmpValue, SSCG_CTL, PORCORE, ENABLE);
     	ulTmpValue = FIELD_SET(ulTmpValue, SSCG_CTL, SSCGPD, OFF);

		pokeRegisterDWord(SSCG_CTL, ulTmpValue );



		ulTmpValue = peekRegisterDWord(SSCG_SET);
      	ulTmpValue = FIELD_SET(ulTmpValue, SSCG_SET, MODE, NO);
   //  	ulTmpValue = FIELD_VALUE(ulTmpValue, SSCG_SET, DIV, 0x246d);
		ulTmpValue = FIELD_VALUE(ulTmpValue, SSCG_SET, DIV, 0x1ae);
		pokeRegisterDWord(SSCG_SET, ulTmpValue );


		

		ulTmpValue = peekRegisterDWord(pllReg);
		ulTmpValue = FIELD_SET(ulTmpValue ,VCLK_PLL, SSCSEL,SSCG);
		pokeRegisterDWord(pllReg, ulTmpValue);

		return;
		
}




void Set_JPUVPU_PLL2_600(void){

	unsigned long ulTmpValue;
    unsigned int timeout = 10000;
    unsigned int pllFlag;
    // Set PLL2_PD = 1 

	pokeRegisterDWord(PLL_SET, FIELD_SET(peekRegisterDWord(PLL_SET),PLL_SET,PLL2PD,ON));


    // PLL2 to 600MHz,PLL2_PRE_SEL=0, PLL2_DIVIDER_SEL[7:0]=8'h30, PLL3_VCO_SEL=3'000 

    ulTmpValue = peekRegisterDWord(PLL2_PLL);

	ulTmpValue = FIELD_SET(ulTmpValue,PLL2_PLL, PRESEL,250);
	ulTmpValue = FIELD_VALUE(ulTmpValue, PLL2_PLL, VCO, 0x0);
	ulTmpValue = FIELD_VALUE(ulTmpValue, PLL2_PLL, DIVIDER, 0x30); 

	pokeRegisterDWord(PLL2_PLL,   ulTmpValue );
		
    // Set PLL2_PD = 0
    pokeRegisterDWord(PLL_SET, FIELD_SET(peekRegisterDWord(PLL_SET),PLL_SET,PLL2PD,OFF));

    do{
        
        pllFlag = FIELD_VAL_GET(peekRegisterDWord(PLL_SET),PLL_SET,PLL2LOCK);


    }while(!pllFlag && (timeout--));

}

__attribute__((unused)) static void Set_Sys_PLL3_400(void){

	unsigned long ulTmpValue;

    unsigned int timeout = 10000;
    unsigned int pllFlag;
	
    // Set SYSCLK_SEL = 3'b010 25MHz
	ulTmpValue = peekRegisterDWord(0x3bc);
	ulTmpValue = (ulTmpValue & (~0x7U)) | 0x02;
	pokeRegisterDWord(0x3bc, ulTmpValue);
	
    // Set PLL3_PD = 1 

	pokeRegisterDWord(PLL3_SET, FIELD_SET(peekRegisterDWord(PLL3_SET),PLL3_SET,PLL3PD,ON));


    // PLL3 APB to 800MHz,PLL3_PRE_SEL=0, PLL3_DIVIDER_SEL[7:0]=8'h20, PLL3_VCO_SEL=3'000 
    ulTmpValue= peekRegisterDWord(PLL3_PLL);
	ulTmpValue =  FIELD_SET(ulTmpValue,PLL3_PLL, PRESEL,250);
	ulTmpValue =  FIELD_VALUE(ulTmpValue, PLL3_PLL, VCO, 0x2);
	ulTmpValue =  FIELD_VALUE(ulTmpValue, PLL3_PLL, DIVIDER, 0x20); 

	pokeRegisterDWord(PLL3_PLL,  ulTmpValue) ;
		
    // Set PLL3_PD = 0
    pokeRegisterDWord(PLL3_SET, FIELD_SET(peekRegisterDWord(PLL3_SET),PLL3_SET,PLL3PD,OFF));

	usleep_range(100, 200);
	
    
    do{
        
        pllFlag = FIELD_VAL_GET(peekRegisterDWord(PLL_SET),PLL_SET,PLL3LOCK);


    }while(!pllFlag && (timeout--));

    // Set SYSCLK_SEL = 3'b001
	ulTmpValue = peekRegisterDWord(0x3bc);
	ulTmpValue = (ulTmpValue & (~0x7U)) | 0x01;
	pokeRegisterDWord(0x3bc, ulTmpValue);


}

static void Set_Sys_PLL3_600(void){

	unsigned long ulTmpValue;

    unsigned int timeout = 10000;
    unsigned int pllFlag;
	
    // Set SYSCLK_SEL = 3'b010 25MHz Crystal Clock
	ulTmpValue = peekRegisterDWord(0x3bc);
	ulTmpValue = (ulTmpValue & (~0x7U)) | 0x02;
	pokeRegisterDWord(0x3bc, ulTmpValue);
	
    // Set PLL3_PD = 1  Need to set 0x230 bit 10

	pokeRegisterDWord(PLL3_SET, FIELD_SET(peekRegisterDWord(PLL3_SET),PLL3_SET,PLL3PD,ON));


    // PLL3 APB to 800MHz,PLL3_PRE_SEL=0, PLL3_DIVIDER_SEL[7:0]=8'h20, PLL3_VCO_SEL=3'000 
    ulTmpValue= peekRegisterDWord(PLL3_PLL);
	ulTmpValue =  FIELD_SET(ulTmpValue,PLL3_PLL, PRESEL,250);
	ulTmpValue =  FIELD_VALUE(ulTmpValue, PLL3_PLL, VCO, 0x01);
	ulTmpValue =  FIELD_VALUE(ulTmpValue, PLL3_PLL, DIVIDER, 0x30); 

	pokeRegisterDWord(PLL3_PLL,  ulTmpValue) ;
		
    // Set PLL3_PD = 0  0x230 bit 10
    pokeRegisterDWord(PLL3_SET, FIELD_SET(peekRegisterDWord(PLL3_SET),PLL3_SET,PLL3PD,OFF));

	usleep_range(100, 200);
	
    
    do{
        
        pllFlag = FIELD_VAL_GET(peekRegisterDWord(PLL_SET),PLL_SET,PLL3LOCK);


    }while(!pllFlag && (timeout--));

    // Set SYSCLK_SEL = 3'b001
	ulTmpValue = peekRegisterDWord(0x3bc);
	ulTmpValue = (ulTmpValue & (~0x7U)) | 0x01;
	pokeRegisterDWord(0x3bc, ulTmpValue);

}




__attribute__((unused)) static void Set_Sys_PLL3_500(void){

	unsigned long ulTmpValue;

    unsigned int timeout = 10000;
    unsigned int pllFlag;
	
    // Set SYSCLK_SEL = 3'b010 25MHz
	ulTmpValue = peekRegisterDWord(0x3bc);
	ulTmpValue = (ulTmpValue & (~0x7U)) | 0x02;
	pokeRegisterDWord(0x3bc, ulTmpValue);
	
    // Set PLL3_PD = 1  Need to set 0x230 bit 10

	pokeRegisterDWord(PLL3_SET, FIELD_SET(peekRegisterDWord(PLL3_SET),PLL3_SET,PLL3PD,ON));


    // PLL3 APB to 800MHz,PLL3_PRE_SEL=0, PLL3_DIVIDER_SEL[7:0]=8'h20, PLL3_VCO_SEL=3'000 
    ulTmpValue= peekRegisterDWord(PLL3_PLL);
	ulTmpValue =  FIELD_SET(ulTmpValue,PLL3_PLL, PRESEL,250);
	ulTmpValue =  FIELD_VALUE(ulTmpValue, PLL3_PLL, VCO, 0x01);
	ulTmpValue =  FIELD_VALUE(ulTmpValue, PLL3_PLL, DIVIDER, 0x28); 

	pokeRegisterDWord(PLL3_PLL,  ulTmpValue) ;
		
    // Set PLL3_PD = 0  0x230 bit 10
    pokeRegisterDWord(PLL3_SET, FIELD_SET(peekRegisterDWord(PLL3_SET),PLL3_SET,PLL3PD,OFF));

	usleep_range(100, 200);
	
    
    do{
        
        pllFlag = FIELD_VAL_GET(peekRegisterDWord(PLL_SET),PLL_SET,PLL3LOCK);


    }while(!pllFlag && (timeout--));

    // Set SYSCLK_SEL = 3'b001
	ulTmpValue = peekRegisterDWord(0x3bc);
	ulTmpValue = (ulTmpValue & (~0x7U)) | 0x01;
	pokeRegisterDWord(0x3bc, ulTmpValue);

}





static void Set_Sys_PLL3_800(void){

	unsigned long ulTmpValue;

    unsigned int timeout = 10000;
    unsigned int pllFlag;
	
    // Set SYSCLK_SEL = 3'b010 25MHz Crystal Clock
	ulTmpValue = peekRegisterDWord(0x3bc);
	ulTmpValue = (ulTmpValue & (~0x7U)) | 0x02;
	pokeRegisterDWord(0x3bc, ulTmpValue);
	
    // Set PLL3_PD = 1 

	pokeRegisterDWord(PLL3_SET, FIELD_SET(peekRegisterDWord(PLL3_SET),PLL3_SET,PLL3PD,ON));


    // PLL3 APB to 800MHz,PLL3_PRE_SEL=0, PLL3_DIVIDER_SEL[7:0]=8'h20, PLL3_VCO_SEL=3'000 
    ulTmpValue= peekRegisterDWord(PLL3_PLL);
	ulTmpValue =  FIELD_SET(ulTmpValue,PLL3_PLL, PRESEL,250);
	ulTmpValue =  FIELD_VALUE(ulTmpValue, PLL3_PLL, VCO, 0x0);
	ulTmpValue =  FIELD_VALUE(ulTmpValue, PLL3_PLL, DIVIDER, 0x20); 

	pokeRegisterDWord(PLL3_PLL,  ulTmpValue) ;
		
    // Set PLL3_PD = 0
    pokeRegisterDWord(PLL3_SET, FIELD_SET(peekRegisterDWord(PLL3_SET),PLL3_SET,PLL3PD,OFF));

	usleep_range(100, 200);
	
    
    do{
        
        pllFlag = FIELD_VAL_GET(peekRegisterDWord(PLL_SET),PLL_SET,PLL3LOCK);


    }while(!pllFlag && (timeout--));

    // Set SYSCLK_SEL = 3'b001
	ulTmpValue = peekRegisterDWord(0x3bc);
	ulTmpValue = (ulTmpValue & (~0x7U)) | 0x01;
	pokeRegisterDWord(0x3bc, ulTmpValue);


}

static void Sys_PLL_Setting(ddr_type_t type){
    
    unsigned long ulTmpValue;

    if( LPDDR4_3P2K_FD == type  ||  LPDDR4_3P2K_HD == type || DDR4_3P2K_FD == type  ||  DDR4_3P2K_HD == type ){
        //system/DDR: 800MHz@3200   CPU: 400MHz   APB: 200MHz   ESM: 200MHz   USB RAM: 400MHz   PCIe AXI: 400MHz   VPU bclk: 400MHz)

        
       ulTmpValue = peekRegisterDWord(0x3b0);

	   // Set CPU clock   0x0003b0[6:4]=3'h4   system clock divided by 2
	   ulTmpValue = (ulTmpValue & (~(0x7U<<4))) | (0x02 << 4);
	   // Set APB clock   0x0003b0[10:8]=3'h4   system clock divided by 4
       ulTmpValue = (ulTmpValue & (~(0x7U<<8))) | (0x04 << 8);

	  // Set HDCP ESM clock  0x0003b0[18:16]=3'h4 system clock divided by 4
       ulTmpValue = (ulTmpValue & (~(0x7U<<16))) | (0x04 << 16);

	   //clock source for sram in the USB mac. system clock divided by 2
	   ulTmpValue = (ulTmpValue & (~(0x7U<<24))) | (0x02 << 24);

	   
       pokeRegisterDWord(0x3b0, ulTmpValue);

		printk("sys pll to 800\n");
  
       Set_Sys_PLL3_800();

    }
    else if( DDR4_2P4K_FD == type  ||  DDR4_2P4K_HD == type  ||  LPDDR4_2P4K_FD == type ||  LPDDR4_2P4K_HD == type){
        //system/DDR: 800MHz@3200   CPU: 400MHz   APB: 200MHz   ESM: 200MHz   USB RAM: 400MHz   PCIe AXI: 400MHz   VPU bclk: 400MHz)

		 ulTmpValue = peekRegisterDWord(0x3b0);
		
		 // Set CPU clock	0x0003b0[6:4]=3'h4	 system clock divided by 2
		 ulTmpValue = (ulTmpValue & (~(0x7U<<4))) | (0x02 << 4);
		 // Set APB clock	0x0003b0[10:8]=3'h4   system clock divided by 4
		 ulTmpValue = (ulTmpValue & (~(0x7U<<8))) | (0x04 << 8);
		
		// Set HDCP ESM clock  0x0003b0[18:16]=3'h4 system clock divided by 4
		 ulTmpValue = (ulTmpValue & (~(0x7U<<16))) | (0x04 << 16);
		
		 //clock source for sram in the USB mac. system clock divided by 2
		 ulTmpValue = (ulTmpValue & (~(0x7U<<24))) | (0x02 << 24);
		
		 
		 pokeRegisterDWord(0x3b0, ulTmpValue);

		
        printk("sys pll to 600\n");
  
        Set_Sys_PLL3_600();
    }
    else{
	    printk("Sys PLL to 400\n");
        //system/DDR: 400MHz@1600   CPU: 400MHz   APB: 200MHz   ESM: 200MHz   USB RAM: 400MHz   PCIe AXI: 400MHz   VPU bclk: 400MHz


		//0x3b0  Power-on Default	0x12111221
        // Set CPU clock   0x0003b0[6:4]=3'h1  system clock (PLL3 output clock in normal mission mode)
        // Set  ESM clock  0x0003b0[18:16]=3'h2   system clock divided by 2
        // Set  USB ram clock  0x0003b0[26:24]=3'h1  system clock (PLL3 output clock in normal mission mode)
       ulTmpValue = peekRegisterDWord(0x3b0);
       ulTmpValue = (ulTmpValue & (~(0x7U<<4))) | (0x01 << 4);
       ulTmpValue = (ulTmpValue & (~(0x7U<<16))) | (0x02 << 16);
       ulTmpValue = (ulTmpValue & (~(0x7U<<24))) | (0x01 << 24);
       pokeRegisterDWord(0x3b0, ulTmpValue);

        // Set  PCIe AXI Clock  0x0003b4[2:0]=3'h1
        // Set  VPU bclk  0x0003b4[6:4]=3'h1
       ulTmpValue = peekRegisterDWord(0x3b4);
       ulTmpValue = (ulTmpValue & (~0x7U)) | 0x01;
       ulTmpValue = (ulTmpValue & (~(0x7U<<4))) | (0x01 << 4);
       pokeRegisterDWord(0x3b4, ulTmpValue);


    }      
}

unsigned int Check_DDR_Rate(void)
{

    unsigned long ulTmpValue;
    ddr_fre_type_t type;
	
    //strap5: 0x010018[1]
    ulTmpValue = peekRegisterDWord(STRAP_PINS2);

	if (FIELD_VAL_GET(ulTmpValue, STRAP_PINS2, DDRRATE)  == STRAP_PINS2_DDRRATE_3200){
        type = DDR4_3200;
    }
    else{
        type = DDR4_1600;
    }
        
    
	return type;
}

void Sys_Init_PLL(ddr_type_t type)
{
    

	Sys_PLL_Setting(type);

}
