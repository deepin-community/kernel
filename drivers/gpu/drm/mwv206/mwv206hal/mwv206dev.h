/*
 * JM7200 GPU driver
 *
 * Copyright (c) 2018 ChangSha JingJiaMicro Electronics Co., Ltd.
 *
 * Author:
 *      rfshen <jjwgpu@jingjiamicro.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */
#ifndef _MWV206DEV_H_
#define _MWV206DEV_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <linux/init.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/pci.h>
#include <linux/mod_devicetable.h>
#include <linux/fs.h>
#include <linux/idr.h>
#include <linux/spinlock.h>
#include <linux/uaccess.h>
#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/i2c-algo-bit.h>
#include "mwv206addrlist.h"
#include "mwv206kconfig.h"
#include "mwv206kdevconfig.h"
#include "mwv206kdebug.h"
#include "mwv206memmgr.h"
#include "mwv206ioctl.h"
#include "mwv206reg.h"
#include "mwv206.h"
#include "jmiic.h"
#include "jmspi.h"
#include "gljos.h"
#include "gljos_kernel.h"


#define	MWV206_VERSION_KERNEL	"2.1.4"
#define	MWV206_BUILD_TIME       "20211027.1045"
#define	MWV206_VERSION_FULL     MWV206_VERSION_KERNEL "-" MWV206_BUILD_TIME
#define MWV206_KVERSTR          "mwv206kernel-" MWV206_VERSION_FULL

typedef enum tag_jme_chip_grade {
	JMV_CHIP_COMMERCIAL = 0,
	JMV_CHIP_INDUSTRIAL,
	JMV_CHIP_MILITARY
} jme_chip_grade;

#define V206DEV009 0x72000731
#define V206DEV010(_pDev) ((_pDev)->V206DEV030 == V206DEV009)

typedef struct V206DEV011 {
	int             dev;
	unsigned long   param;
	int             type;
	MWV206INTRFUNC  V206DEV070;
} V206DEV012;

#define V206DEV013       16384
#define VBIOS_PRJ_STR_LEN       7
#define VBIOS_CFG_VER_STR_LEN   63

struct V206DEV014 {
	int size;
	unsigned long startaddr;
	unsigned long absstartaddr;
	int V206IOCTLCMD018, V206IOCTLCMD020;
	unsigned int V206DEV165;

};

struct V206DEV015 {
	int V206IOCTLCMD018, V206IOCTLCMD020;
	int mask;
	int V206IOCTLCMD019;
	int free_words;
	unsigned int *V206DEV161;

	unsigned long long V206DEV162;
	unsigned long long V206DEV163;
	unsigned int V206DEV164;
	unsigned int V206DEV165;
};

typedef struct V206DEV016 {
	unsigned int mode;
	int (*send)(void *pDev, const unsigned int *cmd, int cmdcount);
	int (*wait)(void *pDev, long timeout);
	int (*isidle)(void *pDev);
	int (*flush)(void *pDev);
	unsigned int first;
} MWV206Cmd3d_t;

#define V206DEV017  (-1U)
#define V206DEV018 (-31)
typedef struct V206DEV019 {
	unsigned int mode;
	int (*send)(void *pDev, const unsigned int *cmd, int cmdcount, unsigned int offset);
	int (*wait)(void *pDev, long timeout);
	int (*isidle)(void *pDev);
	int (*flush)(void *pDev);
	unsigned int first;
	unsigned int offset;
} V206DEV020;

typedef struct V206DEV021 {
	int hpol;
	int vpol;
} V206DEV022;

typedef unsigned int (*V206DEV023)(void *pDev, int size, int align, void *userdata);

struct V206DEV024;
struct scanctrl {
	V206IOCTL159 V206DEV143;
	V206IOCTL159 shadowaddr[3];
	struct V206DEV024 *pDev;
	struct work_struct    work;
	struct hrtimer        timer;
	ktime_t               vsync_interval;
	ktime_t               last_update_time;
	int                   shadow_on;
	int                   drawing;
	int                   drawfb_dirty;
	int                   display_pending;
	int                   crtc_connected;
	int                   crtc_configured;
	int                   last_update_idx;
};

struct mwv206_i2c;


