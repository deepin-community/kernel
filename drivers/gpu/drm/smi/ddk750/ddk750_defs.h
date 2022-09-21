/*******************************************************************
* 
*         Copyright (c) 2008 by Silicon Motion, Inc. (SMI)
* 
*  All rights are reserved. Reproduction or in part is prohibited
*  without the written consent of the copyright owner.
* 
*  DEFS.H (The name of this file is defs.h in 718 DDK)
*  This file contains register and common macro definitions.
* 
*******************************************************************/
#ifndef _DEFS_H_
#define _DEFS_H_

/* For validation purpose only */
/*#define SM750_AA*/
//#include "ddk750_help.h"
#include "ddk750_regsc.h"
#include "ddk750_reggpio.h"
//#include "ddk750_regpwm.h"
//#include "ddk750_regssp.h"
#include "ddk750_regdc.h"
//#include "ddk750_regdma.h"
//#include "ddk750_regde.h"
//#include "ddk750_regzv.h"
#include "ddk750_regi2c.h"

/* Internal macros */
#define _F_START(f)             (0 ? f)
#define _F_END(f)               (1 ? f)
#define _F_SIZE(f)              (1 + _F_END(f) - _F_START(f))
#define _F_MASK(f)              (((1 << _F_SIZE(f)) - 1) << _F_START(f))
#define _F_NORMALIZE(v, f)      (((v) & _F_MASK(f)) >> _F_START(f))
#define _F_DENORMALIZE(v, f)    (((v) << _F_START(f)) & _F_MASK(f))

/* Global macros */
#define SMI_FIELD_GET(x, reg, field) \
( \
    _F_NORMALIZE((x), reg ## _ ## field) \
)

#define SMI_FIELD_SET(x, reg, field, value) \
( \
    (x & ~_F_MASK(reg ## _ ## field)) \
    | _F_DENORMALIZE(reg ## _ ## field ## _ ## value, reg ## _ ## field) \
)

#define SMI_FIELD_VALUE(x, reg, field, value) \
( \
    (x & ~_F_MASK(reg ## _ ## field)) \
    | _F_DENORMALIZE(value, reg ## _ ## field) \
)

#define SMI_FIELD_CLEAR(reg, field) \
( \
    ~ _F_MASK(reg ## _ ## field) \
)

/* FIELD MACROS */
#define SMI_FIELD_START(field)              (0 ? field)
#define SMI_FIELD_END(field)                (1 ? field)
#define SMI_FIELD_SIZE(field)               (1 + SMI_FIELD_END(field) - SMI_FIELD_START(field))
#define SMI_FIELD_MASK(field)               (((1 << (SMI_FIELD_SIZE(field)-1)) | ((1 << (SMI_FIELD_SIZE(field)-1)) - 1)) << SMI_FIELD_START(field))
#define SMI_FIELD_NORMALIZE(reg, field)     (((reg) & SMI_FIELD_MASK(field)) >> SMI_FIELD_START(field))
#define SMI_FIELD_DENORMALIZE(field, value) (((value) << SMI_FIELD_START(field)) & SMI_FIELD_MASK(field))
#define SMI_FIELD_INIT(reg, field, value)   SMI_FIELD_DENORMALIZE(reg ## _ ## field, \
                                                          reg ## _ ## field ## _ ## value)
#define SMI_FIELD_INIT_VAL(reg, field, value) \
                                        (SMI_FIELD_DENORMALIZE(reg ## _ ## field, value))
#define SMI_FIELD_VAL_SET(x, r, f, v)       x = x & ~SMI_FIELD_MASK(r ## _ ## f) \
                                              | SMI_FIELD_DENORMALIZE(r ## _ ## f, r ## _ ## f ## _ ## v)

#define RGB(r, g, b) \
( \
    (unsigned long) (((r) << 16) | ((g) << 8) | (b)) \
)

#define RGB16(r, g, b) \
( \
    (unsigned short) ((((r) & 0xF8) << 8) | (((g) & 0xFC) << 3) | (((b) & 0xF8) >> 3)) \
)

#endif /* _DEFS_H_ */
