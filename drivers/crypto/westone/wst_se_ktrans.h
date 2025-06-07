/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _WST_SE_KTRANS_H
#define _WST_SE_KTRANS_H

#include "wst_se_common_type.h"
#include "wst_se_define.h"

struct tagSWCOMMUDATA {
	unsigned short usFlags;
	unsigned short usInputLen;
	unsigned short usOutputLen;
	unsigned short usReserve;
	unsigned char *pucInbuf;
	unsigned char *pucOutbuf;
};

#endif
