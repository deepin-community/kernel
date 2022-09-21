#ifndef _TXGBE_BP_H_
#define _TXGBE_BP_H_

#include "txgbe.h"
#include "txgbe_type.h"
#include "txgbe_hw.h"

#define CL72_KR_TRAINING_ON



/* Backplane AN73 Base Page Ability struct*/
typedef struct TBKPAN73ABILITY
{
    unsigned int nextPage;    //Next Page (bit0)
    unsigned int linkAbility; //Link Ability (bit[7:0])
    unsigned int fecAbility;  //FEC Request (bit1), FEC Enable  (bit0)
    unsigned int currentLinkMode; //current link mode for local device
}bkpan73ability;

int txgbe_kr_intr_handle(struct txgbe_adapter *adapter);
void txgbe_bp_down_event(struct txgbe_adapter *adapter);
void txgbe_bp_watchdog_event(struct txgbe_adapter *adapter);
int txgbe_bp_mode_setting(struct txgbe_adapter *adapter);
void txgbe_bp_close_protect(struct txgbe_adapter *adapter);

#endif

