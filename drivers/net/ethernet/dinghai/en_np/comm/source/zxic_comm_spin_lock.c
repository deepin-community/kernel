#include "zxic_common.h"
#include "zxic_private.h"

ZXIC_RTN32 zxic_comm_spin_create(ZXIC_SPIN_LOCK_T *p_spin)
{
    ZXIC_COMM_CHECK_POINT(p_spin);

    spin_lock_init(&p_spin->spin_lock);

    return ZXIC_OK;
}

ZXIC_RTN32 zxic_comm_spin_lock(ZXIC_SPIN_LOCK_T *p_spin)
{
    ZXIC_COMM_CHECK_POINT(p_spin);

    spin_lock(&p_spin->spin_lock);

    return ZXIC_OK;
}

ZXIC_RTN32 zxic_comm_spin_try_lock(ZXIC_SPIN_LOCK_T *p_spin)
{
    ZXIC_COMM_CHECK_POINT(p_spin);

    if(spin_trylock(&p_spin->spin_lock))
    {
        return ZXIC_OK;
    }
    else
    {
        return ZXIC_SPIN_LOCK_TRYLOCK_FAIL;
    }
}

ZXIC_RTN32 zxic_comm_spin_unlock(ZXIC_SPIN_LOCK_T *p_spin)
{
    spin_unlock(&p_spin->spin_lock);

    return ZXIC_OK;
}