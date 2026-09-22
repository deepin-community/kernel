/* SPDX-License-Identifier: GPL-2.0 or Linux-OpenIB */
/* Copyright (c) 2023 - 2024 ZTE Corporation */

/**
 * @file        hal_smmu.h
 * @brief        mmu600 ARMV8页表格式头文件
 * @details
 * @author        陈港文
 * @date        2021-04-27
 * @version     V1.0
 * @copyright     Copyright (c) 2018-2020  中兴通讯有限公司
 **********************************************************************************
 * @attention
 *    - 硬件平台:msc4.0
 *    - 架构支持：arm64
 * @warning
 *    -
 * @bug
 *    -
 * @par 修改日志:
 * <table>
 * <tr><th>Date        <th>Version    <th>Author      <th>Description
 * <tr><td>2021/04/27  <td>1.0        <td>陈港文          <td>创建初始版本
 * </table>
 *
 **********************************************************************************
 */

#ifndef HAL_SMMU_H
#define HAL_SMMU_H

/**************************************************************************
 *                         struct                                         *
 **************************************************************************/
/*udMemoryAttribute use
 *
 * SO:strongly-ordered memory
 * DE:device memory
 * NM:nomal memory，
 * IWT: inner cache， write-through，
 * OWT：outer cache， write-through，
 * INC：inner non-cacheable
 * ONC：outer non-cacheable
 * IWB：inner cache，write-back，
 * OWB：outer cache，write-back，
 */

/** bit wide */
#ifdef BW1
#undef BW1
#endif
#define BW1 ((u64)0x00000001)

#ifdef BW2
#undef BW2
#endif
#define BW2 ((u64)0x00000003)

#ifdef BW3
#undef BW3
#endif
#define BW3 ((u64)0x00000007)

#ifdef BW4
#undef BW4
#endif
#define BW4 ((u64)0x0000000f)

#ifdef BW5
#undef BW5
#endif
#define BW5 ((u64)0x0000001f)

#ifdef BW6
#undef BW6
#endif
#define BW6 ((u64)0x0000003f)

#ifdef BW7
#undef BW7
#endif
#define BW7 ((u64)0x0000007f)

#ifdef BW8
#undef BW8
#endif
#define BW8 ((u64)0x000000ff)

#ifdef BW9
#undef BW9
#endif
#define BW9 ((u64)0x000001ff)

#ifdef BW10
#undef BW10
#endif
#define BW10 ((u64)0x000003ff)

#ifdef BW11
#undef BW11
#endif
#define BW11 ((u64)0x000007ff)

#ifdef BW12
#undef BW12
#endif
#define BW12 ((u64)0x00000fff)

#ifdef BW13
#undef BW13
#endif
#define BW13 ((u64)0x00001fff)

#ifdef BW14
#undef BW14
#endif
#define BW14 ((u64)0x00003fff)

#ifdef BW15
#undef BW15
#endif
#define BW15 ((u64)0x00007fff)

#ifdef BW16
#undef BW16
#endif
#define BW16 ((u64)0x0000ffff)

#ifdef BW17
#undef BW17
#endif
#define BW17 ((u64)0x0001ffff)

#ifdef BW18
#undef BW18
#endif
#define BW18 ((u64)0x0003ffff)

#ifdef BW19
#undef BW19
#endif
#define BW19 ((u64)0x0007ffff)

#ifdef BW20
#undef BW20
#endif
#define BW20 ((u64)0x000fffff)

#ifdef BW21
#undef BW21
#endif
#define BW21 ((u64)0x001fffff)

#ifdef BW22
#undef BW22
#endif
#define BW22 ((u64)0x003fffff)

#ifdef BW23
#undef BW23
#endif
#define BW23 ((u64)0x007fffff)

#ifdef BW24
#undef BW24
#endif
#define BW24 ((u64)0x00ffffff)

#ifdef BW25
#undef BW25
#endif
#define BW25 ((u64)0x01ffffff)