typedef struct V206DEV024 {
	void                   *V206DEV027;
	u32                     V206DEV028;
	unsigned int            V206DEV029;
	unsigned long           V206DEV030;
	unsigned long           V206DEV031[2];
	unsigned long           V206DEV032[2];
	unsigned long           V206DEV033;
	unsigned long           V206DEV034;
	unsigned long           V206DEV035;
	unsigned long           V206DEV036;
	unsigned long           V206DEV037;
	unsigned long           V206DEV038;
	unsigned long           V206DEV039;
	unsigned long           V206DEV040;
	unsigned int            V206DEV041;
	unsigned int            V206DEV042;
	unsigned int            V206DEV043;
	unsigned int            V206DEV044[2];
	unsigned int            V206DEV121[6];
	unsigned int            V206DEV045;
	int                     irq;
	unsigned long           V206DEV046;
	unsigned long           V206DEV047;
	unsigned long           V206DEV048;
	int                     V206DEV049;
	unsigned int            V206DEV050;
	JMIIC                   V206DEV051;
#if 0
	JMSPI                   V206DEV052;
#endif

	JMSPI                   V206DEV053;
	JMSPI                   V206DEV054;
	GLJ_LOCK                V206DEV055;
	GLJ_LOCK                V206DEV056;
	GLJ_LOCK                V206DEV057;
	GLJ_LOCK                V206DEV058;
	GLJ_LOCK                V206DEV059;
	GLJ_LOCK                V206DEV060;
	GLJ_LOCK                V206DEV061;
	GLJ_LOCK                V206DEV062;
	GLJ_EVENT               V206DEV064[MWV206KINTR_COUNT + 9];

	int                      V206DEV065;

	unsigned long           V206DEV066[MWV206KINTR_GROUP];
	unsigned int            V206DEV067[MWV206KINTR_COUNT + 9];

	MemMgr                  V206DEV068[4];
	int                     V206DEV069;

	V206DEV012        V206DEV070[MWV206KINTR_COUNT];
	int V206DEV071;
	int V206DEV072;
	int V206DEV073;
	int V206DEV074;
	int V206DEV075;
	unsigned int V206DEV077;
	int V206DEV078[4][2];
	int V206DEV079[4];
	struct V206DEV015 V206DEV080;
	struct V206DEV014 V206DEV081;
	unsigned int V206DEV082[2];
	unsigned int V206DEV083[2];

	int V206DEV084;
	int V206DEV085;

	mwv206_addrlist_t *V206DEV086;
	mwv206_addrlist_t *V206DEV087;

	int V206DEV088;
	unsigned int V206DEV089[0x40000];

	struct {

		GLJ_LOCK V206DEV166;
		GLJ_LOCK V206DEV167[4][2];
		void *V206DEV168[4][2];
		unsigned long long V206DEV169[4][2];
		unsigned int V206DEV170[4][2];
		unsigned int V206DEV171;
		int dma_is_err;
	} V206DEV090;
	int V206DEV091;
	int V206DEV092;
	int V206DEV093[4];

	unsigned int  V206DEV094[(1000 - (64))];
	unsigned int  V206DEV095;
	V206DEV020 V206DEV096;
	MWV206Cmd3d_t V206DEV097;
	struct {
		unsigned long vaddr;
		unsigned long dma_handle;
		int size;
	} V206DEV098;;

	int                     V206DEV099;
	struct semaphore        V206DEV100;
	GLJOS_SPINLOCK          V206DEV101;
	struct pci_dev          *V206DEV103;
	void *fb_info;
	void *V206DEV104;
	struct mwv206_dev_config         V206DEV105;
	struct mwv206_board_config       V206DEV106;
	char                    vbioscfgver[VBIOS_CFG_VER_STR_LEN + 1];
	int                     V206DEV107;
	int                     isdevcfgdefault;
	char                    vbios_prj_str[VBIOS_PRJ_STR_LEN + 1];
	struct {
		int V206DEV172;
		int V206DEV173;
		int V206DEV174;
		int V206DEV175;
		unsigned long total;
		unsigned long V206DEV176;
		spinlock_t lock;
		struct timer_list timer;
	} V206DEV108;
	struct pm {
		int V206DEV109;
		int hdmi_legacy_mode;
		int V206DEV110[4];
		int V206DEV111[4];
		int V206DEV112[MWV206_DP_COUNT];
		int V206DEV113[MWV206_DP_COUNT];
		int V206DEV114[MWV206_DP_COUNT];

		V206IOCTL161 V206DEV115[4];
		V206IOCTL159 V206DEV116[4];
		V206IOCTL172 V206DEV117[MWV206_DP_COUNT];
		V206IOCTL173 V206DEV118[MWV206_DP_COUNT];
		V206IOCTL168 V206DEV119[4];
		SaveMemMgr V206DEV120;
		int V206DEV121[6];
		struct {
			unsigned int V206DEV126;
			unsigned int V206DEV127[16];
			unsigned int V206DEV128;
			unsigned int V206DEV129[639];

			unsigned int V206DEV130;
			unsigned int V206DEV131[20];
			unsigned int V206DEV132;
			unsigned int V206DEV133[64];
		} V206DEV134;
		int V206DEV135[256];
		struct {
			unsigned int frame_addr;
			unsigned int bmp_val;
			unsigned int pos_val;
			unsigned int enable;
		} cursor[4];
		unsigned int win[4][30];
		int palette_valid[4];
		V206IOCTL162 palettes[4];
	} pm;

	wait_queue_head_t V206DEV136;
	unsigned long V206DEV137;
	void *V206DEV138;
	struct V206DEV139 *audio[4];
	unsigned int pixelclock[4] ;
	int V206DEV141[4];
	GLJ_LOCK  V206DEV142;
	V206IOCTL172 V206DEV144[MWV206_DP_COUNT];
	int V206DEV145;
	V206DEV022 V206DEV146[4];
	EDID V206DEV147;
	char V206DEV148[V206CONFIG010];
	char V206DEV149[V206CONFIG010];
	GLJ_LOCK  V206DEV150;
	int V206DEV151;
	int V206DEV152;

	int V206DEV153;
	struct tasklet_struct V206DEV154;
	unsigned char V206DEV155;
	unsigned char V206DEV156;
	unsigned char i2cchansel;
	V206DEV023 V206DEV160;


	struct scanctrl       scanctrl[4];
	int                   disable_vblank_sync;
	int                   vblank_sync;
	int                   zoom_on;
	struct workqueue_struct   *wq;

	unsigned int          chiptype;
	unsigned int          vbios_ver;
	unsigned int          vbios_major;
	unsigned int          vbios_minor;
	unsigned int          vbios_revision;
	struct mwv206_i2c *edid_i2c[V206CONFIG006 + V206CONFIG005];
	int hdmi_voltage[4];
} V206DEV025;

