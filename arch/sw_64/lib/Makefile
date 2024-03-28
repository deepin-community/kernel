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
lib-clear_page-$(CONFIG_DEEP_CLEAR_PAGE) := clear_page_simd.o

lib-clear_user-y := clear_user.o clear_user_simd.o clear_user_simd_align.o

lib-copy_page-y := copy_page.o
lib-copy_page-$(CONFIG_DEEP_COPY_PAGE) := copy_page_simd.o

lib-copy_user-y := copy_user.o copy_user_simd.o copy_user_simd_align.o

lib-memcpy-y := memcpy.o memcpy_simd.o memcpy_simd_align.o

lib-memset-y := memset.o memset_simd.o memset_simd_align.o

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
