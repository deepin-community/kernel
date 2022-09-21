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

#ifndef __GF_MODIFIES_H_
#define __GF_MODIFIES_H_

/*this piece code should be place in drm/drm-fourcc.h in kernel/mesa*/

#include <drm/drm_fourcc.h>
#define DRM_FORMAT_MOD_VENDOR_GF 0x19
/*usage  for display specially requirement*/
#define DRM_FORMAT_MOD_GF_DISPLAY               fourcc_mod_code(GF, 1)
#define DRM_FORMAT_MOD_GF_LINEAR                fourcc_mod_code(GF, 2)
#define DRM_FORMAT_MOD_GF_TILED                 fourcc_mod_code(GF, 3)
#define DRM_FORMAT_MOD_GF_COMPRESS              fourcc_mod_code(GF, 4)
#define DRM_FORMAT_MOD_GF_TILED_COMPRESS        fourcc_mod_code(GF, 5)


#define DRM_FORMAT_MOD_GF_LOCAL                 fourcc_mod_code(GF, 6)
#define DRM_FORMAT_MOD_GF_PCIE                  fourcc_mod_code(GF, 7)

#define DRM_FORMAT_MOD_GF_INVALID               fourcc_mod_code(GF, 119)
#endif