#ifdef BW26
#undef BW26
#endif
#define BW26 ((u64)0x03ffffff)

#ifdef BW27
#undef BW27
#endif
#define BW27 ((u64)0x07ffffff)

#ifdef BW28
#undef BW28
#endif
#define BW28 ((u64)0x0fffffff)

#ifdef BW29
#undef BW29
#endif
#define BW29 ((u64)0x1fffffff)

#ifdef BW30
#undef BW30
#endif
#define BW30 ((u64)0x3fffffff)

#ifdef BW31
#undef BW31
#endif
#define BW31 ((u64)0x7fffffff)

#ifdef BW32
#undef BW32
#endif
#define BW32 ((u64)0xffffffff)

#define BW33 ((u64)0x00000001ffffffff)
#define BW34 ((u64)0x00000003ffffffff)
#define BW35 ((u64)0x00000007ffffffff)
#define BW36 ((u64)0x0000000fffffffff)
#define BW37 ((u64)0x0000001fffffffff)
#define BW38 ((u64)0x0000003fffffffff)
#define BW39 ((u64)0x0000007fffffffff)
#define BW40 ((u64)0x000000ffffffffff)
#define BW41 ((u64)0x000001ffffffffff)
#define BW42 ((u64)0x000003ffffffffff)
#define BW43 ((u64)0x000007ffffffffff)
#define BW44 ((u64)0x00000fffffffffff)
#define BW45 ((u64)0x00001fffffffffff)
#define BW46 ((u64)0x00003fffffffffff)
#define BW47 ((u64)0x00007fffffffffff)
#ifdef BW48
#undef BW48
#endif
#define BW48 ((u64)0x0000ffffffffffff)
#define BW49 ((u64)0x0001ffffffffffff)
#define BW50 ((u64)0x0003ffffffffffff)
#define BW51 ((u64)0x0007ffffffffffff)
#define BW52 ((u64)0x000fffffffffffff)
#define BW53 ((u64)0x001fffffffffffff)
#define BW54 ((u64)0x003fffffffffffff)
#define BW55 ((u64)0x007fffffffffffff)
#define BW56 ((u64)0x00ffffffffffffff)
#define BW57 ((u64)0x01ffffffffffffff)
#define BW58 ((u64)0x03ffffffffffffff)
#define BW59 ((u64)0x07ffffffffffffff)
#define BW60 ((u64)0x0fffffffffffffff)
#define BW61 ((u64)0x1fffffffffffffff)
#define BW62 ((u64)0x3fffffffffffffff)
#define BW63 ((u64)0x7fffffffffffffff)
#define BW64 ((u64)0xffffffffffffffff)

/* ------------------------------------------------------------------------------------------------------------------
 *
 *                                 V8描述符定义
 *  1)一级页表block 1G  2b01---block,2b11---table
 *  2)二级页表block 2M  2b01---block,2b11---table
 *  3)三级页表page  4K  2b11---page
 *
 *-------------------------------------------------------------------------------------------------------------------
 */

#define LONG_DESCRIPTOR_WACFG_POS (55)
#define LONG_DESCRIPTOR_WACFG_MASK (((u64)BW2) << LONG_DESCRIPTOR_WACFG_POS)
#define LONG_DESCRIPTOR_RACFG_POS (57)
#define LONG_DESCRIPTOR_RACFG_MASK (((u64)BW2) << LONG_DESCRIPTOR_RACFG_POS)

/* stage1 */
/* 一级页表的描述符定义
 *  1) block 型的，1G的页表
 *  +-----------------+-------------+-----------+---------+----------+-------------+-----------------+
 *  | 63  |59-62 | 58--55 | 54 |  53 |      52   |   51    |    50    |    49-48    |      47--32     |
 *  +-----------------+----------------+--------+---------+----------+-------------+-----------------+
 *  | RES |PBHA  | ignored| XN |PXN  |contiguous |   DBM   |GP-S1only |      0      |  output address |
 *  +-----------------+----------------+--------+---------+----------+-------------+-----------------+
 *  +-------------------+------------+------+------+---------+----------+------+---------+-----+----------+
 *  |     31 --- 30     |   29--12   |  11  |  10  |  9--8   |   7--6   |   5  |   4--2       |  1  |  0  |
 *  +-------------------+------------+------+------+---------+----------+------+--------------+-----+-----+
 *  |  output address   |      0     |  nG  |  AF  | SH[1:0] | AP[2:1]  |  NS  | MEMATTR[3:0] |  0  |  1  |
 *  +-------------------+------------+------+------+---------+----------+------+--------------+-----+-----+
 */

