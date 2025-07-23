// SPDX-License-Identifier: GPL-2.0+
// Copyright (c) 2023, SiliconMotion Inc.


#ifndef _SMI_SYSFS_H_
#define _SMI_SYSFS_H_




#define REGS_SIZE 0x180000


struct smi_regs{
      int offset0;
      int offset1;
      char *regname;
};



#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 6, 0)
        void smi_debugfs_init(struct drm_minor *minor);
#else
        int smi_debugfs_init(struct drm_minor *minor);
#endif



#endif
