#include <linux/dinghai/driver.h>
#include <linux/types.h>
#include <linux/dinghai/device.h>
#include <linux/dinghai/dh_ifc.h>

static int32_t cmd_status_err(struct dh_core_dev *dev, int32_t err, uint16_t opcode, void *out)
{
    u8 status = DH_GET(mbox_out, out, status);

    return err;
}
static int32_t cmd_exec(struct dh_core_dev *dev, void *in, int32_t in_size, void *out,
                        int32_t out_size, zxdh_cmd_cbk_t callback, void *context,
                        bool force_polling)
{
    return 0;
}

int32_t zxdh_cmd_do(struct dh_core_dev *dev, void *in, int32_t in_size, void *out, int32_t out_size)
{
    int32_t err     = cmd_exec(dev, in, in_size, out, out_size, NULL, NULL, false);
    uint16_t opcode = DH_GET(mbox_in, in, opcode);

    err = cmd_status_err(dev, err, opcode, out);

    return err;
}
EXPORT_SYMBOL(zxdh_cmd_do);

int32_t zxdh_cmd_exec(struct dh_core_dev *dev, void *in, int32_t in_size, void *out, int32_t out_size)
{
    int32_t err = zxdh_cmd_do(dev, in, in_size, out, out_size);

    return zxdh_cmd_check(dev, err, in, out);
}