/* stage2 */
/* 一级页表的描述符定义
 *  1) block 型的，1G的页表
 *  +-----------------+-------------+-----------+---------+----------+-------------+-----------------+
 *  | 63  |59-62 | 58--55 | 54--53  |      52   |   51    |    50    |    49-48    |      47--32     |
 *  +-----------------+----------------+--------+---------+----------+-------------+-----------------+
 *  | RES |PBHA  | ignored|   XN     |contiguous |   DBM   |GP-S1only |      0      |  output address |
 *  +-----------------+----------------+--------+---------+----------+-------------+-----------------+
 *  +-------------------+------------+------+------+---------+----------+------+---------+-----+----------+
 *  |     31 --- 21     |   29--12   |  11  |  10  |  9--8   |   7--6   |         5--2        |  1  |  0  |
 *  +-------------------+------------+------+------+---------+----------+------+--------------+-----+-----+
 *  |  output address   |      0     | FnXS |  AF  | SH[1:0] | AP[1:0]  |        MEMATTR[3:0] |  0  |  1  |
 *  +-------------------+------------+------+------+---------+----------+------+--------------+-----+-----+
 */

/*BIT54 XN
 *  字段作用：设置execute never，memory的区域属性设置，如果为execute never，不能存放指令
 */
#define L1_LONG_DESCRIPTOR_BLOCK_XN_POS (53)
#define L1_LONG_DESCRIPTOR_BLOCK_XN_MASK \
	(((u64)BW2) << L1_LONG_DESCRIPTOR_BLOCK_XN_POS)

/*BIT 39-30
 *字段作用:设置PA地址
 */
#define L1_LONG_DESCRIPTOR_BLOCK_PA_POS (30)
#define L1_LONG_DESCRIPTOR_BLOCK_PA_MASK \
	(((u64)BW18) << L1_LONG_DESCRIPTOR_BLOCK_PA_POS)

/*BIT10 AF -- Access flag
 * 字段作用：访问标志，为0不能读入TLB
 */
#define L1_LONG_DESCRIPTOR_BLOCK_AF_POS (10)
#define L1_LONG_DESCRIPTOR_BLOCK_AF_MASK \
	(BW1 << L1_LONG_DESCRIPTOR_BLOCK_AF_POS)

#define L1_LONG_DESCRIPTOR_BLOCK_NG_POS (11)
#define L1_LONG_DESCRIPTOR_BLOCK_NG_MASK \
	(BW1 << L1_LONG_DESCRIPTOR_BLOCK_NG_POS)

/*BIT[9-8]: SH[1:0]
 * 字段作用：共享标志
 */
#define L1_LONG_DESCRIPTOR_BLOCK_SH1SH0_POS (8)
#define L1_LONG_DESCRIPTOR_BLOCK_SH1SH0_MASK \
	(BW2 << L1_LONG_DESCRIPTOR_BLOCK_SH1SH0_POS)

/*BIT[7-6]: AP[2:1]
 * 字段作用：访问权限AP
 */
#define L1_LONG_DESCRIPTOR_BLOCK_S2AP_POS (6)
#define L1_LONG_DESCRIPTOR_BLOCK_S2AP_MASK \
	(BW2 << L1_LONG_DESCRIPTOR_BLOCK_S2AP_POS)

/*BIT[5:2] MEMATTR[3:0]
 * 字段作用：设置内存属性索引
 */