#define MWV206_I2C_CHAN_DISABLED(pDev, chan)      ((pDev)->V206DEV156 & (1 << (chan)))
#define MWV206_I2C_DISABLE_CHANNEL(pDev, chan)    ((pDev)->V206DEV156 |= (1 << (chan)))
#define MWV206_I2C_CHAN_SELECT_IPCORE(pDev, chan) ((pDev)->i2cchansel |= (1 << (chan)))
#define MWV206_I2C_CHAN_IPCORE(pDev, chan)        ((pDev)->i2cchansel & (1 << (chan)))

typedef struct mwv206_i2c {
	V206DEV025 *pDev;
	struct i2c_algo_bit_data bit;
	struct i2c_adapter adapter;
	struct mutex mutex;
	u32 sda_in_addr;
	u32 sda_mask;
	u32 sda_dir;
	u32 scl_in_addr;
	u32 scl_mask;
	u32 scl_dir;
} mwv206_i2c_t;

struct V206DEV026 {
	struct fb_info      *info;
	struct pci_dev      *V206DEV103;
	char                name[64];

	unsigned long       V206DEV177;
	void __iomem        *V206DEV178;
	unsigned long       V206DEV179;
	unsigned long       V206DEV180;
	void __iomem        *mmio_base;
	unsigned long       mmio_len;
	unsigned long       screen_size;

	unsigned long       V206DEV181;

	u32         pseudo_palette[16];
	int         V206KG2D001, depth;
	int         V206DEV182;
	int         pixel_format;
	int         width, height;
	int         maxwidth, maxheight;
	struct mwv206_port_config *zoom_port;
	V206IOCTL161 V206DEV183[2];
};

#define SWAPLONG(_X_) (((_X_) >> 24) | (((_X_) & 0x00FF0000) >> 8) | (((_X_) & 0x0000FF00) << 8) | ((_X_) << 24))
unsigned int FUNC206HAL472(unsigned int input, unsigned int num);
extern int FUNC206LXDEV137(void);

