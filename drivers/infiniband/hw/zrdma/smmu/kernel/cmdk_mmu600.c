// SPDX-License-Identifier: GPL-2.0 or Linux-OpenIB
/* Copyright (c) 2023 - 2024 ZTE Corporation */

#include "common_define.h"
#include <linux/init.h>
#include <linux/module.h>
#include <asm/io.h>
#include <linux/slab.h>

#include "hal_smmu.h"
#include "cmdk_mmu600_inner.h"
#include "cmdk_mmu600.h"
#include "pub_print.h"

u8 g_ucMmu600PrintModuleId = 0x2;

u32 uswap_32(u32 v)
{
	return v;
}

u64 uswap_64(u64 v)
{
	return v;
}

u32 memset_8byte(u64 *p, u64 data, u64 size)
{
	u32 i = 0;

	if (size % 8)
		return -1;

	for (i = 0; i < (size / 8); i++)
		*(p + i) = data;

	return 0;
}

u32 mpf_sync_msg_send(u8 type, u8 module_id, u8 *msg, u16 len)
{
	//communication channel, other module supply
	return CMDK_OK;
}

u32 mpf_async_msg_send(u8 type, u8 module_id, u8 *msg, u16 len)
{
	//communication channel, other module supply
	return CMDK_OK;
}

u32 CmdkSysMmuCmdTlbSync(void)
{
	struct st2RiscMsg recv_msg = { 0 };

	recv_msg.type = SMMU_MSG_TLB_SYNC;

	mpf_sync_msg_send(0x4, 2, (u8 *)&recv_msg, sizeof(struct st2RiscMsg));

	return CMDK_OK;
}
EXPORT_SYMBOL(CmdkSysMmuCmdTlbSync);

u32 CmdkSysMmuCmdTlbCleanByVa(u32 udSid, u32 udSsid, u64 uddVa, u32 udPageLvl)
{
	return CMDK_OK;
}
EXPORT_SYMBOL(CmdkSysMmuCmdTlbCleanByVa);

u32 CmdkSysMmuCmdTlbCleanByIpa(u32 udSid, u64 uddVa, u32 udPageLvl)
{
	//需覝坑risc-v的smmu驱动坑逝命�?TODO
	struct st2RiscMsg recv_msg = { 0 };

	recv_msg.type = SMMU_MSG_TLB_IPA;
	recv_msg.udStreamid = udSid;
	recv_msg.vaddr = uddVa;
	recv_msg.uddSize = udPageLvl;

	mpf_sync_msg_send(0x4, 2, (u8 *)&recv_msg, sizeof(struct st2RiscMsg));

	return CMDK_OK;
}
EXPORT_SYMBOL(CmdkSysMmuCmdTlbCleanByIpa);

u32 CmdkSysMmuCmdDeletePte(u32 udSid, u64 uddVa, u64 uddSize, u64 uddPteL2DAddr,
			   u64 uddPteL2DLen)
{
	//需覝坑risc-v的smmu驱动坑逝命�?TODO
	struct st2RiscMsg recv_msg = { 0 };

	recv_msg.type = SMMU_MSG_DEL_PTE;
	recv_msg.udStreamid = udSid;
	recv_msg.vaddr = uddVa;
	recv_msg.uddSize = uddSize;
	recv_msg.uddPteL2DAddr = uddPteL2DAddr;
	recv_msg.uddPteL2DLen = uddPteL2DLen;

	mpf_sync_msg_send(0x4, 2, (u8 *)&recv_msg, sizeof(struct st2RiscMsg));

	return CMDK_OK;
}
EXPORT_SYMBOL(CmdkSysMmuCmdDeletePte);

u32 CmdkSysMmuCmdTlbCleanByVmid(u32 udSid)
{
	//需覝坑risc-v的smmu驱动坑逝命�?TODO

	return CMDK_OK;
}
EXPORT_SYMBOL(CmdkSysMmuCmdTlbCleanByVmid);

u32 CmdkSysMmuCmdTlbCleanByAsid(u32 udSid, u32 udSsid)
{
	//需覝坑risc-v的smmu驱动坑逝命�?TODO

	return CMDK_OK;
}
EXPORT_SYMBOL(CmdkSysMmuCmdTlbCleanByAsid);

u32 CmdkSysMmuCmdSteSync(void)
{
	//需覝坑risc-v的smmu驱动坑逝命�?TODO

	return CMDK_OK;
}
EXPORT_SYMBOL(CmdkSysMmuCmdSteSync);

u32 CmdkSysMmuCmdCdSync(u32 udSid)
{
	//需覝坑risc-v的smmu驱动坑逝命�?TODO

	return CMDK_OK;
}
EXPORT_SYMBOL(CmdkSysMmuCmdCdSync);

u32 CmdkSysMmuSetPrintLevel(u32 udPrintLvl)
{
	g_ucMmu600PrintModuleId = (u8)udPrintLvl;

	return CMDK_OK;
}
EXPORT_SYMBOL(CmdkSysMmuSetPrintLevel);

u8 CmdkSysMmuGetPrintLevel(void)
{
	return g_ucMmu600PrintModuleId;
}
EXPORT_SYMBOL(CmdkSysMmuGetPrintLevel);
