subdirs := en_np/agent/
subdirs += en_np/cmd/
subdirs += en_np/comm/
subdirs += en_np/driver/
subdirs += en_np/init/
subdirs += en_np/netlink/
subdirs += en_np/sdk/
subdirs += en_np/table/
subdirs += en_np/qos/
subdirs += en_np/fc/
subdirs += en_np/flow/

# Absolute path of the driver source directory; $(src) is relative to the
# object tree when the kernel is built with a separate output directory.
dinghai_root := $(if $(filter /%,$(src)),$(src),$(srctree)/$(src))
include $(dinghai_root)/en_np/Makefile.include

src_files :=
include $(foreach subdir, $(subdirs), $(dinghai_root)/$(subdir)Kbuild.include)
obj_files := $(src_files:.c=.o)

obj-$(CONFIG_DINGHAI_NP) += dinghai10e_np.o
dinghai10e_np-y := $(obj_files)
dinghai10e_np-y += log.o

