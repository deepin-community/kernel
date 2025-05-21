
ccflags-y += \
	-I$(GFGPU_FULL_PATH)/cbios \
    -I$(GFGPU_FULL_PATH)/cbios/Callback \
    -I$(GFGPU_FULL_PATH)/cbios/Device \
    -I$(GFGPU_FULL_PATH)/cbios/Device/Port \
    -I$(GFGPU_FULL_PATH)/cbios/Device/Monitor \
    -I$(GFGPU_FULL_PATH)/cbios/Device/Monitor/DSIPanel \
    -I$(GFGPU_FULL_PATH)/cbios/Device/Monitor/EDPPanel \
    -I$(GFGPU_FULL_PATH)/cbios/Display \
    -I$(GFGPU_FULL_PATH)/cbios/Init \
    -I$(GFGPU_FULL_PATH)/cbios/Util

cbios-objs := \
    Interface/CBios.o \
    Callback/CBiosCallbacks.o \
    Init/CBiosInit.o      \
    Util/CBiosUtil.o      \
    Util/CBiosEDID.o      \
    Display/CBiosDisplayManager.o \
    Display/CBiosPathManager.o \
    Display/CBiosMode.o      \
    Device/CBiosShare.o \
    Device/CBiosDeviceShare.o \
    Device/CBiosDevice.o    \
    Device/Port/CBiosCRT.o       \
    Device/Port/CBiosDP.o        \
    Device/Port/CBiosDSI.o       \
    Device/Port/CBiosDVO.o       \
    Device/Monitor/CBiosCRTMonitor.o \
    Device/Monitor/CBiosDPMonitor.o     \
    Device/Monitor/CBiosHDMIMonitor.o   \
    Device/Monitor/CBiosEDPPanel.o \
    Device/Monitor/EDPPanel/CBiosITN156.o \
    Device/Monitor/DSIPanel/CBiosDSIPanel.o \
    Device/Monitor/DSIPanel/CBiosHX8392A.o \
    Device/Monitor/DSIPanel/CBiosNT35595.o \
    Device/Monitor/DSIPanel/CBiosR63319.o \
    Device/Monitor/DSIPanel/CBiosR63417.o \
    Hw/CBiosChipFunc.o  \
    Hw/HwInit/CBiosInitHw.o       \
    Hw/HwCallback/CBiosCallbacksHw.o \
    Hw/HwInterface/CBiosHwInterface.o      \
    Hw/HwUtil/CBiosI2C.o       \
    Hw/HwUtil/CBiosUtilHw.o       \
    Hw/Interrupt/CBiosISR.o       \
    Hw/HwBlock/CBiosScaler.o    \
    Hw/HwBlock/CBiosIGA_Timing.o  \
    Hw/HwBlock/CBiosDIU_HDTV.o  \
    Hw/HwBlock/CBiosDIU_HDAC.o  \
    Hw/HwBlock/CBiosDIU_HDCP.o  \
    Hw/HwBlock/CBiosDIU_HDMI.o \
    Hw/HwBlock/CBiosDIU_DP.o \
    Hw/HwBlock/CBiosDIU_CRT.o \
    Hw/HwBlock/CBiosDIU_DVO.o \
    Hw/HwBlock/CBiosDIU_VIP.o \
    Hw/HwBlock/CBiosDIU_CSC.o \
    Hw/HwBlock/CBiosPHY_DP.o \
    Hw/Arise/CBios_Arise.o          \
    Hw/Arise/CBiosVCP_Arise.o

$(DRIVER_NAME)-objs += $(addprefix cbios/, $(cbios-objs))

