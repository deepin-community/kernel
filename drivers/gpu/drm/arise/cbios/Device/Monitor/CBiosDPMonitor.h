/*
 * Copyright © 2021 Glenfly Tech Co., Ltd.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 */

#ifndef _CBIOS_DP_MONITOR_H_
#define _CBIOS_DP_MONITOR_H_

#include "../CBiosDeviceShare.h"

#define HARDCODE_DP1_MAX_LINKSPEED_1620             0x00000001
#define HARDCODE_DP1_MAX_LINKSPEED_2700             0x00000002
#define HARDCODE_DP1_MAX_LINKSPEED_5400             0x00008000
#define HARDCODE_DP1_LANECOUNT_1                    0x00000004
#define HARDCODE_DP1_LANECOUNT_2                    0x00000008
#define HARDCODE_DP1_LANECOUNT_4                    0x00000010
#define HARDCODE_DP1_EDID_ALL_ZERO_BYTES            0x00000100
#define DISABLE_EDP_CONTENT_PROTECTION              0x00001000
#define DEFAULT_USE_EDP_CP_METHOD_3A_ASSR           0x00002000
#define DEFAULT_USE_EDP_CP_METHOD_3B_AF             0x00004000

#define DP_Default_bpc                              8
#define DP_Default_TUSize                           48
#define DP_Min_bpc                                  6

typedef enum _AUX_WORKING_STATUS
{
    AUX_WORKING_STATUS_IDLE = 0x00,
    AUX_WORKING_STATUS_LINKTRAINING = 0x01,
    AUX_WORKING_STATUS_EDID_READING = 0x02,
}AUX_WORKING_STATUS;

typedef struct
{
    CBIOS_U32          EventType;
    CBIOS_U64          Timestamp;
    union
    {
        CBIOS_U32      HpdStatus;
    };
} DP_RECV_EVENT, *PDP_RECV_EVENT;

#define MAX_DP_RECV_QUEUE_EVENTS 64
typedef struct
{
    CBIOS_U32 Head;
    CBIOS_U32 Tail;
    DP_RECV_EVENT Queue[MAX_DP_RECV_QUEUE_EVENTS];
} DP_RECV_QUEUE, *PDP_RECV_QUEUE;

#define QUEUE_SIZE(x)         (sizeof(x.Queue)/sizeof(x.Queue[0]))
#define MAX_QUEUE_DEPTH(x)    (QUEUE_SIZE(x) -1)
#define QUEUE_DEPTH(x)        ((x.Head <= x.Tail)?(x.Tail-x.Head):(QUEUE_SIZE(x)-x.Head+x.Tail))
#define QUEUE_FULL(x)         (QUEUE_DEPTH(x) >= MAX_QUEUE_DEPTH(x))
#define QUEUE_EMPTY(x)        (QUEUE_DEPTH(x) == 0)
#define ADVANCE_QUEUE_HEAD(x) { x.Head = (x.Head < MAX_QUEUE_DEPTH(x))?(x.Head+1):0; }
#define ADVANCE_QUEUE_TAIL(x) { x.Tail = (x.Tail < MAX_QUEUE_DEPTH(x))?(x.Tail+1):0; }
#define LAST_ITEM_INDEX(x)    (x.Tail == 0)?(QUEUE_SIZE(x) -1):(x.Tail-1)
#define RESET_QUEUE(x)        { do {x.Head = 0; x.Tail = 0;} while(0); }

typedef struct _CBIOS_DP_MONITOR_CONTEXT
{
    PCBIOS_DEVICE_COMMON pDevCommon;

    // monitor caps
    struct
    {
        CBIOS_U32    DpSinkVersion;         // sink version
        CBIOS_U32    SinkMaxLaneCount;      // max lane count supported by sink
        CBIOS_U32    SinkMaxLinkSpeed;      // max link speed supported by sink
        CBIOS_BOOL   bSupportASSR;          // Support content protection method 3a, ASSR
        CBIOS_BOOL   bSupportAF;            // Support content protection method 3b, AF
        CBIOS_BOOL   bSupportEnhanceMode;   // Support enhanced framing symbol sequence for BS, SR, CPBS and CPSR
        CBIOS_BOOL   bSupportTPS3;          // Support Training Pattern 3(5.4Gbps)
    };

    // source caps
    struct
    {
        CBIOS_U32    SourceMaxLaneCount;    // max lane count supported by Source
        CBIOS_U32    SourceMaxLinkSpeed;    // max link speed supported by Source
        CBIOS_BOOL   bSourceSupportTPS3;    // Support Training Pattern 3(5.4Gbps)
    };

    // control params
    struct
    {
        CBIOS_U32    LaneNumberToUse;       // 1 ~ 4 lanes
        CBIOS_U32    LinkSpeedToUse;        // 1.62Gbps, 2.7 Gbps or 5.4 Gbps
        CBIOS_U32    LinkPassLaneNum;
        CBIOS_U32    LinkPassSpeed;
        CBIOS_U32    bpc;                   // bit per channel
        CBIOS_U32    TUSize;                // 32 ~ 64, default to 48
        CBIOS_BOOL   EnhancedMode;          // 1 (yes) to support HDCP
        CBIOS_BOOL   AsyncMode;             // 1 (yes), asynchronous mode
        CBIOS_U32    ColorFormat;           // 0: RGB, 1:YCbCr422, 2:YCbCr444
        CBIOS_U32    DynamicRange;          // 0: VESA range, 1:CEA range
        CBIOS_U32    YCbCrCoefficients;     // 0: ITU601, 1: ITU709
        CBIOS_BOOL   LT_Status;             // 0:not yet link trainning (to save LT in DP ON/OFF function)
        CBIOS_U32    DpAuxWorkingStatus;    // To detect/record any simultaneous work(s) via AUX channel.
        CBiosCustmizedDestTiming TestDpcdDataTiming;  //This is only for DP automation test.
        CBIOS_TIMING_ATTRIB TargetTiming;
        CBIOS_BOOL   bInterlace;
        CBIOS_U32    TrainingAuxRdInterval; // Link Status/Adjust Request read interval during Main Link Training
                                            // 0: 100us; 1: 4ms; 2: 8ms; 3: 12ms; 4: 16ms.
        CBIOS_U8     PixelRepetition;
        CBIOS_BOOL   bEnableTPS3;           // 1: transmit Training Pattern 3 during Link Training
    };

    // eDP GPIO
    CBIOS_U8         GpioForEDP1Power;
    CBIOS_U8         GpioForEDP1BackLight;
    CBIOS_U8         GpioForEDP2Power;
    CBIOS_U8         GpioForEDP2BackLight;

    struct
    {
        // protected by spinLock between threads
        DP_RECV_QUEUE    RecvQueue;
        CBIOS_DP_NOTIFICATIONS Notifications;
    };
}CBIOS_DP_MONITOR_CONTEXT, *PCBIOS_DP_MONITOR_CONTEXT;

