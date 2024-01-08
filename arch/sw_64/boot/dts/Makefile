# SPDX-License-Identifier: GPL-2.0
# Built-in dtb

ifeq ($(CONFIG_PLATFORM_XUELANG),y)
builtindtb-y            := chip3
endif

ifeq ($(CONFIG_PLATFORM_JUNZHANG),y)
builtindtb-y            := empty
endif

ifeq ($(CONFIG_BUILTIN_DTB), y)
ifneq ($(CONFIG_BUILTIN_DTB_NAME),"")
        builtindtb-y    := $(patsubst "%",%,$(CONFIG_BUILTIN_DTB_NAME))
endif

obj-y   += $(builtindtb-y).dtb.o
dtb-y := $(builtindtb-y).dtb

# for CONFIG_OF_ALL_DTBS test
dtstree := $(srctree)/$(src)
dtb-    := $(patsubst $(dtstree)/%.dts,%.dtb, $(wildcard $(dtstree)/*.dts))
else
dtb-y := $(builtindtb-y).dtb
endif

clean-files := *.dtb  *.dtb.S