static inline void mwv206_cpu_relax(void)
{
	cpu_relax();
}

#define mwv206_timed_loop(tick, cond, timeout) \
for ((tick) = FUNC206LXDEV134(); (cond) && time_before(FUNC206LXDEV134(), (tick) + (timeout)); mwv206_cpu_relax())

#define mwv206_timed_do(tick, scratch, timeout) \
for ((tick) = FUNC206LXDEV134(), (scratch) = 1;\
		(scratch) || time_before(FUNC206LXDEV134(), (tick) + (timeout));\
		(scratch) = 0, mwv206_cpu_relax())




int FUNC206HAL149(V206DEV025 *pDev);
int FUNC206HAL148(V206DEV025 *priv);
int FUNC206HAL215(void *devInfo);

void mwv206_ddr_status(V206DEV025 *pDev);


int FUNC206HAL379(V206DEV025 *dev);


void FUNC206HAL235(V206DEV025 *pDev,
		unsigned int barno, jjuint32 targAddr);
void FUNC206HAL238(V206DEV025 *pDev,
		unsigned char regionIdx, unsigned int size,
		jjuint32 srcAddr, jjuintptr targAddr);
int mwv206_set_PCIERegion_inBoundBar(V206DEV025 *pDev, long arg);
int mwv206_set_PCIERegion_outBoundMem(V206DEV025 *pDev, long arg);
void FUNC206HAL236(V206DEV025 *pDev, unsigned int barno, jjuint32 targAddr);
void FUNC206HAL237(V206DEV025 *pDev, unsigned int barno, jjuint32 targAddr);
void FUNC206HAL221(struct pci_dev *V206DEV103);


extern unsigned int  FUNC206HAL467(unsigned long addr);
extern void FUNC206HAL470(unsigned long addr, unsigned int data);
extern unsigned short FUNC206HAL468(unsigned long addr);
extern void FUNC206HAL471(unsigned long addr, unsigned short data);
extern unsigned char FUNC206HAL466(unsigned long addr);
extern void FUNC206HAL469(unsigned long addr, unsigned char data);
int FUNC206HAL231(V206DEV025 *pDev, int reg, int mask, int val);
int FUNC206HAL244(V206DEV025 *pDev, unsigned int reg, unsigned int val);
void FUNC206HAL397(V206DEV025 *pDev, unsigned int reg, unsigned int val);
int V206IOCTL130(V206DEV025 *pDev, long arg);

#define V206DEV001(reg)        \
(\
{\
	unsigned int ret;\
	if (unlikely(pDev->V206DEV155 == 0)) {\
		unsigned long flags;\
		flags = FUNC206HAL094(pDev->V206DEV101);\
		if (unlikely(((reg) & MWV206REG_HDMI_REGBAR_MASK) == MWV206REG_HDMI_REGBAR_ADDR)) {\
			FUNC206HAL237(pDev, pDev->V206DEV043, MWV206REG_HDMI_REGBAR_ADDR + 0x02000000);\
			ret = FUNC206HAL467(pDev->V206DEV033 + ((reg) & MWV206REG_HDMI_OFFSET_MASK));\
			FUNC206HAL237(pDev, pDev->V206DEV043, 0x02000000);\
		} else {\
			ret = FUNC206HAL467(pDev->V206DEV033 + (reg));\
		} \
		FUNC206HAL095(pDev->V206DEV101, flags);\
	} else {\
		ret = FUNC206HAL467(pDev->V206DEV033 + (reg));\
	} \
	ret;\
} \
)
#define V206DEV002(reg, val)  \
do {\
	if (unlikely(pDev->V206DEV155 == 0)) {\
		unsigned long flags;\
		flags = FUNC206HAL094(pDev->V206DEV101);\
		if (unlikely(((reg) & MWV206REG_HDMI_REGBAR_MASK) == MWV206REG_HDMI_REGBAR_ADDR)) {\
			FUNC206HAL237(pDev, pDev->V206DEV043, MWV206REG_HDMI_REGBAR_ADDR + 0x02000000);\
			FUNC206HAL470(pDev->V206DEV033 + ((reg) & MWV206REG_HDMI_OFFSET_MASK), val);\
			FUNC206HAL237(pDev, pDev->V206DEV043, 0x02000000);\
		} else {\
			FUNC206HAL470(pDev->V206DEV033 + (reg), val);\
		} \
		FUNC206HAL095(pDev->V206DEV101, flags);\
	} else {\
		FUNC206HAL470(pDev->V206DEV033 + (reg), val);\
	} \
} while (0)