#define L1_LONG_DESCRIPTOR_BLOCK_MEMATTR_POS (2)
#define L1_LONG_DESCRIPTOR_BLOCK_MEMATTR_MASK \
	(BW4 << L1_LONG_DESCRIPTOR_BLOCK_MEMATTR_POS)

#define L1_LONG_DESCRIPTOR_FOR_BLOCK (1)
#define L1_LONG_DESCRIPTOR_FOR_TABLE (3)

/* L1 page table , point to L2 page table base address
 *  1) table 型的，存放2级页表地址
 *  +----------+--------------+----------+-----------+----------------+-------------+----------------+
 *  |    63    |    62--61    |    60    |    59     |    58--52      |    51-48    |     47--32     |
 *  +----------+--------------+----------+-----------+----------------+-------------+----------------+
 *  |                 SBZ                            |    ignored     |      0      | output address |
 *  +----------+--------------+----------+-----------+----------------+-------------+----------------+
 *  +----------------------------------------------+-------------------------------------+-----+-----+
 *  |                      31--12                  |                 11--2               |  1  |  0  |
 *  +----------------------------------------------+-------------------------------------+-----+-----+
 *  |                  output address              |                         ignored     |  1  |  1  |
 *  +----------------------------------------------+-------------------------------------+-----+-----+
 */

/*BIT 39-12
 *字段作用:设置PA地址
 */
#define L1_LONG_DESCRIPTOR_TABLE_PA_POS (12)
#define L1_LONG_DESCRIPTOR_TABLE_PA_MASK \
	(((u64)BW36) << L1_LONG_DESCRIPTOR_TABLE_PA_POS)

/* stage1 */
/* 二级页表的描述符定义
 *  1) block 型的，2M的页表
 *  +-----------------+----------------+--------+---------+----------+-------------+-----------------+
 *  |     63  |59-62 | 58--55 | 54 |  53 |      52   |   51    |      50-48        |      47--32     |
 *  +-----------------+----------------+--------+---------+----------+-------------+-----------------+
 *  |     RES |PBHA  | ignored| XN |PXN  |contiguous |   DBM   |        res        |  output address |
 *  +-----------------+----------------+--------+---------+----------+-------------+-----------------+
 *  +-------------------+------------+------+------+---------+----------+------+---------+-----+----+-----+
 *  |           31 --- 12            |  11  |  10  |  9--8   |   7--6   |   5  |   4--2       |  1  |  0  |
 *  +-------------------+------------+------+------+---------+----------+------+---------+-----+----+-----+
 *  |         output address         |  nG  |  AF  | SH[1:0] | AP[2:1]  |  NS  | MEMATTR[3:0] |  0  |  1  |
 *  +-------------------+------------+------+------+---------+----------+------+---------+-----+----+-----+
 */

/* stage2 */
/* 二级页表的描述符定义
 *  1) block 型的，2M的页表
 *  +-----------------+-------------+-----------+---------+----------+-------------+-----------------+
 *  | 63  |59-62 | 58--55 | 54    53 |      52   |   51    |    50    |    49-48    |      47--32     |
 *  +-----------------+----------------+--------+---------+----------+-------------+-----------------+
 *  | RES |PBHA  | ignored|   XN     |contiguous |   DBM   |GP-S1only |      0      |  output address |
 *  +-----------------+----------------+--------+---------+----------+-------------+-----------------+
 *  +-------------------+------------+------+------+---------+----------+------+---------+-----+----------+
 *  |     31 --- 21     |   29--12   |  11  |  10  |  9--8   |   7--6   |         5--2        |  1  |  0  |
 *  +-------------------+------------+------+------+---------+----------+------+--------------+-----+-----+
 *  |  output address   |      0     | FnXS |  AF  | SH[1:0] | AP[1:0]  |        MEMATTR[3:0] |  0  |  1  |
 *  +-------------------+------------+------+------+---------+----------+------+--------------+-----+-----+
 */

/*BIT54 XN
 *  字段作用：设置execute never，memory的区域属性设置，如果为execute never，不能存放指令
 */
