#ifndef _FT_RAS_H_
#define _FT_RAS_H_
 
#include <linux/types.h>
 
#define RAS_BIND_EVENT            _IOR('k', 0, unsigned int)
#define RAS_NONBIND_EVENT            _IOR('k', 1, unsigned int)

#endif
