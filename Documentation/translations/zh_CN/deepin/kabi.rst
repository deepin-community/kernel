.. SPDX-License-Identifier: GPL-2.0

.. _cn_deepin_kabi:

.. include:: ../disclaimer-zh_CN.rst

:Original: :ref:`Documentation/deepin/kabi.rst <deepin_kabi>`

译者::

    中文版维护者： WangYuli <wangyuli@deepin.org>
    中文版翻译者： WangYuli <wangyuli@deepin.org>

=================
deepin kABI 维护指南
=================

本文档说明 deepin 内核如何维护内核 ABI（kABI）稳定性、如何使用
``DEEPIN_KABI_*`` 辅助宏，以及如何使用 ``scripts/deepin/kabi/`` 目录下的
符号校验和工具进行人工 ABI 检查。

与 CentOS Stream ``rh_kabi`` 的关系
===================================

deepin 的 kABI 基础设施源自 RHEL/CentOS Stream 的 ``rh_kabi`` 体系：

* ``include/linux/deepin_kabi.h`` 是 CentOS Stream
  ``include/linux/rh_kabi.h`` 的去品牌化（debrand）副本。宏集合在语义上
  与其保持 1:1 一致，仅做了命名替换（``RH_KABI_*`` ->
  ``DEEPIN_KABI_*``、``rh_reserved*`` -> ``deepin_reserved*``、
  ``struct foo_rh`` -> ``struct foo_deepin``）。
* ``scripts/deepin/kabi/`` 下的脚本（``check-kabi``、``show-kabi``、
  ``make-kabi``、``update-kabi``、``diff-kabi``、``symtypes``、
  ``symtype-generate``）导入自 CentOS Stream 10
  ``kernel-6.12.0-255.el10`` 的 ``redhat/kabi/`` 目录，仅做了路径与
  命名适配（在源码中以 ``deepin:`` 标出）。
* 与 CentOS Stream 一样，不需要给 ``genksyms``/``modpost`` 打任何补丁。

当前同步点：CentOS Stream 10 ``kernel-6.12.0-255.el10``。
如需将头文件与 CentOS Stream 源码树重新核对，先做命名归一化再 diff::

	sed 's/DEEPIN/RH/g; s/deepin/rh/g' include/linux/deepin_kabi.h | \
		diff -u - /path/to/centos-stream-10/include/linux/rh_kabi.h

预期差异仅有：SPDX/版权行、来源说明注释、额外的
``#include <linux/args.h>``，以及两处 ``#ifdef
CONFIG_DEEPIN_KABI_RESERVE`` 条件门控（deepin 的刻意分叉，见下文）。

配置
====

.. warning::

   ``CONFIG_DEEPIN_KABI_RESERVE`` **默认关闭**。关闭时，所有
   ``DEEPIN_KABI_RESERVE()``/``DEEPIN_KABI_USE()`` 预留位都会被编译
   为空，构建出的内核中不存在任何 kABI 填充。任何需要保持 kABI
   稳定性的构建（例如发布构建）都**必须**显式开启它。

   开启或关闭该选项会改变结构体布局，进而改变所有类型中含预留字段
   的符号的 genksyms CRC。应将开启该选项后的首个构建视为**新的
   kABI 基线**，不要跨越该临界点比较 CRC。

* ``CONFIG_DEEPIN_KABI_RESERVE`` —— 启用预留字段机制
  （``_DEEPIN_KABI_RESERVE()``/``_DEEPIN_KABI_REPLACE()`` 展开为真实
  字段/联合体，而非空）。默认：关。
* ``CONFIG_DEEPIN_KABI_SIZE_ALIGN_CHECKS`` —— 启用
  ``DEEPIN_KABI_REPLACE``、``DEEPIN_KABI_EXTEND_WITH_SIZE`` 和
  ``DEEPIN_KABI_EXCLUDE_WITH_SIZE`` 中的编译期尺寸与对齐断言。
  依赖于 ``CONFIG_DEEPIN_KABI_RESERVE``。默认：跟随
  ``CONFIG_DEEPIN_KABI_RESERVE``（即启用预留机制的构建默认开启）；
  选中会使结构体增大的调试选项（如 ``CONFIG_LOCKDEP``）时由 Kconfig
  强制关闭（调试构建允许结构体尺寸变化）。

宏使用规则
==========

每个宏的权威说明见 ``include/linux/deepin_kabi.h`` 头部的注释块。
实践中最重要的规则：

* ``DEEPIN_KABI_RESERVE(n)`` 在 kABI 冻结前为结构体预留空间。编号从 1
  开始，同一结构体内必须连续。预留位总是放在结构体末尾（或明确界定
  区域的末尾，例如 ``task_struct`` 中的
  ``randomized_struct_fields_end`` 之前）。
* ``DEEPIN_KABI_USE(n, ...)`` 消耗先前预留的字段。当新字段依赖于某个
  Kconfig 选项时，把 ``DEEPIN_KABI_USE()`` 放在 ``#ifdef`` 分支、把对应
  的 ``DEEPIN_KABI_RESERVE()`` 放在 ``#else`` 分支，使布局只随配置
  变化、绝不静默漂移（范例见 ``include/linux/backing-dev-defs.h``
  中的 ``struct bdi_writeback``）。
