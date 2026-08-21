/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_HAOC_PTP_H
#define _LINUX_HAOC_PTP_H

extern void ptp_disable_wp(unsigned long *cr0);
extern void ptp_restore_wp(unsigned long cr0);
extern void ptp_context_enable_wp(int *wp_disabled_cnt, unsigned long *cr0);
extern void ptp_context_restore_wp(int wp_disabled_cnt, unsigned long cr0);

#endif  /* _LINUX_HAOC_PTP_H */
