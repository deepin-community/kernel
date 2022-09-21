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
#include <linux/sched.h>
#include "mwv206ioctl.h"
#include "mwv206dev.h"

static int FUNC206HAL337(V206DEV025 *pDev, long arg)
{
	V206IOCTL166 *gParam;
	gParam = (struct V206IOCTL129 *)arg;
	switch (gParam->param) {
	case MWV206KPARAM_RESMEMSIZE: {
					     gParam->retval = pDev->V206DEV072;  break;
				     }
	case MWV206KPARAM_CORECLOCK: {
					    gParam->retval = pDev->V206DEV073;   break;
				    }
	case MWV206KPARAM_MEMCLOCK: {
					   gParam->retval = pDev->V206DEV075;    break;
				   }
	case MWV206KPARAM_MEMFREE: {
					  gParam->retval = FUNC206HAL418.FUNC206HAL213(pDev->V206DEV068[0]) +
						  FUNC206HAL418.FUNC206HAL213(pDev->V206DEV068[1]) +
						  FUNC206HAL418.FUNC206HAL213(pDev->V206DEV068[2]);
					  break;
				  }
	case MWV206KPARAM_MEMSHOW: {
					  FUNC206HAL374(pDev);
					  gParam->retval = 0;
					  break;
				  }
	case MWV206KPARAM_IRQ:   {
					gParam->retval = pDev->irq;         break;
				}
	case MWV206KPARAM_REFCLOCKFREQ:   {
						 gParam->retval = pDev->V206DEV035 / 2; break;
					 }
	case MWV206KPARAM_TILEADDRESS:   {
						gParam->retval = pDev->V206DEV077; break;
					}
	case MWV206KPARAM_SENDCMDMODE3D:   {
						  gParam->retval = pDev->V206DEV097.mode; break;
					  }
	case MWV206KPARAM_SENDCMDMODE2D:   {
						  gParam->retval = pDev->V206DEV096.mode; break;
					  }
	case MWV206KPARAM_VERTEXORDER:   {
						gParam->retval = pDev->V206DEV091; break;
					}
	case MWV206KPARAM_VERTEXLOCATION: {
						 gParam->retval = pDev->V206DEV092; break;
					 }
	case MWV206KPARAM_GPURBSIZE: {
					    gParam->retval = pDev->V206DEV081.size; break;
				    }
	case MWV206KPARAM_CPURBSIZE: {
					    gParam->retval = pDev->V206DEV080.V206IOCTLCMD019 * 4; break;
				    }
	case MWV206KPARAM_MEMBAR_BASEADDR: {
						  gParam->retval = pDev->V206DEV031[1]; break;
					  }
	case MWV206KPARAM_MEMBLOCKSIZE: {
					       gParam->retval = pDev->V206DEV045; break;
				       }
	case MWV206KPARAM_DDRCNT: {
					 gParam->retval = pDev->V206DEV041; break;
				 }
	case MWV206KPARAM_BARNO: {
					gParam->retval = (pDev->V206DEV043) | (pDev->V206DEV044[1] << 8)
						| (pDev->V206DEV044[0] << 16);
					V206DEV005("regbar = 0x%x, mem3dbar = 0x%x, mem2dbar = 0x%x\n",
							pDev->V206DEV043, pDev->V206DEV044[1], pDev->V206DEV044[0]);
					V206DEV005("bar = 0x%x\n", gParam->retval);
					break;
				}
	case MWV206KPARAM_BUSNO: {


					V206DEV005("pDev->busno = %ld\n\n\n", pDev->V206DEV047);
					gParam->retval = pDev->V206DEV047;
					break;
				}

	case MWV206KPARAM_DDR0USERCHECKADDR: {
						    gParam->retval = pDev->V206DEV083[0] + 4;
						    break;
					    }

	case MWV206KPARAM_DDR1USERCHECKADDR: {
						    gParam->retval = pDev->V206DEV083[1] + 4;
						    break;
					    }
	case MWV206KPARAM_DDR0SIZE: {
					   gParam->retval = pDev->V206DEV038;
					   break;
				   }
	case MWV206KPARAM_DDR1SIZE: {
					   gParam->retval = pDev->V206DEV039;
					   break;
				   }
	case MWV206KPARAM_FRAMERATELIMIT: {
						 gParam->retval = pDev->V206DEV049;
						 break;
					 }
	case MWV206KPARAM_DEVID: {
					gParam->retval = pDev->V206DEV029;
					break;
				}
	case MWV206KPARAM_MAXCORECLOCK: {
					       gParam->retval = pDev->V206DEV074;
					       break;
				       }
	default:
				       V206KDEBUG002("unsupported paramid 0x%x.\n\n\n", gParam->param);
				       return -1;
				       break;
	}
	V206DEV005("return 0\n");
	return 0;
}

