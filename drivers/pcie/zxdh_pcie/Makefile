subdir-ccflags-y += -I$(src)
subdir-ccflags-y += -I$(CWD)/include/
subdir-ccflags-y += -include $(CWD)/autoconf.h
ccflags-y += -Werror
# 追加额外的编译选项（如 CCFLAGS_ADD）
ifneq ($(CCFLAGS_ADD),)
ccflags-y += $(CCFLAGS_ADD)
endif

ifneq ($(CONFIG_DRIVER_VERSION),)
    EXTRA_CFLAGS += -DDRIVER_VERSION_VAL=\"$(CONFIG_DRIVER_VERSION)\"
    $(info CONFIG_HPF_DRIVER_VERSION is $(CONFIG_DRIVER_VERSION),\
      EXTRA_CFLAGS=$(EXTRA_CFLAGS))
else
    VERSION_FILE := $(shell cat $(CWD)/drivers/version.ini 2>/dev/null)
    ifneq ($(VERSION_FILE),)
        EXTRA_CFLAGS += -DDRIVER_VERSION_VAL=\"$(VERSION_FILE)\"
        $(info version.ini is $(VERSION_FILE), EXTRA_CFLAGS=$(EXTRA_CFLAGS))
    else
        EXTRA_CFLAGS += -DDRIVER_VERSION_VAL=\"1.0-1\"
        $(info Using default version: 1.0-1, EXTRA_CFLAGS=$(EXTRA_CFLAGS))
    endif
endif

obj-$(CONFIG_DINGHAI_HPF) += dinghai10e_hpf.o
dinghai10e_hpf-y := zxdh_pcie.o function_hotplug.o pcie_msix.o pcie_common.o bar_msg.o cmd/dh_cmd.o cmd/msg_chan_lock.o