.. SPDX-License-Identifier: GPL-2.0

.. _deepin_kabi:

======================
deepin kABI Maintainer's Guide
======================

This document describes how the deepin kernel maintains kernel ABI (kABI)
stability, how to use the ``DEEPIN_KABI_*`` helper macros, and how the
symbol-checksum tooling in ``scripts/deepin/kabi/`` is used for manual ABI checks.

Relationship to CentOS Stream ``rh_kabi``
=========================================

The deepin kABI infrastructure is derived from the RHEL/CentOS Stream
``rh_kabi`` infrastructure:

* ``include/linux/deepin_kabi.h`` is a debranded copy of CentOS Stream
  ``include/linux/rh_kabi.h``.  The macro set is kept semantically
  identical (1:1) with only naming changes (``RH_KABI_*`` ->
  ``DEEPIN_KABI_*``, ``rh_reserved*`` -> ``deepin_reserved*``,
  ``struct foo_rh`` -> ``struct foo_deepin``).
* The scripts in ``scripts/deepin/kabi/`` (``check-kabi``, ``show-kabi``,
  ``make-kabi``, ``update-kabi``, ``diff-kabi``, ``symtypes``,
  ``symtype-generate``) are imported from CentOS Stream 10
  ``kernel-6.12.0-255.el10``, directory ``redhat/kabi/``, with only
  path/naming adaptations (marked ``deepin:`` in the sources).
* No ``genksyms``/``modpost`` patches are required, same as upstream
  CentOS Stream.

Current sync point: CentOS Stream 10 ``kernel-6.12.0-255.el10``.
To re-verify the header against a CentOS Stream tree, normalize the names
and diff::

	sed 's/DEEPIN/RH/g; s/deepin/rh/g' include/linux/deepin_kabi.h | \
		diff -u - /path/to/centos-stream-10/include/linux/rh_kabi.h

The expected differences are only: SPDX/copyright lines, the provenance
comment, the extra ``#include <linux/args.h>``, and the two
``#ifdef CONFIG_DEEPIN_KABI_RESERVE`` gates (a deliberate deepin
divergence, see below).

Configuration
=============

.. warning::

   ``CONFIG_DEEPIN_KABI_RESERVE`` defaults to **off**.  When it is off,
   every ``DEEPIN_KABI_RESERVE()``/``DEEPIN_KABI_USE()`` slot is compiled
   out and none of the kABI padding exists in the built kernel.  Any build
   that is supposed to keep kABI stability (e.g. release builds) **must**
   enable it explicitly.

   Enabling or disabling this option changes struct layouts and therefore
   the genksyms CRCs of all symbols whose types contain reserved fields.
   Treat the first build with the option enabled as a **new kABI
   baseline**: do not compare CRCs across the transition.

* ``CONFIG_DEEPIN_KABI_RESERVE`` -- enables the reserved-field mechanism
  (``_DEEPIN_KABI_RESERVE()`` / ``_DEEPIN_KABI_REPLACE()`` expand to real
  fields/unions instead of nothing).  Default: off.
* ``CONFIG_DEEPIN_KABI_SIZE_ALIGN_CHECKS`` -- enables compile-time size
  and alignment assertions in ``DEEPIN_KABI_REPLACE``,
  ``DEEPIN_KABI_EXTEND_WITH_SIZE`` and ``DEEPIN_KABI_EXCLUDE_WITH_SIZE``.
  Depends on ``CONFIG_DEEPIN_KABI_RESERVE``.  Default: follows
  ``CONFIG_DEEPIN_KABI_RESERVE`` (``default DEEPIN_KABI_RESERVE``), i.e.
  on for builds with reserve support; force-disabled when struct-growing
  debug options (e.g. ``CONFIG_LOCKDEP``) are selected, because debug
  builds allow struct sizes to change.

Macro usage rules
=================

The authoritative description of every macro is the comment block in
``include/linux/deepin_kabi.h``.  Rules that matter in practice:

* ``DEEPIN_KABI_RESERVE(n)`` pads a struct before a kABI freeze.  Numbers
  start at 1 and must be consecutive within a struct.  Always place the
  reservations at the end of the struct (or at the end of a well-defined
  region, e.g. before ``randomized_struct_fields_end`` in
  ``task_struct``).
* ``DEEPIN_KABI_USE(n, ...)`` consumes previously reserved fields.  When
  the new fields are conditional on a Kconfig option, keep the
  ``DEEPIN_KABI_USE()`` in the ``#ifdef`` branch and the matching
  ``DEEPIN_KABI_RESERVE()`` entries in the ``#else`` branch so that the
  layout only changes with the config, never silently
  (see ``struct bdi_writeback`` in ``include/linux/backing-dev-defs.h``).
* ``DEEPIN_KABI_EXTEND()`` may only be used for structs that are not
  embedded in other structs and are not allocated by drivers.