#define L2_LONG_DESCRIPTOR_BLOCK_XN_POS (53)
#define L2_LONG_DESCRIPTOR_BLOCK_XN_MASK \
	(((u64)BW2) << L2_LONG_DESCRIPTOR_BLOCK_XN_POS)

/*BIT 39-21
 *字段作用:设置PA地址
 */
#define L2_LONG_DESCRIPTOR_BLOCK_PA_POS (21)
#define L2_LONG_DESCRIPTOR_BLOCK_PA_MASK \
	(((u64)BW27) << L2_LONG_DESCRIPTOR_BLOCK_PA_POS)

/*BIT10 AF -- Access flag
 * 字段作用：访问标志，为0不能读入TLB
 */
#define L2_LONG_DESCRIPTOR_BLOCK_AF_POS (10)
#define L2_LONG_DESCRIPTOR_BLOCK_AF_MASK \
	(BW1 << L2_LONG_DESCRIPTOR_BLOCK_AF_POS)

#define L2_LONG_DESCRIPTOR_BLOCK_NG_POS (11)
#define L2_LONG_DESCRIPTOR_BLOCK_NG_MASK \
	(BW1 << L2_LONG_DESCRIPTOR_BLOCK_NG_POS)

/*BIT[9-8]: SH[1:0]
 * 字段作用：共享标志
 */
#define L2_LONG_DESCRIPTOR_BLOCK_SH1SH0_POS (8)
#define L2_LONG_DESCRIPTOR_BLOCK_SH1SH0_MASK \
	(BW2 << L2_LONG_DESCRIPTOR_BLOCK_SH1SH0_POS)

/*BIT[7-6]: AP[2:1]
 * 字段作用：访问权限AP
 */
#define L2_LONG_DESCRIPTOR_BLOCK_S2AP_POS (6)
#define L2_LONG_DESCRIPTOR_BLOCK_S2AP_MASK \
	(BW2 << L2_LONG_DESCRIPTOR_BLOCK_S2AP_POS)

/*BIT[5:2] MemAttr[3:0]
 * 字段作用：设置内存属性索引
 */
#define L2_LONG_DESCRIPTOR_BLOCK_MEMATTR_POS (2)
#define L2_LONG_DESCRIPTOR_BLOCK_MEMATTR_MASK \
	(BW4 << L2_LONG_DESCRIPTOR_BLOCK_MEMATTR_POS)

#define L2_LONG_DESCRIPTOR_FOR_BLOCK (1)
#define L2_LONG_DESCRIPTOR_FOR_TABLE (3)

/* 二级页表的描述符定义
 *  1) table 型的，存放3级页表地址
 *  +----------+--------------+----------+-----------+----------------+-------------+----------------+
 *  |    63    |    62--61    |    60    |    59     |    58--52      |    51-40    |     47--32     |
 *  +----------+--------------+----------+-----------+----------------+-------------+----------------+
 *  |                 SBZ                            |    ignored     |      0      | output address |
 *  +----------+--------------+----------+-----------+----------------+-------------+----------------+
 *  +----------------------------------------------+-------------------------------------+-----+-----+
 *  |                      31--12                  |                 11--2               |  1  |  0  |
 *  +----------------------------------------------+-------------------------------------+-----+-----+
 *  |                  output address              |                         ignored     |  1  |  1  |
 *  +----------------------------------------------+-------------------------------------+-----+-----+
 */

/*BIT 39-12
 *字段作用:设置PA地址
 */
#define L2_LONG_DESCRIPTOR_TABLE_PA_POS (12)
#define L2_LONG_DESCRIPTOR_TABLE_PA_MASK \
	(((u64)BW36) << L2_LONG_DESCRIPTOR_TABLE_PA_POS)

