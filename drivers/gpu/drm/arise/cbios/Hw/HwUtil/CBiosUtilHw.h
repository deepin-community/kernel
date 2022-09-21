//*****************************************************************************
//  Copyright (c) 2021 Glenfly Tech Co., Ltd.. 
//  All Rights Reserved.
//
//  This is UNPUBLISHED PROPRIETARY SOURCE CODE of Glenfly Tech Co., Ltd..;
//  the contents of this file may not be disclosed to third parties, copied or
//  duplicated in any form, in whole or in part, without the prior written
//  permission of Glenfly Tech Co., Ltd..
//
//  The copyright of the source code is protected by the copyright laws of the People's
//  Republic of China and the related laws promulgated by the People's Republic of China
//  and the international covenant(s) ratified by the People's Republic of China. 
//*****************************************************************************


/*****************************************************************************
** DESCRIPTION:
** CBios hw utility common functions prototype.
**
** NOTE:
** This header file CAN ONLY be included by hw layer those files under Hw folder. 
******************************************************************************/

#ifndef _CBIOS_UTIL_HW_H_
#define _CBIOS_UTIL_HW_H_

// Bit 2 of Backward Comp2 Register (CR33 read-only) is Vertical Sync Active.
// If set, then display is in the vertical retrace mode;
// if clear, then display is in display mode.
#define VSYNC_ACTIVE_CR33     0x04    
#define VBLANK_ACTIVE_CR34    0x08

//mmioOffset of registers
#define MMIO_OFFSET_SR_GROUP_A  0x8600
#define MMIO_OFFSET_SR_GROUP_B  0x8700
#define MMIO_OFFSET_SR_GROUP_T  0x9400  // IGA3 registers
#define MMIO_OFFSET_SR_GROUP_F  0x9000
#define MMIO_OFFSET_CR_GROUP_A  0x8800
#define MMIO_OFFSET_CR_GROUP_B  0x8900
#define MMIO_OFFSET_CR_GROUP_C  0x8A00
#define MMIO_OFFSET_CR_GROUP_D  0x8B00  // for 4-channel MIU CR_D registers
#define MMIO_OFFSET_CR_GROUP_D0 0x8C00  // MIU channel 0 CR_D registers
#define MMIO_OFFSET_CR_GROUP_D1 0x8D00  // MIU channel 1 CR_D registers
#define MMIO_OFFSET_CR_GROUP_D2 0x8E00  // MIU channel 2 CR_D registers
#define MMIO_OFFSET_CR_GROUP_D3 0x8F00  // MIU channel 3 CR_D registers
#define MMIO_OFFSET_CR_GROUP_T  0x9500  // IGA3 registers
#define MMIO_OFFSET_CR_GROUP_F  0x9100

#define CB_ATTR_ADDR_REG            0x83C0
#define CB_ATTR_DATA_WRITE_REG      0x83C0
#define CB_ATTR_DATA_READ_REG       0x83C1
#define CB_GRAPH_ADDR_REG           0x83CE
#define CB_GRAPH_DATA_REG           0x83CF
#define CB_INPUT_STATUS_0_REG       0x83C2
#define CB_INPUT_STATUS_1_REG       0x83DA
#define CB_ATTR_INITIALIZE_REG      CB_INPUT_STATUS_1_REG
#define CB_MISC_OUTPUT_READ_REG     0x83CC
#define CB_MISC_OUTPUT_WRITE_REG    0x83C2
#define CB_SEQ_ADDR_REG             0x83C4
#define CB_SEQ_DATA_REG             0x83C5
#define CB_CRT_ADDR_REG             0x83D4
#define CB_CRT_DATA_REG             0x83D5

// HDCP
#define HDCP1              1
#define HDCP2              2
#define HDCP3              3
#define HDCP4              4

#define HDCPCTL1_DEST      0x82B4
#define HDCPCTL2_DEST      0x82B8
#define HDCP2_CTL1_DEST   0x33C60
#define HDCP2_CTL2_DEST    0x33C64
#define HDCP3_CTL1_DEST    0x34360
#define HDCP3_CTL2_DEST    0x34364
#define HDCP4_CTL1_DEST    0x34A60
#define HDCP4_CTL2_DEST    0x34A64


