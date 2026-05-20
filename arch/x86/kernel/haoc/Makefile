obj-y	+= haoc.o bitmap.o
obj-y	+= iee/
obj-$(CONFIG_PTP) += ptp/
obj-$(CONFIG_CREDP) += credp/
obj-$(CONFIG_KEYP) += keyp/
ccflags-$(CONFIG_IEE_SELINUX_P) += -I$(srctree)/security/selinux -I$(srctree)/security/selinux/include
obj-$(CONFIG_IEE_SELINUX_P) += selinuxp/
obj-$(CONFIG_VARP) += varp/