/* stage1 */
/* 三级页表的描述符定义
 *  1) page 型的，4K的页表
 *  +-----------------+----------------+--------+---------+----------+-------------+-----------------+
 *  |     63  |59-62 | 58--55 | 54 |  53 |      52   |   51    |      50-48        |      47--32     |
 *  +-----------------+----------------+--------+---------+----------+-------------+-----------------+
 *  |     RES |PBHA  | ignored| XN |PXN  |contiguous |   DBM   |        res        |  output address |
 *  +-----------------+----------------+--------+---------+----------+-------------+-----------------+
 *  +-------------------+------------+------+------+---------+----------+------+---------+-----+----+-----+
 *  |           31 --- 12            |  11  |  10  |  9--8   |   7--6   |   5  |   4--2       |  1  |  0  |
 *  +-------------------+------------+------+------+---------+----------+------+---------+-----+----+-----+
 *  |         output address         |  nG  |  AF  | SH[1:0] | AP[2:1]  |  NS  | MEMATTR[3:0] |  0  |  1  |
 *  +-------------------+------------+------+------+---------+----------+------+---------+-----+----+-----+
 */

/* stage2 */
/* 三级页表的描述符定义
 *  1) page 型的，4K的页表
 *  +-----------------+-------------+-----------+---------+----------+-------------+-----------------+
 *  | 63  |59-62 | 58--55 | 54    53 |      52   |   51    |    50    |    49-48    |      47--32     |
 *  +-----------------+----------------+--------+---------+----------+-------------+-----------------+
 *  | RES |PBHA  | ignored|   XN     |contiguous |   DBM   |GP-S1only |      0      |  output address |
 *  +-----------------+----------------+--------+---------+----------+-------------+-----------------+
 *  +-------------------+------------+------+------+---------+----------+------+---------+-----+----------+
 *  |     31 --- 21     |   29--12   |  11  |  10  |  9--8   |   7--6   |         5--2        |  1  |  0  |
 *  +-------------------+------------+------+------+---------+----------+------+--------------+-----+-----+
 *  |  output address   |      0     | FnXS |  AF  | SH[1:0] | AP[1:0]  |        MEMATTR[3:0] |  0  |  1  |
 *  +-------------------+------------+------+------+---------+----------+------+--------------+-----+-----+
 */

/*BIT54 XN
 *  字段作用：设置execute never，memory的区域属性设置，如果为execute never，不能存放指令
 */
#define L3_LONG_DESCRIPTOR_BLOCK_XN_POS (53)
#define L3_LONG_DESCRIPTOR_BLOCK_XN_MASK \
	(((u64)BW2) << L3_LONG_DESCRIPTOR_BLOCK_XN_POS)

/*BIT 39-12
 *字段作用:设置PA地址
 */
#define L3_LONG_DESCRIPTOR_BLOCK_PA_POS (12)
#define L3_LONG_DESCRIPTOR_BLOCK_PA_MASK \
	(((u64)BW36) << L3_LONG_DESCRIPTOR_BLOCK_PA_POS)

/*BIT10 AF -- Access flag
 * 字段作用：访问标志，为0不能读入TLB
 */
#define L3_LONG_DESCRIPTOR_BLOCK_AF_POS (10)
#define L3_LONG_DESCRIPTOR_BLOCK_AF_MASK \
	(BW1 << L3_LONG_DESCRIPTOR_BLOCK_AF_POS)

#define L3_LONG_DESCRIPTOR_BLOCK_NG_POS (11)
#define L3_LONG_DESCRIPTOR_BLOCK_NG_MASK \
	(BW1 << L3_LONG_DESCRIPTOR_BLOCK_NG_POS)

/*BIT[9-8]: SH[1:0]
 * 字段作用：共享标志
 */
#define L3_LONG_DESCRIPTOR_BLOCK_SH1SH0_POS (8)
#define L3_LONG_DESCRIPTOR_BLOCK_SH1SH0_MASK \
	(BW2 << L3_LONG_DESCRIPTOR_BLOCK_SH1SH0_POS)

/*BIT[7-6]: AP[2:1]
 * 字段作用：访问权限AP
 */