//for HDCP 0x82B8/0x33008 bit definition
#define HDCP_I2C_ENABLE_DEST      0x00000001
#define HDCP_I2C_START_DEST       0x00000010
#define HDCP_I2C_WDAV_DEST        0x00000008
#define HDCP_I2C_RDREQ_DEST       0x00000040
#define HDCP_I2C_RDAV_DEST        0x00000080
#define HDCP_I2C_STOP_DEST        0x00000020
#define HDCP_I2C_STATUS_DEST      0x00000800
#define HDCP_I2C_REQUEST_DEST     0x00000002
#define HDCP_KEY_RW_DEST          0x00004000
#define HDCP_KEY_AVL_DEST         0x00008000
#define HDCP_KEY_LAST_DEST        0x40000000
#define HDCP_KEY_RKEY_DEST        0x80000000

/*
*  Following is Port80 value been used in CBIOS
*  0xA0C0xFF for CBIOS
*  and 0xF0-0xF4 for CBIOS team internal debug
*  Notes:Following Port80 value has been used in VBIOS,can not been used in CBIOS
*       0A0h 0A1h 0A2h 0A3h 0A4h 0A5h 0A6h 0A7h 0A9h 0ABh 0ADh 0AFh
*       0B0h 0B1h 0B2h 0B3h 0B4h 0B5h 0B6h 0B7h
*       0D0h 0D1h 0D2h 0D3h 0D4h 0D6h 0D7h 0D8h 0D9h 0DCh
*       0F9h 0FAh 0FBh
*/
#define CBIOS_PORT80_ID_CBiosPost_Enter                              0xB8
#define CBIOS_PORT80_ID_CBiosPost_Exit                               0xB9
#define CBIOS_PORT80_ID_CBiosSetModeToIGA_Enter                      0xC3
#define CBIOS_PORT80_ID_CBiosSetModeToIGA_Exit                       0xC4
#define CBIOS_PORT80_ID_CBiosSetClock_Enter                          0xC8
#define CBIOS_PORT80_ID_CBiosSetClock_Exit                           0xC9
#define CBIOS_PORT80_ID_CBiosVoltageFunction_Enter                   0xCA
#define CBIOS_PORT80_ID_CBiosVoltageFunction_Exit                    0xCB
#define CBIOS_PORT80_ID_DestructiveDeviceDetect_Enter                0xCC
#define CBIOS_PORT80_ID_DestructiveDeviceDetect_Exit                 0xCD
#define CBIOS_PORT80_ID_NonDestructiveDeviceDetect_Enter             0xCE
#define CBIOS_PORT80_ID_NonDestructiveDeviceDetect_Exit              0xCF
#define CBIOS_PORT80_ID_CBiosI2CDataWrite_Enter                      0xEC
#define CBIOS_PORT80_ID_CBiosI2CDataWrite_Exit                       0xED
#define CBIOS_PORT80_ID_CBiosI2CDataRead_Enter                       0xEE
#define CBIOS_PORT80_ID_CBiosI2CDataRead_Exit                        0xEF
/*              CBIOS post internal function                           */
#define CBIOS_PORT80_ID_AfterChipEnable                              0xBA
#define CBIOS_PORT80_ID_AfterPrePost                                 0xBB
#define CBIOS_PORT80_ID_AfterLoadDefautTables                        0xBC
#define CBIOS_PORT80_ID_AfterInitMemory                              0xBD
#define CBIOS_PORT80_ID_AfterIsMemoryWriteable                       0xBE
#define CBIOS_PORT80_ID_AfterReadHWPanelID                           0xBF
#define CBIOS_PORT80_ID_AfterProgramPCISSID                          0xC0
#define CBIOS_PORT80_ID_AfterSetupVGA                                0xC1
#define CBIOS_PORT80_ID_AfterEndOfPost                               0xC2
#define CBIOS_PORT80_ID_AfterLoadSWBootStrap                         0xF5
#define CBIOS_PORT80_ID_AfterLoadMIUTable                            0xF6
#define CBIOS_PORT80_ID_AfterAutoMemConfig                           0xF7
#define CBIOS_PORT80_ID_AfterLoadModeExtRegDefault                 0xF8
#define CBIOS_PORT80_ID_AfterLoadMode3SRRegTable                   0xFC
#define CBIOS_PORT80_ID_AfterLoadMode3CRRegTable                   0xFD
#define CBIOS_PORT80_ID_AfterLoadMode3RegOnIGA2                    0xFE
/*                Set Mode To IGA                                      */
#define CBIOS_PORT80_ID_AfterSetSrcTiming                            0xC5
#define CBIOS_PORT80_ID_AfterSetDstTiming                            0xC6
#define CBIOS_PORT80_ID_AfterSetScalerSize                           0xC7
/*                Set Source Mode                                      */
#define CBIOS_PORT80_ID_AfterModeEnvSetup                            0xDA
#define CBIOS_PORT80_ID_AfterSetCRTimingReg                          0xDB
#define CBIOS_PORT80_ID_AfterRamDacSet                               0xDD
#define CBIOS_PORT80_ID_AfterPlanarSet                               0xDE
#define CBIOS_PORT80_ID_AfterSetBPSL                                 0xDF
#define CBIOS_PORT80_ID_AfterSetDacRegisters                         0xE0
/*                Set Destination Mode                                 */
#define CBIOS_PORT80_ID_AfterDstTimingInitial                        0xE1
#define CBIOS_PORT80_ID_AfterDstTimingPatchForTech                   0xE2
#define CBIOS_PORT80_ID_AfterDstTimingRegSetting                     0xE3
#define CBIOS_PORT80_ID_AfterDstTimingContractionSetting             0xE4
#define CBIOS_PORT80_ID_AfterDstTimingPositionSetting                0xE5
#define CBIOS_PORT80_ID_AfterStreamPathMux                           0xE6
#define CBIOS_PORT80_ID_AfterDstTimingPatch                          0xE7
#define CBIOS_PORT80_ID_AfterGlobalModePatch                         0xE8
/*                 Set Scaler Size                                     */
#define CBIOS_PORT80_ID_AfterEnableUpScaler                          0xE9
#define CBIOS_PORT80_ID_AfterDisableUpScaler                         0xEA
#define CBIOS_PORT80_ID_AfterEnableCentering                         0xEB

