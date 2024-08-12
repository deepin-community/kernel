/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2020 - 2023, Chengdu BeiZhongWangXin Technology Co., Ltd. */

#ifndef _NE6X_INTERRUPT_H
#define _NE6X_INTERRUPT_H

#include "ne6x.h"

int ne6x_init_interrupt_scheme(struct ne6x_pf *pf);
int ne6x_adpt_setup_vectors(struct ne6x_adapter *adpt);
void ne6x_adpt_free_q_vectors(struct ne6x_adapter *adpt);
int ne6x_adpt_request_irq(struct ne6x_adapter *adpt, char *basename);
void ne6x_adpt_configure_msix(struct ne6x_adapter *adpt);
int ne6x_adpt_enable_irq(struct ne6x_adapter *adpt);
void ne6x_adpt_free_irq(struct ne6x_adapter *adpt);
void ne6x_clear_interrupt_scheme(struct ne6x_pf *pf);
void ne6x_adpt_disable_irq(struct ne6x_adapter *adpt);
irqreturn_t ne6x_linkint_irq_handler(int irq, void *data);
int ne6x_enable_link_irq(struct ne6x_pf *pf);
int ne6x_disable_link_irq(struct ne6x_pf *pf);
int ne6x_init_link_irq(struct ne6x_pf *pf);
void ne6x_free_link_irq(struct ne6x_pf *pf);
int ne6x_init_mailbox_irq(struct ne6x_pf *pf);
void ne6x_free_mailbox_irq(struct ne6x_pf *pf);
int ne6x_disable_mailbox_irq(struct ne6x_pf *pf);

#endif