#define L3_LONG_DESCRIPTOR_BLOCK_S2AP_POS (6)
#define L3_LONG_DESCRIPTOR_BLOCK_S2AP_MASK \
	(BW2 << L3_LONG_DESCRIPTOR_BLOCK_S2AP_POS)

/*
 * BIT[5:2] MemAttr[3:0]
 * 字段作用：设置内存属性索引
 */
#define L3_LONG_DESCRIPTOR_BLOCK_MEMATTR_POS (2)
#define L3_LONG_DESCRIPTOR_BLOCK_MEMATTR_MASK \
	(BW4 << L3_LONG_DESCRIPTOR_BLOCK_MEMATTR_POS)

#define L3_LONG_DESCRIPTOR_FOR_PAGE (3)

#define SMMU_PAGETABLE_SO (0) /**<  strongly-ordered memory*/
#define SMMU_PAGETABLE_DE (1) /**<  device memory*/
#define SMMU_PAGETABLE_NM_ONC_INC (5) /**<  nomal memory, non-cache*/
#define SMMU_PAGETABLE_NM_ONC_IWT (6)
#define SMMU_PAGETABLE_NM_ONC_IWB (7)
#define SMMU_PAGETABLE_NM_OWT_INC \
	(9) /**<  nomal memory, L1 non-cache, L2 cache WT*/
#define SMMU_PAGETABLE_NM_OWT_IWT (10) /**<  nomal memory, WT*/
#define SMMU_PAGETABLE_NM_OWT_IWB (11)
#define SMMU_PAGETABLE_NM_OWB_INC \
	(13) /**< nomal memory, L1 non-cache，L2 cache WB*/
#define SMMU_PAGETABLE_NM_OWB_IWT \
	(14) /**<  nomal memory,L1 cache WT，L2 cache WB*/
#define SMMU_PAGETABLE_NM_OWB_IWB (15)
/* Allocate default */
#define READ_NOALLOCATE (3 << 4)
#define WRITE_NOALLOCATE (3 << 6)

#define SMMU_PAGETABLE_NM_ONC_IWT_RNA \
	(READ_NOALLOCATE | SMMU_PAGETABLE_NM_ONC_IWT)
#define SMMU_PAGETABLE_NM_ONC_IWB_RNA \
	(READ_NOALLOCATE | SMMU_PAGETABLE_NM_ONC_IWB)
#define SMMU_PAGETABLE_NM_OWT_INC_RNA \
	(READ_NOALLOCATE | SMMU_PAGETABLE_NM_OWT_INC)
#define SMMU_PAGETABLE_NM_OWT_IWT_RNA \
	(READ_NOALLOCATE | SMMU_PAGETABLE_NM_OWT_IWT)
#define SMMU_PAGETABLE_NM_OWT_IWB_RNA \
	(READ_NOALLOCATE | SMMU_PAGETABLE_NM_OWT_IWB)
#define SMMU_PAGETABLE_NM_OWB_INC_RNA \
	(READ_NOALLOCATE | SMMU_PAGETABLE_NM_OWB_INC)
#define SMMU_PAGETABLE_NM_OWB_IWT_RNA \
	(READ_NOALLOCATE | SMMU_PAGETABLE_NM_OWB_IWT)
#define SMMU_PAGETABLE_NM_OWB_IWB_RNA \
	(READ_NOALLOCATE | SMMU_PAGETABLE_NM_OWB_IWB)

#define SMMU_PAGETABLE_NM_ONC_IWT_WNA \
	(WRITE_NOALLOCATE | SMMU_PAGETABLE_NM_ONC_IWT)
#define SMMU_PAGETABLE_NM_ONC_IWB_WNA \
	(WRITE_NOALLOCATE | SMMU_PAGETABLE_NM_ONC_IWB)
#define SMMU_PAGETABLE_NM_OWT_INC_WNA \
	(WRITE_NOALLOCATE | SMMU_PAGETABLE_NM_OWT_INC)
#define SMMU_PAGETABLE_NM_OWT_IWT_WNA \
	(WRITE_NOALLOCATE | SMMU_PAGETABLE_NM_OWT_IWT)
