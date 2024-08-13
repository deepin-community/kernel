.. SPDX-License-Identifier: GPL-2.0

====================================================
Linux Kernel Driver for 3SNIC Intelligent NIC family
====================================================

Contents
========

- `Overview`_
- `Supported PCI vendor ID/device IDs`_
- `Supported features`_
- `Product specification`_
- `Support`_

Overview:
=========
SSSNIC is a network interface card that can meet the demand of a range
of application scenarios,such as the Data Center Area,cloud computing
and Financial industry,etc.

The construction of SSSNIC card facilities mainly depends on servers and
switches. 3S910, 920 and 930 are PCIe standard cards adapted to servers,
which provide extended external business interfaces for servers.

The driver supports a range of link-speed devices (100GE (40GE
compatible) and 25GE (10GE compatible)).A negotiated and extendable
feature set also supported.

Supported PCI vendor ID/device IDs:
===================================

1f3f:9020 - SSSNIC PF

Supported features:
===================

1. Support single-root I/O virtualization (SR-IOV)
2. Support virtual machine multi queue (VMMQ)
3. Support receive side scaling (RSS)
4. Support physical function (PF) passthrough VMs
5. Support the PF promiscuous mode,unicast or multicast MAC filtering, and
all multicast mode
6. Support IPv4/IPv6, checksum offload,TCP Segmentation Offload (TSO), and
Large Receive Offload (LRO)
7. Support in-band one-click logs collection
8. Support loopback tests
9. Support port location indicators

Product specification
=====================

        ===================     ======= =============================	===============================================
        PCI ID (pci.ids)        OEM     Product							PCIe port
        ===================     ======= =============================	===============================================
        1F3F:9020               3SNIC 	3S910(2 x 25GE SFP28 ports)		PCIe Gen3 x8(compatible with Gen2/ Gen1)
        1F3F:9020               3SNIC 	3S920(4 x 25GE SFP28 ports)		PCIe Gen4 x16, compatible with Gen3/ Gen2/ Gen1
        1F3F:9020               3SNIC 	3S930(2 x 100GE QSFP28 ports)	PCIe Gen4 x16, compatible with Gen3/ Gen2/ Gen1
        ===================     ======= =============================	===============================================


Support
=======

If an issue is identified with the released source code on the supported kernel
with a supported adapter, email the specific information related to the issue to
https://www.3snic.com.
