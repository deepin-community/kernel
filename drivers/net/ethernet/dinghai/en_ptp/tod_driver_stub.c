#include <linux/module.h>
//#include "../msg_chan_driver/msg_chan_pub.h"
#include <linux/dinghai/dh_cmd.h>
#if 0
typedef uint16_t (*rsc_recv_func_ptr)(uint8_t *pay_load, uint16_t len, uint8_t *reps_buffer, uint16_t *reps_len);

static rsc_recv_func_ptr g_riscv_event[100] = {0};

uint16_t RSC_MsgProRegister(uint8_t event_id, rsc_recv_func_ptr msg_pro_fun)
{
    if(100 <= event_id)
        return 0;
    g_riscv_event[event_id] = msg_pro_fun;
    return 0;
}
EXPORT_SYMBOL(RSC_MsgProRegister);

uint16_t RSC_MsgProUnregister(uint8_t event_id)
{
    if(100 <= event_id)
        return 0;
    g_riscv_event[event_id] = NULL;
    return 0;
}
EXPORT_SYMBOL(RSC_MsgProUnregister);


// int zxdh_bar_chan_sync_msg_send(struct zxdh_pci_bar_msg *in, struct zxdh_msg_recviver_mem *result)
// {
//     uint16_t reps_len = 0;
//     uint8_t *reps_buffer = NULL;
//     rsc_recv_func_ptr ptr = NULL;

//     reps_buffer = (uint8_t*)kmalloc(2048 - 12 + 4, GFP_KERNEL);
//     if (reps_buffer == NULL)
//     {
//         printk(KERN_ERR "%s: no space left on device.\n", __FUNCTION__);
//         return -ENOSPC;
//     }
//     memset(reps_buffer, 0x00, 2048 - 12 + 4); // 消息应答净荷长度最大2048 - 12, 再加上4B消息头

//     ptr = g_riscv_event[in->event_id];
//     if (ptr(in->payload_addr, in->payload_len, reps_buffer + 4, &reps_len) != 0)
//     {
//         kfree(reps_buffer);
//         printk(KERN_ERR "%s: rsc_recv_func_ptr failed.\n", __FUNCTION__);
//         return -EINVAL;
//     }

//     *(uint8_t*)reps_buffer = 0xFF;
//     *(uint16_t*)((((uint8_t*)reps_buffer )+ 1)) = reps_len;

//     memcpy(result->recv_buffer, reps_buffer, result->buffer_len);

//     kfree(reps_buffer);

//     return 0;
// }
// EXPORT_SYMBOL(zxdh_bar_chan_sync_msg_send);

MODULE_LICENSE("GPL");
#endif
