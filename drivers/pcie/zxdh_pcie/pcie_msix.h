#ifndef _ZXDH_PF_HOT_PLUG_H_
#define _ZXDH_PF_HOT_PLUG_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <linux/msi.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/kernel.h>

typedef irqreturn_t (*irq_handler)(int irq_no, void *data);
typedef irqreturn_t (*irq_thread)(int irq, void *data);

#define IRQ_NO_INIT_VALUE                               (-1)
#define MSIX_NAME_LEN                                   30
struct msix_handler_info {
    int irq_id;
    irq_handler irq_handler_func;
    irq_thread irq_thread_func;
    char irq_name[MSIX_NAME_LEN];
};

#ifdef __cplusplus
}
#endif

#endif