* The ``DEEPIN_KABI_BROKEN_*`` family and ``DEEPIN_KABI_EXCLUDE*``
  deliberately break or bypass kABI.  Any use must be justified in the
  commit message and paired with ``DEEPIN_KABI_FORCE_CHANGE`` where
  applicable.
* Every use of a kABI macro must be explained in the patch that
  introduces it, and must be reviewed by the DEEPIN KABI-HELPERS
  maintainers (see ``MAINTAINERS``).

DEEPIN_KABI_FORCE_CHANGE is placement sensitive
===============================================

``DEEPIN_KABI_FORCE_CHANGE(ver)`` expands (under ``__GENKSYMS__``) to
``__attribute__((deepin_kabi_change##ver))``.  Stock genksyms handles
unknown attributes in two different ways:

* in a qualifier position (e.g. after ``*``, or among the declaration
  specifiers) the attribute text is **kept** in the symbol expansion and
  the CRC **changes**;
* in the trailing declarator position the attribute is **silently
  discarded** and the CRC does **not** change.

So this silently does nothing::

	int foo(int arg) DEEPIN_KABI_FORCE_CHANGE(1);	/* WRONG: CRC unchanged */

Place it where genksyms keeps it, e.g.::

	int DEEPIN_KABI_FORCE_CHANGE(1) foo(int arg);	/* OK */

Verified on this tree (6.6, stock genksyms): the trailing form produced
an identical ``#SYMVER`` checksum (``0x5240c7ab``) with and without the
attribute, while the qualifier/declaration-specifier forms changed it
(``0x980d7919`` / ``0x30731164``).  To re-verify::

	printf 'struct foo { int a; };\nstruct foo *bar(struct foo *x);\n__GENKSYMS_EXPORT_SYMBOL(bar);\n' > /tmp/t.c
	gcc -E -D__GENKSYMS__ /tmp/t.c | scripts/genksyms/genksyms -T /tmp/t.sym

compare the output against the same file with the attribute added in the
position you intend to use.

Extending task_struct: use the AUX mechanism
============================================

``task_struct`` carries two extension hang points:

* ``struct task_struct_extend`` + the ``task_struct_extend`` pointer --
  **deprecated, do not use for new work**.  Kept only to preserve the
  existing layout; it must not be removed.
* ``struct task_struct_deepin`` + ``DEEPIN_KABI_AUX_PTR(task_struct)`` --
  the preferred mechanism for new extensions.

Before the first field is added to ``struct task_struct_deepin``, the
auxiliary structure must be wired up, otherwise ``DEEPIN_KABI_AUX()``
will always report the field as absent:

1. allocate ``struct task_struct_deepin`` where the base struct is
   allocated (fork path) and free it accordingly;
2. call ``DEEPIN_KABI_AUX_SET_SIZE(task, task_struct)`` at every
   allocation site (or use ``DEEPIN_KABI_AUX_INIT_SIZE()`` for static
   instances);
3. guard every field access with ``DEEPIN_KABI_AUX(task, task_struct,
   field)``.

The same procedure applies to any other struct extended with
``DEEPIN_KABI_AUX_PTR``/``DEEPIN_KABI_AUX_EMBED``.

Manual symbol baseline workflow
===============================

The deepin tree does not carry an in-tree symbol baseline.  Maintainers
keep baselines outside the tree and check builds manually with the tools
in ``scripts/deepin/kabi/``:

* generate a reference ``Module.kabi`` from a known-good build's
  ``Module.symvers`` and a stablelist of tracked symbols::

	python3 scripts/deepin/kabi/make-kabi -k Module.kabi_x86_64 -s Module.symvers -w kabi-module/kabi_x86_64/

  ``-w`` selects the symbol stablelist; it may be a
  ``kabi-module/kabi_<arch>/`` directory (see ``make-kabi -h``) or a
  stablelist file produced by ``show-kabi``.

* check a new build against the reference::

	python3 scripts/deepin/kabi/check-kabi -k Module.kabi_x86_64 -s Module.symvers

  exit status 1 means a CRC mismatch (ABI breakage) or a symbol that
  moved between vmlinux and modules; both are reported on stdout.

* ``show-kabi``/``update-kabi``/``diff-kabi``/``symtypes``/
  ``symtype-generate`` manage a per-symbol ``kabi-module/kabi_<arch>/``
  database; see ``-h`` of each tool.  These tools default to the
  ``scripts/deepin/`` directory as their data root (the ``REDHAT`` environment
  variable name is kept for script compatibility and can be used to
  override the location).

Errata
======

* Commit ``efd0fdb72737e`` ("kabi: Introduce CONFIG_KABI_RESERVE") names
  the option ``CONFIG_KABI_RESERVE`` in its commit message; the symbol
  that was actually introduced is ``CONFIG_DEEPIN_KABI_RESERVE``.