#define V206DEV003(reg, bitmask, val) \
do {\
	unsigned int base;\
	base = (V206DEV001(reg)) & ~(bitmask);\
	V206DEV002(reg, base | (val & bitmask));\
} while (0)


void V206DEV006(void *devInfo, unsigned int regAddr, unsigned int value);
unsigned int V206DEV007(void *devInfo, unsigned int regAddr);


#undef PCI_WRITE_U32
#ifndef __PCIE_CONFIGSPACE_MAXREG__
#define __PCIE_CONFIGSPACE_MAXREG__ 0x1000
#endif
#define PCI_WRITE_U32(dev, reg, val) do { \
V206DEV025 *priv = pci_get_drvdata(dev); \
if ((reg) < __PCIE_CONFIGSPACE_MAXREG__) \
pci_write_config_dword((dev), (reg), (val));    \
else \
FUNC206HAL470(priv->V206DEV034 + (reg), (val)); \
} while (0)

#undef PCI_READ_U32
#define PCI_READ_U32(dev, reg, val) do { \
V206DEV025 *priv = pci_get_drvdata(dev); \
if ((reg) < __PCIE_CONFIGSPACE_MAXREG__) \
pci_read_config_dword((dev), (reg), (val)); \
else \
*(val) = FUNC206HAL467(priv->V206DEV034 + (reg)); \
} while (0)


int FUNC206HAL320(V206DEV025 *pDev, int arg);
int FUNC206HAL319(V206DEV025 *pDev, int op, int chan, int arg);
int V206IOCTL126(V206DEV025 *pDev, long arg);
int FUNC206HAL328(V206DEV025 *pDev, long arg);
int FUNC206HAL327(V206DEV025 *pDev, int op, int chan, unsigned int V206IOCTLCMD009, unsigned char *V206DEV031,
		int size);
jjuint32 FUNC206HAL323(V206DEV025 *pDev, jjuint32 startaddr);
int FUNC206HAL321(V206DEV025 *pDev);
void FUNC206HAL324(V206DEV025 *pDev);


int mwv206_mem_rw(V206DEV025 *pDev, int op, u8 *V206DEV031, u32 V206IOCTLCMD009, int width);
int mwv206_mem_rw_block(V206DEV025 *pDev, int op, u8 *V206DEV031, int V206KG2D033, u32 V206IOCTLCMD009, int mwv206stride, int width, int height);


int FUNC206HAL369(V206DEV025 *pDev, int arg);
int FUNC206HAL316(V206DEV025 *pDev, int arg);
int FUNC206HAL312(V206DEV025 *pDev, int arg);
int FUNC206HAL317(V206DEV025 *pDev, int arg);
int FUNC206HAL382(V206DEV025 *pDev, int arg);


int FUNC206HAL333(V206DEV025 *pDev, long arg);
int mwv206fb_init_early(struct pci_dev *V206DEV103);
int mwv206fb_register(struct pci_dev *V206DEV103);
void FUNC206LXDEV085(struct pci_dev *V206DEV103);
int mwv206fb_active(V206DEV025 *priv);
int mwv206fb_clear(struct V206DEV026 *V206FB005);


void FUNC206HAL374(V206DEV025 *pDev);
int V206IOCTL135(V206DEV025 *pDev, long arg, void *userdata);
int V206IOCTL136(V206DEV025 *pDev, long arg, void *userdata);
int FUNC206HAL370(V206DEV025 *pDev);
int V206IOCTL134(V206DEV025 *pDev, long arg);
unsigned long FUNC206HAL334(V206DEV025 *pDev, unsigned int barIdx, unsigned int addr);
void *FUNC206HAL071(unsigned int addr, unsigned int addrHi);
void FUNC206HAL228(V206DEV025 *pDev,
	const unsigned char *V206IOCTLCMD006, unsigned int V206IOCTLCMD009, unsigned int size);
int FUNC206HAL229(V206DEV025 *pDev, unsigned int V206IOCTLCMD009,
	const unsigned char *V206IOCTLCMD006, unsigned int size);