CBIOS_BOOL cbDPMonitor_AuxReadEDID(PCBIOS_VOID pvcbe, PCBIOS_DP_MONITOR_CONTEXT pDPMonitorContext, PCBIOS_UCHAR pEDIDBuffer, CBIOS_U32 ulBufferSize);
CBIOS_BOOL cbDPMonitor_AuxReadEDIDOffset(PCBIOS_VOID pvcbe, PCBIOS_DP_MONITOR_CONTEXT pDPMonitorContext, PCBIOS_UCHAR pEDIDBuffer, CBIOS_U32 ulBufferSize, CBIOS_U32 ulReadEdidOffset);
CBIOS_STATUS cbDPMonitor_GetCustomizedTiming(PCBIOS_VOID pvcbe, PCBIOS_DP_MONITOR_CONTEXT pDPMonitorContext, PCBIOS_DP_CUSTOMIZED_TIMING pDPCustomizedTiming);
CBIOS_STATUS cbDPMonitor_Isr(PCBIOS_VOID pvcbe, PCBIOS_DP_MONITOR_CONTEXT pDPMonitorContext, PCBIOS_DP_ISR_PARA pDPIsrPara);
CBIOS_BOOL cbDPMonitor_WorkThreadMainFunc(PCBIOS_VOID pvcbe, PCBIOS_DP_MONITOR_CONTEXT pDPMonitorContext, PCBIOS_DP_WORKTHREAD_PARA pDPWorkThreadPara);
CBIOS_STATUS cbDPMonitor_SetNotifications(PCBIOS_VOID pvcbe, PCBIOS_DP_MONITOR_CONTEXT pDPMonitorContext, PCBIOS_DP_NOTIFICATIONS pDPNotifications);
CBIOS_STATUS  cbDPMonitor_GetInt(PCBIOS_VOID pvcbe, PCBIOS_DP_MONITOR_CONTEXT pDPMonitorContext, PCBIOS_DP_INT_PARA pDPIntPara);
CBIOS_STATUS cbDPMonitor_HandleIrq(PCBIOS_VOID pvcbe, PCBIOS_DP_MONITOR_CONTEXT pDPMonitorContext, PCBIOS_DP_HANDLE_IRQ_PARA pDPHandleIrqPara);
CBIOS_U32 cbDPMonitor_GetMaxSupportedDclk(PCBIOS_VOID pvcbe, PCBIOS_DP_MONITOR_CONTEXT pDPMonitorContext);
CBIOS_BOOL cbDPMonitor_Detect(PCBIOS_VOID pvcbe, PCBIOS_DP_MONITOR_CONTEXT pDPMonitorContext, CBIOS_BOOL bHardcodeDetected, CBIOS_U32 FullDetect);
CBIOS_VOID cbDPMonitor_SetMode(PCBIOS_VOID pvcbe, PCBIOS_DP_MONITOR_CONTEXT pDPMonitorContext, PCBIOS_DISP_MODE_PARAMS pModeParams);
CBIOS_VOID cbDPMonitor_UpdateModeInfo(PCBIOS_VOID pvcbe, PCBIOS_DP_MONITOR_CONTEXT pDPMonitorContext, PCBIOS_DISP_MODE_PARAMS pModeParams);
CBIOS_VOID cbDPMonitor_QueryAttribute(PCBIOS_VOID pvcbe, PCBIOS_DP_MONITOR_CONTEXT pDPMonitorContext, PCBiosMonitorAttribute pMonitorAttribute);
CBIOS_VOID cbDPMonitor_OnOff(PCBIOS_VOID pvcbe, PCBIOS_DP_MONITOR_CONTEXT pDPMonitorContext, CBIOS_BOOL bOn);
CBIOS_VOID cbDPMonitor_SetDither(PCBIOS_VOID pvcbe, CBIOS_U32 bpc, CBIOS_BOOL bOn, CBIOS_MODULE_INDEX DPModuleIndex);
#endif