#define SMMU_PAGETABLE_NM_OWT_IWB_WNA \
	(WRITE_NOALLOCATE | SMMU_PAGETABLE_NM_OWT_IWB)
#define SMMU_PAGETABLE_NM_OWB_INC_WNA \
	(WRITE_NOALLOCATE | SMMU_PAGETABLE_NM_OWB_INC)
#define SMMU_PAGETABLE_NM_OWB_IWT_WNA \
	(WRITE_NOALLOCATE | SMMU_PAGETABLE_NM_OWB_IWT)
#define SMMU_PAGETABLE_NM_OWB_IWB_WNA \
	(WRITE_NOALLOCATE | SMMU_PAGETABLE_NM_OWB_IWB)

#define SMMU_PAGETABLE_NM_ONC_IWT_RNA_WNA \
	(READ_NOALLOCATE | WRITE_NOALLOCATE | SMMU_PAGETABLE_NM_ONC_IWT)
#define SMMU_PAGETABLE_NM_ONC_IWB_RNA_WNA \
	(READ_NOALLOCATE | WRITE_NOALLOCATE | SMMU_PAGETABLE_NM_ONC_IWB)
#define SMMU_PAGETABLE_NM_OWT_INC_RNA_WNA \
	(READ_NOALLOCATE | WRITE_NOALLOCATE | SMMU_PAGETABLE_NM_OWT_INC)
#define SMMU_PAGETABLE_NM_OWT_IWT_RNA_WNA \
	(READ_NOALLOCATE | WRITE_NOALLOCATE | SMMU_PAGETABLE_NM_OWT_IWT)
#define SMMU_PAGETABLE_NM_OWT_IWB_RNA_WNA \
	(READ_NOALLOCATE | WRITE_NOALLOCATE | SMMU_PAGETABLE_NM_OWT_IWB)
#define SMMU_PAGETABLE_NM_OWB_INC_RNA_WNA \
	(READ_NOALLOCATE | WRITE_NOALLOCATE | SMMU_PAGETABLE_NM_OWB_INC)
#define SMMU_PAGETABLE_NM_OWB_IWT_RNA_WNA \
	(READ_NOALLOCATE | WRITE_NOALLOCATE | SMMU_PAGETABLE_NM_OWB_IWT)
#define SMMU_PAGETABLE_NM_OWB_IWB_RNA_WNA \
	(READ_NOALLOCATE | WRITE_NOALLOCATE | SMMU_PAGETABLE_NM_OWB_IWB)

// udExecuteNever use
#define SMMU_PAGETABLE_EXECUTE_NEVER (1) /* XN, can not prefetch */
#define SMMU_PAGETABLE_EXECUTE (0)

// udPageType use
#define SMMU_PAGETABLE_PAGESIZE_4KB (0) /* 4KB, small page*/
#define SMMU_PAGETABLE_PAGESIZE_64KB (1) /* 64KB,large page*/
#define SMMU_PAGETABLE_PAGESIZE_1MB (2) /* 1MB,section*/
#define SMMU_PAGETABLE_PAGESIZE_2MB (3) /* 2MB,block*/
#define SMMU_PAGETABLE_PAGESIZE_16MB (4) /* 16MB,surper-section*/
#define SMMU_PAGETABLE_PAGESIZE_512MB (5) /* 16MB,surper-section*/
#define SMMU_PAGETABLE_PAGESIZE_1G (6) /* 1G,block*/

struct smmu_pte_cfg {
	u64 uddPABaseAddr; /* Block base address */
	u64 udExecuteNever; /* Executable code memory */
	u32 udShareable; /* SH */
	u32 udAccessPermission; /* AP */
	u32 udMemoryAttribute; /* MemAttr */
	u32 udPageType; /* Page size */
	u64 udWACFG; /* Write-Allocate */
	u64 udRACFG; /* Read-Allocate */
	u32 udEndian; /* Endian */
	u32 udPageFormat; /* page table format */
};

#endif