int FUNC206HAL230(V206DEV025 *pDev, unsigned int V206IOCTLCMD009,
	const unsigned char *pAddr, unsigned int size);
int FUNC206HAL314(V206DEV025 *pDev, unsigned int V206IOCTLCMD009);
int FUNC206HAL370(V206DEV025 *pDev);
int V206IOCTL136(V206DEV025 *pDev, long arg, void *userdata);
int V206IOCTL135(V206DEV025 *pDev, long arg, void *userdata);
void FUNC206HAL226(void *pDev, unsigned int addr);
unsigned int FUNC206HAL223(void *pDev, int size, int alignment);
int FUNC206HAL315(V206DEV025 *pDev, int memfreq);


void *FUNC206LXDEV009(struct device *dev,
	unsigned long long *dma_handle, int size);
int FUNC206LXDEV100(struct device *dev,
	int size, void *vaddr, unsigned long long dma_handle);
unsigned long long FUNC206LXDEV011(unsigned long long vaddr);


int FUNC206HAL381(V206DEV025 *pDev, long arg);
int FUNC206HAL380(V206DEV025 *pDev, long arg);
int FUNC206HAL134(V206DEV025 *devInfo, int plltype, int *freq);
int FUNC206HAL136(V206DEV025 *pDev, e_mwv206_pll_t pllID, int freq);
int FUNC206HAL135(V206DEV025 *devInfo, int plltype, int configfreq, int enable);


int FUNC206HAL388(V206DEV025 *pDev, long arg);
int FUNC206HAL393(V206DEV025 *pDev, long arg);
int FUNC206HAL394(V206DEV025 *pDev, long arg);
int FUNC206HAL390(V206DEV025 *pDev, long arg);
int FUNC206HAL391(V206DEV025 *priv, int crtc, int width, int height, int isInterleaved);
int FUNC206HAL396(V206DEV025 *pDev, long arg, int legacy_mode, int mask_ioctl);
int mwv206_sethdmimode_params(V206DEV025 *pDev, long arg, int legacy_mode, int mask_ioctl);
int FUNC206HAL389(V206DEV025 *pDev, long arg);
void FUNC206HAL351(V206DEV025 *pDev);
int FUNC206HAL423(V206DEV025 *dev, int win);
int FUNC206HAL422(V206DEV025 *dev, int screen);
int FUNC206HAL191(V206DEV025 *dev, unsigned int channel, int paletteno, unsigned char data[768]);
int mwv206_resethdmiphy(V206DEV025 *pDev, int mask);
int FUNC206HAL392(V206DEV025 *pDev, long arg);



int FUNC206LXDEV149(V206DEV025 *pDev, unsigned long userdata);
void FUNC206LXDEV052(V206DEV025 *pDev);
void FUNC206LXDEV053(struct pci_dev *V206DEV103);
void FUNC206HAL330(V206DEV025 *pDev);
void mwv206_edid_custom_detect(V206DEV025 *pDev);
int FUNC206LXDEV151(V206DEV025 *pDev, int arg);
int mwv206_edid_is_dvi(unsigned char *edid);


int FUNC206LXDEV156(V206DEV025 *pDev, long arg);
int FUNC206LXDEV155(int bus, int dev, int func);
int FUNC206HAL385(V206DEV025 *devInfo);
int FUNC206HAL384(V206DEV025 *devInfo, signed int *temp);
int FUNC206HAL352(V206DEV025 *pDev,  long arg);
int FUNC206HAL402(V206DEV025 *pDev,  long arg);
int FUNC206HAL412(V206DEV025 *dev, int screen, unsigned int data[4096]);
void FUNC206HAL184(JMSPI pSpiDev);
void FUNC206HAL183(JMSPI pSpiDev);

#if	defined(SUPPORT_SND_PCM)

int FUNC206LXDEV141(V206DEV025 *pDev, long args);
int FUNC206LXDEV142(V206DEV025 *pri, long args);
void FUNC206LXDEV024(struct pci_dev *V206DEV103);
void FUNC206LXDEV021(V206DEV025 *pDev, int V206HDMIAUDIO027, unsigned int rate);
int FUNC206LXDEV138(V206DEV025 *pDev, int V206HDMIAUDIO027);


int mwv206_hdmi_resume(V206DEV025 *pDev);
int mwv206_hdmi_suspend(V206DEV025 *pDev);
#else