int FUNC206HAL369(V206DEV025 *pDev, int arg)
{
	if (arg) {
		return FUNC206LXDEV116(pDev->V206DEV055);
	} else {
		return FUNC206LXDEV136(pDev->V206DEV055);
	}
}

int FUNC206HAL382(V206DEV025 *pDev, int arg)
{
	if (arg) {
		return FUNC206LXDEV116(pDev->V206DEV062);
	} else {
		return FUNC206LXDEV136(pDev->V206DEV062);
	}
}

int FUNC206HAL312(V206DEV025 *pDev, int arg)
{
	if (arg) {
		return FUNC206LXDEV116(pDev->V206DEV057);
	} else {
		return FUNC206LXDEV136(pDev->V206DEV057);
	}
}

int FUNC206HAL316(V206DEV025 *pDev, int arg)
{
	if (arg) {
		return FUNC206LXDEV116(pDev->V206DEV056);
	} else {
		return FUNC206LXDEV136(pDev->V206DEV056);
	}
}

int FUNC206HAL317(V206DEV025 *pDev, int arg)
{
	if (arg) {
		return FUNC206LXDEV116(pDev->V206DEV058);
	} else {
		return FUNC206LXDEV136(pDev->V206DEV058);
	}
}

int FUNC206HAL291(V206DEV025 *pDev, int arg)
{
	V206DEV005("%d\n", arg);
	if (arg) {
		return FUNC206LXDEV116(pDev->V206DEV059);
	} else {
		return FUNC206LXDEV136(pDev->V206DEV059);
	}
}

int FUNC206HAL282(V206DEV025 *pDev, int arg)
{
	V206DEV005("%d\n", arg);
	if (arg) {
		return FUNC206LXDEV116(pDev->V206DEV060);
	} else {
		return FUNC206LXDEV136(pDev->V206DEV060);
	}
}

int FUNC206HAL332(V206DEV025 *pDev, int arg)
{
	if (arg) {
		return FUNC206LXDEV116(pDev->V206DEV061);
	} else {
		return FUNC206LXDEV136(pDev->V206DEV061);
	}
}


static int FUNC206HAL401(V206DEV025 *pDev, long arg)
{
	int order = (int)arg;

	if (order >= V206API046) {
		V206KDEBUG002("Invalid vertex order: 0x%x.\n", order);
		return -1;
	} else {
		V206KDEBUG002("vertex order: %d\n", order);
		pDev->V206DEV091 = order;
		return 0;
	}
}

static int FUNC206HAL400(V206DEV025 *pDev, long arg)
{
	int location = (int)arg;

	if (location != V206API048) {
		V206KDEBUG002("Invalid vertex location: 0x%x.\n", location);
		return -1;
	} else {
		V206KDEBUG002("vertex loction: %d\n", location);
		pDev->V206DEV092 = location;
		return 0;
	}
}

int FUNC206HAL392(V206DEV025 *pDev, long arg)
{
	int ret;
	V206IOCTL162 *setdisppalette;
	setdisppalette = (V206IOCTL162 *)arg;
	if ((setdisppalette->V206FB011 < 0) || (setdisppalette->V206FB011 > 3)) {
		return -2;
	}

	FUNC206HAL369(pDev, 1);
	ret = FUNC206HAL191(pDev, setdisppalette->V206FB011, setdisppalette->paletteid, setdisppalette->palette);
	FUNC206HAL369(pDev, 0);

	if (pDev->pm.V206DEV109 == 0 && ret == 0) {
		pDev->pm.palette_valid[setdisppalette->V206FB011] = 1;
		pDev->pm.palettes[setdisppalette->V206FB011] = *setdisppalette;
	}

	return ret;
}