//Register
CBIOS_VOID cbUnlockSR(PCBIOS_EXTENSION_COMMON pcbe);
CBIOS_VOID cbUnlockCR(PCBIOS_EXTENSION_COMMON pcbe);
CBIOS_VOID cbWaitForInactive(PCBIOS_EXTENSION_COMMON pcbe);
CBIOS_VOID cbWaitForActive(PCBIOS_EXTENSION_COMMON pcbe);
CBIOS_VOID cbWaitForBlank(PCBIOS_EXTENSION_COMMON pcbe);
CBIOS_VOID cbWaitForDisplay(PCBIOS_EXTENSION_COMMON pcbe);
CBIOS_U32 cbGetParallelRegIndex(PCBIOS_PARALLEL_REGISTER pRegTbl, CBIOS_U32 RegNameIndex, CBIOS_U32 ModuleIndex);
CBIOS_U8 cbBiosMMIOReadReg(PCBIOS_EXTENSION_COMMON pcbe, CBIOS_U16 type_index, CBIOS_U32 IGAIndex);
CBIOS_VOID cbBiosMMIOWriteReg(PCBIOS_EXTENSION_COMMON pcbe, CBIOS_U16 type_index, CBIOS_U8 value, CBIOS_U8 mask, CBIOS_U32 IGAIndex) ;                  
CBIOS_U8 cbMMIOReadReg(PCBIOS_EXTENSION_COMMON pcbe, CBIOS_U16 type_index);
CBIOS_VOID cbMMIOWriteReg(PCBIOS_EXTENSION_COMMON pcbe, CBIOS_U16 type_index, CBIOS_U8  value, CBIOS_U8 mask);
CBIOS_VOID cbMMIOWriteReg32(PCBIOS_EXTENSION_COMMON pcbe, CBIOS_U32 Index, CBIOS_U32 Value, CBIOS_U32 Mask);
CBIOS_U32 cbMapMaskRead(PCBIOS_EXTENSION_COMMON pcbe, CBREGISTER_IDX *regTable, CBIOS_U32 IGAIndex);
CBIOS_VOID cbMapMaskWrite(PCBIOS_EXTENSION_COMMON pcbe, CBIOS_U32 Value, CBREGISTER_IDX *regTable, CBIOS_U32 IGAIndex);
CBIOS_VOID cbLoadtable(PCBIOS_EXTENSION_COMMON pcbe, CBREGISTER *pRegTable, CBIOS_U32 Table_Size, CBIOS_U32 IGAIndex);
CBIOS_VOID cbLoadMemoryTimingTbl(PCBIOS_EXTENSION_COMMON pcbe, MMIOREGISTER* pRegTable, CBIOS_U32 Table_Size);
CBIOS_VOID cbSaveRegTableU8(PCBIOS_EXTENSION_COMMON pcbe, CBREGISTER *pRegTable, const CBIOS_U32 TableSize, CBIOS_U8* SavedRegTable);
CBIOS_VOID cbRestoreRegTableU8(PCBIOS_EXTENSION_COMMON pcbe, const CBREGISTER *pRegTable, const CBIOS_U32 TableSize, CBIOS_U8* SavedRegTable);
CBIOS_VOID cbSaveRegTableU32(PCBIOS_EXTENSION_COMMON pcbe, CBIOS_REGISTER32 *pRegTable, const CBIOS_U32 TableSize);
CBIOS_VOID cbRestoreRegTableU32(PCBIOS_EXTENSION_COMMON pcbe, const CBIOS_REGISTER32 *pRegTable, const CBIOS_U32 TableSize);
CBIOS_BOOL cbProgClock(PCBIOS_EXTENSION_COMMON pcbe, CBIOS_U32 ClockFreq, CBIOS_U32 ClockType, CBIOS_U8 IGAIndex);
CBIOS_U32 cbGetProgClock(PCBIOS_EXTENSION_COMMON pcbe, CBIOS_U32 *ClockFreq, CBIOS_U32 ClockType);
CBIOS_BOOL cbWaitVBlank( PCBIOS_EXTENSION_COMMON pcbe, CBIOS_U8 IGAIndex);
CBIOS_BOOL cbWaitNonFullVBlank(PCBIOS_EXTENSION_COMMON pcbe, CBIOS_U8 IGAIndex);
CBIOS_BOOL cbWaitVSync(PCBIOS_EXTENSION_COMMON pcbe, CBIOS_U8 IGAIndex);
CBIOS_BOOL cbHDCPDDCciPortWrite(PCBIOS_EXTENSION_COMMON pcbe,CBIOS_UCHAR Port, CBIOS_UCHAR Offset, const PCBIOS_UCHAR pWriteDataBuf, CBIOS_U32 DataLen, CBIOS_S32 HDCPChannel);
CBIOS_BOOL cbHDCPDDCciPortRead(PCBIOS_EXTENSION_COMMON pcbe, CBIOS_UCHAR Port, CBIOS_UCHAR Offset, const PCBIOS_UCHAR pReadDataBuf, CBIOS_U32 DataLen, CBIOS_S32 HDCPChannel);
CBIOS_BOOL cbIsMMIOWell(PCBIOS_EXTENSION_COMMON pcbe);
CBIOS_BOOL cbDumpRegisters(PCBIOS_EXTENSION_COMMON pcbe, CBIOS_DUMP_TYPE DumpType);
CBIOS_VOID cbDumpModeInfo(PCBIOS_EXTENSION_COMMON pcbe);
CBIOS_VOID cbDumpClockInfo(PCBIOS_EXTENSION_COMMON pcbe);
CBIOS_BOOL cbDisableHdcp(PCBIOS_EXTENSION_COMMON pcbe, CBIOS_U32 ulHDCPNum);
CBIOS_VOID cbEnableHdcpStatus(PCBIOS_EXTENSION_COMMON pcbe, CBIOS_U32 ulHDCPNum);
CBIOS_BOOL cbHDCPProxyGetEDID(PCBIOS_EXTENSION_COMMON pcbe, CBIOS_U8 EDIDData[], CBIOS_U32 ulReadEdidOffset, CBIOS_U32 ulBufferSize, CBIOS_U32 ulHDCPNum, CBIOS_U8 nSegNum);
CBIOS_BOOL cbNormalI2cGetEDID(PCBIOS_EXTENSION_COMMON pcbe, CBIOS_U8 I2CBUSNum, CBIOS_U8 EDIDData[], CBIOS_U32 ulReadEdidOffset, CBIOS_U32 ulBufferSize, CBIOS_U8 nSegNum);

//Port 80
CBIOS_VOID cbWritePort80(PCBIOS_VOID pcbe, CBIOS_U8 value);

#endif