* ``DEEPIN_KABI_EXTEND()`` 只能用于不内嵌于其他结构体、也不由驱动
  分配的结构体。
* ``DEEPIN_KABI_BROKEN_*`` 家族与 ``DEEPIN_KABI_EXCLUDE*`` 是有意破坏
  或绕过 kABI 的手段。任何使用都必须在提交信息中说明理由，并在适用
  时配合 ``DEEPIN_KABI_FORCE_CHANGE``。
* 每一处 kABI 宏的使用都必须在引入它的补丁中解释其正当性，并经过
  DEEPIN KABI-HELPERS 维护者评审（见 ``MAINTAINERS``）。

DEEPIN_KABI_FORCE_CHANGE 对放置位置敏感
========================================

``DEEPIN_KABI_FORCE_CHANGE(ver)`` 在 ``__GENKSYMS__`` 下展开为
``__attribute__((deepin_kabi_change##ver))``。原版 genksyms 对未知
attribute 有两种截然不同的处理方式：

* 处于限定符位置（例如 ``*`` 之后，或声明说明符之中）时，attribute
  文本会被**保留**在符号展开式中，CRC **会改变**；
* 处于声明符尾部位置时，attribute 会被**静默丢弃**，CRC **不变**。

因此下面这种写法会静默失效::

	int foo(int arg) DEEPIN_KABI_FORCE_CHANGE(1);	/* 错误：CRC 不变 */

请放在 genksyms 会保留的位置，例如::

	int DEEPIN_KABI_FORCE_CHANGE(1) foo(int arg);	/* 正确 */

本树（6.6，原版 genksyms）实测：尾置形式加不加 attribute 得到的
``#SYMVER`` 校验和完全相同（均为 ``0x5240c7ab``），而限定符位/
声明说明符位形式会改变校验和（``0x980d7919``/``0x30731164``）。
可用下面的方法复验::

	printf 'struct foo { int a; };\nstruct foo *bar(struct foo *x);\n__GENKSYMS_EXPORT_SYMBOL(bar);\n' > /tmp/t.c
	gcc -E -D__GENKSYMS__ /tmp/t.c | scripts/genksyms/genksyms -T /tmp/t.sym

将输出与在目标位置加入 attribute 后的同一份文件做对比即可。

扩展 task_struct：请使用 AUX 机制
=================================

``task_struct`` 上有两个扩展挂点：

* ``struct task_struct_extend`` 与 ``task_struct_extend`` 指针 ——
  **已弃用，请勿用于新工作**。仅为保持既有布局而保留，不得删除。
* ``struct task_struct_deepin`` 与
  ``DEEPIN_KABI_AUX_PTR(task_struct)`` —— 新扩展的首选机制。

首次向 ``struct task_struct_deepin`` 添加字段之前，必须先完成辅助
结构体的接线，否则 ``DEEPIN_KABI_AUX()`` 永远判定字段不存在：

1. 在分配基结构体的路径（fork 路径）上分配
   ``struct task_struct_deepin``，并相应地释放；
2. 在每个分配点调用 ``DEEPIN_KABI_AUX_SET_SIZE(task, task_struct)``
   （静态实例则使用 ``DEEPIN_KABI_AUX_INIT_SIZE()``）；
3. 每次访问字段都用 ``DEEPIN_KABI_AUX(task, task_struct, field)``
   做存在性守卫。

同样的步骤适用于任何使用 ``DEEPIN_KABI_AUX_PTR`` 或
``DEEPIN_KABI_AUX_EMBED`` 扩展的结构体。

符号基线的人工管理流程
======================

deepin 源码树不内置符号基线。基线由维护者在树外保存，并使用
``scripts/deepin/kabi/`` 中的工具对构建产物做人工检查：

* 用已知良好的构建产物 ``Module.symvers`` 与受跟踪符号的
  stablelist 生成参考 ``Module.kabi``::

	python3 scripts/deepin/kabi/make-kabi -k Module.kabi_x86_64 -s Module.symvers -w kabi-module/kabi_x86_64/

  ``-w`` 指定符号 stablelist，可以是 ``kabi-module/kabi_<arch>/``
  目录（参见 ``make-kabi -h``），也可以是 ``show-kabi`` 生成的
  stablelist 文件。

* 将新构建与参考基线比对::

	python3 scripts/deepin/kabi/check-kabi -k Module.kabi_x86_64 -s Module.symvers

  退出码为 1 表示存在 CRC 不匹配（ABI 破坏）或符号在 vmlinux 与
  模块之间发生了移动，具体情况会打印到标准输出。

* ``show-kabi``/``update-kabi``/``diff-kabi``/``symtypes``/
  ``symtype-generate`` 用于管理按符号组织的
  ``kabi-module/kabi_<arch>/`` 数据库，各工具的 ``-h`` 有详细说明。
  这些工具默认以 ``scripts/deepin/`` 目录作为数据根目录（出于脚本兼容性
  保留了 ``REDHAT`` 这一环境变量名，可用它覆盖数据根目录位置）。

勘误
====

* 提交 ``efd0fdb72737e``（"kabi: Introduce CONFIG_KABI_RESERVE"）的
  提交信息中写的选项名是 ``CONFIG_KABI_RESERVE``，而实际引入的符号
  是 ``CONFIG_DEEPIN_KABI_RESERVE``。