static inline int FUNC206LXDEV141(V206DEV025 *pDev, long args)
{
	return 0;
}

static inline int FUNC206LXDEV142(V206DEV025 *pri, long args)
{
	return 0;
}

static inline void FUNC206LXDEV024(struct pci_dev *V206DEV103)
{
}

static inline void FUNC206LXDEV021(V206DEV025 *pDev, int V206HDMIAUDIO027, unsigned int rate)
{
}

static inline int FUNC206LXDEV138(V206DEV025 *pDev, int V206HDMIAUDIO027)
{
	return 0;
}


static inline int mwv206_hdmi_resume(V206DEV025 *pDev)
{
	return 0;
}

static inline int mwv206_hdmi_suspend(V206DEV025 *pDev)
{
	return 0;
}
#endif


int FUNC206HAL256(V206DEV025 *pDev);
int FUNC206HAL255(V206DEV025 *pDev);
void FUNC206HAL265(V206DEV025 *pDev);
int FUNC206HAL257(V206DEV025 *pDev);
void FUNC206HAL263(V206DEV025 *pDev, struct file *filp);
int FUNC206HAL264(V206DEV025 *pDev, unsigned long userdata);
int FUNC206HAL258(V206DEV025 *pDev, unsigned long arg);
int FUNC206HAL395(V206DEV025 *pDev, long arg);
int FUNC206HAL004(V206DEV025 *pDev);
int FUNC206HAL241(V206DEV025 *pDev);
int FUNC206HAL233(V206DEV025 *pDev);


int FUNC206HAL245(V206DEV025 *pDev, long arg);
int FUNC206HAL253(V206DEV025 *pDev, long arg);
int FUNC206HAL248(V206DEV025 *pDev, long arg);
int FUNC206HAL251(V206DEV025 *pDev, long arg);
int FUNC206LXDEV154(V206DEV025 *pDev, unsigned long mmio, unsigned long dst, int V206DEV182, int V206KG2D001, int x, int y,
	int width, int height, unsigned long color, unsigned long mask, int rop);
int FUNC206HAL247(V206DEV025 *pDev, V206IOCTL157 *mbit, unsigned int offset,  unsigned int *cmd,
	int opt);
int FUNC206HAL252(V206DEV025 *pDev,  V206IOCTL157 *mbit);
int FUNC206HAL246(V206DEV025 *pDev, long arg);
int FUNC206HAL477(V206DEV025 *pDev, V206IOCTL157 *mbit);
int FUNC206HAL250(V206DEV025 *pDev, V206IOCTL165 *V206KG2D021);


int FUNC206HAL398(V206DEV025 *pDev, int is3d, unsigned int mode, unsigned int rbsize);
int FUNC206HAL274(V206DEV025 *pDev, int mode2d, int mode3d);
int FUNC206HAL399(V206DEV025 *pDev, long arg);
int FUNC206HAL242(V206DEV025 *pDev, const char *pBuf, int nBytes);
int FUNC206HAL410(V206DEV025 *pDev, long arg);
int FUNC206HAL411(V206DEV025 *pDev, long arg);
int FUNC206HAL282(V206DEV025 *pDev, int arg);
int FUNC206HAL291(V206DEV025 *pDev, int arg);
int FUNC206HAL298(void *dev, long arg);
int MWV206SendCommand(V206DEV025 *pDev, const unsigned int *pCmd, int count);


int FUNC206LXDEV167(struct pci_dev *V206DEV103, bool suspend, bool fbcon);
int FUNC206LXDEV166(struct pci_dev *V206DEV103, bool resume, bool fbcon);
int mwv206pm_thaw(struct pci_dev *dev);
int FUNC206LXDEV164(V206DEV025 *pDev);


int FUNC206LXDEV143(V206DEV025 *pDev, long arg);
int FUNC206LXDEV144(struct pci_dev *V206DEV103);
void FUNC206LXDEV038(void);
void FUNC206LXDEV039(V206DEV025 *pDev);



