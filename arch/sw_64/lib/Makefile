# SPDX-License-Identifier: GPL-2.0
#
# Makefile for sw-specific library files..
#

asflags-y := $(KBUILD_CFLAGS)
ccflags-y := -Werror

lib-y =	__divlu.o __remlu.o __divwu.o __remwu.o \
	udelay.o \
        memmove.o \
        checksum.o \
        csum_partial_copy.o \
        fpreg.o \
        strcpy.o \
        strncpy.o \
        fls.o \
        csum_ipv6_magic.o \
	string.o \
	uaccess.o

lib-clear_page-y := clear_page.o
lib-clear_page-$(CONFIG_DEEP_CLEAR_PAGE) := deep-clear_page.o

lib-clear_user-y := clear_user.o deep-clear_user-no_una_check.o deep-clear_user-force_ali_addr.o

lib-copy_page-y := copy_page.o
lib-copy_page-$(CONFIG_DEEP_COPY_PAGE) := deep-copy_page.o

lib-copy_user-y := copy_user.o deep-copy_user-no_una_check.o deep-copy_user-force_ali_addr.o

lib-memcpy-y := memcpy.o deep-memcpy-no_una_check.o deep-memcpy-force_ali_addr.o

lib-memset-y := memset.o deep-memset-no_una_check.o deep-memset-force_ali_addr.o

lib-$(CONFIG_ARCH_HAS_UACCESS_FLUSHCACHE) += uaccess_flushcache.o

lib-y += $(lib-clear_page-y) $(lib-clear_user-y) $(lib-copy_page-y) $(lib-copy_user-y) $(lib-memcpy-y) $(lib-memset-y)

obj-y = iomap.o
obj-y += iomap_copy.o

# The division routines are built from single source, with different defines.
AFLAGS___divlu.o = -DDIV
AFLAGS___remlu.o =       -DREM
AFLAGS___divwu.o = -DDIV       -DINTSIZE
AFLAGS___remwu.o =       -DREM -DINTSIZE

$(addprefix $(obj)/,__divlu.o __remlu.o __divwu.o __remwu.o): \
						$(src)/divide.S FORCE
	$(call if_changed_rule,as_o_S)