int FUNC206HAL222(V206DEV025 *pDev, unsigned int function, unsigned long arg, void *userdata)
{
	int ret;
	ret = -1;

	switch (function) {
	case V206IOCTL063:
		ret = FUNC206HAL336(pDev, arg);
		break;
	case V206IOCTL064:
		ret = V206IOCTL135(pDev, arg, userdata);
		break;
	case V206IOCTL065:
		ret = V206IOCTL134(pDev, arg);
		break;
	case V206IOCTL066:
		ret = V206IOCTL136(pDev, arg, userdata);
		break;
	case V206IOCTL067:
		ret = V206IOCTL126(pDev, arg);
		break;
	case V206IOCTL068:
		ret = FUNC206HAL328(pDev, arg);
		break;
	case V206IOCTL069:
		ret = FUNC206HAL370(pDev);
		break;
	case V206IOCTL070:
		ret = V206IOCTL130(pDev, arg);
		break;
	case V206IOCTL071:
		ret = FUNC206HAL337(pDev, arg);
		break;
	case V206IOCTL072:
		ret = FUNC206HAL331(pDev, arg);
		break;
	case V206IOCTL073:
		ret = FUNC206HAL333(pDev, arg);
		break;
	case V206IOCTL074:
		ret = FUNC206HAL248(pDev, arg);
		break;
	case V206IOCTL075:
		ret = FUNC206HAL245(pDev, arg);
		break;
	case V206IOCTL076:
		ret = FUNC206HAL246(pDev, arg);
		break;
	case V206IOCTL077:
		ret = FUNC206HAL253(pDev, arg);
		break;
	case V206IOCTL078:
		ret = FUNC206HAL251(pDev, arg);
		break;
	case V206IOCTL079:
		ret = FUNC206HAL410(pDev, arg);
		break;
	case V206IOCTL080:
		ret = FUNC206HAL411(pDev, arg);
		break;
	case V206IOCTL083:
		ret = FUNC206HAL399(pDev, arg);
		break;
	case V206IOCTL084:
		ret = FUNC206HAL401(pDev, arg);
		break;
	case V206IOCTL085:
		ret = FUNC206HAL400(pDev, arg);
		break;
	case V206IOCTL095:
		ret = FUNC206HAL395(pDev, arg);
		break;
	case V206IOCTL091:
		ret = FUNC206HAL389(pDev, arg);
		break;
	case V206IOCTL092:
		ret = FUNC206HAL392(pDev, arg);
		break;
	case V206IOCTL090:
		ret = FUNC206HAL393(pDev, arg);
		break;
	case V206IOCTL093:
		ret = FUNC206HAL394(pDev, arg);
		break;
	case V206IOCTL089:
		ret = FUNC206HAL390(pDev, arg);
		break;
	case V206IOCTL088:
		ret = FUNC206HAL388(pDev, arg);
		break;
	case V206IOCTL087:
		ret = FUNC206HAL396(pDev, arg, 1, 2);
		break;
	case V206IOCTL100:
		ret = FUNC206LXDEV156(pDev, arg);
		break;
	case V206IOCTL103:
		ret = FUNC206LXDEV141(pDev, arg);
		break;
	case V206IOCTL104:
		ret = FUNC206LXDEV142(pDev, arg);
		break;
	case V206IOCTL105:
		ret = FUNC206LXDEV149(pDev, arg);
		break;
	case V206IOCTL082:
		FUNC206HAL369(pDev, 1);
		ret = FUNC206HAL264(pDev, (unsigned long)userdata);
		FUNC206HAL369(pDev, 0);
		break;

	case V206IOCTL081:
		FUNC206HAL369(pDev, 1);
		ret = FUNC206HAL258(pDev, arg);
		FUNC206HAL369(pDev, 0);
		break;

	case V206IOCTL086:
		ret = FUNC206HAL296(pDev, arg);
		break;

	case V206IOCTL096:
		ret = FUNC206HAL384(pDev, (signed int *)arg);
		break;

	case V206IOCTL097:
		ret = FUNC206HAL381(pDev, arg);
		break;

	case V206IOCTL098:
		ret = FUNC206HAL380(pDev, arg);
		break;

	case V206IOCTL094:
		ret = FUNC206HAL408(pDev, arg);
		break;

	case V206IOCTL099:
		ret = FUNC206LXDEV155((arg >> 16) & 0xff,
				(arg >> 8) & 0xff,
				arg & 0xff);
		break;

	case V206IOCTL101:
		ret = FUNC206LXDEV157(pDev, arg);
		break;

	case V206IOCTL102:
		ret = FUNC206LXDEV143(pDev, arg);
		break;

	case V206IOCTL106:
		ret = FUNC206HAL352(pDev, arg);
		break;

	case V206IOCTL107:
		ret = FUNC206HAL402(pDev, arg);
		break;

	case IOCTL_MWV206_SETHDMIMODE_NEW:
		ret = FUNC206HAL396(pDev, arg, 0, 2);
		break;

	default:
		V206KDEBUG002("error ioctl function 0x%x.", function);
		break;
	}
	return ret;
}