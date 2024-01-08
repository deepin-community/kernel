#!/bin/sh
# SPDX-License-Identifier: GPL-2.0+

grep "__vdso_" | sed 's/\([0-9a-f]*\) T \([a-z0-9_]*\)\(@@LINUX_.*\)*/.globl\t\2\n\2:\n.quad\t0x\1/'
