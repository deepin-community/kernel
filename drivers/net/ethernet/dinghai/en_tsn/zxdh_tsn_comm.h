#ifndef __ZXDH_TSN_COMM_H__
#define __ZXDH_TSN_COMM_H__

#ifdef __cplusplus
extern "C"{
#endif

#include <linux/types.h>
#include "log.h"

#ifndef TSN_OK
#define TSN_OK                   (0)
#endif

#define ZXDH_TSN_COMM_CHECK_RC(rc)\
do{\
    if(TSN_OK != (rc))\
    {\
        DH_LOG_ERR(MODULE_TSN, "[ErrorCode: %d] !\n", rc);\
        return rc;\
    }\
}while(0)

#define ZXDH_TSN_COMM_CHECK_RC_RETURN_NONE(rc)\
do{\
    if(TSN_OK != (rc))\
    {\
        DH_LOG_ERR(MODULE_TSN, "[ErrorCode: %d] !\n", rc);\
        return;\
    }\
}while(0)

#define ZXDH_TSN_COMM_CHECK_RC_UNLOCK_RETURN_NONE(rc, lock)\
do{\
    if(TSN_OK != (rc))\
    {\
        DH_LOG_ERR(MODULE_TSN, "[ErrorCode: %d] !\n", rc);\
        spin_unlock(lock);\
        return;\
    }\
}while(0)

#define ZXDH_TSN_COMM_CHECK_RC_UNLOCK_RETURN_VALUE(rc, lock, value)\
do{\
    if(TSN_OK != (rc))\
    {\
        DH_LOG_ERR(MODULE_TSN, "[ErrorCode: %d] !\n", rc);\
        spin_unlock(lock);\
        return value;\
    }\
}while(0)

#define ZXDH_TSN_COMM_CHECK_RC_UNLOCKIRQ_RETURN_VALUE(rc, lock, flags, value)\
do{\
    if(TSN_OK != (rc))\
    {\
        DH_LOG_ERR(MODULE_TSN, "[ErrorCode: %d] !\n", rc);\
        spin_unlock_irqrestore(lock, flags);\
        return value;\
    }\
}while(0)

#define ZXDH_TSN_COMM_CHECK_RC_UNLOCKIRQ_MEMORY_FREE(rc, lock, flags, ptr)\
do{\
    if(TSN_OK != (rc))\
    {\
        DH_LOG_ERR(MODULE_TSN, "[ErrorCode: %d] !\n", rc);\
        spin_unlock_irqrestore(lock, flags);\
        kfree(ptr);\
        return rc;\
    }\
}while(0)

#define ZXDH_TSN_COMM_CHECK_RC_UNLOCK_MEMORY_FREE_RETURN_NONE(rc, lock, ptr)\
do{\
    if(TSN_OK != (rc))\
    {\
        DH_LOG_ERR(MODULE_TSN, "[ErrorCode: %d] !\n", rc);\
        spin_unlock(lock);\
        kfree(ptr);\
        return;\
    }\
}while(0)

#define ZXDH_TSN_COMM_CHECK_RC_UNLOCK_MEMORY_FREE_RETURN_VALUE(rc, lock, ptr, value)\
do{\
    if(TSN_OK != (rc))\
    {\
        DH_LOG_ERR(MODULE_TSN, "[ErrorCode: %d] !\n", rc);\
        spin_unlock(lock);\
        kfree(ptr);\
        return value;\
    }\
}while(0)

#define ZXDH_TSN_COMM_CHECK_RC_MEMORY_FREE(rc, ptr)\
do{\
    if(TSN_OK != (rc))\
    {\
        DH_LOG_ERR(MODULE_TSN, "[ErrorCode: %d] !\n", rc);\
        kfree(ptr);\
        return rc;\
    }\
} while(0)

#define ZXDH_TSN_COMM_CHECK_POINT(point)\
do{\
    if(NULL == (point))\
    {\
        DH_LOG_ERR(MODULE_TSN, "[Error: POINT NULL] !\n");\
        return -EINVAL;\
    }\
}while(0)

#define ZXDH_TSN_COMM_CHECK_POINT_RETURN_NONE(point)\
do{\
    if(NULL == (point))\
    {\
        DH_LOG_ERR(MODULE_TSN, "[Error: POINT NULL] !\n");\
        return;\
    }\
}while(0)

#define ZXDH_TSN_COMM_CHECK_POINT_RETURN_VALUE(point, value)\
do{\
    if(NULL == (point))\
    {\
        DH_LOG_ERR(MODULE_TSN, "[Error: POINT NULL] !\n");\
        return value;\
    }\
}while(0)

#define ZXDH_TSN_COMM_CHECK_POINT_UNLOCK_RETURN_NONE(point, lock)\
do{\
    if(NULL == (point))\
    {\
        DH_LOG_ERR(MODULE_TSN, "[Error: POINT NULL] !\n");\
        spin_unlock(lock);\
        return;\
    }\
}while(0)

#define ZXDH_TSN_COMM_CHECK_POINT_UNLOCK_RETURN_VALUE(point, lock, value)\
do{\
    if(NULL == (point))\
    {\
        DH_LOG_ERR(MODULE_TSN, "[Error: POINT NULL] !\n");\
        spin_unlock(lock);\
        return value;\
    }\
}while(0)

#define ZXDH_TSN_COMM_CHECK_POINT_UNLOCK_MEMORY_FREE_RETURN_NONE(point, lock, ptr)\
do{\
    if(NULL == (point))\
    {\
        DH_LOG_ERR(MODULE_TSN, "[Error: POINT NULL] !\n");\
        spin_unlock(lock);\
        kfree(ptr);\
        return;\
    }\
}while(0)

#define ZXDH_TSN_COMM_CHECK_INDEX(val, min, max)\
do{\
    if(((val) < (min)) || ((val) > (max)))\
    {\
        DH_LOG_ERR(MODULE_TSN, "[Error: VALUE %u INVALID] [MIN %u MAX %u] !\n", val, min, max);\
        return -EINVAL;\
    }\
}while(0)

#define ZXDH_TSN_COMM_CHECK_INDEX_64(val, min, max)\
do{\
    if(((val) < (min)) || ((val) > (max)))\
    {\
        DH_LOG_ERR(MODULE_TSN, "[Error: VALUE %llu INVALID] [MIN %llu MAX %llu] !\n", val, min, max);\
        return -EINVAL;\
    }\
}while(0)

#define ZXDH_TSN_COMM_CHECK_INDEX_MAX(val, max)\
do{\
    if((val) > (max))\
    {\
        DH_LOG_ERR(MODULE_TSN, "[Error: VALUE %u INVALID] [MAX %u] !\n", val, max);\
        return -EINVAL;\
    }\
}while(0)

#define ZXDH_TSN_COMM_CHECK_INDEX_MAX_MEMORY_FREE(val, max, ptr)\
do{\
    if((val) > (max))\
    {\
        DH_LOG_ERR(MODULE_TSN, "[Error: VALUE %u INVALID] [MAX %u] !\n", val, max);\
        kfree(ptr);\
        return -EINVAL;\
    }\
}while(0)

#define ZXDH_TSN_COMM_CHECK_INDEX_EQUAL(val, equal)\
do{\
    if((val) != (equal))\
    {\
        DH_LOG_ERR(MODULE_TSN, "[Error: VALUE %u INVALID] [EQUAL %u] !\n", val, equal);\
        return -EINVAL;\
    }\
}while(0)

#define ZXDH_TSN_COMM_CHECK_INDEX_EQUAL_64(val, equal)\
do{\
    if((val) != (equal))\
    {\
        DH_LOG_ERR(MODULE_TSN, "[Error: VALUE %llu INVALID] [EQUAL %llu] !\n", val, equal);\
        return -EINVAL;\
    }\
}while(0)

#define ZXDH_TSN_COMM_CHECK_INDEX_EQUAL_RETURN_OK(val, equal)\
do{\
    if((val) != (equal))\
    {\
        return TSN_OK;\
    }\
}while(0)

#define ZXDH_TSN_COMM_CHECK_INDEX_EQUAL_RETURN_NONE(val, equal)\
do{\
    if((val) != (equal))\
    {\
        return;\
    }\
}while(0)

#ifdef __cplusplus
}
#endif
#endif /* __ZXDH_TSN_COMM_H__ */