extern int FUNC206HAL368(V206DEV025 *pDev, int no);
int mwv206_intrPCIESelect(V206DEV025 *pDev, long arg);
int FUNC206HAL408(V206DEV025 *pDev, long arg);
int mwv206_waitfor_polling_delay(V206DEV025 *pDev, long arg);
int FUNC206HAL331(V206DEV025 *pDev, long arg);
int FUNC206HAL405(V206DEV025 *pDev, int display);
irqreturn_t FUNC206HAL364(int irq, void *devInfo);
int FUNC206HAL366(V206DEV025 *pDev, int no, int select);
int FUNC206HAL367(V206DEV025 *pDev, int no, int select);
void FUNC206HAL355(V206DEV025 *pDev);
int mwv206_intr_init(V206DEV025 *pDev);
void mwv206_intr_destroy(V206DEV025 *pDev);
int FUNC206HAL359(int intrsrc, V206DEV025 *pDev);
void FUNC206HAL363(unsigned long arg);
void FUNC206HAL361(void);


int FUNC206LXDEV158(struct pci_dev *V206DEV103);
void FUNC206LXDEV076(struct pci_dev *V206DEV103);


int FUNC206HAL222(V206DEV025 *dev, unsigned int cmd, unsigned long arg, void *userdata);
int FUNC206HAL336(V206DEV025 *pDev, long arg);
int mwv206_power_manager(V206DEV025 *pDev, long arg);
int FUNC206HAL006(V206DEV025 *pDev);
int FUNC206HAL296(V206DEV025 *V206DEV103, long arg);
int FUNC206LXDEV157(V206DEV025 *pDev, long arg);
void *FUNC206HAL377(void *dst, const unsigned char *src, int len);
int mwv206_memcpy_rw(V206DEV025 *pDev, int op, u8 *V206DEV031, u32 V206IOCTLCMD009, int width);
void *FUNC206HAL378(void *dst, unsigned char val, int len);
int mwv206_vbios_cmp(V206DEV025 *pDev, int major, int minor, int rev, int *paccurate);
void mwv206_validate_cfg(V206DEV025 *pDev);
int mwv206_dev_cnt_get(void);

void mwv206_shadow_switch_on_nolock(V206DEV025 *pDev);
void mwv206_shadow_switch_off_nolock(V206DEV025 *pDev);
void mwv206_shadow_crtc_fallback_nolock(V206DEV025 *pDev);
void mwv206_shadow_try_sync(V206DEV025 *pDev);
void mwv206_shadow_init(V206DEV025 *pDev);
void mwv206_shadow_deinit(V206DEV025 *pDev);
void mwv206_shadow_mark_draw(V206DEV025 *pDev);

void FUNC206HAL130(struct mwv206_port_config *pport);
jme_chip_grade mwv206_get_chipgrade(V206DEV025 *pDev);


extern struct device *audio_dev;
extern struct device *event_dev;
extern unsigned long FUNC206HAL067[4][2], FUNC206HAL068[4];
extern int FUNC206HAL065;
extern const memmgr_t FUNC206HAL418;


int FUNC206HAL146(void *V206DEV103);


int FUNC206HAL145(void *V206DEV103);

static inline int addr_in_fb_range(V206DEV025 *pDev, unsigned int addr)
{
	unsigned int fb0start;
	unsigned int fb1start;
	unsigned int fb0end;
	unsigned int fb1end;

	fb0start = (unsigned int)0x00000000 + pDev->V206DEV082[0];
	fb1start = (unsigned int)0x80000000 + pDev->V206DEV082[1];
	fb0end   = (unsigned int)0x00000000 + pDev->V206DEV038;
	fb1end   = (unsigned int)0x80000000 + pDev->V206DEV039;

	if (addr >= fb0start && addr < fb0end) {
		return 1;
	}

	if (addr >= fb1start && addr < fb1end) {
		return 1;
	}

	return 0;
}


static inline int size_in_fb_range(V206DEV025 *pDev, unsigned int addr, unsigned int size)
{
	unsigned int msb_mask;

	if (size == 0 || size > pDev->V206DEV045) {
		return 0;
	}


	if (((pDev->V206DEV045 - 1) & pDev->V206DEV045)) {

		return 1;
	}

	msb_mask = ~(pDev->V206DEV045 - 1);
	if (((addr + size - 1) & msb_mask) != (addr & msb_mask)) {
		return 0;
	}

	return 1;
}

void mwv206pm_winreg_save(V206DEV025 *pDev);
void mwv206pm_winreg_reload(V206DEV025 *pDev);

#ifdef __cplusplus
}
#endif

#endif